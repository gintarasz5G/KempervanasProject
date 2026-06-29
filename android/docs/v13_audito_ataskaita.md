# 🔍 KemperisApp v13 — Pilnas Audito Pranešimas
**Data:** 2026-06-15  
**Audituotas katalogas:** `C:\Users\ginta\kempervanas\veikiantisKEMPERIS\KemperisApp`  
**Metodas:** tiesioginiai failų skaitymai + YAML kryžminė patikra (be kešuotos informacijos)

---

## 1. 📁 Projekto struktūra ir failai

| Failas | Dydis | Paskutinė modifikacija | Būklė |
|--------|-------|----------------------|-------|
| `www/index.html` | 228 087 B | 2026-06-15 13:11 | ✅ Pilnas (2811 eilučių) |
| `android/app/src/main/assets/public/index.html` | 228 087 B | 2026-06-15 13:27 | ✅ Sinchronizuotas |
| `android/app/src/main/java/lt/kemperis/app/MainActivity.java` | 37 697 B | 2026-06-15 11:39 | ✅ |
| `android/app/src/main/java/lt/kemperis/app/KemperisService.java` | 3 514 B | 2026-06-10 | ✅ |
| `android/app/build.gradle` | 2 410 B | 2026-06-12 | ✅ versionCode 13 |
| `android/app/src/main/AndroidManifest.xml` | 4 369 B | 2026-06-15 11:39 | ✅ |
| `version.json` | 745 B | 2026-06-15 12:26 | ✅ v13, tinkamas apk_url |
| `kemperis_v13.apk` | 4 247 211 B | 2026-06-15 13:36 | ⚠️ **NEPERSTATYTAS** |
| `android/.../app-release.apk` | 3 268 038 B | 2026-06-13 | ✅ Paskutinis Gradle build |
| `kempervanas_v22veikiantis .yaml` | 108 498 B | — | ✅ ESPHome YAML |

---

## 2. 🧠 index.html — JS logikos auditas

### 2.1 Pagrindiniai kintamieji (L510-590)

| Kintamasis | Vertė / Aprašas |
|---|---|
| `ESP_IP` | `'192.168.4.1'` |
| `ESPHOME_URL` | `'http://192.168.4.1'` |
| `ESP_WIFI_SSID` | `'Kemperis-Valdymas'` |
| `ESP_WIFI_PASS` | `'kemperis123'` |
| `sensorCache` | Iš localStorage su try/catch ✅ |
| `SENSOR_ID_MAP` | 58 įrašai ✅ |
| `DAILY_LOG_KEY` | `'kemper_daily_log'` |
| `MAX_LOG_ROWS` | `4000` |
| `PIN_PROTECTED` | `['settings','alarms']` |
| `PIN_UNLOCK_MS` | `300000` (5 min) |
| `SWIPE_TABS` | 8 skirtukų, neįtraukiant PIN apsaugotų ✅ |
| `_isOffline` | Deklaruota L1788 ✅ |
| `_offlineSheetsPromise` | Deklaruota L1790 ✅ |

### 2.2 Funkcijų sąrašas (99 deklaracijos + 8 arrow fn)

Visos 99 funkcijos rasta ir audituotos:

| Grupė | Funkcijos | Būklė |
|---|---|---|
| Init / SSE | `init()`, `startSSE()` | ✅ |
| UI atnaujinimas | `updateUI()`, `updateLocalLogCounter()`, `setInputValue()`, `escapeHtml()` | ✅ |
| Dienos žurnalas | `startAutoLocalLog()`, `stopAutoLocalLog()`, `addLocalLogRow()`, `downloadLocalCSV()`, `downloadGPX()`, `saveTerminal()` | ✅ |
| Sensorių prieiga | `getCache()`, `getBlock()` | ✅ |
| Aliarmai | `checkAlarms()`, `checkTheftVoice()`, `renderAlarmSettings()`, `loadAlarmConfig()`, `saveAlarmConfig()`, `updateAlarmMessage()`, `updateAlarmThreshold()` | ✅ |
| TTS / garso | `speakText()`, `speakNow()`, `playChime()`, `nativeVolGet()`, `nativeVolSet()`, `getTtsVolPct()` | ✅ |
| Offline / Sheets | `setOfflineMode()`, `_fetchSheetsNow()`, `netUnbindForInternet()` | ✅ |
| WiFi tiltas | `getCurrentSSID()`, `suggestWifi()`, `openWifiSettings()` | ✅ |
| Nustatymai | `setNumber()`, `setText()`, `loadAllNumbers()`, `loadAllTexts()` | ✅ |
| PIN | `unlockSettings()`, `lockSettings()`, `navigateToTab()` | ✅ |
| SMS | `sendSms()`, `sendOfflineSms()`, `sendButton()` | ✅ |
| Atnaujinimas | `checkUpdate()`, `onUpdateResult()` | ✅ |
| GPS / Žemėlapis | `initMap()`, `updateMapMarker()` | ✅ |
| Energija | `_loadEnergyState()`, `_saveEnergyState()` | ✅ |

