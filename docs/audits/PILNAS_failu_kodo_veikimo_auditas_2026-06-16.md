# KemperisApp — PILNAS auditas (failai + kodas + veikimas)

**Data:** 2026-06-16
**HEAD:** `c171dfb` (pushintas). **Necommit'intas:** native WiFi pataisymas (`MainActivity.java`).
**Metodas:** statinis kodo + git pjūvis + 2026-06-16 logų ir 13 ekrano nuotraukų analizė (ESP web vs programėlė). Be realaus įrenginio testavimo.

---

## A. FAILŲ AUDITAS

### Esminiai failai
| Failas | Eil. | Vaidmuo |
|--------|------|---------|
| `KemperisApp/www/index.html` | 2909 | Visa programėlės logika (HTML+CSS+JS), 102 funkcijos |
| `MainActivity.java` | 751 | Native tiltai (18 `@JavascriptInterface`), WiFi rišimas, OTA, SMS, TTS |
| `KemperisService.java` | 88 | Foreground service (Android 14 `specialUse`) |
| `version.json` (šaknis) | — | OTA: version 14, apk_url v14 |
| `ESPHome/GoogleSheetsScript.js` | 125 | Apps Script (doGet/doPost) offline duomenims |

### 🟠 Failų higienos pastabos
- **`KemperisApp/KemperisV5.html`** — sekamas git, bet tai **senas/dublikatas** (realiai naudojamas `www/index.html`). Klaidina. Rekomendacija: pašalinti iš repo.
- **`www/lib/leaflet.css` ir `leaflet.js`** — **0 baitų (tušti)**. Žemėlapis kraunamas iš CDN, todėl offline neveikia. Tušti failai klaidina — arba įdėti realų Leaflet, arba pašalinti.
- **`.artifacts/...walkthrough.artifact.md`** — sekami du artefaktai; nebūtini repo.
- **`.esphome/` build artefaktai istorijoje** — ~100 MB šiukšlių git istorijoje (žr. D sk.).

---

## B. KODO AUDITAS

### B1. Web app (`www/index.html`) — ✅ brandus
Patvirtinta HEAD'e (be dublikatų, JS sintaksė švari):
- TTS eilė (`_speechQueue`), vagystė turi prioritetą.
- `escapeHtml()` SMS + loguose (XSS).
- Temperatūros filtras (`!esp_`) — 42°C klaida pašalinta.
- SOC: `setSensorValue('junc_soc','soc_value'/'soc_d')` + Ah→% atsarga (`ah/cap`).
- arm/disarm offline: `saveAdminNumber` + `localStorage['admin_number']`.
- Visi rodikliai (vanduo, dujos, GSM, akum temp, sistema) turi tinkamą `setSensorValue` atvaizdavimą **ir** SSE atpažinimą — atvaizdavimo grandinė pilna.

### B2. Native (`MainActivity.java`)
- **🔴→✅ WiFi su mobiliais duomenimis (pataisyta, necommit'inta):** `NetworkRequest.Builder()` numatytai reikalavo `NET_CAPABILITY_INTERNET`, todėl ESP AP (be interneto) nebuvo pagaunamas → su mobiliais duomenimis srautas neidavo į 192.168.4.1. Pridėta `.removeCapability(NET_CAPABILITY_INTERNET)`. Tai taip pat įjungia esamus `rebindNetwork()/unbindNetwork()` (veikia tik kai `boundNetwork` nustatytas callback'e).
- Tiltai (18 metodų): SMS, TTS, Volume, Update, File, WiFi — `onDestroy` švarus.
- `KemperisService` — Android 14 atitiktis, START_STICKY.

### B3. Kodo kokybės rizikos (galiojančios)
- **Trapus SSE atpažinimas** — ilga `rawId.includes('...')` grandinė (pvz. `'signalas____'`, `'esp_temperat'`). Veikia, bet jautru ESP pavadinimų pokyčiams. Geriau — aiškūs `id:` ESP YAML'e.
- **`localStorage` rašymas kas SSE žinutę** — našumo/baterijos našta; vertas debounce.
- **Tinklo bind/unbind** dalijasi keli procesai (SSE/Sheets/OTA) — galimas „offline blyksnis"; vertas mutex.
- **Nėra automatinių testų** (tik Capacitor šablonai).

---

## C. VEIKIMO AUDITAS (iš 2026-06-16 logų + nuotraukų)

