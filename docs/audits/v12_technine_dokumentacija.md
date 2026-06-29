# 🛠️ Techninė Dokumentacija — Kemperis App

## 1. Sistemos Architektūra

Sistema susideda iš trijų sluoksnių:

```
┌─────────────────────────────────────────────────────────┐
│  Android App (Capacitor WebView)                        │
│  ┌─────────────────────┐  ┌────────────────────────┐   │
│  │  UI/Logic            │  │  Java Bridges           │   │
│  │  index.html (JS)     │◄─┤  KemperisSms           │   │
│  │  - SSE handler       │  │  KempTts               │   │
│  │  - sensorCache       │  │  KemperisWifi          │   │
│  │  - alarmų sistema    │  │  KemperisVol           │   │
│  │  - offline mode      │  │  UpdateBridge          │   │
│  └─────────┬───────────┘  └────────────────────────┘   │
└────────────┼────────────────────────────────────────────┘
             │ WiFi (192.168.4.1)         │ 4G
             ▼                            ▼
    ┌─────────────────┐        ┌─────────────────────┐
    │  ESP32 + ESPHome│        │  Google Apps Script  │
    │  SSE stream     │        │  (Google Sheets)     │
    │  REST API       │        │  doGet/doPost        │
    └─────────────────┘        └─────────────────────┘
```

---

## 2. Failų struktūra

```
KemperisApp/
├── www/
│   └── index.html              ← Vienintelis UI failas (~2720 eilučių)
├── android/
│   └── app/src/main/
│       ├── assets/public/
│       │   └── index.html      ← SINCHRONIZUOTA KOPIJA (MD5 turi sutapti)
│       └── java/.../
│           └── MainActivity.java
├── docs/                       ← Ši dokumentacija
└── version.json                ← App atnaujinimo versijų failas
```

**SVARBU:** `www/index.html` ir `android/.../assets/public/index.html` turi būti identiški. Po kiekvieno redagavimo būtina sinchronizuoti.

---

## 3. Tinklo Valdymas (Smart Network Orchestration)

App veikia dviem WiFi/4G scenarijais vienu metu:

| Situacija | Tinklas | Naudojamas |
|---|---|---|
| Kemperis šalia | WiFi `Kemperis-Valdymas` | SSE + REST prie ESP32 |
| Sheets fetch / update | 4G (laikinas unbind) | Google Apps Script API |
| Kemperis toli | 4G nuolat | tik Sheets (offline mode) |

**Smart Unbind logika (`MainActivity.java`):**
- `NetworkCallback` seka WiFi prisijungimą
- `bindProcessToNetwork(wifiNetwork)` — ESP32 paketus siunčia per WiFi
- `bindProcessToNetwork(null)` — laikinai per 4G (Sheets fetch, APK download)
- Po Sheets/update grįžta prie WiFi bind

**Numatytasis ESP32 IP:** `192.168.4.1` (konfigūruojama Settings → ESP32 IP)

---

## 4. SSE (Server-Sent Events) Protokolas

**Endpoint:** `http://192.168.4.1/events`

**Event tipai:**

| Tipas | Aprašas |
|---|---|
| `ping` | Keepalive kas ~30s — atnaujina watchdog laikmatį |
| `state` | Sensoriaus būsena: `{"id":"sensor_name","value":"123.4"}` |
| `log` | ESP32 log žinutė (rodoma Logs skirtuke) |

**Raktų normalizavimas (SSE → sensorCache):**
- ESPHome 2026.7+: `id` laukas = `name_id` (tarpai → `_`)
- Klaidos forma: `sensor name` → normalizuojama į `sensor_name`
- Specialūs aliasai L560-580: pvz. `koordinat` → `gps_coords_sensor`

**SSE Watchdog:** Jei 90s negaunamas joks `ping` arba `state` — automatinis reconnect.

---

## 5. REST API (ESP32)

Bazinis URL: `http://{ESP_IP}`

| Metodas | Endpoint | Aprašas |
|---|---|---|
| POST | `/button/{id}/press` | Mygtukas (pvz. `btn_water_empty`) |
| POST | `/switch/{id}/turn_on` | Įjungti jungiklį |
| POST | `/switch/{id}/turn_off` | Išjungti jungiklį |
| POST | `/number/{id}/set?value={v}` | Nustatyti skaičių |
| POST | `/text/{id}/set?value={v}` | Nustatyti tekstą |