### 2.3 init() ir startSSE() — logikos grandinė

```
DOMContentLoaded (L2789)
  → _loadEnergyState()
  → renderAlarmSettings()
  → init() (L1992)
      → getCurrentSSID() — patikrina WiFi
      → startAutoLocalLog() — paleidžia 10s intervalą
      → _offlineSheetsPromise = _fetchSheetsNow()
      → _offlineSheetsPromise.finally → startSSE()
          → EventSource(ESPHOME_URL/events)
          → onmessage: SENSOR_ID_MAP + NFKD fallback → sensorCache → updateUI()
          → onopen: status-dot ← online
          → onerror: status-dot ← offline
          → ping → _lastSseMsg reset + setOfflineMode(false)
          → log → sysLog('[ESP32]...')
          → watchdog (15s polling): jei >30s be žinučių → perkuria SSE
          → loadAllNumbers() + loadAllTexts() (REST GET)
```

**Tvarka: ✅ logiška, be race condition**

### 2.4 addLocalLogRow() — adaptyvus fiksavimas

- Greitis > 3 km/h → 10s intervalas; stovint → 60s ✅
- 23:59 auto-išsaugojimas į Downloads ✅
- FIFO: `localLogRows.shift()` kai > MAX_LOG_ROWS ✅
- `localStorage` sinchronizuojamas po kiekvieno įrašo ✅

### 2.5 speakText() — TTS su garso valdymu

- Prieš kalbant: išsaugo esamą garsumą → padidina iki nustatyto pct% → `_volBusy = true`
- Po kalbant (`finally`): grąžina buvusį garsumą → `_volBusy = false`
- `_volBusy` sąlygoja lygiagretumo apsaugą ✅

### 2.6 checkAlarms() — aliarmų logika

- Visi aliarmai iteruojami per `ALARM_CONFIG` (dinamiškai pakraunama)
- `theft_alarm` — specialus, tvarkymas per `checkTheftVoice()`
- Pakartojimas: `repeatInterval` > 0 ir praėjo pakankamai laiko
- Snooze: `alarmSnooze[id] > now` blokuoja balsą, bet ne vėliavą
- Aliarmų būsena išsaugoma `localStorage` ✅

### 2.7 _fetchSheetsNow() — Sheets offline

- Palaukia 600ms (kol SSE nusistovi), tada tikrina `_isOffline`
- Jei online — atsijungia be fetch ✅
- `netUnbindForInternet()` → 300ms → fetch Google Script → rebind ✅
- `autoBindPaused` flagas sinchronizuoja su Java puse ✅

### 2.8 Rastos smulkios pastabos

| Vieta | Aprašas | Poveikis |
|---|---|---|
| L2093 | `} else if` 17 tarpų vietoj 12 | Tik kosmetinis, JS ignoruoja ✅ |
| `water_empty_cm` SENSOR_ID_MAP | Gali sutapti tik per NFKD fallback (ne fast path) | Nekenkia — UI skaito `num_water_empty_cm` per REST ✅ |
| `sistema___esp_temperat_ra` ir 2 variantai | NFKD redundancija sys_esp_temp | Nekenkia — fast path `sys_esp_temp` veikia ✅ |

---

