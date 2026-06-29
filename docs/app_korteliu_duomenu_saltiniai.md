# App kortelių duomenų šaltiniai ir atnaujinimo dažnis

> Sudaryta: 2026-06-29. Šaltinis: `android/www/index.html` + `firmware/kempervanas.yaml`.
> App yra **event-driven** — ESPHome pati „pushina" sensoriaus reikšmę per SSE, app tą akimirką
> perpiešia kortelę. Todėl kortelės atnaujinimo dažnis = to sensoriaus `update_interval` firmware'e.

## Kaip app gauna duomenis (3 šaltiniai)

1. **ESP32 SSE srautas** (`http://192.168.4.1/events`) — pagrindinis. Event-driven push;
   dažnis = sensoriaus `update_interval` firmware'e.
2. **Google Sheets** (offline atsarginis) — kai SSE tyli, app kas **60 s** traukia paskutines
   reikšmes iš debesų (`android/www/index.html:1922`).
3. **Apskaičiuojama pačioje app** — kai kurios kortelės nėra atskiras sensorius, o app skaičiuoja
   iš atplaukiančio srauto (energija Wh, maršruto statistika, žvejybos patarimas, vietinis CSV).

---

## Kortelės pagal skiltis

### 🏠 Apžvalga
| Kortelė | Šaltinis | Dažnis |
|---|---|---|
| 🔋 Akum. įkrova / ⚡ statusas | Junctek BLE šuntas (`junc_soc`,`junc_v`,`junc_a`) | 5 s |
| 💧 Vanduo | VL53L0X lazeris (`water_level_pct`) | 5 s |
| 🔥 Dujos | HX711 svoris (`gas_remaining_pct`) | 5 s |
| 🌡️ Temperatūra / 💦 Drėgmė / 🌬️ Oro kokybė | BME688 (`bme_temp`,`bme_humidity`) | 30 s |
| 🛏️ CO₂ miegamasis | SCD4x (`bedroom_co2`) | 30 s |
| 💡 Ką daryti? | ESP template `system_advice` | 30 s |
| ☁️ Orų prognozė | ESP template `weather_forecast` (iš BME slėgio) | 30 s |
| ⚡ 220V | PCF8574 CH3/CH4 → `power_source_220` | 5 s |
| 🔐 Apsauga | switch būsena (push'inama įvykus pokyčiui) | event |

### 📍 GPS
| Kortelė | Šaltinis | Dažnis |
|---|---|---|
| Koordinatės | A7670E GPS `gps_coords_sensor` | 5 s |
| Greitis / Aukštis / Kryptis | A7670E `gps_speed/alt/heading` | 5 s |
| Iš viso palydovų + NAVSTAR/GLONASS/BeiDou/Galileo | A7670E `gps_sats_*` | 10 s |
| Maršrutas / GPX / Atstumas / Vid. greitis / Maks. greitis | **App skaičiuoja** iš GPS srauto | event (kaupia) |

### 🔋 Energija
| Kortelė | Šaltinis | Dažnis |
|---|---|---|
| Statusas / SOC / Įtampa / Srovė / Galia / Ah likutis / Temperatūra / Likęs veikimas | Junctek **BLE** šuntas (`junc_*`) | ~5 s |
| Aktyvus šaltinis / Išorinė 220V / Keitiklis | PCF8574 skaitmeniniai įėjimai `power_source_220`,`inverter_220_active` | 5 s |
| Apyvarta šiandien / Vid. galia / Maks. galia / Sekama nuo | **App skaičiuoja** (integruoja galią laike) | event (kaupia) |

> ⚠️ **Pastaba:** CLAUDE.md teigia, kad Junctek jungiamas per **RS485 Modbus**, bet realiame firmware
> jis skaitomas per **BLE** (`ble_client`, UUID FFF0/FFF1). Renogy saulės kroviklio/akum atskiro BLE
> sensoriaus firmware'e **nėra** — „Energija" duomenys ateina iš Junctek šunto, ne iš Renogy.

### 🌿 Aplinka / Resursai
| Kortelė | Šaltinis | Dažnis |
|---|---|---|
| Temperatūra / Drėgmė / Atm. slėgis / Oro kokybė / Orų prognozė | BME688 (`bme_*`, `weather_forecast`) | 30 s |
| Vandens kokybė (TDS) | ADC jutiklis `tds_ppm` | 10 s |
| 🔥 Dujos likutis | HX711 `gas_remaining_pct` | 5 s |
| 🛏️ Miegamasis oro kokybė / CO₂ | SCD4x | 30 s (CO₂ tendencija 120 s) |
| 🎣 Žvejybos paruoštukas | **App skaičiuoja** iš slėgio/prognozės | event |

### 📡 GSM / Ryšys
| Kortelė | Šaltinis | Dažnis |
|---|---|---|
| Operatorius / Tinklo registracija / Roaming | A7670E `gsm_operator/network/roaming` | 10 s |
| GSM Signalas | `gsm_signal_pct`/`dbm` | 10 s |
| Google buferis | ESP `google_buffer_status` | 10 s |
| 💬 Gautos SMS | text sensoriai `sms_1..5` | event |

### ⚙️ Pokrypis / Sistema / Diagnostika
| Kortelė | Šaltinis | Dažnis |
|---|---|---|
| Šoninis / Išilginis pokrypis | ADXL345 `roll_sensor`/`pitch_sensor` | `never` — perskaičiuojama interval lambda (≈realiu laiku) |
| Vibracija / Judėjimas | `vib_level` (2 s), `alert_movement` (5 s) | 2–5 s |
| ESP32 Būklė: RAM / ESP temp / Uptime | template sensoriai | 60 s |
| Modemo būsena / Paskutinė klaida | template | 5–10 s |
| 📋 Vietinis kaupiklis (CSV) | **App generuoja eilutę** lokaliai | kas 10 s (`index.html:677`) |
| 🖥️ Terminalas | ESP `log` SSE įvykiai | event |

---

## Santrauka pagal dažnį

- **5 s** — vanduo, dujos, GPS pozicija, Junctek akum, 220V, judėjimas
- **10 s** — palydovai, GSM, TDS, Google buferis, vietinis CSV
- **30 s** — BME688 aplinka, miegamasis, orų prognozė, „Ką daryti"
- **60 s** — sistema (RAM, uptime), offline Google Sheets traukimas
- **120 s** — CO₂ tendencija
- **realiu laiku / event** — pokrypis, SMS, jungiklių būsenos, log; app skaičiuojami (maršrutas, energija Wh, žvejyba)
