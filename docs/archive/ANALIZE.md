# ANALIZE.md — Kempervanas ESPHome projektas
## Data: 2026-04-29

---

## 1. GPIO žemėlapis (iš kempervanas.yaml)

| GPIO | Funkcija | Magistralė |
|------|----------|-----------|
| GPIO7 | TDS jutiklis | ADC |
| GPIO9 | I2C SDA | I2C |
| GPIO10 | I2C SCL | I2C |
| GPIO16 | RS485 TX (Modbus) | UART (uart_junctek) |
| GPIO17 | RS485 RX (Modbus) | UART (uart_junctek) |
| GPIO40 | A7670E TX | UART (uart_a7670e) |
| GPIO41 | A7670E RX | UART (uart_a7670e) |
| GPIO47 | HX711 DOUT | GPIO |
| GPIO48 | HX711 CLK | GPIO |

**I2C įrenginiai (0x53, 0x29, 0x77, 0x21, 0x22, 0x24, 0x25)** — visi unikalūs, nėra konfliktų.

---

## 2. GPIO konfliktai

✅ **Nėra GPIO konfliktų** tarp šiuo metu naudojamų pinų.

⚠️ **GPIO48 pastaba**: ESP32-S3 naudoja GPIO48 kaip FSPICLK (SPI clock). Kadangi SPI nenaudojamas, konflikto nėra, bet tai reikia žinoti ateičiai.

---

## 3. Sensorių atitikimas reikalavimams

### GPS (A7670E)
| Reikalavimas | YAML | Statusas |
|---|---|---|
| Koordinatės (6 ženklai) | gps_coords text_sensor | ✅ |
| Aukštis | gps_alt_sensor | ✅ |
| Greitis km/h | gps_speed, accuracy_decimals: 1 | ⚠️ turi būti 0 |
| Kursas 0–360° | gps_course, accuracy_decimals: 0 | ✅ |
| GPS palydovai | gps_sats (bendras) | ⚠️ trūksta atskirų GPS/GLONASS/BeiDou |
| Fix statusas tekstas | Tik binary_sensor | ❌ trūksta "2D/3D/No fix" |

### Pokrypis (ADXL345)
| Reikalavimas | YAML | Statusas |
|---|---|---|
| Roll (K/D) | roll_sensor, accuracy_decimals: 1 | ⚠️ turi būti 0 |
| Pitch (P/G) | pitch_sensor, accuracy_decimals: 1 | ⚠️ turi būti 0 |
| Zero kalibravimas | btn_cal_tilt | ✅ |
| Domkratų patarimas (mm) | Nėra | ❌ trūksta |

### Dujų svoris (HX711)
| Reikalavimas | YAML | Statusas |
|---|---|---|
| Likutis kg | gas_net | ✅ |
| Procentai (%) | Nėra | ❌ trūksta |
| Tuščio baliono svoris | cylinder_tare (tara) | ✅ (veikia, kitaip pavadintas) |
| Pilno baliono svoris | Nėra | ❌ trūksta (reikia % skaičiavimui) |
| Tare mygtukas | btn_gas_tare | ✅ |
| Span kalibravimas | btn_gas_cal_span | ✅ |

### BMP180 (bmp085)
| Reikalavimas | YAML | Statusas |
|---|---|---|
| Temperatūra su ofset'u | cabin_temp | ✅ |
| Slėgis | cabin_pres | ✅ |
| Barometrinis aukštis | altitude_baro | ✅ |
| Orų prognozė | weather_forecast | ⚠️ slenkstinės reikšmės skiriasi |
| Temperatūros ofset'as | temp_offset | ✅ |
| QNH slėgis | sea_level_pressure | ✅ |

**Orų prognozė — slenkstinių reikšmių neatitikimas:**
- YAML naudoja: ±1 / ±3 / ±6 hPa/3h
- CLAUDE.md reikalauja: ±0.5 / ±2 / ±4 hPa/3h