### ESP web (192.168.4.1) vs programėlė
| Laukas | ESP | App | Priežastis |
|--------|-----|-----|-----------|
| Įtampa/srovė/galia/Ah | 13.3V/0.1A/3W/101.8 | rodo | ✅ |
| Temp/drėgmė/slėgis | 13.5/71.5/996 | rodo | ✅ |
| GSM operatorius / 220V / vanduo / dujos | yra | rodo | ✅ |
| **SOC %** | 93% | **0%** | atvaizdavimo klaida (senas APK) |
| **Akum temp, GSM signalas/tinklas/roaming, Google buferis, oro kokybė, ESP Temp/RAM/Uptime** | yra | **---** | duomenys neatėjo (nestabilus SSE) |

### Dvi priežastys (ne daug atskirų klaidų)
1. **SOC 0% = atvaizdavimo klaida.** SOC ateina su srove/Ah; duomuo pasiekia, tik skaičius nerašomas (senasis kodas). Pataisyta šaltinyje.
2. **Visi „---" laukai = nestabilus SSE ryšys.** Atvaizdavimas ir SSE atpažinimas kode **yra**; tušti todėl, kad `sensorCache` jų neturi. Modelis: rodomi tik dažnai srautu siunčiami (Junctek, operatorius), o lėtieji (GSM ~60s, sistema, akum temp, oro kokybė, buferis) nespėja išsiųsti būsenos. Loge SSE timeout kas ~30s. **Šaknis — WiFi/mobiliųjų rišimo klaida (B2)**, dėl kurios ryšys su 192.168.4.1 nestabilus. Pataisius rišimą, SSE turi stabilizuotis ir lėtieji laukai užsipildyti.

### Hardware pastaba (ne app)
- Junctek „D9 nerasta" kartojasi: shuntas C0 (įtampa) siunčia dažnai, D9 (srovė+SOC+Ah) retai → SOC/srovė atsinaujina su pertrūkiais. BLE patikimumo klausimas.

---

## D. 🔴 SAUGUMAS / GIT — nepakitę (kritinė skola)

| Tikrinta | Būsena |
|----------|--------|
| `kemperis.jks` istorijoje | yra (commit `274887b`) |
| `kemperis2026` (build.gradle) istorijoje | 13 commit'ų |
| Paslaptys HEAD `index.html` | WiFi `kemperis123`, Apps Script ID, telefonai |
| `.git` dydis | 107 MB (`.esphome/` artefaktai) |
| Istorijos perrašymas / rakto rotacija | neatlikta |
| Repo | viešas |

**Veiksmai:** `git filter-repo` (keystore, `kemperis2026`, `.esphome/`, `KemperisV5.html`) + `push --force-with-lease`; rotuoti pasirašymo raktą ir Apps Script ID; paslaptis iš HEAD į `.gitignore` failą — **arba** repo → privatus.

---

## E. 🟠 VERSIJOS
`version.json`=14, `CURRENT_VERSION`=14, bet `build.gradle` **versionCode 13 / versionName "13"**. Naujam OTA leidimui kelti į **15** (nes „v14" jau paskelbta — `14 > 14` netiesa).

---

## F. 🔴 BUILD / DEPLOY — pagrindinė kliūtis
Visi pataisymai šaltinyje, bet **įdiegtas APK senas** (įrodyta: app rodo įtampą/srovę/Ah gerai, bet SOC=0% — senojo kodo požymis). GitHub v14 APK įkeltas prieš pataisymus; build išvestyje naujo APK nėra. WiFi pataisymas dar **necommit'intas**.

---

## G. PRIORITETINIS VEIKSMŲ SĄRAŠAS

**Dabar (kad telefonas gautų pataisymus):**
1. `git commit` native WiFi pataisymą (`MainActivity.java`).
2. `build.gradle` → `versionCode 15`, `versionName "15"`.
3. `build_and_sync.bat` → naujas `app-release.apk`.
4. Įdiek telefone → `grant_sms.bat`.
5. **Testas:** su įjungtais mobiliais duomenimis prisijunk, palik atvirą ~2 min. Tikėtina: SOC ~93%, ir GSM/sistema/akum temp/oro kokybė **užsipildo** (ryšys stabilus). Jei koks laukas užsispyręs lieka „---" — tada tai SSE mapping klaida tam sensoriui (tiksliai pataisysiu).

**Saugumas (kai pasiruošęs):**
6. Istorijos perrašymas + rakto/Script ID rotacija **arba** repo → privatus.

**Kokybė (neprivaloma):**
7. `localStorage` debounce; tinklo mutex; aiškūs `id:` ESP YAML'e; pašalinti `KemperisV5.html` + tuščius `lib/leaflet.*`.

> **Verdiktas:** kodas brandus ir gerai struktūruotas; beveik visi „neveikimai" telefone — ne atskiros klaidos, o dvi priežastys (senas APK + nestabilus ryšys), abi adresuotos šaltinyje. Grandinė stringa ties **build/deploy**. Saugumo skola (viešas raktas + paslaptys) — atskira, bet svarbi.
