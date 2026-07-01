# Užduotis: v31 — sutvarkyti visas audito problemas (Kemperis Android app)

> **Pakeičia** `docs/v31_pataisymu_uzduotis.md`. Pagrindas: `docs/audits/pilnas_auditas_2026-07-01_app.md`.
> **Bendra taisyklė:** v31 apima **tik native Java** pataisymus (`MainActivity.java`, `BootReceiver.java`). `www/index.html`, Google Apps Script, firmware YAML ir PIN saugumą — **NELIESTI** (4 punktas atidėtas).
> Po darbų: pakelti `versionCode`/`versionName` į **31**, build, kopijuoti į `android/kemperis_v31.apk`, `git add -f`, commit + push.

---

## 1. Failų išsaugojimas per MediaStore (kritinė — pakeičia seną Pataisymą 1)

**Failas:** `android/android/app/src/main/java/lt/kemperis/app/MainActivity.java`, klasė `KemperisFileBridge` (~487 eil.).

**Problema:** dvi klaidos viename: (a) JS kviečia `saveTextFile(failas, mime, turinys)`, o Java signatūra `(folder, name, content)` → kelias sugadinamas (`…/Download/x.txt/text/plain`); (b) `targetSdk=34` → tiesioginis rašymas į viešą Downloads per `Environment.getExternalStoragePublicDirectory` **blokuojamas** Android 10+ (scoped storage), tad net pataisius (a) planšetėje toliau lūžtų su „Permission denied".

**Sprendimas** — signatūra `(name, mime, content)`, MediaStore API 29+, grąžinti `"ok"`:

```java
@JavascriptInterface
public String saveTextFile(String name, String mime, String content) {
    try {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            ContentValues cv = new ContentValues();
            cv.put(MediaStore.Downloads.DISPLAY_NAME, name);
            cv.put(MediaStore.Downloads.MIME_TYPE, (mime == null || mime.isEmpty()) ? "text/plain" : mime);
            cv.put(MediaStore.Downloads.RELATIVE_PATH, Environment.DIRECTORY_DOWNLOADS);
            android.net.Uri uri = ctx.getContentResolver()
                .insert(MediaStore.Downloads.EXTERNAL_CONTENT_URI, cv);
            if (uri == null) return "Error: MediaStore insert null";
            try (OutputStream os = ctx.getContentResolver().openOutputStream(uri)) {
                os.write(content.getBytes(StandardCharsets.UTF_8));
            }
            return "ok";
        } else {
            File dir = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS);
            if (!dir.exists()) dir.mkdirs();
            File file = new File(dir, name);
            try (FileOutputStream fos = new FileOutputStream(file)) {
                fos.write(content.getBytes(StandardCharsets.UTF_8));
            }
            return "ok";
        }
    } catch (Exception e) { return "Error: " + e.getMessage(); }
}
```

**JS keisti nereikia** — visi 3 kviečiantieji (`saveTerminal`, `downloadLocalCSV`, `downloadGPX`) jau siunčia `(failas, mime, turinys)` ir tikrina `result === 'ok'`.
**Priėmimas:** „Išsaugoti logą", „Atsisiųsti CSV", GPX sukuria failą Downloads aplanke (matomas per failų naršyklę / MediaStore) ir terminale rodo žalią „✅".

---

## 2. TTS diagnostika + abiejų kalbų (EN ir LT) tikrinimas (kritinė)

**Failas:** tas pats `MainActivity.java`, klasė `KempTtsBridge` (~452 eil.).

**Problema:** planšetėje `speak()` tyli (`ready=false`), nes trūksta sistemos TTS variklio arba balso duomenų; jokio pranešimo nerodo. Pyptelėjimas girdisi (chime nepriklauso nuo TTS).

**Reikalavimai:**
1. `onInit`: jei `status != SUCCESS` — pranešti į app terminalą.
2. Po `SUCCESS` — su `isLanguageAvailable(...)` patikrinti **EN ir LT**; rezultatą išvesti; jei bent vienos trūksta duomenų — pasiūlyti `ACTION_INSTALL_TTS_DATA`.
3. `applyLang`: tikrinti `setLanguage` grąžą (`LANG_MISSING_DATA` → siūlyti diegimą; `LANG_NOT_SUPPORTED` → įspėti).
4. `speak()`: jei `!ready` — žurnaluoti, o ne tylėti.