### GSM (A7670E)
| Reikalavimas | YAML | Statusas |
|---|---|---|
| Operatorius | gsm_op | ✅ |
| Ryšio kokybė % | gsm_signal | ✅ |
| RSSI raw (0–31) | Nėra | ❌ trūksta |
| Tinklo registracija (AT+CREG) | Nėra | ❌ trūksta |
| SIM statusas | Nėra (buvo senoje versijoje) | ❌ trūksta |
| Atnaujinimo intervalas | 20s | ⚠️ turi būti 60s |

### SMS "Status?"
| Reikalavimas | YAML | Statusas |
|---|---|---|
| Case-insensitive | Tikrina tik "Status" | ⚠️ nepilnas |
| Formatas pagal CLAUDE.md | Skiriasi (nėra tilt, yra batt_temp) | ❌ neatitinka |
| ASCII <160 simbolių | ✅ | ✅ |

### Vandens bakas (VL53L0X)
| Reikalavimas | YAML | Statusas |
|---|---|---|
| Lygis cm | water_cm | ✅ |
| Lygis % | water_pct | ✅ |
| Lygis litrais | Nėra | ❌ trūksta |
| Bako tūris number | Nėra | ❌ trūksta |
| Kalibravimo mygtukai (tuščias/pilnas) | Nėra | ❌ trūksta |

### Šviesos reliai (PCF8574)
| Reikalavimas | YAML | Statusas |
|---|---|---|
| 8 šviesos (sviesa_1..8) | **Nėra jokių light/switch** | ❌ KRITINIS trūkumas |
| HTML POSTas /light/sviesa_N/toggle | Neveikia | ❌ KRITINIS |

---

## 4. Trūkstami komponentai (CLAUDE.md hardware tabel vs YAML)

| Komponentas | Turi būti | Statusas |
|---|---|---|
| VL53L0X #2 (XSHUT) | PCF8574 P0/P1 → 0x30 | ❌ Neįgyvendinta (sudėtinga boot logika) |
| SSD1306 OLED (0x3C) | I2C | ❌ Etapas C |
| AM2301 (GPIO39, DHT) | 1-Wire DHT | ❌ Pridėta kaip komentaras |
| MQ dujų jutiklis (GPIO1) | ADC | ❌ Pridėta kaip komentaras |
| UV LED (OUT10/pcf_out2 P1) | PWM arba switch | ❌ Pridėta kaip komentaras |
| Inverteris (OUT9/pcf_out2 P0) | Switch | ❌ Pridėta kaip komentaras |

---

## 5. Pakeitimai kempervanas_v2.yaml faile

### Kritiniai (HTML neveiks be jų)
1. ✅ Pridėtos `output:` platformos PCF8574 pinams (P0–P7 iš pcf_out1)
2. ✅ Pridėtos `light:` platformos sviesa_1..sviesa_8 (binary light per GPIO output)
   - ID `sviesa_N` atitinka HTML `/light/sviesa_N/toggle` formatą

### GPS pataisymai
3. ✅ `gps_speed` → `accuracy_decimals: 0`
4. ✅ Pridėtas `gps_fix_mode` global (0/2/3)
5. ✅ Pridėtas `gps_fix_text` text_sensor ("No fix" / "2D fix" / "3D fix")
6. ✅ Pridėti atskiri palydovų sensoriai: `gps_sats_gps_sensor`, `gps_sats_glonass_sensor`, `gps_sats_beidou_sensor`

### Pokrypis
7. ✅ `roll_sensor` → `accuracy_decimals: 0`
8. ✅ `pitch_sensor` → `accuracy_decimals: 0`
9. ✅ Pridėtas `jack_advice` text_sensor (domkratų patarimas mm, VW Crafter: track=1800mm, wheelbase=3665mm)

### Dujos
10. ✅ Pridėtas `gas_full_weight` number (pilno baliono svoris, default 23.5 kg)
11. ✅ Pridėtas `gas_pct` sensor (dujų likutis %)

### Vanduo
12. ✅ Pridėtas `tank_volume_liters` number (bako tūris litrais, default 100L)
13. ✅ Pridėtas `water_liters` sensor
14. ✅ Pridėti mygtukai: `btn_water_set_empty`, `btn_water_set_full`