**Offline apsauga:** Visos šios funkcijos turi `if (_isOffline) return;` — offline metu REST kvietimai blokuojami.

---

## 6. Google Sheets / Apps Script API

**URL forma:** `https://script.google.com/macros/s/{SCRIPT_ID}/exec`

| Parametras | Reikšmė | Aprašas |
|---|---|---|
| `action=latest` | GET | Grąžina paskutinius duomenis iš Sheets |
| (POST, be params) | POST | Gauna duomenis iš ESP32 (doPost) |

**Grąžinamas JSON (`action=latest`):**
```json
{
  "soc": 85.2,
  "v": 13.1,
  "a": -2.5,
  "power": -32.8,
  "water": 67,
  "gas": 45,
  "temp": 22.1,
  "humidity": 58,
  "pressure": 1013.2,
  "co2": 650,
  "lat": 54.6872,
  "lon": 25.2797,
  "timestamp": "2026-06-13 14:32:00"
}
```

---

## 7. Duomenų Šaltiniai (Offline vs Online)

| sensorCache raktas | SSE | Sheets | Pastabos |
|---|---|---|---|
| `junc_soc` | ✅ | ✅ | SOC % |
| `junc_v` | ✅ | ✅ | Įtampa V |
| `junc_a` | ✅ | ✅ | Srovė A |
| `junc_power` | ✅ | ✅ | Galia W |
| `junc_ah_remaining` | ✅ | ✅ | Ah likutis |
| `water_pct` | ✅ | ✅ | Vanduo % |
| `tds_ppm` | ✅ | ✅ | Vandens kokybė ppm |
| `gas_pct` | ✅ | ✅ | Dujos % |
| `bme_temp` | ✅ | ✅ | Temperatūra lauke |
| `bme_humidity` | ✅ | ✅ | Drėgmė lauke |
| `bme_pressure` | ✅ | ✅ | Atmosferos slėgis |
| `bedroom_co2` | ✅ | ✅ | CO₂ miegamajame |
| `bedroom_temp` | ✅ | ✅ | Temp miegamajame |
| `bedroom_humidity` | ✅ | ✅ | Drėgmė miegamajame |
| `gps_coords_sensor` | ✅ | ✅ | "lat,lon" eilutė |
| `gps_speed_sensor` | ✅ | ❌ | Greitis km/h |
| `gps_heading_sensor` | ✅ | ❌ | Kryptis ° |
| `gps_alt_sensor` | ✅ | ❌ | Aukštis m |
| `sys_free_ram` | ✅ | ❌ | Laisva RAM bytes |
| `sys_esp_temp` | ✅ | ❌ | ESP32 temp °C |
| `sys_uptime` | ✅ | ❌ | Uptime s |
| `sw_security_armed` | ✅ | ❌ | Apsauga on/off |
| `sw_google_upload` | ✅ | ❌ | Google upload on/off |
| `alert_movement_sent` | ✅ | ❌ | Vagystės trigeris |
| `vib_level` | ✅ | ❌ | Vibracijos lygis |
| `power_source_220` | ✅ | ❌ | 220V šaltinis |
| `gsm_operator_sensor` | ✅ | ❌ | GSM operatorius |
| `gsm_signal_pct` | ✅ | ❌ | GSM signalas % |
| `roll_sensor` | ✅ | ❌ | Šoninis polinkis ° |
| `pitch_sensor` | ✅ | ❌ | Pirmyn/atgal polinkis ° |
| `sms_1`…`sms_5` | ✅ | ❌ | SMS istorija |

---

## 8. Offline Mode Logika

**Trigeris:** SSE `onerror` arba 90s watchdog timeout → `setOfflineMode(true)`

**Kas atsitinka offline:**
- `_isOffline = true`
- Visos ESP32 REST funkcijos blokuojamos (`sendButton`, `setNumber`, `setText`, `toggleSecurity`, `toggleGoogleUpload`)
- `data-esp-btn` mygtukai → `disabled=true`, `opacity:0.35`
- `data-esp-switch` perjungėjai → `pointerEvents:none`, `disabled=true`
- `data-offline-dim` kortelės → pilkėja
- `checkAlarms()` → `return` (tik gyvi duomenys svarbūs)
- Health banner → paslėptas
- `_fetchSheetsNow()` → kviečiamas nedelsiant + kas 60s

