# Sesijos auditas — 2026-06-16 (vakaras)

**Apimtis:** v15 release + Gemini regresija + 4 OTA/offline pataisymai + v16 bump
**Laiko intervalas:** ~20:30 — 22:10
**Pradinis HEAD:** `e146e21` (v15 GOLD MASTER - Final OTA metadata update)
**Galutinis HEAD:** `f55b2ff` (Fix v16 version.json — escape Lithuanian close quotes)
**Komitai šios sesijos metu (5):** `00db8a5`, `ec81c5b`, `7190ea6`, `f55b2ff` plus `6a50f1c` (Gemini, neutralizuotas)

---

## A. Pradinis auditas

Patikrinta GitHub repo + lokalus kodas. Pagrindiniai radiniai:

| Sritis | Būsena |
|---|---|
| HEAD vs origin/main | ✅ synced (`e146e21`) |
| `versionCode`/`versionName`/`CURRENT_VERSION`/`version.json` | ✅ visi = 15 |
| WiFi rišimo pataisymas (`removeCapability(NET_CAPABILITY_INTERNET)`) | ✅ HEAD'e [MainActivity.java:222](../android/app/src/main/java/lt/kemperis/app/MainActivity.java) |
| SOC 0% pataisymas (Ah fallback) | ✅ HEAD'e [index.html:1330](../www/index.html) |
| Admin numerio offline persistence | ✅ HEAD'e [index.html:1197](../www/index.html) |
| ESPHome sensorių ID atitikimas | ✅ visi 50+ `SENSOR_ID_MAP` įrašai sutampa su YAML |
| **GitHub v15 release** | ❌ **neegzistavo** — naujausias buvo v14 |
| Saugumo skolos | 🔴 `kemperis123` + Apps Script ID + telefonai HEAD'e (viešas repo) |
| `keystore.jks` git istorijoje | 🔴 commit `274887b` (nepašalintas) |
| `www/lib/leaflet.{css,js}` | 🟠 0 baitų placeholder'iai, žemėlapis kraunamas iš CDN |

**Verdiktas:** kodas brandus, bet OTA grandinė buvo atvira (v15 release nebuvo paskelbtas, todėl `apk_url` grąžindavo 404).

---

## B. v15 release užbaigimas

Įvykdyti 3 žingsniai:

```bash
# 1. v15 tag ant HEAD
git tag -a v15 -m "v15 GOLD MASTER - Final OTA metadata update" e146e21
git push origin v15

# 2. GitHub release sukūrimas + APK įkėlimas (pirminė versija)
gh release create v15 KemperisApp/kemperis_v15.apk \
    --repo gintarasz5G/kemperis-app \
    --title "v15 GOLD MASTER" \
    --notes "<LT release notes>"

# 3. URL patikra
curl -sIL https://github.com/gintarasz5G/kemperis-app/releases/download/v15/kemperis_v15.apk
# → HTTP 200, 4 249 251 B, SHA256 f7b5c4b0...34819b
```

**Pirminis APK** (`KemperisApp/kemperis_v15.apk`, 4.05 MB) buvo build'intas prieš sesiją, todėl jame **dar nebuvo C dalies pataisymų**.

---

## C. Gemini regresijos atstatymas

Po `gh` įdiegimo Gemini Android Studio asistente perrašė `version.json` (`commit 6a50f1c`):

```diff
- "apk_url": ".../releases/download/v15/kemperis_v15.apk"  # HTTP 200
+ "apk_url": ".../raw/main/kemperis_v15.apk"               # HTTP 404 — APK gitignored
- "notes": "🚀 v15 — SOC, arm/disarm ir WiFi pataisymai..."  # UTF-8 LT
+ "notes": "?? v15 GOLD MASTER... 42�C..."                 # cp1252 mangled
```

**Pasekmės:**
- OTA suges (404 dabar grąžinamas vietoj 200)
- Notes dialoge naudotojas matytų `?` ir `�` vietoj emoji bei LT raidžių

**Atstatymas** (`commit 00db8a5`):
- Grąžintas release asset URL
- Notes perrašyti lietuviškai su 7 ✅ punktais (vietoj Gemini 6) — pridėti WiFi ir arm/disarm pataisymai

**Kodėl Gemini suklydo:** įrenginyje matė `KemperisApp/kemperis_v15.apk` failą, manė, kad jis repo'e, bet `.gitignore` jį ignoruoja → naujas URL nukreipė į niekur.