```java
private void reportLangs() {
    int en = tts.isLanguageAvailable(Locale.US);
    int lt = tts.isLanguageAvailable(new Locale("lt","LT"));
    logToWeb("TTS EN: " + langStatus(en) + " | LT: " + langStatus(lt));
    if (en == TextToSpeech.LANG_MISSING_DATA || lt == TextToSpeech.LANG_MISSING_DATA) {
        Intent i = new Intent(TextToSpeech.Engine.ACTION_INSTALL_TTS_DATA);
        i.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        try { activity.startActivity(i); } catch (Exception ignored) {}
    }
}
private String langStatus(int r) {
    switch (r) {
        case TextToSpeech.LANG_AVAILABLE:
        case TextToSpeech.LANG_COUNTRY_AVAILABLE:
        case TextToSpeech.LANG_COUNTRY_VAR_AVAILABLE: return "OK";
        case TextToSpeech.LANG_MISSING_DATA:  return "truksta duomenu";
        case TextToSpeech.LANG_NOT_SUPPORTED: return "nepalaikoma";
        default: return "nezinoma(" + r + ")";
    }
}
private void logToWeb(String msg) {
    final String safe = msg.replace("'", " ");
    activity.runOnUiThread(() ->
        webView.evaluateJavascript("sysLog && sysLog('[TTS] " + safe + "','warn')", null));
}
```
`applyLang` papildyti grąžos tikrinimu (kaip aukščiau). `onInit` po `ready=true; applyLang("en");` iškviesti `reportLangs();`.

**Priėmimas:** startuojant terminale matoma, pvz., `TTS EN: OK | LT: truksta duomenu`; trūkstant — pasirodo diegimo langas; įdiegus tariama EN ir LT.

---

## 3. TTS pabaigos signalas (`onTtsDone`) per UtteranceProgressListener (aukšta)

**Failas:** `MainActivity.java`, `KempTtsBridge`.

**Problema:** JS `speakNow` laukia `window.onTtsDone()` „tikslaus pabaigos signalo", bet native `speak()` **nenustato** `UtteranceProgressListener` (nors importas 30 eil. yra). Todėl pabaiga nustatoma tik pagal `text.length*100 ms` laikmatį → netikslus garsumo atkūrimas, galimas ilgų frazių nukirpimas.

**Sprendimas:** konstruktoriuje užregistruoti listener'į, `speak()` perduoti `utteranceId`:

```java
public KempTtsBridge(Activity a, WebView wv) {
    this.activity = a; this.webView = wv;
    tts = new TextToSpeech(a, status -> {
        if (status == TextToSpeech.SUCCESS) {
            ready = true;
            applyLang("en");
            reportLangs();
            tts.setOnUtteranceProgressListener(new UtteranceProgressListener() {
                @Override public void onStart(String id) {}
                @Override public void onDone(String id) {
                    activity.runOnUiThread(() ->
                        webView.evaluateJavascript("window.onTtsDone && window.onTtsDone()", null));
                }
                @Override public void onError(String id) {
                    activity.runOnUiThread(() ->
                        webView.evaluateJavascript("window.onTtsDone && window.onTtsDone()", null));
                }
            });
        } else {
            logToWeb("variklis nerastas (status=" + status + ")");
        }
    });
}
@JavascriptInterface
public void speak(String text) {
    if (ready) tts.speak(text, TextToSpeech.QUEUE_ADD, null, "uid");
    else logToWeb("TTS neparuostas - nutylima");
}
```

**Priėmimas:** debug loge po kalbos matomas `[NATIVE] TTS Finished`; garsumas atkuriamas tik pasakius frazę iki galo.

---

## 4. SSE object ID atvaizdavimo sutvirtinimas — ⏸️ ATIDĖTA (NE v31)

