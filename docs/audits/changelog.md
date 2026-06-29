# 📋 Versijų Istorija — Kemperis App

---

## v16 (2026-06-16) — Dabartinė — OTA stabilumas

### Pataisos
- 🔧 **OTA race condition** — JS watchdog 15s → 25s, daugiau nebebus klaidingo „užtruko" pranešimo kai mobilus ryšys lėtas (Java timeout 20s + 5s marża)
- 🔧 **Offline fetch spam** — `loadAllNumbers`/`loadAllTexts` pridėtas `_isOffline` guard'as. Be jo SSE reconnect kas 33s paleisdavo 19 fetch'ų į 192.168.4.1 ir užplūsdavo terminalą „❌ Failed to fetch" klaidomis
- 🔧 **Paraleliai paleidimo apsauga** — `_loadInProgress` mutex neleidžia antrai `loadAllNumbers()` instancijai pradėti darbo kol pirmoji dar veikia
- 🔧 **Iteracijų nutraukimas** — jei ryšys dingsta įkrovimo metu, ciklas nutraukiamas iškart (per-iteration `_isOffline` break), nebėra 16 sekančių užklausų į niekur
- 🔧 **version.json kabučių escape** — perėjimas iš ASCII `"..."` į tipografines `„...“` (U+201E + U+201C), kurios JSON viduje legalios

### Techniniai pakeitimai
- `build.gradle` versionCode/versionName: 16
- `MainActivity.CURRENT_VERSION` = 16
- `version.json` v16 (su LT notes ir teisingu URL)
- Commit'ai: `ec81c5b`, `7190ea6`, `f55b2ff`

---

## v15.1 (2026-06-16) — APK rebuild (vidinis, ta pati versija)

GitHub v15 release APK perrašytas su sekančiais pakeitimais (`versionCode` liko 15, todėl OTA nesuaktyvėjo):

- ⚠️ **Pataisymai iš commit `ec81c5b`** — visi v16 fix'ai jau šiame APK (OTA race, offline guard, mutex)
- 🔧 **APK dydis** sumažėjo 4.05 MB → 3.13 MB — švari Capacitor sync būsena pašalino seną cache krovinį
- 🔧 **SHA256** `3ac71b5d...265f` (vietoj originalaus `f7b5c4b0...34819b`)

> 💡 **Pasekmė:** OTA naudotojams nepasiūlys atnaujinimo iki v16, nes versija ta pati. Pataisymus gauna tik tie, kas rankiniu būdu perdiegia iš GitHub release.

---

## v15 (2026-06-16) — SOC, arm/disarm, WiFi pataisymai

### Naujos funkcijos / pataisos
- 🔧 **SOC 0% klaida pataisyta** — procentas skaičiuojamas iš `ah_remaining/capacity` fallback'u, kai tiesioginė `junc_soc` reikšmė nulinė
- 🔧 **arm/disarm SMS offline** — admin numeris saugomas `localStorage` (`saveAdminNumber`), todėl apsauga veikia ir be ESP32 ryšio
- 🔧 **WiFi pririšimas su mobiliais duomenimis** — `NetworkRequest.Builder().removeCapability(NET_CAPABILITY_INTERNET)` leidžia Android pagauti ESP AP (be interneto). Be šito su įjungtais mobiliais duomenimis srautas neidavo į 192.168.4.1
- 🔧 **TTS speech queue** — `_speechQueue` neleidžia balso pranešimams persidengti ar prapulti
- 🔧 **42°C temperatūros klaida** — ESP32 lusto temperatūra (`sistema_` prefix) nebepatenka į aplinkos rodmenį (`bme_temp`)
- 🔧 **XSS apsauga** — `escapeHtml()` SMS ir sistemos terminale, neleidžia kenksmingam HTML pakliūti į DOM
- 🔧 **RAM, uptime, ESP temp** — vieninga atvaizdavimo logika footer'yje ir Logs kortelėje

### Release proceso pataisymai šios sesijos metu
- 🚀 **v15 tag + GitHub release sukurtas** — pradžioje release neegzistavo, todėl `version.json` rodė į 404 URL
- 🔧 **Gemini regresijos atstatymas** (`commit 00db8a5`) — Gemini Android Studio asistente pakeitė `apk_url` į neegzistuojantį `raw/main/` kelią ir sumaišė UTF-8 kodavimą notes laukelyje. Grąžinta veikianti release asset URL ir lietuviški notes

### Techniniai pakeitimai
- `build.gradle` versionCode/versionName: 15
- `MainActivity.CURRENT_VERSION` = 15
- `version.json` v15
- Commit'ai (sesijos metu): `00db8a5`

---

## v14 (2026-06-15) — Failų tiltas, aliarmai

### Naujos funkcijos
- 📁 **Natyvus failų tiltas (KemperisFileBridge)** — CSV, GPX, Terminal išsaugojimas tiesiai į Downloads (MediaStore API Android 10+, failų sistema Android 9-)
- ⚙️ **Aliarmų nustatymai UI** — aktyvavimo slenkstis ir balso tekstas keičiamas tiesiai programėlėje (`renderAlarmSettings`)
- 🌑 **Background mode** — Cordova background plugin integruotas; balso aliarmai veikia ekraną išjungus

### Pataisos
- 🔧 **CURRENT_VERSION 13→14** — app nebesiūlo klaidingo atnaujinimo kiekvieną kartą paleidžiant
- 🔧 **Saugumo skola** — plain-text slaptažodžiai pašalinti iš HEAD'o, `keystore.jks` pašalintas iš naujų commit'ų (tačiau LIEKA istorijoje, žr. žemiau)
- 🔧 **Update URL** — `V14` → `v14` (case sensitive GitHub release)
- 🔧 **assets/public sinchronizacija** — www/index.html visada atitinka Android build failą

