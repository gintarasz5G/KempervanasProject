# Projekto auditas, rinkos palyginimas ir pasiūlymai — 2026-07-02

**Apimtis:** visas repo (firmware 2673 eil., app 3428 eil., Apps Script 125 eil., dokumentacija), rinkos palyginimas (komerciniai + open source), asistento audito `deep_audit_and_proposals_v32_3.md` vertinimas.
**Metodai:** kodo analizė su eilučių nuorodomis, git istorija, web paieška (2026-07).

---

## 1. Santrauka

Projektas techniškai brandus — hibridinis logeris, GPS galios valdymas, GSM trianguliacija ir SSE grandinė yra rimtesnės inžinerijos nei daugumoje open source kemperių projektų. Didžiausios problemos ne funkcinės, o **saugumo** (4 skylės, iš jų 1 kritinė) ir **dokumentacijos dreifo** (CLAUDE.md aprašo neegzistuojančią techninę įrangą, o tai klaidina ir žmogų, ir AI agentą).

| # | Radinys | Svarba |
|---|---------|--------|
| S1 | SMS komandos vykdomos iš **bet kokio** siuntėjo (įsk. `+disarm`) | 🔴 Kritinė |
| S2 | OTA be slaptažodžio + API be encryption key + web_server be auth | 🔴 Aukšta |
| S3 | Apps Script `doPost` be autentifikacijos + formulės injekcijos rizika | 🟠 Vidutinė |
| S4 | WiFi AP slaptažodis atviru tekstu commitintame YAML ir CLAUDE.md | 🟠 Vidutinė |
| D1 | CLAUDE.md ↔ firmware neatitikimai (BMP180, PCF8574 šviesos, OLED, AM2301, TDS pinas…) | 🟠 Aukšta (procesui) |
| D2 | Domkratų patarimas įgyvendintas **app'e** (ne firmware, kaip sako CLAUDE.md spec) — web UI ir SMS jo neturi | 🟡 Žema |
| K1 | Laikinas `junc_hex` HEX dump INFO lygiu kiekvienai BLE notifikacijai | 🟡 Žema |
| K2 | Nėra CI (net `esphome config` validacijos) ir automatinių SSE mapping testų | 🟠 Vidutinė |

---

## 2. Saugumo auditas (svarbiausia dalis)

### S1. SMS komandos be siuntėjo patikros — kritinė
`firmware/kempervanas.yaml`: `sms_sender` išgaunamas (~1745 eil.), bet naudojamas **tik SMS istorijai** (~1754 eil.). Komandų blokas (~1772–1841 eil.) vykdo `+disarm`, `+stop google`, `+ikelk` ir kt. nepriklausomai nuo siuntėjo. Kas žino SIM numerį, gali SMS'u **išjungti vagystės apsaugą**. Atsakymai siunčiami tik `txt_admin_number`, bet būsenos keitimas įvyksta.

**Taisymas (~3 eilutės):** komandų bloko pradžioje:
```cpp
// Tik admin numeris gali valdyti (lyginame pagal galūnę, nes formatai gali skirtis)
std::string adm = id(txt_admin_number).state;
bool authorized = !adm.empty() && id(sms_sender).length() >= 8 &&
  (adm.find(id(sms_sender).substr(id(sms_sender).length() - 8)) != std::string::npos);
if (!authorized) { ESP_LOGW("sms", "Neautorizuotas siuntejas: %s", id(sms_sender).c_str()); }
else { /* esamas komandų blokas */ }
```

### S2. OTA / API / web_server be apsaugos
37–47 eil.: `web_server` be auth, `api` be `encryption.key`, `ota` be `password`. AP slaptažodis viešas → bet kas šalia stovintis kemperio gali prisijungti ir **perrašyti firmware per OTA** arba spaudyti mygtukus per web UI. Taisymas: `ota: password: !secret ota_password`; `api: encryption: key: !secret api_key`; web_server `auth: username/password` (patikrinti, ar app SSE moka basic auth — jei ne, bent OTA+API apsaugoti, tai nekeičia SSE).