> **Sprendimas 2026-07-01: NEDARYTI v31.** SSE atvaizdavimas dabar **veikia** — tai vidinis švarinimas be naudos vartotojui, o pats perrašymas rizikingas (klaida tyliai „pamestų" kortelę). Saugiai perdaryti reikia gyvo ESP verifikacijos. v31 apima tik realiai sugedusius dalykus (failų išsaugojimas, TTS). Šis punktas paliekamas kaip **dokumentuota techninė skola** atskiram etapui. Žemiau esanti analizė ir slug'ų lentelė — ateities darbo pagrindas; dabar `startSSE` grandinės **NELIESTI**.

**Failai (kai bus daroma):** `android/www/index.html` (`startSSE`, `SENSOR_ID_MAP`). ESP perflash'inti nereikia, firmware neliesti.

**PATIKSLINTA (2026-07-01):** web_server v3 SSE `id` **NĖRA** C++ `id:` — jis yra **slugifikuotas lietuviškas `name:`**. Pvz.:
- „GPS \| Greitis" → `sensor_gps_greitis` (ne `gps_speed_sensor`)
- „Energija \| Akum SOC %" → `sensor_energija_akum_soc_` (ne `junc_soc`)
- „Pokrypis \| Soninis K-D" → `sensor_pokrypis_soninis_k_d` (ne `roll_sensor`)

> **DĖMESIO — slug taisyklė:** ESPHome lietuviškas raides ir specialius simbolius paverčia į `_`, **netransliteruoja**:
> - „ESP temperat**ū**ra" → `sistema_esp_temperat_ra` (ū → `_`, NE „u")
> - pavadinimas su „%", „(EMA)", „(ppm)", „(dBm)" gale → **galinis `_`** (pvz. `energija_akum_soc_`, `resursai_vanduo_lygis_`, `gsm_signalas_`).
> App papildomai suspaudžia kelis `_` į vieną (`[^a-z0-9]+ → _`). Šaltinis: lentelė žemiau sudaryta iš dabartinio `firmware/kempervanas.yaml` pavadinimų (autoritetinga), bet **galutinį tikslų slug'ą lemia ESP** — privaloma gyvo ESP patikra (žr. verifikaciją).

> **Įkelti logai buvo iš ankstesnės app versijos** — bet object_id priklauso nuo **firmware**, ne app, tad slug'ai galioja. Palyginus su dabartiniu YAML, papildomai atsirado: `energija_akum_temperatura`, `energija_akum_total_ah`, `miegamasis_co2_tendencija`, realūs SMS pavadinimai ir **`binary_sensor` domenas** (220V) — įtraukta žemiau.

Todėl dabartinis **deterministinis `SENSOR_ID_MAP` (raktai = C++ id) beveik NIEKO nesugauna** — realų darbą atlieka trapi `includes()` grandinė. **NEGALIMA** tiesiog ištrinti grandinės ir palikti seno žemėlapio — sugriūtų visos kortelės.

**Tikra migracija — perrakiuoti `SENSOR_ID_MAP` pagal realius pavadinimų slug'us (tikslus atitikimas), tada pašalinti `includes()` grandinę.** Realūs object_id (iš logų, po app normalizacijos `[^a-z0-9]+ → _`, be domeno prefikso) → app cache raktas:

| Realus slug (stripped) | Cache raktas |
|---|---|
| `energija_akum_soc_` | `junc_soc` |
| `energija_akum_itampa` | `junc_v` |
| `energija_akum_srove` | `junc_a` |
| `energija_akum_galia` | `junc_power` |
| `energija_akum_ah_likutis` | `junc_ah_remaining` |
| `energija_akum_veikimo_laikas` | `junc_time_remaining` |
| `energija_akum_temperatura` | `junc_temp` |
| `energija_akum_total_ah` | `junc_total_ah` (patikrinti ar naudojama) |
| `gps_greitis` | `gps_speed_sensor` |
| `gps_kryptis` | `gps_heading_sensor` |
| `gps_aukstis` | `gps_alt_sensor` |
| `gps_visi_palydovai` | `gps_sats_total_sensor` |
| `gps_palydovai_navstar_jav_` | `sats_gps` |
| `gps_palydovai_glonass_rusija_` | `sats_glonass` |
| `gps_palydovai_beidou_kinija_` | `sats_bds` |
| `gps_palydovai_galileo_europa_` | `sats_galileo` |
| `gps_koordinates` (text) | `gps_coords_sensor` |
| `gps_fix_statusas` (text) | `gps_fix_status` |
| `pokrypis_soninis_k_d` | `roll_sensor` |
| `pokrypis_isilginis_p_g` | `pitch_sensor` |
| `pokrypis_vibracija_ema_` | `vib_level` |
| `aplinka_temperatura` | `bme_temp` |
| `aplinka_dregme` | `bme_humidity` |
| `aplinka_slegis` | `bme_pressure` |
| `aplinka_aukstis_slegis_` | `pressure_altitude` |
| `aplinka_slegio_tendencija` | `pressure_trend_hpa` |
| `aplinka_oru_prognoze` (text) | `weather_forecast` |
| `miegamasis_co2` | `bedroom_co2` |
| `miegamasis_temperatura` | `bedroom_temp` |
| `miegamasis_dregme` | `bedroom_humidity` |
| `miegamasis_co2_tendencija` | `bedroom_co2_trend` |
| `miegamasis_oro_kokybe` (text) | `bedroom_air_quality` |
| `resursai_vanduo_lygis_` | `water_pct` |
| `resursai_vandens_atstumas_lazeris_` | `water_distance` |
| `resursai_vandens_tds_ppm_` | `tds_ppm` |
| `resursai_dujos_likutis_` | `gas_pct` |
| `gsm_signalas_` | `gsm_signal_pct` |
| `gsm_signalas_dbm_` | `gsm_signal_dbm` |
| `gsm_operatorius` (text) | `gsm_operator_sensor` |
| `gsm_tinklas` (text) | `gsm_network_sensor` |
| `gsm_roaming_statusas` (text) | `gsm_roaming_sensor` |
| `energija_220v_saltinis` (text) | `power_source_220` |
| `energija_isorine_220v` (**binary**) | `shore_power_present` |
| `energija_keitiklio_220v` (**binary**) | `inverter_220_active` |
| `sistema_rysio_aliarmas` (**binary**) | `link_alarm` |
| `sauga_judejimo_aliarmas` (text) | `alert_movement_sent` |
| `sistema_esp_temperat_ra` | `sys_esp_temp` |
| `sistema_laisva_ram` | `sys_free_ram` |
| `sistema_uptime` (text) | `sys_uptime` |
| `sistema_patarimai` (text) | `system_advice` |
| `sistema_google_buferis` (text) | `google_buffer_status` |
| `sistema_modemo_busena` (text) | `modem_status` |
| `sistema_paskutine_klaida` (text) | `last_error` |
| `gauta_sms_1_naujausia` (text) | `sms_1` |
| `gauta_sms_2` (text) | `sms_2` |
| `gauta_sms_3` (text) | `sms_3` |
| `gauta_sms_4` (text) | `sms_4` |
| `gauta_sms_5` (text) | `sms_5` |

`number_*` domenas jau atitinka (raktai = `num_...`), jį palikti kaip yra. **`binary_sensor` domeną būtina apdoroti** (dabar yra 3 binary_sensoriai — žr. lentelę; senas app kodas galėjo jų netvarkyti). `text_sensor_energija_junctek_raw_hex` — debug, galima ignoruoti.

> **Cache raktus (dešinė stulpelio pusė) privaloma sutikrinti su faktiniu app naudojimu** (`getCache(...)`, kortelių renderavimas). Dalis binary/naujų raktų čia preliminarūs (pvz. `shore_power_present`, `inverter_220_active`, `link_alarm`, `bedroom_co2_trend`, `junc_temp`) — jei app kortelė naudoja kitą raktą, naudoti app raktą, o ne šį.

**Žingsniai:**
1. Perrašyti `SENSOR_ID_MAP` naudojant realius slug'us (lentelė aukščiau) kaip raktus.
2. `startSSE`: palikti `number/text/switch/sensor` domeno atskyrimą ir **tik** `SENSOR_ID_MAP[stripped]` paiešką; **pašalinti** visą `else if (rawId.includes('...'))` grandinę.
3. Nerasti id → `debugLog('[SSE UNMATCHED] …')` (jau yra) — kad matytųsi, jei ką praleidom.