### GSM
15. ✅ Pridėtas `gsm_creg_val` global
16. ✅ Pridėtas `sim_status` global (grąžinta iš senesnės versijos)
17. ✅ Pridėtas `gsm_creg` text_sensor (tinklo registracija)
18. ✅ Pridėtas `sim_status_sensor` text_sensor
19. ✅ UART parseris papildytas `+CREG:` ir `+CPIN:` apdorojimu
20. ✅ Intervalai padalinti: GPS 10s, GSM statusas 60s
21. ✅ `a7670e_init` papildytas `AT+CREG=1` komanda

### SMS
22. ✅ Formatas pakeistas pagal CLAUDE.md (pridėtas tilt, pašalintas batt_temp)
23. ✅ Case-insensitive "Status?" patikrinimas

### Orų prognozė
24. ✅ Slenkstinės reikšmės pakeistos pagal CLAUDE.md (0.5/2/4 hPa/3h)

---

## 6. Nepridėta (ateičiai)

- **VL53L0X #2** — reikia žinoti tikslų XSHUT PCF8574 pino numerį ir implementuoti boot-time adresų perskirstymą
- **SSD1306 OLED** — Etapas C
- **AM2301 / MQ / UV LED / Inverteris** — pridėta kaip komentarai, aktyvuoti kai hardware prijungtas
- **Google Sheets** — Etapas D

---

## 7. ETAPAS 2 — Kompiliacija ✅ UŽBAIGTAS

**Statinė analizė** (Claude + dokumentacija + ESPHome 2026.4 source) atskleidė 4 kritines klaidas:

| # | Klaida | Vieta | Pataisa |
|---|---|---|---|
| 1 | `external_components: path: components` — aplankas neegzistuoja | eilutės 13–16 | Blokas pašalintas |
| 2 | `web_server_base:` ir `dashboard_endpoint:` — komponentų nėra ESPHome 2026.4 | apie 30–39 | Abu blokai pašalinti, `web_server:` paliktas |
| 3 | `pressure_hist` su `type: std::array<float,7>` + `restore_value: true` — `RestoringGlobalsComponent` template klaida | apie 158–165 | `restore_value: false` (ir `pressure_hist_count` taip pat) |
| 4 | `attenuation: 11db` — deprecated nuo 2025.x | apie 521 | Pakeista į `12db` |

**Rezultatas:**
- `esphome config kempervanas_v2.yaml` → `INFO Configuration is valid!`
- `esphome compile kempervanas_v2.yaml` → praėjo be klaidų (per Windows PowerShell)

### User'io pirminės analizės netikslumai (paneigti tyrimu)

