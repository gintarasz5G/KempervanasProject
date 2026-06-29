# KemperisApp — gilus kodo ir veikimo auditas

**Data:** 2026-06-15
**Versija:** v14 (CURRENT_VERSION = 14, version.json)
**Apimtis:** `www/index.html` (2811 eil.), native Android sluoksnis (MainActivity.java, KemperisService.java), AndroidManifest.xml, Capacitor konfigūracija, build procesas, GoogleSheetsScript.js
**Metodas:** statinė kodo analizė (be kompiliavimo / dinaminio testavimo įrenginyje)

---

## 1. Architektūros santrauka

KemperisApp yra **Capacitor (v8) WebView programėlė** — visa logika sukoncentruota viename `www/index.html` faile (~2800 eil. JS). Native pusė pateikia 6 `@JavascriptInterface` tiltus:

| Tiltas | Paskirtis |
|--------|-----------|
| `KemperisVol` | Media garso lygio skaitymas/keitimas (AudioManager) |
| `KemperisWifi` | SSID skaitymas, WiFi pasiūlymas, tinklo pririšimas (`bindProcessToNetwork`) |
| `KempTts` | Native TextToSpeech su pabaigos signalu (UtteranceProgressListener) |
| `KemperisUpdate` | Versijos tikra (GitHub), APK parsisiuntimas + diegimas |
| `KemperisSms` | SMS siuntimas, savo SIM numerio nuskaitymas |
| `KemperisFile` | CSV/GPX/log išsaugojimas į Downloads (MediaStore) |

**Duomenų srautas:** ESP32 (ESPHome) → WiFi AP `Kemperis-Valdymas` → SSE (`/events`) → `sensorCache` → `updateUI()`. Kai ESP32 nepasiekiamas > 30 s → **offline režimas**, duomenys imami iš Google Sheets per Apps Script. Fone veikimą užtikrina native `KemperisService` (foreground, `specialUse` tipas) + `cordova-plugin-background-mode`, kad balso aliarmai (CO₂, vagystė) veiktų ir išjungus ekraną.

Bendras įspūdis: **funkciškai turtinga, apgalvota programėlė** su realiais offline / fono / tinklo perjungimo sprendimais. Kodas vienas failas, bet gerai komentuotas (lietuviškai). Pagrindinės problemos — saugumo ir vienas korektiškumo defektas, kurie aprašyti žemiau pagal prioritetą.

---

## 2. Kritiniai / aukšto prioriteto radiniai

### 🔴 H1. Pasirašymo raktas (`keystore/kemperis.jks`) yra git repozitorijoje
- **Vieta:** `KemperisApp/keystore/kemperis.jks` — patvirtinta `git ls-files` (failas sekamas versijavime).
- **Rizika:** `version.json` nurodo `github.com/gintarasz5G/kemperis-app`. Jei repozitorija **vieša**, release pasirašymo raktas yra prieinamas viešai. Kartu su automatinio atnaujinimo mechanizmu (programėlė pati parsisiunčia ir diegia APK iš GitHub), užpuolikas, turintis raktą + slaptažodį, gali pasirašyti kenkėjišką APK, kurį Android priimtų kaip teisėtą atnaujinimą.
- **Rekomendacija:** nedelsiant pašalinti `.jks` iš git istorijos (`git filter-repo` / BFG), pridėti `keystore/` į `.gitignore`, **sugeneruoti naują raktą jei repo buvo viešas**, slaptažodžius laikyti tik lokaliai / CI secret'uose. Patikrinti ar repozitorija nėra vieša.