### S3. Apps Script be autentifikacijos + formulių injekcija
`GoogleSheetsScript.js doPost` (62 eil.) priima bet kokį POST — URL atspėjus/nutekėjus, bet kas gali rašyti į Sheets. Be to, gaunami laukai rašomi `setValues` be sanitizacijos: eilutė, prasidedanti `=`, `+`, `-`, `@`, Sheets bus interpretuota kaip formulė (pati skripto `HYPERLINK` formulė 92 eil. tai įrodo). Taisymas: (a) firmware prideda `&k=<token>` ir skriptas tikrina; (b) prieš `push` sanitizuoti: `if (/^[=+\-@]/.test(cols[c])) cols[c] = "'" + cols[c];` (išskyrus sąmoningą 1-ą stulpelį).

### S4. Paslaptys repo
`kemperis123` yra ir YAML (35 eil.), ir CLAUDE.md — repo yra GitHub'e. Kemperio SIM numeris hardcoded (455 eil.), Junctek MAC (69 eil.). Perkelti į `secrets.yaml` (jis teisingai gitignore'intas; `secrets.yaml.example` jau yra — geras šablonas).

---

## 3. Dokumentacijos ir kodo neatitikimai (D1)

CLAUDE.md yra agento „konstitucija", bet aprašo kitą įrenginį nei realus firmware:

| CLAUDE.md teigia | Firmware realybė |
|---|---|
| BMP180 @0x77 **ir** BME688 @0x76 | Tik `bme680` @ **0x77** (813–814 eil.); BMP180/bmp085 nėra |
| PCF8574 ×3, 8 šviesos + 8 jungikliai, grupė „Šviesos \|" | PCF8574 ir šviesų firmware **nėra visai** |
| SSD1306 OLED, AM2301 1-Wire | Nėra |
| TDS ant GPIO7 | TDS ant **GPIO6** (884 eil.); GPIO7 = 220V shore ADC (860 eil.) |
| 220V per PCF8574 DIN | Per ADC GPIO7/GPIO5 (859–880 eil.) |
| Domkratų patarimas — firmware text_sensor | Firmware jo neturi; **app skaičiuoja pats** (index.html 1808–1827 eil., naudoja `wheelbase_cm`/`track_cm` iš number'ių). Funkcija yra, tik kitame sluoksnyje — web UI ir SMS „Tilt" eilutė patarimo nerodo (D2) |
| Orų prognozės 7 lygių tekstai („Labai greitai gerėja"…) | Kiti 5 tekstai („Saulėta ☀️"… 1341–1345 eil.) |
| SCD4x tik „Miegamasis" grupėje (yra ✓) | ✓ atitinka |

Pasekmė: AI agentas, sekdamas CLAUDE.md, „taisys" YAML pagal neegzistuojančią spec. **Rekomendacija:** atnaujinti CLAUDE.md, kad atspindėtų faktinę būseną, o planus perkelti į „Ateities etapai" skyrių (kaip jau padaryta su MQ/OBD2). Tai pigiausias didelės vertės pataisymas visame audite.

---

## 4. Kodo kokybė

Kas gerai: BLE paketo defragmentacija su „uodegos" buferiu (1250–1319 eil.), modem_busy watchdog (1436–1449), buferio retinimas 50 % (thin_buffer), Haversine dreifavimo filtras, GPS „žadink agresyviai, patvirtink per GPS" logika, HTTP buferis valomas tik po patvirtinto 200/302 — visa tai brandu.

Kas taisytina:

1. **K1 — `junc_hex` dump** (1263–1270 eil.): pažymėtas „LAIKINAS", bet shipintas. INFO lygiu loguoja kiekvieną BLE notifikaciją (~kas 1–2 s) — UART log flood + CPU. Pašalinti arba pakabinti ant `debug_log`.
2. **K2 — nėra CI.** Minimalus GitHub Actions: `esphome config firmware/kempervanas.yaml` + jau egzistuojančios node SSE simuliacijos (iš v35 audito) pavertimas repo testu (`tools/sse_mapping_test.js`). Tai automatiškai pagautų „pervadinus jutiklį — app reikšmė tyliai dingsta" klasės regresijas, kurios šiame projekte jau kartojosi.
3. **Monolitas.** 2673 eil. YAML — ESPHome `packages:` leidžia skaidyti (gps.yaml, sms.yaml, junctek.yaml, logger.yaml) be funkcinių pakeitimų. Nekritiška, bet mažina merge klaidų riziką.
4. **Junctek parseris.** Savas BLE parseris veikia, bet egzistuoja bendruomenės komponentai (Sleeper85/esphome-junctek_khf, gianfrdp — UART per LINK portą, jowser7/Junctek_BLE). Kadangi valdiklio dėžutė sudegusi ir jungtis tiesiai prie šunto, savas BLE sprendimas pagrįstas — bet verta palyginti checksum logiką su jų PROTOCOL analize (dabartinis kodas checksum **netikrina**, tik atmeta pagal ribas).
5. **Smulkmenos:** GPS greičio `accuracy_decimals: 1` (spec sako 0); keliamųjų metų logika dubliuota dviejose šakose (1720–1734 eil.); `logger.level: INFO` + gausus ESP_LOGI kiekvienam įvykiui.

---

## 5. Rinkos palyginimas

### Komerciniai sprendimai

| Sistema | Stiprybė | Kaina | Ko Kemperis neturi |
|---|---|---|---|
| **Victron Cerbo GX + VRM** | Energijos ekosistema, cloud istorija, saulės prognozė | ~€300 + ekosistema | Ilgalaikė istorijos analitika, ekosistemos integracija |
| **Simarine Pico** | Atskirų vartotojų (load) monitoringas | ~€400+ | Srovės šakų matavimas per kanalą |
| **Truma iNet X** | Klimato valdymas, klaidų aiškinimas | Panel ~€250 | Šildymo/klimato valdymas |
| **CaraControl** | NB-IoT/LTE-M, 16 belaidžių durų/langų jutiklių, 120 dB sirena, tilt alarm, Keyless | €665 bazinis + tarifas | Sirena, durų jutikliai, keyless, profesionalus tracking tinklas |
| **CarPro-Tec Fusion 4G** | Specializuota signalizacija + GPS + SIM | ~€400–700 | — (Kemperis dengia SMS aliarmais) |
| **Smarty Van** (JAV) | Užsakomoji integracija ant Home Assistant + Victron: pilnas valdymas (šviesos, HVAC, vandens vožtuvai, lovos keltuvai), kameros su ML asmens aptikimu, presence detection (BT/WiFi), Starlink valdymas, automatizacijų scenarijai | Individuali sąmata (bespoke paslauga, kainos neskelbia) | Apkrovų valdymas, automatizacijų variklis („jei-tai"), presence detection, kameros |

### Open source projektai

| Projektas | Kas tai | Palyginimas su Kemperiu |
|---|---|---|
| **OpenSRV / OpenSRVmini** (campingtech.de) | DE bendruomenės kemperio automatika (ESP32, BME280, MPU6050) | Platesnė bendruomenė, bet be GSM/SMS ir vagystės sekimo |
| **ConnectedVan** | ESPHome + Home Assistant integracija (bakai, baterija, klimatas, GPS) | Reikalauja HA serverio vane; Kemperis savarankiškesnis |
| **oh2mp/esp32_smart_rv** | BLE ekranas (Mopeka dujų jutikliai, Ruuvi) | Tik monitoringo ekranas |
| **blinken/vandal** | 10 kanalų šviesų valdymas, LoRa, CAN | Turi tai, ko Kemperiui trūksta — apkrovų valdymą |
| **HerrRiebmann/Caravan_Leveler_2** | Lygiavimo hotspot įrankis | Kemperio ADXL345 + kalibracija + app domkratų cm skaičiavimas — pranašesnis |

### Kemperio unikalumai rinkoje
Nė vienas rastas projektas nekombinuoja: (1) SMS valdymo be jokio interneto/cloud, (2) GSM trianguliacijos kaip GPS atsargos vagystės aptikimui, (3) hibridinio GPS galios valdymo (~70 mA taupymas su heartbeat pre-wake), (4) nemokamo cloud per Google Sheets, (5) TTS balso aliarmų, (6) žvejybos prognozės. Tai reali niša — „autonominis, be prenumeratos" segmentas, kur CaraControl ima €665 + tarifą.

### Didžiausias atsilikimas nuo rinkos
**Valdymas ir automatizacija.** Visi komerciniai konkurentai ir Vandal valdo apkrovas (šviesos, šildymas, relės); Kemperis — tik monitoringas. CLAUDE.md 8 šviesų spec yra būtent šios spragos planas, bet neįgyvendintas. Smarty Van rodo, kur šis segmentas jau nuėjęs: ne tik valdymas, bet automatizacijų variklis („welcome home" šviesos pagal presence, inverterio auto-išjungimas, vožtuvų uždarymas) — v32.3 audito roadmap idėjos iš esmės atkartoja Smarty Van funkcijų sąrašą. Skirtumas: Smarty Van reikalauja HA serverio, Victron ekosistemos ir interneto (Starlink), o Kemperio niša — autonomija be prenumeratų. Antras atsilikimas — **ilgalaikė analitika**: duomenys Sheets'e yra, bet app rodo tik paskutinę eilutę.

---

## 6. Asistento audito (v32.3) vertinimas

**Kas gerai:** lyginamosios lentelės ašys parinktos taikliai (duomenų gavimas, pranešimai, valdymas, unikalumas); teisingai identifikuota, kad programėlė pasyvi ir trūksta analitikos; roadmap idėjos (Solar Readiness, Adaptive Time-to-Empty, „Saugus išvykimas", Blackout Mode) — protingos ir vertos backlog'o.

**Trūkumai:**

1. **Apimtis siauresnė nei pavadinimas.** Tai app auditas, ne projekto: firmware, Apps Script ir saugumas nepaliesti. Nė viena iš šio audito S1–S4 skylių nepaminėta — o SMS autorizacijos nebuvimas tiesiogiai paverčia jo siūlomą „Geofencing & Security Plus" (v35) beprasmiu, kol `+disarm` gali atsiųsti bet kas.
2. **Nepatikrinti rinkos teiginiai.** „Truma iNet X — dažni BT lūžiai" ir „VictronConnect — priklauso nuo Cerbo GX" pateikti be šaltinių; VRM turi push pranešimus ir plačią cloud analitiką, tad palyginimo eilutė „Pranešimų forma" Victron'ui neatspindi realybės. CaraControl „Global LTE-M" — tiksliau NB-IoT/LTE-M su mokamu tarifu, tai svarbus TCO argumentas Kemperio naudai, kurio auditas neišnaudojo.
3. **Nepastebėta, kad dalis roadmap jau egzistuoja.** „Virtuali tvora, nuolatinis lokacijos transliavimas jei pajuda" (v35) — firmware **jau turi**: ADXL judesio aliarmą, GSM trianguliacijos poslinkio aliarmą (>800 m), sekimo režimą kas 30 s su nedelsiamu upload'u (2059–2080 eil.). Siūlyti tai kaip naują v35 funkciją — ženklas, kad firmware nebuvo skaitytas.
4. **„Welcome Home" (BT aptikimas)** reikalauja šviesų valdymo, kurio nėra — priklausomybė nenurodyta. (Rinkoje tai jau standartas: Smarty Van presence detection per BT/WiFi + automatinės šviesos — idėja pagrįsta, bet eiliškumas privalo būti: pirma šviesų valdymas, tada presence.)
5. Nėra prioritetų/įgyvendinamumo vertinimo ir eilučių nuorodų — pasiūlymai neveiksmingi kaip darbo sąrašas.

**Verdiktas:** naudingas kaip vizijos dokumentas, nepakankamas kaip auditas. Jo roadmap perimtas žemiau su pataisytais prioritetais.

---

## 7. Pasiūlymai (prioritetų eile)

**P0 — sauga (prieš bet kokias naujas funkcijas, ~1 vakaras):**
SMS siuntėjo autorizacija (S1); `ota.password` + `api.encryption.key` (S2); Apps Script token + formulių sanitizacija (S3); slaptažodžių iškėlimas į secrets.yaml (S4).

**P1 — procesas ir patikimumas (~1–2 vakarai):**
CLAUDE.md sinchronizacija su realybe (D1); `junc_hex` dump pašalinimas (K1); GitHub Actions su `esphome config` + SSE mapping node testu (K2); atidėtų app pataisymų sąrašas iš ankstesnių auditų (binary_sensor SSE šaka, TTS EN-fallback).

**P2 — funkcinės spragos prieš rinką:**
1. **Šviesų valdymas (PCF8574)** — užbaigia CLAUDE.md spec ir uždaro „tik monitoringas" spragą prieš CaraControl/Vandal. Didžiausia funkcinė vertė.
2. **Domkratų patarimas firmware'e** (neprivaloma) — app jau skaičiuoja; text_sensor firmware'e pridėtų jį į web UI ir SMS `+status` (~20 eil.), arba tiesiog atnaujinti CLAUDE.md, kad spec vieta = app.
3. **Adaptive Time-to-Empty** (iš v32.3 audito) — slankusis srovės vidurkis vietoje Junctek momentinio.
4. **Istorinės tendencijos app'e** — Sheets jau kaupia; `?action=history&hours=24` endpoint + sparkline kortelėse.

**P3 — vizija (perimta iš v32.3 audito, pataisyta tvarka):**
„Saugus išvykimas" checklist (v34 idėja — gera, nereikalauja naujos įrangos); Solar Readiness pagal orų prognozę; Blackout Mode; „Welcome Home" — tik po P2.1.

---

## Šaltiniai
- [CaraControl paketai ir kainos](https://www.caracontrol.eu/), [Basic Package €665](https://www.caracontrol.eu/v3-basic-package/p89), [Security Package](https://www.caracontrol.eu/security-package/p6)
- [CarPro-Tec Fusion 4G](https://carprotec.eu/en/products/4g-carpro-tec-gps-wohnmobilalarmanlage)
- [Smarty Van — bespoke mobile smart home integration](https://smartyvan.com/)
- [Victron Cerbo GX apžvalga (Panbo, Simarine palyginimas)](https://panbo.com/victron-cerbo-gx-good-ac-dc-power-monitoring-gets-better/), [nohma Cerbo GX vanams](https://nohma.com/van-conversion/electrics/cerbo-gx-full-electrical-system-monitoring-for-campervans/)
- [OpenSRV (campingtech.de)](https://campingtech.de/opensrv), [oh2mp/esp32_smart_rv](https://github.com/oh2mp/esp32_smart_rv), [blinken/vandal](https://github.com/blinken/vandal), [HerrRiebmann/Caravan_Leveler_2](https://github.com/HerrRiebmann/Caravan_Leveler_2), [GitHub campervan tema](https://github.com/topics/campervan)
- [Sleeper85/esphome-junctek_khf](https://github.com/Sleeper85/esphome-junctek_khf), [gianfrdp/esphome-junctek_khf](https://github.com/gianfrdp/esphome-junctek_khf), [jowser7/Junctek_BLE](https://github.com/jowser7/Junctek_BLE)

*Audito data: 2026-07-02. Bazinė versija: v32.3 (commit 8d19a1d).*