### Techniniai pakeitimai
- `build.gradle` versionCode/versionName: 14
- `version.json` notes atnaujinti su LT tekstu

---

## v13 (2026-06-15)

### Pataisos
- 🔧 **Kritinė: index.html buvo apkarpytas** — trūko ~98 eilučių (DOMContentLoaded, renderAlarmSettings, map-container); programa niekada neprasidėdavo
- 🔧 **CURRENT_VERSION 12→13** — app nebesiūlo klaidingo atnaujinimo kiekvieną kartą paleidžiant
- 🔧 **sendOfflineSms indentacija** — 8→4 tarpai
- 🔧 **assets/public sinchronizacija** — www/index.html visada atitinka Android build failą

### Techniniai pakeitimai
- `build.gradle` versionCode/versionName: 13
- Versijų nuorodos UI ir fallback: v12→v13
- `version.json` notes atnaujinti

---

## v12 (2026-06-13)

### Naujos funkcijos
- ⚡ **Force Live** — rankinis prisijungimas prie SSE be laukimo
- 🗺️ **Žemėlapis** — interaktyvus šios dienos maršrutas su rodykle
- 📋 **Auto-logger** — žurnalai įsijungia automatiškai app starte
- 🌡️ **Dew Point / Kondensatas** — kondensato rizikos stebėjimas realiu laiku
- 🔐 **SMS Apsauga** — papildoma savininko numerio patikra
- 🔕 **Snooze sistema** — 15/30/60 min nutildymas 6 aliarmų grupėms
- ⚠️ **ESP32 sveikatos baneris** — RAM < 50KB arba temp > 70°C
- 📳 **Vibracijos aliarmas** — EMA filtravimas per ADXL345
- 🎣 **Žvejybos paruoštukas** — slėgio tendencijų analizė
- 🔋 **Energijos sekimas** — apyvarta Wh, vid./maks. galia
- ⏱️ **Laikas iki pilno/tuščio** — `junc_time_remaining` sensorius
- 📊 **Mini grafikai** — slėgio ir SOC istorija kortelėse

### Pataisos (šioje sesijoje)
- 🔧 **ESP32 mygtukai offline** — 8 mygtukai ir 2 perjungėjai automatiškai išjungiami offline mode
- 🔧 **setNumber/setText offline** — 17 nustatymų laukai blokuojami offline
- 🔧 **Health banner offline** — nebero klaidingo „0 KB" offline mode
- 🔧 **Health banner TTS** — kalba su skaičiais: „Atmintis baigiasi, liko X kilobaitų"
- 🔧 **SMS TTS** — `+status` atsakas TTS nesukelia, tik arm/disarm
- 🔧 **toggleSecurity debounce** — neleidžia dvigubam paspaudimui
- 🔧 **Live GPS markeris** — atsinaujina realiu laiku kai žemėlapis atidarytas
- 🔧 **Offline-dim kortelės** — 20+ kortelių prislopinamos offline visuose skirtukuose

### Techniniai pakeitimai
- SSE `name_id` palaikymas (ESPHome 2026.7+ future-proof)
- SSE log stream Logs skirtuke
- GPS kelionės statistika (haversine atstumai)
- LocalStorage — aliarmų istorija, snooze, energijos sekimas išlieka po minimize

---

## v11 (2026-05) — GPS ir Aliarmai

### Naujos funkcijos
- 📍 GPS skirtukas — koordinatės, greitis, palydovai
- 🔔 Pilna aliarmų sistema — CO₂, SOC, vanduo, dujos
- 📊 Lygiavimo skirtukas — ADXL345 interaktyvūs grafikai
- 💬 SMS skirtukas — arm/disarm/status per SMS
- 🔒 Apsaugos perjungėjas UI
- 📤 Google Sheets manual upload mygtukas
- 🚨 Vagystės aliarmas su GPS greičio detekcija

### Pataisos
- LocalStorage alarm state persistence (alarmFlags išlieka po minimize)
- Timestamp formatas `yyyy.mm.dd HH:mm`
- CSS offline kortelių stiliai

---

## v10 (2026-04) — Offline Mode

### Naujos funkcijos
- 📵 **Offline mode** — automatinis perėjimas kai WiFi nepasiekiamas
- 🌐 **Google Sheets sinchronizacija** — duomenys offline per 4G
- 💾 **LocalStorage cache** — sensorCache išlieka po app perkrovimo
- ⚡ **SSE Watchdog** — 90s timeout, automatinis reconnect
- 🔄 **Smart WiFi bind** — NetworkCallback, laikinas unbind Sheets fetch metu

### Pataisos
- Race condition fix — SSE pradedamas 3s po offline mode nustatymo
- `init()` `DOMContentLoaded` hook
- CSS `.panel-card` → `.card` klasių pataisymai

---

## v9 ir ankstesnės (2026-02 – 2026-03)

- Pradinis Capacitor WebView app paleidimas
- Java bridges: KemperisSms, KempTts, KemperisWifi, KemperisVol, UpdateBridge
- Bazinis SSE stream iš ESPHome
- Energijos kortelės (SOC, įtampa, srovė)
- Aplinkos kortelės (BME280, SCD41)
- App atnaujinimas per DownloadManager
- Android 13/14 RECEIVER_EXPORTED, WifiNetworkSuggestion
- FOREGROUND_SERVICE_TYPE_SPECIAL_USE