| Teiginys | Tikrovė |
|---|---|
| NBSP simboliai sugadina YAML | Faile **0 NBSP** simbolių (patikrinta byte'ais) |
| `!= 0` su I2C — type mismatch | `i2c::ErrorCode` yra plain `enum` (ne `enum class`), `ERROR_OK = 0`. Veikia. |
| Naudoti `- platform: adxl345` | ADXL345 **NĖRA** ESPHome 2026.4 core komponentas. Lambda metodas yra vienintelis stock variantas. |
| `0` vs `0.0f` snprintf | C++17 implicit promotion `int → float` argumentuose veikia. Ne klaida. |
| `pressure_hist_count: int` su `restore_value: true` neveikia | `int` su `restore_value` veikia. Tik `std::array` problema. |

---

## 8. Sekantis etapas — ETAPAS 3 (Sensorių funkcionalumas)

Reikia hardware (KC868-A16v3.1 prijungto):
- BMP180, VL53L0X, HX711, ADXL345, A7670E (GPS+GSM+SMS), BME688
- 8 šviesos + jungikliai (jungiklis_1..8 fizinių jungiklių dar **nėra YAML** — pridėti kai prijungti)
- Renogy BLE — atskiras tyrimas (ETAPAS 1 krok. 6, **neatliktas**)

### YAML papildymai (po user'io užklausos 2026-04-29)

✅ **Pridėta**:
- `binary_sensor: jungiklis_1..8` — PCF8574 0x22 P0–P7, `inverted: true`, `on_press: light.toggle: sviesa_N`
- `button: btn_lights_all_off` — virtualus mygtukas, vienu metu išjungiantis visas 8 šviesas
- `pcf_lights` (0x24) ir `pcf_switches` (0x22) pervadinta — aiškesni vardai
- Pašalintas `pcf_in1 0x21` (deklaruotas, nenaudojamas — 220V perkeltas į atskirą ESP32)
- Pašalintas `binary_sensor: power_220v` ir `220V:` eilutė SMS šablone
- Pašalinti komentuoti AM2301/MQ/UV LED/Inverter blokai (galima atstatyti iš git ar v1 kai bus hardware)

⚠️ **Likę**:
- **Dashboard prefiksai `:` vs `|`** — CLAUDE.md reikalauja `|` separatoriaus web_server UI grupavimui. Bet projektas naudoja `kemperis1.html` per REST API (kuris naudoja sensoriaus `id:`, ne `name:`). Todėl pakeitimas grynai kosmetinis, taikomas tik native ESPHome web UI. **Nėra kompiliacijos klaida.**
- **`cylinder_tare` logikos klausimas** — `gas_net = gas_total - cylinder_tare`. Jei tare mygtukas naudojamas su pilnu balionu, gauname dvigubą atimtį. Reikia perskaityti UX scenarijų.

---

## 9. Renogy BLE tyrimas (CLAUDE.md ETAPAS 1 krok. 6) ✅ UŽBAIGTAS

### Hardware galimybės

✅ **KC868-A16v3.1 (ESP32-S3) palaiko BLE 5.0** — patvirtinta. Veikia paraleliai su WiFi AP (atskirti radio stackai).

### Renogy BLE protokolas

Abu įrenginiai (DCC50S ir PRO 100Ah LiFePO4) naudoja **identišką BLE Modbus protokolą** per integruotą BT modulį:

| Parametras | Reikšmė |
|---|---|
| Service UUID (write) | `FFD0` |
| Write Characteristic | `FFD1` |
| Service UUID (notify) | `FFF0` |
| Notify Characteristic | `FFF1` |
| Modbus device ID DCC50S | `96` (0x60) |
| Modbus device ID Renogy LiFePO4 | `48–55` (priklauso nuo BMS) |
| Update interval | 30s rekomenduojama (battery saving) |

### Sprendimo variantai

**Variantas A — native KC868 BLE (rekomenduojamas):**
- ESP32-S3 yra dual-mode (BT + WiFi). BLE veiks lygiagrečiai su `web_server`, `wifi.ap`, GSM UART
- ESPHome komponentai: `esp32_ble_tracker` + 2× `ble_client` (po vieną kiekvienam įrenginiui)
- Bazė: kopijuoti `mavenius/renogy-bt-esphome` Rover YAML, pakeisti device ID į 96 (DCC50S)
- Modbus komandų konstravimas + parsing per lambda
- **Privalumai**: vienas hardware, paprasta
- **Trūkumai**: KC868 RAM apkrova padidės (BLE stack ~64KB), reikia patikrinti stabilumą

**Variantas B — atskiras ESP32 + ESP-NOW arba HTTP:**
- Atskiras ESP32 (~3€) skaito Renogy per BLE → siunčia į KC868 per ESP-NOW (no internet) ar HTTP API (per WiFi AP)
- **Privalumai**: izoliuotas, neapkrauna pagrindinio kontrolerio
- **Trūkumai**: dar vienas įrenginys, dar viena maitinimo linija

**Variantas C — ESP-IDF Bluetooth kaip atskiras ESP32 → MQTT:**
- Reikia MQTT brokerio (no internet → tinka tik su mosquitto AP)
- Sudėtingiau, neaktualu šiame etape

### Rekomendacija

**Variantas A** — pradėti su native KC868 BLE. Jei stabilumo problemos atsiras (pvz. WiFi+BLE konfliktai, RAM perkrova), pereiti į Variantą B.

### Šaltiniai

- [mavenius/renogy-bt-esphome](https://github.com/mavenius/renogy-bt-esphome) — Rover protokolo bazė ESPHome formatu (DCC50S diskutuojamas issues)
- [cyrils/renogy-bt](https://github.com/cyrils/renogy-bt) — Python referencinis implementavimas, palaiko DCC50S ir LiFePO4
- [esphome-renogy-ble.yaml gist](https://gist.github.com/mateuszdrab/922c760582fce29d63608a1a405c541b) — minimalus BLE batterijos pavyzdys
- [ESPHome ble_client docs](https://esphome.io/components/ble_client.html)
- [Home Assistant: BLE_Client read Modbus register Renogy](https://community.home-assistant.io/t/ble-client-read-modbus-register-as-sensor-similar-to-modbus-controller-renogy/763721)

### Sekantis žingsnis (kai bus pasiruošta įgyvendinti)

1. Surasti DCC50S ir batterijos BLE MAC adresus (su `esp32_ble_tracker` skenavimu, po flash'inimo)
2. Pridėti `esp32_ble_tracker:` ir 2× `ble_client:` blokus
3. Parsinti atsakymus per `on_notify` lambda (Modbus CRC patikrinimas + duomenų išskyrimas)
4. Publikuoti per `template` sensorius su CLAUDE.md reikalaujamais matavimais

---

## 11. 📍 Sustojimo taškas — 2026-05-01

### Dabartinė būsena

- ✅ `kempervanas_v3.yaml` (1349 eil., 40.7KB) praeina `esphome config` be klaidų ir warning'ų
- ✅ Pridėtas Renogy BLE: 2× ble_client, ble_write užklausos, on_notify dekodavimas
- ✅ Logger nustatytas į VERBOSE su BLE komponentų tracking'u (laikinai)
- ✅ MAC adresai patvirtinti (user'is patikrino)
- ✅ BME688 ir Junctek Modbus **sąmoningai NEPRIDĖTI** — projekte nenaudojami

### Sekantis veiksmas (user'is praneš)

User'is **fiziškai eis prijungti KC868** ir paleis:
```powershell
esphome run kempervanas_v3.yaml
esphome logs kempervanas_v3.yaml
```

### Ką stebėti log'uose (su VERBOSE)

**1. Boot tvarka (pirmosios 30s):**
- `[I2C]` skenavimas — turi rodyti adresus `0x29, 0x53, 0x77, 0x22, 0x24, 0x25`
- ADXL345 inicializacija (0x53 register 0x2D = 0x08)
- PCF8574 inicializacija visiems trims (0x22, 0x24, 0x25)
- A7670E AT komandos (`AT`, `AT+CMGF=1`, `AT+CGNSSPWR=1` ir kt.)

**2. Renogy BLE (po 5–60s nuo boot):**
- `[ble_client] Connected to ...` arba `Connection failed` — jei MAC neteisingas, nepasijungs
- `[ble_client] Discovering services` — characteristic'us atras
- `[ble_client] Notify characteristic ...` — subscribe pavyko
- Periodiškai (kas 10s/13s): `[ble_client] Writing to FFD1` + `[ble_client] Notify received N bytes`

**3. GPS fix (gali trukti 1–10 min iš naujo paleidus):**
- `+CGNSSINFO: 0,...` = no fix
- `+CGNSSINFO: 3,N,M,K,...` = 3D fix (N=GPS, M=GLONASS, K=BeiDou palydovų)

**4. Sensorių publikavimas:**
- `[sensor]` template — bus tik `INFO` lygis (sumažintas per `logs:`)
- `Pokrypis Kaire-Desine: 0` — ADXL345 atsako
- `Vanduo: Lygis (cm)` — VL53L0X duoda atstumą
- `Atmosferos slėgis: 1013` — BMP180 veikia

### Greiti diagnostiniai filtrai

Jei log'as per triukšmingas, paleisk su filtru:
```powershell
# Tik BLE įvykiai:
esphome logs kempervanas_v3.yaml | Select-String "ble_"

# Tik klaidos:
esphome logs kempervanas_v3.yaml | Select-String "ERROR|WARN|fail"

# Tik Renogy raw bytes:
esphome logs kempervanas_v3.yaml | Select-String "renogy|FFF1|notify"
```

### Po sėkmingo paleidimo — ką patikrinti

| Patikra | Ko ieškoti |
|---|---|
| Šviesos | Web UI (192.168.4.1) → toggle sviesa_1 → relė turi spragtelėti |
| Jungikliai | Fizinis jungiklis_1 → log'as `light.toggle: sviesa_1` → relė reaguoja |
| GPS | Po 1–10 min: `gps_fix_text` → "3D fix", koordinatės teisingos |
| Renogy bat | `bat_voltage` → ~12.8–13.4V, `bat_soc` → 0–100% |
| Renogy DCDC | `dcdc_bat_v` → ~12V, `dcdc_solar_power`/`dcdc_alt_power` jei kraunama |
| HX711 | Tare → svoris ~0kg, uždėjus daiktą → keičiasi |
| Pokrypis | Pakreipus kontrolerį → roll/pitch reaguoja |
| TDS | Į vandenį panardinus → reikšmė >0 |

### Jei kažkas neveiks

Visi log'ai išsaugok į failą:
```powershell
esphome logs kempervanas_v3.yaml > "$env:TEMP\kempervanas_log_$(Get-Date -Format 'yyyyMMdd_HHmm').txt"
```
Atsiųsk tą failą sekančioje sesijoje analizei.

---

## 10. 📍 Sustojimo taškas — 2026-04-29 vakaras

### Dabartinė būsena

- ✅ `kempervanas_v2.yaml` praeina `esphome config` (ESPHome 2026.4.0)
- ✅ `esphome compile` praėjo be klaidų (per Windows PowerShell, ne MSys bash)
- ✅ ETAPAS 1 (analizė), ETAPAS 2 (kompiliacija) — užbaigti
- ✅ Pridėti: 8 fiziniai jungikliai, virtual all-off mygtukas
- ✅ Išvalyta: nenaudojami PCF8574 deklaracijos, komentuoti future blokai, power_220v
- ✅ Renogy BLE protokolas ištyrtas — sprendimas pasirinktas (Variantas A, native KC868 BLE)

### Ką galima daryti BE hardware (sekančioje sesijoje)

1. **`cylinder_tare` UX logikos auditas** — patikrinti ar tare scenarijus su pilnu/tuščiu balionu duoda teisingą `gas_net`
2. **Dashboard prefiksai `:` → `|`** — jei reikia native ESPHome web UI grupavimo (kosmetinis, nekritinis)
3. **HTML dashboard pakeitimai** — nauji elementai: jungiklis_1..8 statusas, btn_lights_all_off mygtukas, jei norisi rodyti UI

### Ką galima daryti TIK SU hardware prijungtu

1. **Pirmas flash + sensorių patikrinimas** — `esphome run kempervanas_v2.yaml` per USB-C ar OTA
2. **ETAPAS 3 — sensorių funkcionalumas** (BMP180, VL53L0X, HX711, ADXL345, BME688, A7670E GPS+GSM, šviesos, jungikliai)
3. **Renogy BLE MAC scan** — pridėti laikinai `esp32_ble_tracker: scan: true`, peržiūrėti logus, surasti DCC50S ir LiFePO4 MAC
4. **Renogy `ble_client:` integracija** — pridėti 2 BLE klientus su rastais MAC, parsinti Modbus atsakymus

### Ką dar reikia patikrinti (atidėta)

- BME688 — `bme680_bsec` ar `bme68x` komponentas? CLAUDE.md sako patikrinti https://esphome.io/components/sensor/bme680.html
- VL53L0X #2 (jei kada nors bus pridėtas) — XSHUT pino numeris ant PCF8574

### Failai, kurie šiuo metu yra autoritetiniai

- [kempervanas_v2.yaml](kempervanas_v2.yaml) — galutinė versija (validuota)
- [ANALIZE.md](ANALIZE.md) — šis dokumentas (sekcijos 7–10)
- `kempervanas.yaml` — sena versija, **neredaguoti**, palikta atsarginei kopijai
- `kempervanas veikiantis su gps ir sms.yaml` — referencinė veikianti senesnė iteracija