## 3. ☕ Android / Java auditas

### 3.1 MainActivity.java

| Elementas | Vertė | Būklė |
|---|---|---|
| `CURRENT_VERSION` | `13` (L66) | ✅ |
| `VERSION_JSON_URL` | `https://raw.githubusercontent.com/gintarasz5G/kemperis-app/main/version.json` | ✅ |
| Bridges registruoti | KemperisVol, KemperisWifi, KempTts, KemperisUpdate, KemperisSms, KemperisFile | ✅ 6/6 |
| `autoBindPaused` | `volatile boolean` (L69) | ✅ thread-safe |

### 3.2 Bridges → JS tilteliai (pilna kryžminė patikra)

| JS naudojimas | Java bridge | @JavascriptInterface | Būklė |
|---|---|---|---|
| `KemperisVol.getVolumePct()` | `VolBridge` | ✅ | ✅ |
| `KemperisVol.setVolumePct()` | `VolBridge` | ✅ | ✅ |
| `KemperisWifi.getCurrentSSID()` | `WifiBridge` | ✅ | ✅ |
| `KemperisWifi.suggestWifi()` | `WifiBridge` | ✅ | ✅ |
| `KemperisWifi.openWifiSettings()` | `WifiBridge` | ✅ | ✅ |
| `KemperisWifi.unbindNetwork()` | `WifiBridge` | ✅ | ✅ |
| `KemperisWifi.rebindNetwork()` | `WifiBridge` | ✅ | ✅ |
| `KempTts.speak()` | `KempTtsBridge` | ✅ | ✅ |
| `KempTts.setLang()` | `KempTtsBridge` | ✅ | ✅ |
| `KempTts.stop()` | `KempTtsBridge` | ✅ | ✅ |
| `KempTts.isReady()` | `KempTtsBridge` | ✅ | ✅ |
| `KemperisUpdate.checkUpdate()` | `UpdateBridge` | ✅ | ✅ |
| `KemperisUpdate.downloadAndInstall()` | `UpdateBridge` | ✅ | ✅ |
| `KemperisUpdate.getCurrentVersion()` | `UpdateBridge` | ✅ | ✅ |
| `KemperisSms.hasPermission()` | `SmsBridge` | ✅ | ✅ |
| `KemperisSms.send()` | `SmsBridge` | ✅ | ✅ |
| `KemperisSms.getOwnPhoneNumber()` | `SmsBridge` | ✅ | ✅ |
| `KemperisFile.saveTextFile()` | `KemperisFileBridge` | ✅ | ✅ |

**Visi 18 tiltelių: ✅ nėra našlaičių metodų**

### 3.3 KemperisFileBridge.saveTextFile()

- Android 10+ (Q): `MediaStore.Downloads` per `content://media/external/downloads` ✅
  - ⚠️ **Minor**: geriau būtų `MediaStore.Downloads.EXTERNAL_CONTENT_URI` (API 29), o ne hardcoded URI — bet funkcionuoja vienodai
- Android 9-: `Environment.getExternalStoragePublicDirectory(DIRECTORY_DOWNLOADS)` su `WRITE_EXTERNAL_STORAGE` patikrinimu ✅
- `IS_PENDING=1` → rašymas → `IS_PENDING=0` (atominis) ✅
- Toast pranešimas UI thread ✅

### 3.4 AndroidManifest.xml

Visi reikalingi leidimai deklaruoti:
- `INTERNET`, `WAKE_LOCK`, `FOREGROUND_SERVICE`, `FOREGROUND_SERVICE_SPECIAL_USE` ✅
- `POST_NOTIFICATIONS`, `MODIFY_AUDIO_SETTINGS` ✅
- `ACCESS_WIFI_STATE`, `CHANGE_WIFI_STATE`, `CHANGE_NETWORK_STATE`, `ACCESS_FINE_LOCATION` ✅
- `REQUEST_INSTALL_PACKAGES` (APK diegimas) ✅
- `WRITE_EXTERNAL_STORAGE maxSdkVersion=28` (Android 9-) ✅
- `SEND_SMS`, `RECEIVE_SMS`, `READ_PHONE_STATE`, `READ_PHONE_NUMBERS` ✅
- `FileProvider`: `lt.kemperis.app.fileprovider` ✅
- `KemperisService` + `cordova backgroundMode service`: abu `FOREGROUND_SERVICE_SPECIAL_USE` ✅

