# Pilnas programėlės ir kodo auditas — 2026-07-01

**Apimtis:** Android app (`android/www/index.html` 3342 eil.; native Java 4 failai), firmware sąsaja (SSE object_id), Google Apps Script, manifestas, build konfigūracija.
**Tikslas:** įvertinti veikimą prieš perduodant v31 pataisymų užduotį asistentui; pateikti patobulinimų pasiūlymus.

---

## Santrauka pagal svarbą

| # | Radinys | Svarba | Ką paliesti |
|---|---------|--------|-------------|
| 1 | Failų išsaugojimas neveiks Android 10+ (scoped storage) | 🔴 Kritinė | `MainActivity.java` `KemperisFileBridge` |
| 2 | `saveTextFile` argumentų neatitikimas (jau užduotyje) | 🔴 Kritinė | `MainActivity.java` |
| 3 | TTS tyli be variklio/balso (jau užduotyje) | 🔴 Kritinė | `MainActivity.java` `KempTtsBridge` |
| 4 | SSE object_id atvaizdavimas trapus (didžiulė `includes()` grandinė) | 🟠 Aukšta | `index.html` `startSSE` + firmware |
| 5 | `onTtsDone` niekada nekviečiamas native pusėje | 🟠 Aukšta | `KempTtsBridge` |
| 6 | Apps Script be autentifikacijos ir be 30 d. valymo | 🟡 Vidutinė | `GoogleSheetsScript.js` |
| 7 | PIN atvira tekstine forma, numatyta „0000", tikrinama kliente | 🟡 Vidutinė | `index.html` |
| 8 | APK auto-diegimas be statuso/kontrolinės sumos patikros | 🟡 Vidutinė | `UpdateBridge` |
| 9 | Autostart per Activity paleidimą — Android 10+ blokuoja | 🟢 Žema | `BootReceiver.java` |
| 10 | Versija saugoma 3 vietose (rankinis sinchronizavimas) | 🟢 Žema | keli failai |
| 11 | Du persidengiantys tinklo bridge'ai (`KemperisWifi` / `KemperisNet`) | 🟢 Žema | `MainActivity.java` |

---

## Kritiniai radiniai

### 1. Failų išsaugojimas neveiks Android 10+ (scoped storage) — svarbiausia

`targetSdkVersion = 34` (Android 14). `KemperisFileBridge.saveTextFile` rašo tiesiai į viešą Downloads per `Environment.getExternalStoragePublicDirectory(DIRECTORY_DOWNLOADS)` + `FileOutputStream`. Nuo Android 10 (API 29) tai **blokuojama** (scoped storage) — direktinė `File` prieiga prie viešų aplankų meta `EACCES (Permission denied)`. `WRITE_EXTERNAL_STORAGE` manifeste turi `maxSdkVersion="28"`, todėl naujesnėse versijose leidimo išvis nėra.

**Pasekmė:** net **pataisius** argumentų klaidą (Pataisymas 1), išsaugojimas Huawei planšetėje (beveik tikrai Android 10+) toliau **neveiks** — tik klaida pasikeis iš „No such file or directory" į „Permission denied".

