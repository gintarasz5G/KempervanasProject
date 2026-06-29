# Pilnas veikimo auditas — SSE reikšmių susiejimas (app ↔ ESP)

**Data:** 2026-06-29
**Apimtis:** ar kiekviena app rodoma reikšmė tikrai paimama iš teisingo ESP sensoriaus.
**Metodas:** statinis kryžminis auditas + tikslus matching simuliatorius (Node), kuris atkartoja
app SSE apdorojimo logiką ir paleidžia ją su kiekvieno firmware entiteto realiu ESPHome `object_id` slug'u.

---

## 0. Kaip veikia susiejimas (patvirtinta)

1. ESP siunčia SSE `state` įvykius į app (`EventSource(/events)`, web_server **version 3**).
2. SSE `id` laukas = `<domenas>-<object_id>`, kur **object_id kyla iš `name:`** (ne iš C++ `id:`).
   ESPHome taisyklė (`to_snake_case` + `to_sanitized_char`, patvirtinta iš oficialaus šaltinio):
   - mažosios raidės, tarpas → `_`
   - kiekvienas simbolis ne iš `[a-z0-9_-]` → `_` (NE pašalinamas; `-` paliekamas)
   - **Pasekmė:** `" | "` skyriklis → `"___"` (trigubas pabraukimas). Pvz. `"GPS | Greitis"` → `gps___greitis`.
3. App apdoroja per:
   - **SENSOR_ID_MAP** (greitas kelias; raktai daugiausia C++ id — todėl daugumai NEpataiko ir krenta į fallback)
   - **fallback grandinė** `rawId.includes(...)` — tikrasis darbo arklys, suderintas su pavadinimų slug'ais.
4. ⚠️ **Struktūrinė detalė:** eil. 2209 prasideda `if` (ne `else if`) → kodas turi **dvi nepriklausomas grandines**:
   - Block A (eil. 2197–2207): SENSOR_ID_MAP + junc/akum/bedroom_temp
   - Block B (eil. 2209–2268): bme_temp … sys_uptime
   Abi grandinės vykdomos **kiekvienam** sensoriui → galimi dvigubi priskyrimai (žr. §2.1).

---

## 1. Rezultatų lentelė (kiekviena reikšmė)

Legenda: ✅ pataiko teisingai · 🐞 klaida · ⚪ nenaudojama UI (nekenkia)

