# 📡 Sensorių Žemėlapis — Pilna Lentelė

Visi `sensorCache` raktai, jų šaltiniai ir UI elementai.

**Šaltiniai:** SSE = realiu laiku per EventSource | Sheets = per Google Apps Script (kas 60s offline)

---

## Energija

| sensorCache raktas | SSE entity | Sheets laukas | UI element ID | Vienetas | Pastabos |
|---|---|---|---|---|---|
| `junc_soc` | `junc_soc` | `soc` | `soc_value`, `soc_d` | % | SOC = State of Charge |
| `junc_v` | `junc_v` | `v` | `voltage` | V | Baterijos įtampa |
| `junc_a` | `junc_a` | `a` | `current` | A | Srovė (- = iškrova) |
| `junc_power` | `junc_power` | `power` | `power` | W | Momentinė galia |
| `junc_ah_remaining` | `junc_ah_remaining` | — | `ah_remaining` | Ah | Likutis |
| `junc_time_remaining` | `junc_time_remaining` | — | `time_remaining` | min | Laikas iki pilno/tuščio |
| `junc_temp` | `junc_temp` | — | `junc_temp` | °C | Jungties temperatūra |
| `bat_capacity_ah` | `num_bat_capacity` | — | — | Ah | Konfigūruojama |
| `power_source_220` | `power_source_220` | — | `overview-220v` | tekstas | Shore/Inverter/Off |

---

## Vanduo

| sensorCache raktas | SSE entity | Sheets laukas | UI element ID | Vienetas | Pastabos |
|---|---|---|---|---|---|
| `water_pct` | `water_level_pct` | `water` | `water_pct`, `water_pct_d` | % | VL53L0X lazeris |
| `tds_ppm` | `tds_ppm` | `tds` | `tds` | ppm | Vandens kokybė |
| `water_distance` | `water_distance_sensor` | — | — | cm | Žalias lazerio matavimas |
| `water_empty_cm` | `num_water_empty_cm` | — | `num_water_empty_cm` | cm | Kalibracija |
| `water_full_cm` | `num_water_full_cm` | — | `num_water_full_cm` | cm | Kalibracija |
| `water_offset` | `num_water_offset` | — | — | cm | Korekcija |

---

## Dujos

| sensorCache raktas | SSE entity | Sheets laukas | UI element ID | Vienetas | Pastabos |
|---|---|---|---|---|---|
| `gas_pct` | `gas_pct` | `gas` | `gas_pct`, `gas_pct_d` | % | Apskaičiuota |
| `gas_empty_weight` | `num_gas_empty_kg` | — | — | kg | |
| `gas_full_weight` | `num_gas_full_kg` | — | — | kg | |
| `gas_weight_offset` | `num_gas_weight_offset` | — | — | kg | Tare korekcija |
| `hx711_scale` | `num_hx711_scale` | — | — | — | Sverstuvų kalibr. |

---

## Aplinka (lauke — BME280)

| sensorCache raktas | SSE entity | Sheets laukas | UI element ID | Vienetas |
|---|---|---|---|---|
| `bme_temp` | `bme_temp` | `temp` | `temp`, `temp_d` | °C |
| `bme_humidity` | `bme_humidity` | `humidity` | `humid`, `humid_d` | % |
| `bme_pressure` | `bme_pressure` | `pressure` | `pressure` | hPa |
| `pressure_trend_hpa` | — | — | `pressure-trend` | hPa/h | Apskaičiuota |
| `pressure_altitude` | — | — | — | m | Apskaičiuota |
| `bedroom_air_quality` | `bedroom_air_quality` | — | `air-quality-d` | tekstas | |
| `temp_offset` | `num_temp_offset` | — | — | °C | Kalibracija |
| `humid_offset` | `num_humid_offset` | — | — | % | Kalibracija |

---

## Miegamasis (SCD41)

| sensorCache raktas | SSE entity | Sheets laukas | UI element ID | Vienetas |
|---|---|---|---|---|
| `bedroom_co2` | `bedroom_co2` | `co2` | `bedroom-co2` | ppm |
| `bedroom_temp` | `bedroom_temp` | — | `bedroom-temp` | °C |
| `bedroom_humidity` | `bedroom_humidity` | — | `bedroom-humid` | % |
| `bedroom_co2_trend` | — | — | `co2-trend` | ppm/h | Apskaičiuota |