**Sprendimas:** `saveTextFile` perrašyti su **MediaStore** API 29+ atveju (reikalingi `import`'ai jau yra: `MediaStore`, `ContentValues`, `OutputStream`), o senesniems — palikti `File`:

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

> **Tai turi pakeisti Pataisymą 1 v31 užduotyje** — kitaip išsaugojimas planšetėje liks nepataisytas.

### 2. `saveTextFile` argumentų neatitikimas
Aprašyta `docs/v31_pataisymu_uzduotis.md` (Pataisymas 1). MediaStore variantas (aukščiau) šią klaidą taip pat išsprendžia — signatūra tampa `(name, mime, content)`, grąžina `"ok"`.

### 3. TTS tyli be variklio/balso
Aprašyta `docs/v31_pataisymu_uzduotis.md` (Pataisymas 2) su EN+LT tikrinimu. Šaknis: `KempTtsBridge` `ready=false`, kai planšetėje nėra TTS variklio/balso; `speak()` tyliai nieko nedaro.

---

## Aukštos svarbos radiniai

### 4. SSE object_id atvaizdavimas labai trapus
`startSSE` naudoja didelę, nuo eiliškumo priklausomą `else if (rawId.includes('...'))` grandinę su daugybe lietuviškų slug atsarginių variantų (pvz. `akum___tampa`, `akum_i_tampa`, `akum__tampa`…). Rizikos: dalinių sutapimų kolizijos (`temperat`, `slegis`), tylus reikšmės dingimas pervadinus jutiklį (tą pačią riziką mini ir `CLAUDE.md`). Deterministinis `SENSOR_ID_MAP` egzistuoja, bet grandinė vis tiek dominuoja.

**Pasiūlymas:** visiems jutikliams firmware'e nustatyti **stabilius `web_server` object ID** arba `id:`, ir palikti tik `SENSOR_ID_MAP` (deterministinį), pašalinant lietuviškų pavadinimų fallback'us. Tai labiausiai sumažintų ilgalaikį trapumą.

### 5. `onTtsDone` niekada nekviečiamas native pusėje
`speakNow` laukia `window.onTtsDone()` „tikslaus pabaigos signalo per UtteranceProgressListener", bet `KempTtsBridge.speak()` **nenustato** jokio `UtteranceProgressListener` (nors `MainActivity` jį importuoja, 30 eil.). Todėl pabaiga nustatoma tik pagal `text.length*100` ms laikmatį → netikslus garsumo atkūrimas ir galimas ilgų frazių nukirpimas.

**Pasiūlymas:** `KempTtsBridge` pridėti `tts.setOnUtteranceProgressListener(...)`, kuris `onDone` kviestų `webView.evaluateJavascript("window.onTtsDone && window.onTtsDone()")`, ir `speak()` perduoti `utteranceId`.

---

## Vidutinės svarbos radiniai

### 6. Apps Script — be autentifikacijos ir be retencijos
`doGet`/`doPost` neturi jokio rakto — bet kas su URL gali skaityti ir rašyti į lentelę. Script deployment ID įkomponuotas kliente (nuskaitomas iš APK). Be to, `CLAUDE.md` Etapas D numato „30 dienų langą (auto-delete)", bet **skripte tokios logikos nėra** — mėnesių lapai kaupiasi neribotai.

**Pasiūlymas:** (a) pridėti paprastą `token` parametrą tikrinamą prieš rašymą; (b) pridėti laikinį trigerį, kuris trina senesnius nei 30 d. įrašus / senus mėnesių lapus.

### 7. PIN saugumas
PIN saugomas atviru tekstu `localStorage`, numatytas `0000`, tikrinamas kliente per `prompt()` (`entry !== systemPIN`). Tinka „nuo atsitiktinio paspaudimo", bet nėra reali apsauga.

**Pasiūlymas:** aiškiai dokumentuoti, kad tai tik UI užraktas; jei reikia rimčiau — saugoti hash'ą ir riboti bandymus.

### 8. APK auto-diegimas be patikros
`UpdateBridge.downloadAndInstall` naudoja fiksuotą `kemperis_update.apk`, o `onReceive` diegia **netikrindamas** `DownloadManager` statuso (kodas pats komentuoja: „Simplification: In real app we query DM for actual path"). Nepavykęs/dalinis atsisiuntimas gali bandyti diegti seną failą.

**Pasiūlymas:** patikrinti `COLUMN_STATUS == STATUS_SUCCESSFUL`, naudoti DM grąžintą URI, (nebūtina) tikrinti dydį/SHA256 iš `version.json`.

---

## Žemos svarbos / šlifavimas

**9. Autostart.** `BootReceiver` per `startActivity` — Android 10+ riboja Activity paleidimą iš fono; autostart gali tyliai neveikti. Patikimiau — paleisti `KemperisService` (foreground) iš `BootReceiver`.

**10. Versijos šaltinis.** Versija kartojama 3 vietose: `MainActivity.CURRENT_VERSION`, `build.gradle versionCode`, `version.json`. Rankinio sinchronizavimo rizika — pravartu automatizuoti (pvz. generuoti `version.json` iš gradle).

**11. Tinklo bridge'ų dubliavimas.** `KemperisWifi` (WifiBridge: unbind/rebind) ir `KemperisNet` (KemperisNetBridge: setMode/httpGet) persidengia. Galima konsoliduoti į vieną.

**12. `usesCleartextTraffic="true"` + `MIXED_CONTENT_ALWAYS_ALLOW`** — būtina vietiniam ESP HTTP, priimtina, bet plati; jei norima, apriboti tik ESP host'ui per `network_security_config`.

**13. Testų nėra** — nei unit, nei instrumentinių. Bent SSE mapping ir `saveTextFile` verti padengti.

---

## Ką rekomenduoju įtraukti į v31 (prioritetas)

1. **Failų išsaugojimas per MediaStore** (radinys 1) — pakeisti Pataisymą 1. Be to išsaugojimas planšetėje liks sugedęs.
2. **TTS diagnostika EN+LT** (Pataisymas 2, jau paruošta).
3. **TTS `UtteranceProgressListener`** (radinys 5) — pigus, gerina patikimumą.
4. (Jei bus laiko) Apps Script token + 30 d. valymas (radinys 6); autostart per servisą (radinys 9).

Stabilių SSE object ID (radinys 4) migracija — atskiras, didesnis darbas; rekomenduoju planuoti kaip atskirą etapą, ne v31.

---

## Ką patikrinau
- Native: `MainActivity.java` (520 eil.), `KemperisService.java`, `BootReceiver.java`, `AndroidManifest.xml`, `variables.gradle`, `build.gradle` (signing, minify).
- Frontend: `index.html` funkcijų sąrašas, `startSSE` mapping, TTS grandinė (`speakText`→`_processSpeechQueue`→`speakNow`), PIN/`selectTab`, failų išsaugojimas, App Script ID.
- Debesis: `GoogleSheetsScript.js`, `version.json`.
- Git: švarus (`main` = `origin/main`), tik nauji `docs/` failai neįtraukti.