| Firmware pavadinimas | slug | app cache raktas | UI naudoja? | Būsena |
|---|---|---|---|---|
| Energija \| Akum SOC % | energija___akum_soc__ | junc_soc | taip | ✅ |
| Energija \| Akum itampa | energija___akum_itampa | junc_v | taip | ✅ |
| Energija \| Akum srove | energija___akum_srove | junc_a | taip | ✅ |
| Energija \| Akum galia | energija___akum_galia | junc_power | taip | ✅ |
| Energija \| Akum Ah likutis | energija___akum_ah_likutis | junc_ah_remaining | taip | ✅ |
| Energija \| Akum temperatura | energija___akum_temperatura | junc_temp **+ bme_temp** | taip | 🐞 §2.1 |
| Energija \| Akum veikimo laikas | energija___akum_veikimo_laikas | junc_time_remaining | taip | ✅ |
| Energija \| Akum Total Ah | energija___akum_total_ah | — (UNMATCHED) | ne | ⚪ |
| Aplinka \| Temperatura | aplinka___temperatura | bme_temp | taip | ✅ (bet žr. §2.1) |
| Aplinka \| Slegis | aplinka___slegis | bme_pressure | taip | ✅ |
| Aplinka \| Dregme | aplinka___dregme | bme_humidity | taip | ✅ |
| Aplinka \| Slegio tendencija | aplinka___slegio_tendencija | pressure_trend_hpa | taip | ✅ |
| Aplinka \| Aukstis (slegis) | aplinka___aukstis__slegis_ | pressure_altitude | taip | ✅ |
| Aplinka \| Oru prognoze | aplinka___oru_prognoze | weather_forecast | taip | ✅ |
| Resursai \| Vanduo lygis % | resursai___vanduo_lygis__ | water_pct | taip | ✅ |
| Resursai \| Vandens atstumas (lazeris) | …vandens_atstumas… | water_distance | ne | ⚪ |
| Resursai \| Vandens TDS (ppm) | …vandens_tds… | tds_ppm | taip | ✅ |
| Resursai \| Dujos likutis % | resursai___dujos_likutis__ | gas_pct | taip | ✅ |
| Pokrypis \| Soninis K-D | pokrypis___soninis_k-d | roll_sensor | taip | ✅ |
| Pokrypis \| Isilginis P-G | pokrypis___isilginis_p-g | pitch_sensor | taip | ✅ |
| Pokrypis \| Vibracija (EMA) | pokrypis___vibracija__ema_ | vib_level | taip | ✅ |
| GPS \| Koordinates | gps___koordinates | gps_coords_sensor → gps_lat/lon | taip | ✅ |
| GPS \| Greitis | gps___greitis | gps_speed_sensor | taip | ✅ |
| GPS \| Aukstis | gps___aukstis | gps_alt_sensor | taip | ✅ |
| GPS \| Kryptis | gps___kryptis | gps_heading_sensor | taip | ✅ |
| GPS \| Visi palydovai | gps___visi_palydovai | gps_sats_total_sensor | taip | ✅ |
| GPS \| Palydovai NAVSTAR/GLONASS/BeiDou/Galileo | … | sats_gps/glonass/bds/galileo | taip | ✅ |
| GPS \| Fix statusas | gps___fix_statusas | — (UNMATCHED) | ne* | ⚪ §2.3 |
| GSM \| Operatorius | gsm___operatorius | gsm_operator_sensor | taip | ✅ |
| GSM \| Tinklas | gsm___tinklas | gsm_network_sensor | taip | ✅ |
| GSM \| Roaming statusas | gsm___roaming_statusas | gsm_roaming_sensor | taip | ✅ |
| GSM \| Signalas (%) | gsm___signalas____ | gsm_signal_pct | taip | ✅ |
| GSM \| Signalas (dBm) | gsm___signalas__dbm_ | — (UNMATCHED) | ne | ⚪ §2.4 |
| Miegamasis \| CO2 | miegamasis___co2 | bedroom_co2 | taip | ✅ |
| Miegamasis \| Temperatura | miegamasis___temperatura | bedroom_temp | taip | ✅ |
| Miegamasis \| Dregme | miegamasis___dregme | bedroom_humidity | taip | ✅ |
| Miegamasis \| CO2 tendencija | miegamasis___co2_tendencija | bedroom_co2_trend | taip | ✅ |
| Miegamasis \| Oro kokybe | miegamasis___oro_kokybe | bedroom_air_quality | taip | ✅ |
| Sauga \| Judejimo aliarmas | sauga___judejimo_aliarmas | alert_movement_sent | taip | ✅ |
| Sistema \| Patarimai | sistema___patarimai | system_advice | taip | ✅ |
| Sistema \| Google buferis | sistema___google_buferis | google_buffer_status | taip | ✅ |
| Sistema \| Laisva RAM | sistema___laisva_ram | sys_free_ram | taip | ✅ |
| Sistema \| ESP temperatūra | sistema___esp_temperat_ra | sys_esp_temp (×2, ta pati reikšmė) | taip | ✅ |
| Sistema \| Uptime | sistema___uptime | sys_uptime | taip | ✅ |
| Energija \| 220V saltinis | energija___220v_saltinis | power_source_220 | taip | ✅ |
| Gauta SMS 1–5 | gauta_sms_N… | sms_1…5 | taip | ✅ |
| Sistema \| Modemo busena | sistema___modemo_busena | — (UNMATCHED) | ne | ⚪ |
| Sistema \| Paskutine klaida | sistema___paskutine_klaida | — (UNMATCHED) | ne | ⚪ |
| Energija \| Junctek Raw HEX | energija___junctek_raw_hex | — (UNMATCHED) | ne (vidinis) | ⚪ |
| Energija \| Isorine 220V (binary_sensor) | … | IGNORUOJAMA | ne | ⚪ §2.5 |
| Energija \| Keitiklio 220V (binary_sensor) | … | IGNORUOJAMA | ne | ⚪ §2.5 |
| Sistema \| Rysio aliarmas (binary_sensor) | … | IGNORUOJAMA | ne | ⚪ §2.5 |

**Kalibravimo `number` reikšmės** (num_water_empty_cm, num_gas_*, num_*_offset, num_bat_capacity ir kt.)
ateina ne per sensorių grandinę, o per `number` domeną / `loadAllNumbers()` REST → saugomos kaip `num_*`
ir rodomos nustatymų laukuose. ✅ Tvarkoma teisingai.

---

## 2. Rastos problemos