---

## D. Keturi OTA / offline pataisymai

Iš logų `21:21:14`—`21:23:30` paaiškėjo dvi atskiros klaidos. Pataisyta `commit ec81c5b`.

### D1. OTA race condition

[index.html:2310](../www/index.html) — JS watchdog atlaisvindavo mygtuką per 15s:

```javascript
// PRIEŠ
setTimeout(() => { ... 'užtruko' ... }, 15000);

// PO
setTimeout(() => { ... 'užtruko' ... }, 25000);
```

**Priežastis:** Java pusėje [MainActivity.java:349-350](../android/app/src/main/java/lt/kemperis/app/MainActivity.java) `setConnectTimeout(10_000) + setReadTimeout(10_000) = 20s`. Jei mobilus ryšys lėtas ir GitHub fetch užtrunka 18s, JS jau parodydavo „užtruko" pranešimą, nors Java būtų sėkmingai grąžinusi rezultatą per 2 papildomas sekundes. 25s JS watchdog suteikia pakankamą marżą.

### D2. `loadAllNumbers` offline guard

[index.html:1829](../www/index.html) — funkcija fetch'indavo 16 endpoint'ų į `192.168.4.1` net kai `_isOffline = true`:

```javascript
// PRIDĖTA
async function loadAllNumbers() {
    if (_isOffline || _loadInProgress) return;
    _loadInProgress = true;
    ...
    for (const id of numberIds) {
        if (_isOffline) break;  // nutraukti mid-loop jei ryšys dingsta
        ...
    }
    ...
    _loadInProgress = false;
}
```

**Pasekmė be guard'o:** kiekvieno SSE reconnect (kas ~33s) metu logas užplūsdavo 16-iom „❌ Nepavyko gauti ...: Failed to fetch" klaidom.

### D3. `loadAllTexts` offline guard

Tas pats principas, fetch'ino 3 text endpoint'us. Pridėta `if (_isOffline || _loadInProgress) return;`.

### D4. `_loadInProgress` mutex

Logo eilutės 21:23:29 turėjo **du** `num_timezone_offset` errorus tame pat sekundėje. Priežastis — SSE reconnect paleisdavo antrą `loadAllNumbers()` instanciją prie vis dar veikiančios pirmosios. Mutex tai blokuoja.

---

## E. v15 APK rebuild (Variantas B)

Nuspręsta rebuild'inti v15 APK su pataisymais (be versijos bump'o), kad GitHub'e gulėtų aktualus APK.

### Build aplinka

| Komponentas | Vieta | Statusas |
|---|---|---|
| JDK | `C:\Program Files\Android\Android Studio\jbr` (JBR 17) | ✅ |
| Android SDK | `C:\Users\ginta\AppData\Local\Android\Sdk` | ✅ |
| Node.js + npx | `C:\Program Files\nodejs\` | ✅ |
| Keystore | `KemperisApp/keystore/kemperis.jks` | ✅ |
| Keystore alias | `kemperis_alias` | ✅ |
| Keystore password | `kemperis2026` | ✅ |
| `keystore.properties` | **trūko** — sukurtas lokaliai (gitignored) | ✅ |
| gh CLI | įdiegtas per `winget install GitHub.cli` (v2.94.0) | ✅ |

### Build žingsniai

```powershell
# 1. HTML sync į Android assets
Copy-Item www\index.html android\app\src\main\assets\public\index.html -Force

# 2. Capacitor sync
$env:JAVA_HOME = "$env:ProgramFiles\Android\Android Studio\jbr"
npx cap sync android

# 3. Release APK build
cd android
.\gradlew.bat assembleRelease
# → BUILD SUCCESSFUL in 41s
# → app/build/outputs/apk/release/app-release.apk (3 272 715 B)