### 3.5 build.gradle

| Parametras | Vertė | Būklė |
|---|---|---|
| `versionCode` | `13` | ✅ |
| `versionName` | `"13"` | ✅ |
| `minSdkVersion` | `24` (Android 7.0) | ✅ |
| `compileSdk` | `36` | ✅ |
| `targetSdk` | `36` | ✅ |
| Keystore | `kemperis.jks`, alias `kemperis_alias` | ✅ |

---

## 4. 🔌 ESPHome YAML vs SENSOR_ID_MAP auditas

**YAML:** `kempervanas_v22veikiantis .yaml` (2672 eilutės)  
**SENSOR_ID_MAP:** 58 įrašai

### 4.1 Sutapimo rezultatai

| Kategorija | Skaičius | Pavyzdys |
|---|---|---|
| ✅ Pilnas sutapimas (fast path) | 50 | `junc_soc`, `gps_speed_sensor`, `bedroom_co2`, etc. |
| ⚠️ Dead-code fast path (bet veikia per NFKD fallback) | 5 | `water_empty_cm`, `water_full_cm`, `water_offset`, `gas_empty_weight`, `gas_full_weight` |
| ℹ️ NFKD redundancija (sys_esp_temp variantai) | 3 | `sistema___esp_temperat_ra`, `sistema___esp_temperatura`, `sistema___esp_temperatra` |

**Paaiškinimas:** `water_empty_cm` ir pan. yra globalūs YAML kintamieji, eksponuojami kaip `number/num_water_empty_cm`. SSE juos pristato per `number/` prefiksą → `sensorCache['num_water_empty_cm']`. UI skaito `getCache('num_water_empty_cm')`. SENSOR_ID_MAP `water_empty_cm` įrašas (`sensor/` greitai ieško) niekada nesusitapatins SSE metu — **bet nekenkia**, nes `loadAllNumbers()` teisingai pakrauna šias reikšmes per REST.

### 4.2 NFKD fallback grandinė

SSE `rawId` `includes()` patikra (L2103-2170) dengia **visus 58 sensorius** — tiek pagal `id:`, tiek pagal vardą. Jokių nerandomų sensorių nėra.

---

## 5. 🌐 Online / Konfigūracijos patikrinimas

### 5.1 Capacitor ir priklausomybės

| Paketas | Versija | Būklė |
|---|---|---|
| `@capacitor/core` | `^8.3.4` | ✅ |
| `@capacitor/android` | `^8.3.4` | ✅ |
| `@capacitor/cli` | `^8.3.4` | ✅ |
| `@capacitor-community/text-to-speech` | `^8.0.0` | ✅ |
| `cordova-plugin-background-mode` | `^0.7.3` | ✅ |

`capacitor.config.ts`: `webDir: 'www'`, `androidScheme: 'http'` (reikalinga HTTP ESP32 ryšiui) ✅

### 5.2 Git būsena

**Paskutinis commit:** `78830f7` — "v12: fix apk_url to releases/download"

**Nepateikti pakeitimai (reikia commitinti):**

| Failas | Keitimas |
|---|---|
| `www/index.html` | Pataisytas, v13 ✅ |
| `android/app/src/main/java/lt/kemperis/app/MainActivity.java` | CURRENT_VERSION=13 ✅ |
| `android/app/build.gradle` | versionCode/Name=13 ✅ |
| `android/app/src/main/AndroidManifest.xml` | KemperisFileBridge, leidimai ✅ |
| `version.json` | v13, kemperis_v13.apk URL ✅ |
| `docs/changelog.md` | v13 sekcija ✅ |
| `docs/v12_technine_dokumentacija.md` | Nauja ✅ |
| `docs/v12_vartotojo_instrukcija.md` | Nauja ✅ |

### 5.3 version.json