### 🔴 H2. Dvigubas `speakText()` apibrėžimas — TTS eilė yra „mirusi" kodo dalis
- **Vieta:** `www/index.html:2479` ir `www/index.html:2618` — dvi funkcijos tuo pačiu pavadinimu.
- **Problema:** JS funkcijų deklaracijos „hoist'inamos" ir **vėlesnė perrašo ankstesnę**. Veikia tik antroji (2618) versija, kuri naudoja `_volBusy` ir **tiesiog atmeta** naują frazę (`if (_volBusy) return;`), jei TTS jau groja. Pirmoji versija (2479) su `_speechQueue` / `_processSpeechQueue()` (kuri turėjo užtikrinti, kad „vienas laukia kito") **niekada nevykdoma** — `_speechQueue` ir `_processSpeechQueue` yra negyvas kodas.
- **Pasekmė saugos požiūriu:** tai saugos stebėjimo programėlė. Jei vagystės aliarmas suveikia tuo metu, kai grojamas CO₂ įspėjimas (ar atvirkščiai), **antrasis aliarmas tyliai dingsta** — neištariamas ir neįeina į eilę. v14 release pastaba teigia, kad ši problema ištaisyta („vienas laukia kito"), tačiau realiai veikianti `speakText` versija to nedaro.
- **Rekomendacija:** pašalinti vieną iš dviejų apibrėžimų. Palikti **eilės** (queue) versiją (2479) ir ištrinti `_volBusy`-drop versiją (2618), kad aliarmai būtų rikiuojami, o ne metami.

### 🔴 H3. XSS per `innerHTML` — gaunamų SMS ir ESP32 log turinys neapsaugotas
- **Vietos:**
  - `www/index.html:1093` — SMS terminalas: `content += '<div class="sms-line">📩 ${msg}</div>'`, kur `msg = sensorCache['sms_1..5']` (gauto SMS tekstas).
  - `www/index.html:601` — `sysLog`: `out.innerHTML += '<div ...>[${now}] ${msg}</div>'` (ESP32 logai, klaidų pranešimai).
- **Problema:** turinys įterpiamas per `innerHTML` **be `escapeHtml()`**. Gaunamo SMS tekstą valdo išorinis siuntėjas (bet kas, žinantis kemperio SIM numerį). Kenkėjiškas SMS su HTML/`<img onerror=...>` gali įvykdyti JavaScript privilegijuotame WebView, kuriame pasiekiami galingi tiltai (`KemperisSms.send`, `KemperisFile.saveTextFile`, `KemperisUpdate.downloadAndInstall`). Tai realus nuotolinio kodo įvykdymo vektorius.
- **Rekomendacija:** visur, kur į `innerHTML` patenka ne statinis tekstas, naudoti jau egzistuojančią `escapeHtml()` funkciją (ji jau naudojama `renderAlarmHistory`). SMS atveju: `'📩 ' + escapeHtml(msg)`. `sysLog` atveju — escape'inti `msg`.

---

## 3. Vidutinio prioriteto radiniai

### 🟡 M1. Į kodą įrašyti asmens duomenys ir kredencialai
- **Vieta:** `init()` (`www/index.html:2050-2052`):
  - `_DEFAULT_SCRIPT_ID` (Google Apps Script ID),
  - `_DEFAULT_CAMPER_PHONE = '+37065758821'`,
  - `_DEFAULT_ADMIN_PHONE = '+37064730025'`.
  - Taip pat WiFi slaptažodis `'kemperis123'` (eil. 514).
- **Rizika:** APK yra išskleidžiamas (web turinys plain-text `assets/public/index.html`). Telefono numeriai = asmens duomenys; Apss Script ID yra atviras read/write galas (`doPost` rašo į Sheets be autentifikacijos — bet kas su ID gali rašyti/skaityti duomenis). WiFi slaptažodis viešas.
- **Rekomendacija:** numatytuosius numerius/ID perkelti į pirmojo paleidimo konfigūracijos formą (palikti tuščius). Apps Script gale pridėti bent token tikrinimą (`doPost`/`doGet`). WiFi slaptažodį priverstinai keisti pirmo nustatymo metu.

### 🟡 M2. PIN apsauga yra tik vizualinė
- **Vieta:** `selectTab()` (eil. 1272), `systemPIN` (eil. 1240).
- **Problema:** PIN saugo tik UI skirtukus (`settings`, `alarms`), **ne pačias ESP32 komandas**. `sendButton`, `toggleSecurity`, `setNumber` ir kt. nereikalauja PIN. Numatytasis PIN `0000`, laikomas atviru tekstu `localStorage`. `AndroidManifest` turi `allowBackup="true"` — PIN ir kiti `localStorage` raktai patenka į atsargines kopijas.
- **Rekomendacija:** jei PIN turi ką nors saugoti realiai — tikrinti jį prieš pavojingas komandas (apsaugos jungimą/išjungimą). `allowBackup="false"` jautriems duomenims. Nelaikyti PIN atviru tekstu.

### 🟡 M3. SMS arm/disarm autentifikacijos spragos
- **Vieta:** `sendOfflineSms()` (eil. 1981-1998) ir `onSmsReceived()` (eil. 2020-2046).
- **Problemos:**
  1. Jei `getOwnPhoneNumber()` grąžina tuščią (dažna — operatoriai nebeteikia Line1Number), arm/disarm **siunčiama be patikros** (tik su įspėjimu logo).
  2. `onSmsReceived` keičia UI apsaugos būseną (`sw_security_armed`) pagal `bl.includes('ijungta'/'isjungta')` iš **bet kurio siuntėjo** — galima suklastoti UI būseną atsiuntus SMS su tais žodžiais. (Pati ESP apsauga nepaveikiama, tik programėlės rodmuo.)
- **Rekomendacija:** atmesti arm/disarm, jei savo numerio nepavyksta patikrinti (fail-closed). `onSmsReceived` keisti būseną tik jei siuntėjas = kemperio SIM numeris (`camper_phone`).

### 🟡 M4. Cleartext HTTP įjungtas globaliai
- **Vieta:** `AndroidManifest` `usesCleartextTraffic="true"`; `MainActivity` `MIXED_CONTENT_ALWAYS_ALLOW`; `capacitor.config.ts` `cleartext: true`.
- **Vertinimas:** **pateisinama** ESP32 lokaliam ryšiui (192.168.4.1, HTTP-only). Tačiau leidimas yra visuotinis — bet koks cleartext srautas leidžiamas.
- **Rekomendacija:** susiaurinti per `network_security_config.xml` — leisti cleartext tik `192.168.4.1` / `192.168.x.x` poaibiui, likusiam srautui reikalauti HTTPS.

### 🟡 M5. Automatinio atnaujinimo grandinė
- **Vieta:** `UpdateBridge.downloadAndInstall` (MainActivity), `REQUEST_INSTALL_PACKAGES`.
- **Vertinimas:** `version.json` ir APK imami per **HTTPS** (gerai). Tačiau nėra papildomo APK kontrolinės sumos / parašo tikrinimo programėlės pusėje — pasikliaujama tik OS parašo tikrinimu diegimo metu. Kartu su **H1** (raktas repo) tai tampa realia rizika.
- **Rekomendacija:** išspręsti H1; papildomai galima tikrinti APK SHA-256 prieš diegiant (įdėti į `version.json`).

---

## 4. Žemo prioriteto / kokybės radiniai

- **L1. `localStorage` rašymas kiekvienos SSE žinutės metu.** `updateUI()` (kviečiama kas SSE `state` įvykį, gali būti kelis kartus per sekundę) pabaigoje rašo `localStorage.setItem('_sensorCache', ...)` (eil. 1781), o `checkAlarms` rašo `_alarmFlags`/`_alarmLastTriggered`. Tai našumo ir flash dėvėjimo našta. **Rekomendacija:** rašyti su debounce (pvz., kas 5–10 s), o ne kiekvieną žinutę.

- **L2. Trapus SSE pavadinimų atitikimas.** Didžiulė `if/else if` grandinė (eil. 2161-2229) remiasi `rawId.includes('...')` su NFKD-slugifikuotais lietuviškais pavadinimais (pvz. `'altinis'`, `'esp_temp'`, `'galileo'`). Plataus „includes" gali klaidingai suderinti. Jau yra apsauga per `SENSOR_ID_MAP` (deterministinis) ir tvarką, bet grandinė sunkiai prižiūrima. **Rekomendacija:** ilgainiui visiems ESP entity priskirti aiškius `id:` ESPHome konfigūracijoje ir atsisakyti pavadinimų-spėjimo grandinės.

- **L3. Tušti `www/lib/leaflet.js` ir `leaflet.css` (0 baitų).** Leaflet realiai kraunamas iš `unpkg.com` CDN (eil. 8, su SRI integrity — gerai). Lokalūs failai klaidina ir reiškia, kad žemėlapis **neveikia be interneto** (kodas tai korektiškai apdoroja `if (typeof L === 'undefined')`). **Rekomendacija:** arba įdėti realų lokalų Leaflet (offline žemėlapiui), arba pašalinti tuščius failus.

- **L4. `version.json` `notes` lauko koduotės klaidos (mojibake).** Tekste matyti `??` ir `�` vietoj lietuviškų raidžių/emoji — atnaujinimo dialoge vartotojas matys sugadintus simbolius. **Rekomendacija:** išsaugoti `version.json` UTF-8 be sugadintų simbolių.

- **L5. Tinklo atrišimo/pririšimo „žonglieravimas".** `_fetchSheetsNow`, `checkInternetAndEnableUpload`, `uploadLocalDataToGoogle` naudoja `netUnbindForInternet()` → delay → `netRebindEsp()` seką, kad pasiektų Google, o paskui grįžtų prie ESP. Logika apsaugota `try/catch` ir delays, bet **lenktynių (race) jautri** — keli vienu metu vykstantys ciklai gali konfliktuoti dėl `bindProcessToNetwork`. Kol kas valdoma `_offlineSheetsPromise` sekos garantu. **Rekomendacija:** įvesti vieną „tinklo užimtumo" mutex'ą.

- **L6. Nėra automatinių testų.** Tik Capacitor šablonai (`ExampleUnitTest`, `ExampleInstrumentedTest`). Saugos kritinei programėlei verta padengti bent aliarmų logiką (`checkAlarms`, `checkTheftVoice`, `getMoonAge`, niveliavimo skaičiavimus) unit testais (logiką galima iškelti į atskirą JS modulį).

- **L7. Repozitorijos higiena.** Darbo aplankas ~153 MB; daug APK kopijų (`kemperis_v1..v14`, ~50 MB) ir Android build artefaktų. `android/app/build` ir `node_modules` nesekami git (gerai), bet darbo aplanke kaupiasi. **Rekomendacija:** APK leidimus laikyti tik GitHub Releases, valyti senas kopijas.

---

## 5. Kas padaryta gerai

- Apgalvotas **offline režimas** su Google Sheets fallback ir aiškiu UI pilkinimu (`data-offline-dim`, `data-esp-btn` disable).
- **SSE watchdog** (30 s tyla → perjungimas) ir senos `EventSource` uždarymas prieš naują (eil. 2128) — taisyklingai sutvarkytas dvigubo ryšio (#8) defektas.
- Native `onDestroy` **švarus išteklių atlaisvinimas** (callback'ai, receiver'iai, TTS shutdown).
- Aliarmų **snooze su auto-atstatymu** ir būsenos išsaugojimu per app perkrovimą (`localStorage`).
- TTS pabaigos signalas per `UtteranceProgressListener` (tikslesnis nei laiko spėjimas) su fallback laikmačiu.
- Android 14 reikalavimų laikymasis: `FOREGROUND_SERVICE_SPECIAL_USE`, `RECEIVER_EXPORTED` su komentaru-pagrindimu, `startForeground` su tipu.
- `escapeHtml` jau egzistuoja ir naudojama aliarmų istorijoje (tik trūksta pritaikyti SMS/log vietose — žr. H3).

---

## 6. Prioritetų santrauka

| Nr. | Radinys | Prioritetas | Pastangos |
|-----|---------|-------------|-----------|
| H1 | Pasirašymo raktas git repo | 🔴 Kritinis | Maža (pašalinti + regeneruoti) |
| H2 | Dvigubas `speakText` — aliarmai dingsta | 🔴 Aukštas | Maža (ištrinti vieną apibr.) |
| H3 | XSS per innerHTML (SMS, log) | 🔴 Aukštas | Maža (escapeHtml) |
| M1 | Įkoduoti tel. numeriai / Script ID / WiFi pass | 🟡 Vidutinis | Vidutinė |
| M2 | PIN tik vizualinis | 🟡 Vidutinis | Vidutinė |
| M3 | SMS arm/disarm autentifikacija | 🟡 Vidutinis | Maža |
| M4 | Globalus cleartext HTTP | 🟡 Vidutinis | Maža (network config) |
| M5 | APK atnaujinimo parašo tikrinimas | 🟡 Vidutinis | Vidutinė |
| L1–L7 | Našumas, kokybė, higiena | 🟢 Žemas | Įvairi |

**Rekomenduojama pradėti nuo H1–H3** — visi trys yra maža apimties, bet didelio poveikio pataisymai, o H2 ir H3 liečia tiesiogiai saugos funkcionalumą.