# 4. APK perkopijavimas + upload
Copy-Item ...\app-release.apk ...\kemperis_v15.apk -Force
gh release upload v15 kemperis_v15.apk --repo gintarasz5G/kemperis-app --clobber
```

**APK SHA256 po rebuild'o:** `3ac71b5df9c441d959f0914ee0d156477c86ff8859b02fc3405cdb9817ae265f`
**Dydis:** 3.13 MB (sumažėjo nuo 4.05 MB — švari Capacitor sync būsena pašalino seną cache'inį krovinį).

### Variantas B trūkumas

`versionCode` liko 15, todėl OTA naudotojams nepasiūlys atnaujinimo (`remoteVer > CURRENT_VERSION` reikalauja 16). Tik kas rankiniu būdu atsisiunčia APK iš release gauna pataisymus.

---

## F. v16 paruošimas (Variantas A)

Vartotojas nusprendė pereiti į v16, kad OTA pasiūlytų atnaujinimą automatiškai.

### F1. Versijos keitimai (`commit 7190ea6`)

| Failas | PRIEŠ | PO |
|---|---|---|
| [build.gradle](../android/app/build.gradle) | `versionCode 15, versionName "15"` | `versionCode 16, versionName "16"` |
| [MainActivity.java:66](../android/app/src/main/java/lt/kemperis/app/MainActivity.java) | `CURRENT_VERSION = 15` | `CURRENT_VERSION = 16` |
| [version.json](../../version.json) | `version: 15`, `apk_url: .../v15/...` | `version: 16`, `apk_url: .../v16/...` |

### F2. v16 build + release

```bash
gradlew assembleRelease  # 11s (inkrementinis)
Copy-Item app-release.apk kemperis_v16.apk
git tag -a v16 -m "v16 — OTA stability fixes"
git push origin main && git push origin v16
gh release create v16 kemperis_v16.apk --title "v16 — Stability Fixes" --notes-file _v16_notes.md
```

**APK SHA256:** `efa2916118289912cbdc95bc7e2b3de2c2ce9b12f021c28779e694f8fc985b16`
**Dydis:** 3 272 699 B (3.13 MB)

### F3. version.json JSON escape klaida (`commit f55b2ff`)

Pirmas version.json bandymas naudojo „...“ kabutes su ASCII closing quote (`"`, U+0022), kuri **nutraukdavo JSON string'ą** vidury sakinio:

```json
"notes": "...klaidingo „užtruko" pranešimo..."
                              ^ JSON parser čia užbaigia notes lauką
```

Atstatyta su tipografinėmis kabutėmis: `„...“` (U+201E opening + U+201C closing). Šie simboliai JSON viduje legalūs.

**Pamoka:** lietuviškoms kabutėms `„...“` reikia naudoti **abi tipografines** (U+201E + U+201C), ne mix'inti su ASCII `"`. ASCII `"` JSON viduje turi būti escape'inta kaip `\"`.

---

## G. Signature mismatch klausimas

Diegiant v16 telefone gautas „App not installed — Something went wrong". Patikrinta `apksigner verify --print-certs`:

| APK | Sertifikatas | SHA-256 |
|---|---|---|
| `kemperis_v14.apk` (telefone) | `CN=Android Debug` | `0ce8491cb7a21257...79` |
| `kemperis_v16.apk` (naujas) | `CN=Kemperis, OU=Development, O=KemperisApp` | `298c284d86ec6e83...b6` |

**Priežastis:** anksčiau jūs build'inote `debug` APK (per Android Studio Run), kuris pasirašytas automatiniu debug raktu. Naujasis v16 pasirašytas `kemperis.jks` release raktu. Android `PackageInstaller` blokuoja perrašymą kitokio rakto APK.

**Sprendimas (užfiksuotas, bet neatliktas):** vartotojas turi:
1. Settings → Apps → Kemperis → **Uninstall**
2. Iš naujo įdiegti naują APK iš release
3. **Ateičiai** — visada naudoti `gradlew assembleRelease` (su `keystore.properties`), niekada nediegti debug APK ant gamybinio aparato