**Grįžimas online:** SSE `onopen` → `setOfflineMode(false)` → visos apsaugos atšaukiamos.

---

## 9. Alarmų Sistema

### ALARM_CONFIG

| ID | Grupė | Slenkstis | Sąlyga | Pakartojimas |
|---|---|---|---|---|
| `water_critical` | `water` | 5% | vanduo < 5% | — |
| `water_low` | `water` | 15% | vanduo < 15% | — |
| `co2_danger` | `co2` | 2500 ppm | CO₂ > 2500 | kas 60s |
| `co2_elevated` | `co2` | 1800 ppm | CO₂ 1800–2500 | kas 60s |
| `soc_critical` | `soc` | 15% | SOC < 15% | — |
| `soc_low` | `soc` | 25% | SOC < 25% | — |
| `gas_low` | `gas` | 10% | dujos < 10% | — |
| `theft_alarm` | — | — | `alert_movement_sent=true` | kas 120s |
| `high_vibration` | `vibration` | 0.4 | apsauga įjungta + vib > 0.4 | kas 20s |
| `condensation` | `condensation` | — | temp < 8°C IR drėgmė > 65% | — |

### Snooze Sistema

| Grupė | Aliarmai | Kortelė | Skirtukas |
|---|---|---|---|
| `co2` | co2_danger, co2_elevated | bedroom-co2-card | Aplinka |
| `water` | water_critical, water_low | card-water | Aplinka |
| `gas` | gas_low | card-gas | Aplinka |
| `soc` | soc_critical, soc_low | card-soc | Energija |
| `vibration` | high_vibration | pc-security | Apžvalga |
| `condensation` | condensation | card-condensation | Aplinka |

Galimi snooze laikotarpiai: **15 min / 30 min / 60 min**

---

## 10. GPS ir Maršruto Registravimas

**Auto-logger:** Kas 10s įrašo eilutę į `localLogRows[]` masyvą.

**CSV formatas (localLogHeader stulpeliai):**

| Indeksas | Pavadinimas | Vienetas |
|---|---|---|
| 0 | Laikas | `yyyy.mm.dd HH:mm:ss` |
| 1 | (tuščias) | — |
| 2 | Platuma | `°` |
| 3 | Ilguma | `°` |
| 4 | Aukštis | `m` |
| 5 | Greitis | `km/h` |
| 6 | Kryptis | `°` |
| 7 | SOC | `%` |
| 8 | Įtampa | `V` |
| 9 | Srovė | `A` |
| 10 | Vanduo% | `%` |
| 11 | TDS | `ppm` |
| 12 | Dujos% | `%` |
| 13 | Temp | `°C` |
| 14 | Drėgmė | `%` |
| 15 | Slėgis | `hPa` |
| 16 | CO2 | `ppm` |

**Auto-eksportas:** Kas 10 min arba kai `localLogRows` viršija 500 eilučių — automatiškai išsaugoma į CSV failą.

**Gyvas markeris žemėlapyje:** Kiekvienam SSE event'ui `updateUI()` atnaujina `_mapMarker.setLatLng([lat,lon])` ir SVG rodyklę su `gps_heading_sensor` kampu — jei žemėlapis atidarytas.

---

## 11. ESP32 Sveikatos Monitoringas

**Baneris rodomas jei:**
- Laisva RAM < 50 000 bytes (50 KB) → rodo kiek KB liko
- ESP32 temperatūra > 70°C

**Offline:** Baneris visada paslėptas (duomenų nėra).

**TTS (tik pirmą kartą):** `"Dėmesio. Atmintis baigiasi, liko X kilobaitų. Temperatūra per aukšta, X laipsnių."`

---

## 12. Android Java Bridges

### `KemperisSms`
```java
sendSms(String number, String message)    // Siunčia SMS
window.onSmsReceived(String sender, String body)  // JS callback kai gauta SMS
```

### `KempTts`
```java
speak(String text, boolean flush)  // Kalba tekstą (flush=true pertraukia ankstesnį)
setLanguage(String lang)           // "lt-LT" arba "en-US"
```