---

## GPS

| sensorCache raktas | SSE entity | Sheets laukas | UI element ID | Vienetas | Pastabos |
|---|---|---|---|---|---|
| `gps_coords_sensor` | `gps_coords_sensor` | `lat`+`lon` | `gps-coords` | "lat,lon" | Sujungta eilutė |
| `gps_lat` | — | — | — | ° | Išskiriama iš coords |
| `gps_lon` | — | — | — | ° | Išskiriama iš coords |
| `gps_speed_sensor` | `gps_speed_sensor` | — | `gps-speed` | km/h | |
| `gps_alt_sensor` | `gps_alt_sensor` | — | `gps-alt` | m | |
| `gps_heading_sensor` | `gps_heading_sensor` | — | `gps-heading` | ° | Rodyklė žemėlapyje |
| `gps_sats_total_sensor` | `gps_sats_total_sensor` | — | `gps-sats` | vnt | |
| `sats_gps` | `sats_gps` | — | `sats-gps` | vnt | GPS palydovai |
| `sats_glonass` | `sats_glonass` | — | `sats-glonass` | vnt | |
| `sats_bds` | `sats_bds` | — | `sats-bds` | vnt | BeiDou |
| `sats_galileo` | `sats_galileo` | — | `sats-galileo` | vnt | |

---

## Lygiavimas (ADXL345)

| sensorCache raktas | SSE entity | UI element ID | Vienetas | Pastabos |
|---|---|---|---|---|
| `roll_sensor` | `roll_sensor` | `roll-value` | ° | Šoninis polinkis |
| `pitch_sensor` | `pitch_sensor` | `pitch-value` | ° | Pirmyn/atgal |
| `cal_roll` | `num_roll_cal` | — | ° | Zero offset |
| `cal_pitch` | `num_pitch_cal` | — | ° | Zero offset |
| `wheelbase_cm` | `num_wheelbase_cm` | — | cm | Ašių atstumas |
| `track_cm` | `num_track_cm` | — | cm | Pločio atstumas |
| `vib_level` | `vib_level` | `vib-level` | g | Vibracijos lygis |

---

## Ryšys (GSM/SMS)

| sensorCache raktas | SSE entity | UI element ID | Vienetas |
|---|---|---|---|
| `gsm_operator_sensor` | `gsm_operator_sensor` | `gsm-operator` | tekstas |
| `gsm_network_sensor` | `gsm_network_sensor` | `gsm-network` | `4G`/`3G`/`2G` |
| `gsm_signal_pct` | `gsm_signal_pct` | `gsm-signal` | % |
| `gsm_roaming_sensor` | `gsm_roaming_sensor` | `gsm-roaming` | `true`/`false` |
| `txt_apn` | `txt_apn` | — | tekstas | GSM APN |
| `txt_admin_number` | `txt_admin_number` | — | tekstas | SMS admin nr. |
| `sms_1`…`sms_5` | `sms_1`…`sms_5` | — | tekstas | SMS istorija |

---

## Sistema (ESP32)

| sensorCache raktas | SSE entity | UI element ID | Vienetas | Pastabos |
|---|---|---|---|---|
| `sys_free_ram` | `sys_free_ram` | — | bytes | Laisva RAM |
| `sys_esp_temp` | `sys_esp_temp` | — | °C | ESP32 chip temp |
| `sys_uptime` | `sys_uptime` | — | s | Laikas nuo startupo |
| `sw_security_armed` | `sw_security_armed` | `chk_security_armed` | `on`/`off` | |
| `sw_google_upload` | `sw_google_upload` | `chk_google_upload` | `on`/`off` | |
| `alert_movement_sent` | `alert_movement_sent` | — | `true`/`false` | Vagystės trigeris |
| `google_buffer_status` | `google_buffer_status` | — | tekstas | |
| `timezone_offset` | `num_timezone_offset` | — | h | UTC poslinkis |

---

## Nustatymai (tik konfigūraciniai)

| sensorCache raktas | Šaltinis | Aprašas |
|---|---|---|
| `weather_forecast` | Sheets | Orų prognozė (JSON) |
| `system_advice` | Sheets | Sisteminis patarimas |