**PRIVALOMA verifikacija:** lentelė aukščiau surinkta iš 3 logų (2026-06-24/26/30), bet juose gali nebūti kai kurių jutiklių (pvz. `sms_1..5`, šviesų/jungiklių `switch`, retai keičiamų). Prieš baigiant — su įjungtu ESP užfiksuoti pilną SSE srautą ir įsitikinti, kad **kiekvienas** `[SSE]` id turi atitikmenį `SENSOR_ID_MAP`; nė viena kortelė nerodo „---". Rezultatą įrašyti į `docs/audits/`.

**Priėmimas:** visos kortelės rodo reikšmes kaip prieš tai; `startSSE` nebeturi `includes()` grandinės; verifikacija patvirtina 100% padengimą.

---

## 5. APK auto-diegimas su statuso patikra (vidutinė)

**Failas:** `MainActivity.java`, `UpdateBridge.downloadAndInstall` / `onReceive` (~421 eil.).

**Problema:** `onReceive` diegia netikrindamas `DownloadManager` statuso (kodas pats žymi „Simplification"). Nepavykęs atsisiuntimas gali bandyti diegti seną/dalinį failą.

**Sprendimas:** `onReceive` užklausti DM per `DownloadManager.Query`, tikrinti `COLUMN_STATUS == STATUS_SUCCESSFUL`, naudoti `COLUMN_LOCAL_URI` grąžintą kelią; kitaip — pranešti klaidą. (Nebūtina, bet rekomenduojama: tikrinti dydį/SHA256, jei `version.json` jį pateiktų.)

**Priėmimas:** nepavykus atsisiųsti — aiškus pranešimas, diegimas neinicijuojamas su senu failu.

---

## 6. Autostart per foreground servisą (žema)

**Failas:** `BootReceiver.java`.

**Problema:** po įkrovos paleidžia `MainActivity` per `startActivity` — Android 10+ riboja Activity paleidimą iš fono; autostart gali tyliai neveikti.

**Sprendimas:** `BootReceiver.onReceive`, kai `autostart_enabled`, paleisti **foreground servisą** vietoj Activity:

```java
if (autostart) {
    Intent i = new Intent(context, KemperisService.class);
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O)
        context.startForegroundService(i);
    else context.startService(i);
}
```
(Servisas jau `START_STICKY` ir rodo notifikaciją; iš jos vartotojas atidaro app.)

**Priėmimas:** po perkrovimo servisas pakyla be Activity; notifikacija matoma.

---

## 7. Versijos vienas šaltinis (žema, nebūtina)

Versija kartojama: `MainActivity.CURRENT_VERSION`, `build.gradle versionCode/Name`, `version.json`. Rekomenduojama generuoti `version.json` iš gradle arba bent aprašyti privalomą 3 vietų atnaujinimą prie release. (Šioje užduotyje — bent sinchronizuoti visus į 31.)

---

## Ko NEKEISTI šioje užduotyje
- **Google Apps Script** (`google-sheets/GoogleSheetsScript.js`) — **NELIESTI**. Taisome tik APK.
- **PIN apsauga** (`systemPIN`, `selectTab`, `changeSystemPIN`) — palikti kaip yra.
- **Firmware** (`firmware/kempervanas.yaml`) — neliesti.
- `android/www/index.html` — **neliesti** (4 punktas atidėtas; JS nekeičiamas v31).
- Tinklo bridge'ų (`KemperisNet`/`KemperisWifi`) konsolidacija — **atidėta** (tik pastaba audite; nekeisti, kad nesugadinti veikiančio tinklo režimo).

## Užbaigimas
1. `versionCode`/`versionName` → **31** (`variables.gradle` naudoja; keisti `app/build.gradle` `defaultConfig` arba `variables.gradle`), `MainActivity.CURRENT_VERSION = 31`, `version.json` → 31.
2. `cd android/android && ./gradlew assembleRelease`.
3. `app/build/outputs/apk/release/app-release.apk` → `android/kemperis_v31.apk`, `git add -f`.
4. Commit + push.
5. Testai Huawei planšetėje: išsaugojimo mygtukai kuria failus; balsas EN+LT (ar diegimo pasiūlymas); su ESP — visos kortelės rodo reikšmes.