### `KemperisWifi`
```java
getCurrentSsid()   // Grąžina dabartinį WiFi SSID
suggestNetwork(String ssid, String pass)  // Siūlo WiFi tinklą sistemai
```

### `KemperisVol`
```java
setVolume(int percent)   // Nustato sistemos garsumą aliarmų metu
getVolume()              // Grąžina dabartinį garsumą
```

### `UpdateBridge` (JS → Java)
```javascript
window.KemperisUpdate.checkAndDownload(apkUrl)  // Parsiunčia ir įdiegia APK
```

---

## 13. LocalStorage Raktai

| Raktas | Turinys | Pavyzdys |
|---|---|---|
| `esp_ip` | ESP32 IP adresas | `"192.168.4.1"` |
| `esp_ssid` | Kemperio WiFi SSID | `"Kemperis-Valdymas"` |
| `esp_pass` | WiFi slaptažodis | `"kemperis123"` |
| `app_script_id` | Google Apps Script ID | `"AKfycb..."` |
| `camper_phone` | Kemperio SIM numeris | `"+37060000000"` |
| `system_pin` | SMS komandų PIN | `"1234"` |
| `tts_lang` | TTS kalba | `"lt"` arba `"en"` |
| `tts_volume_pct` | TTS garsumas 0–100 | `"80"` |
| `chime_type` | Aliarmų signalo tipas | `"beep"` |
| `kemper_alarm_config` | Pakeistitų alarmų slenkščiai | JSON masyvas |
| `_alarmFlags` | Aktyvių aliarmų būsena | `{"co2_danger":true}` |
| `_alarmSnooze` | Snooze galiojimo laikai | `{"co2_danger":1749810000000}` |
| `_alarmLastTriggered` | Paskutinio aliarmų laikas | `{"co2_danger":1749809940000}` |
| `_alarmHistory` | Aliarmų istorija (maks 50) | JSON masyvas |
| `_sensorCache` | Paskutiniai sensorių duomenys | JSON objektas |
| `_energyState` | Energijos sekimo būsena | JSON objektas |
| `kemper_local_log` | GPS/duomenų žurnalas (CSV) | Eilutės |
| `theft_` | Vagystės aliarmų TTS indeksas | `"2"` |

**Kaip išvalyti:** Android → App Settings → Storage → Clear Data. Arba JavaScript konsolėje: `localStorage.clear()`

---

## 14. Energijos Sekimo Sistema

`_updateEnergyTracking()` kviečiamas kiekvieną SSE ciklą. Seka:
- **Apyvarta:** `|junc_a| × junc_v × Δt` — sukauptoji Wh
- **Vidutinė galia:** slenkamasis vidurkis
- **Maksimali galia:** didžiausia matyta
- **Sekama nuo:** sesijos pradžios timestamp
- Išsaugoma į `localStorage._energyState`

---

## 15. SMS Komandų Sistema

| SMS komanda | Siuntėjas | ESP32 atsakas | TTS app'e |
|---|---|---|---|
| `+arm` | App → Kemperis SIM | Patvirtinimas (`ARMED`) | „Apsauga įjungta" |
| `+disarm` | App → Kemperis SIM | Patvirtinimas (`DISARMED`) | „Apsauga išjungta" |
| `+status` | App → Kemperis SIM | Statuso žinutė | ❌ (tik sysLog) |
| `+lokacija` | App → Kemperis SIM | GPS koordinatės | ❌ (tik sysLog) |
| `+ikelk` | App → Kemperis SIM | Triggers Sheets upload | ❌ (tik sysLog) |

**`_lastSmsCmd` logika:** TTS skamba TIK kai app gavo patvirtinimą (`ARMED`/`DISARMED`) ir paskutinė siųsta komanda buvo `arm`/`disarm`. `+status` atsakas TTS nesukelia — tik `sysLog`.

**Saugumo patikra:** Prieš siunčiant `+arm`/`+disarm` — patikrinamas `system_pin`. SMS siunčiamas tik žinomu telefono numeriu (`camper_phone`).

---

**Versija:** 12.0 (su v12+ pataisomis)  
**Atnaujinta:** 2026-06-13  