```json
{
  "version": 13,
  "apk_url": "https://raw.githubusercontent.com/gintarasz5G/kemperis-app/main/kemperis_v13.apk"
}
```
✅ Versija ir URL teisingi.

---

## 6. 🚨 Kritiniai veiksmai (PRIVALOMA prieš diegiant)

### ⚠️ #1 — kemperis_v13.apk NEPERSTATYTAS

```
kemperis_v13.apk  = 4 247 211 B  ← IDENTIŠKAS kemperis_v12.1.apk
app-release.apk   = 3 268 038 B  ← paskutinis Gradle build (Jun 13, senasis kodas)
```

**Dabartinis `kemperis_v13.apk` yra tiesiog pervadintas v12 APK.** Jame nėra:
- Pataisyto `index.html` (su DOMContentLoaded ir visomis funkcijomis)
- `CURRENT_VERSION = 13` Java kode
- `KemperisFileBridge` (CSV/GPX išsaugojimas)

**Privaloma:** paleisti `build_and_sync.bat` → pervadinti `app-release.apk` → `kemperis_v13.apk`

### ⚠️ #2 — Nepateikti pakeitimai Git

Visi kodų pakeitimai dar nėra commitinti ir kelti į GitHub.

---

## 7. 📋 Rekomenduojami žingsniai

### A. Kompiliacija (PIRMIAUSIAI)

```bat
cd C:\Users\ginta\kempervanas\veikiantisKEMPERIS\KemperisApp
build_and_sync.bat
```

Po sėkmingo build:
```bat
copy android\app\build\outputs\apk\release\app-release.apk kemperis_v13.apk
```

### B. Git commit ir push

```bash
cd C:\Users\ginta\kempervanas\veikiantisKEMPERIS\KemperisApp

git add www/index.html
git add android/app/src/main/java/lt/kemperis/app/MainActivity.java
git add android/app/build.gradle
git add android/app/src/main/AndroidManifest.xml
git add android/settings.gradle
git add version.json
git add docs/changelog.md
git add docs/v12_technine_dokumentacija.md
git add docs/v12_vartotojo_instrukcija.md
git add kemperis_v13.apk

git commit -m "v13: KemperisFileBridge, background mode, alarm UI, DOMContentLoaded fix"
git push origin main
```

### C. Minor pagerinimas (nebūtinas)

`KemperisFileBridge.java` L637 — pakeisti hardcoded URI:
```java
// Dabartinis:
Uri collection = Uri.parse("content://media/external/downloads");
// Geriau (API 29+):
Uri collection = MediaStore.Downloads.EXTERNAL_CONTENT_URI;
```
Veikia abejais būdais ant visų žinomų Android 10+ įrenginių.

---

## 8. ✅ Audito santrauka

| Komponentas | Būklė | Pastabos |
|---|---|---|
| `www/index.html` | ✅ GERAI | Pilnas, 2811 eilučių, v13 |
| `assets/public/index.html` | ✅ GERAI | Sinchronizuotas |
| JS logika (99 fn) | ✅ GERAI | Jokių klaidingų logikos grandinių |
| SSE dual-lookup | ✅ GERAI | Fast path + NFKD fallback |
| `init()` → `startSSE()` grandinė | ✅ GERAI | DOMContentLoaded ✓, `finally` SSE start ✓ |
| Dienos žurnalas | ✅ GERAI | Adaptyvus, 23:59 auto-save |
| TTS garsas | ✅ GERAI | `_volBusy`, `prevVol` restore |
| Aliarmai | ✅ GERAI | Snooze, repeat, vagystė |
| MainActivity.java | ✅ GERAI | CURRENT_VERSION=13, visi 18 tilteliai |
| KemperisFileBridge | ✅ GERAI | MediaStore API teisingai |
| AndroidManifest | ✅ GERAI | Visi leidimai |
| build.gradle | ✅ GERAI | versionCode/Name=13 |
| SENSOR_ID_MAP vs YAML | ✅ GERAI | 50/58 tiesioginiai, 8 harmless |
| `kemperis_v13.apk` | 🔴 BLOKER | Neperstatytas — lygus v12 APK |
| Git | ⚠️ | ~8 failų nepateikta, nėra v13 commit |