### 2.1 🐞 KRITINĖ: akumuliatoriaus temperatūra užteršia aplinkos temperatūrą (`bme_temp`)
- **Kur:** `android/www/index.html:2209`
- **Priežastis:** Block B sąlyga `(...||rawId.includes('temperat')) && !miegamasis && !esp_ && !sistema_`
  pagauna ir `"Energija | Akum temperatura"` slug'ą (`energija___akum_temperatura` — turi `temperat`).
  Kadangi eil. 2209 yra atskiras `if` (ne `else if`), jis suveikia NET kai sensorius jau buvo
  priskirtas `junc_temp` Block A grandinėje. → tas pats akum. temp. įrašomas ir į `bme_temp`.
- **Pasekmė:** kai Junctek BLE prijungtas (firmware publikuoja `junc_temp`, eil. 1303, ruožas −40..85°C,
  visada < 90 → filtras nepadeda), **„Temperatūra" kortelė ir apžvalga rodo akumuliatoriaus temperatūrą**
  (atnaujinama kas 5 s) vietoj BME688 aplinkos (kas 30 s). 5/6 laiko rodoma klaidinga reikšmė.
- **Pataisa (rekomenduojama):** eil. 2209 `if` → `else if` (sujungti į vieną grandinę — kiekvienas
  sensorius priskiriamas tik kartą). Alternatyva (minimali): pridėti `&& !rawId.includes('akum')`.

### 2.2 ℹ️ `45.0 → 90.0` pakeitimas (neseniai)
- `android/www/index.html:2209` sanity riba pakeista iš `45.0` į `90.0`. Tai **neišsprendžia** §2.1
  (akum temp ~25°C praeina abi ribas) ir susilpnina apsaugą nuo absurdiškų reikšmių.

### 2.3 ⚪ GPS Fix statusas nerodomas
- `gps___fix_statusas` neturi fallback taisyklės → UNMATCHED. UI „📍 Koordinatės" paantraštėje
  vietoj fix būsenos rodo palydovų skaičių (`gps_sats_total_sensor`, eil. 1542). Sensorius tiesiog nenaudojamas.

### 2.4 ⚪ GSM Signalas (dBm) nerodomas
- `gsm___signalas__dbm_` neturi taisyklės. UI naudoja tik `%` (`gsm_signal_pct`), kuris veikia. Nekenkia.

### 2.5 ⚪ binary_sensor entitetai ignoruojami
- App SSE blokas tvarko tik `sensor` / `text_sensor` domenus. Trys `binary_sensor`
  („Isorine 220V", „Keitiklio 220V", „Rysio aliarmas") visiškai ignoruojami. Nekenkia, nes 220V
  būsena rodoma per text_sensor `Energija | 220V saltinis` (power_source_220).

### 2.6 ⚪ Nenaudojami sensoriai (UNMATCHED, UI neskaito)
- „Akum Total Ah", „Modemo busena", „Paskutine klaida", „Junctek Raw HEX". Jei jų reikės UI —
  trūks fallback taisyklių.

### 2.7 ⚪ Negyvas mapping
- SENSOR_ID_MAP/fallback įrašai `water_offset, water_empty_cm, water_full_cm, gas_empty_weight,
  gas_full_weight, hx711_scale` niekada nesuveikia (tai `number` entitetai, ne `sensor`).
  Kalibravimas iš tikrųjų ateina per `num_*` (REST). Tik negyvas kodas — nekenkia.

---

## 3. Bendra išvada

- **Veikia teisingai:** visi pagrindiniai rodikliai — akumuliatorius (SOC/V/A/W/Ah/laikas),
  vanduo, dujos, BME688 (slėgis/drėgmė/tendencija/aukštis/prognozė), pokrypis, vibracija,
  visas GPS (koordinatės/greitis/aukštis/kryptis/palydovai), GSM (operatorius/tinklas/roaming/signalas%),
  miegamasis (CO2/temp/drėgmė/tendencija/kokybė), sauga, sistema (RAM/ESP temp/uptime/patarimai/buferis),
  220V šaltinis, SMS, kalibravimo reikšmės.
- **1 reali klaida:** §2.1 — akumuliatoriaus temperatūra užteršia aplinkos temperatūros kortelę.
- Kita — kosmetika arba nenaudojami sensoriai.

> Patikra atlikta statiškai (įrenginys 192.168.4.1 buvo nepasiekiamas — PC ne Kemperis AP tinkle).
> Slug modelis patvirtintas iš ESPHome `to_sanitized_char`/`to_snake_case` šaltinio kodo.
> Simuliatorius: `scratchpad/sim.js`.