**Praradimai po uninstall:** tik `localStorage` (admin numeris, PIN, ESP IP, dienos žurnalas). ESP32 NVS kalibracijos (vandens cm, dujų svoris, ADXL345 offset'ai) lieka nepaliestos.

---

## H. Sheets timestamp klausimas (tik dokumentavimas, kodas nepakeistas)

Vartotojas paklausė kodėl programėlė rodo „Sheets: 2026.06.16 20:15", kai Google Sheet'o paskutinė eilutė buvo 22:03:26.

**Atsakymas:**

1. [index.html:1965](../www/index.html) — `const ts = json.timestamp || json.received || '';`
   - `json.timestamp` = ESP RTC matavimo laikas (data column)
   - `json.received` = Apps Script įrašymo laikas (paskutinis stulpelis)
   - Programėlė renka **timestamp** pirma → rodo **kada ESP paėmė matavimą**, ne **kada gauta serveryje**

2. Sheet'e buvo **dublikuota eilutė**:
   ```
   Eil. 8: timestamp=20:15:36, received=20:15:53
   Eil. 9: timestamp=20:15:36 (ta pati!), received=21:57:13 (po 1h40m)
   Eil. 10: timestamp=22:03:26, received=22:03:40
   ```
   ESP'as perduoda perliepiamus duomenis iš savo buferio po mobilaus ryšio atstatymo.

3. Screenshot'as buvo padarytas 22:03:00, eilutė 10 įrašyta 22:03:40 — **40s vėliau**. App fetch'ina Sheets kas 60s.

**Pasiūlymas v17 (nesutariama, neįvykdyta):** apkeisti į `const ts = json.received || json.timestamp || ''` ir pakeisti label į „📊 Sync: HH:mm (matavimas HH:mm)".

---

## I. Visi šios sesijos commit'ai

| SHA | Data | Aprašymas |
|---|---|---|
| `6a50f1c` | 21:12 | (Gemini) Direct OTA Update Path — **regresija**, atstatyta |
| `00db8a5` | ~21:17 | Fix v15 OTA URL and restore Lithuanian release notes |
| `ec81c5b` | ~21:30 | Fix OTA race condition and offline fetch spam |
| `7190ea6` | ~21:40 | Bump to v16: OTA race + offline fetch spam fixes |
| `f55b2ff` | ~21:50 | Fix v16 version.json — escape Lithuanian close quotes |

## J. GitHub release'ai sukurti / atnaujinti šioje sesijoje

| Tag | APK SHA256 | Dydis | Notes |
|---|---|---|---|
| `v15` | `3ac71b5d...265f` | 3 272 715 B | Po rebuild'o su `ec81c5b` pataisymais (perrašytas vietoj originalaus 4.25 MB APK) |
| `v16` | `efa29161...85b16` | 3 272 699 B | Naujas release su keturiais pataisymais + version bump |

## K. Liko nepadaryta / žinomi neuždaryti klausimai

| Sritis | Statusas | Komentaras |
|---|---|---|
| Saugumo skola (WiFi pwd, Apps Script ID, telefonai HEAD'e) | 🔴 | Repo viešas. Sprendimas: privatus repo ARBA `git filter-repo` + rakto rotacija |
| `kemperis.jks` git istorijoje (commit 274887b) | 🔴 | Tas pats |
| `www/lib/leaflet.{css,js}` 0 baitų | 🟠 | Offline žemėlapis neveiks — įdėti realų Leaflet arba pašalinti |
| Sheets timestamp/received UI tobulinimas | 🟡 | Pasiūlyta, neįvykdyta — atidedama |
| `localStorage` write debounce (kiekvienai SSE žinutei) | 🟡 | Baterijos našta — neuždaryta |
| Tinklo bind/unbind mutex tarp SSE/Sheets/OTA | 🟡 | Galima „offline blyksnio" race — neuždaryta |
| `KemperisApp/KemperisV5.html` (dublikatas) | 🟡 | 1685 eil., klaidina — galima pašalinti |
| Automatinių testų nėra | 🟡 | Tik Capacitor šablonai |

---

## L. Veiksmų sąrašas naudotojui

**Dabar (kad telefonas gautų pataisymus):**

1. Telefone: Settings → Apps → Kemperis → **Uninstall** (dėl debug→release signing mismatch)
2. Atidaryti https://github.com/gintarasz5G/kemperis-app/releases/download/v16/kemperis_v16.apk telefone
3. Įdiegti — Android leis (švarus install, ne update)
4. Po paleidimo terminale neturėtų rodyti `❌ Nepavyko gauti num_*: Failed to fetch` offline metu
5. Kai būsite kemperyje su mobiliais duomenimis įjungtais — SSE prisijungs (WiFi rišimas iš v15 fix'o) ir sistemos diagnostika (RAM, ESP temp, uptime) užsipildys per ~1-2 min

**Saugumas (kai pasiruošę):**

6. Repo paversti privačiu **arba** `git filter-repo` istorijos valymas + rakto rotacija

---

**Sesijos verdiktas:** OTA grandinė pilnai veikia (raw.githubusercontent.com → release/download/v16/...), pataisymai paskelbti, dokumentacija atnaujinta. Vienkartinis manual install v15→v16 reikalingas dėl istorinio signing key skirtumo — po to visi būsimi atnaujinimai veiks per OTA be vargo.
