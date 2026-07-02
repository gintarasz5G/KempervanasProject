# Pilnas programėlės auditas — 2026-07-01 (v35 / v32.3)

**Apimtis:** visas app (frontend + native), po v32.2 (SSE regex fix) ir v32.3 (cloud aliarmai, onopen dedup).
**Metodai:** kodo analizė, **`node` SSE grandinės simuliacija su realiais object_id**, ESPHome/Android dok. online, scenarijų imitacija.
**Bendra išvada:** programėlė **žymiai sveikesnė** nei praeitame audite (v33). Kritinė SSE problema **išspręsta ir simuliacija patvirtinta**. Liko keli smulkūs, nekritiniai gap'ai.

---

## Santrauka pagal svarbą

| # | Radinys | Svarba | Statusas |
|---|---------|--------|----------|
| 1 | **SSE mapinimas VEIKIA** — 39/42 realių jutiklių teisingai (simuliacija) | 🟢 Gerai | Patvirtinta node'e; laukia gyvos ESP verifikacijos |
| 2 | 3 SSE-nesutampantys jutikliai (visi nekritiniai/nerodomi) | 🟡 Žema | Neprivaloma taisyti |
| 3 | `binary_sensor` domenas SSE išvis neapdorojamas | 🟡 Žema | 220V veikia per text_sensor; ryšio aliarmas gali dingti |
| 4 | Cloud aliarmų laiko zonos rizika (TZ) | 🟡 Vidutinė | Verta patikrinti |
| 5 | Laikina `[SSE BRANCH]` diagnostika shipuota | 🟢 Info | Pašalinti po verifikacijos |
| 6 | onopen dedup, versijų sync, apk_url — sutvarkyta | 🟢 Gerai | v32.3 |

---

## 1. SSE mapinimas — SIMULIACIJA PATVIRTINO (svarbiausia)

`node` simuliacija paleido **visą aktyvuotą `includes()` grandinę** (su v32.2 regex+tvarka) prieš **42 realius firmware object_id**. Rezultatas: **39 teisingai** mapinami. Pvz.:
- `energija_akum_soc_`→junc_soc, `akum_itampa`→junc_v, `akum_srove`→junc_a, `akum_galia`→junc_power, `akum_ah_likutis`→junc_ah_remaining, `akum_temperatura`→junc_temp, `akum_veikimo_laikas`→junc_time_remaining.
- `aplinka_temperatura`→bme_temp, `aplinka_slegis`→bme_pressure, `aplinka_dregme`→bme_humidity, `aplinka_slegio_tendencija`→pressure_trend_hpa, `aplinka_aukstis_slegis_`→pressure_altitude.
- `resursai_*`→tds_ppm/gas_pct/water_distance/water_pct; `pokrypis_*`→vib_level/roll_sensor/pitch_sensor.
- `gps_*`→gps_alt/speed/heading/sats; `gsm_signalas_`→gsm_signal_pct.
- `miegamasis_*`→bedroom_co2/temp/humidity/co2_trend; `sistema_laisva_ram`→sys_free_ram, `sistema_esp_temperat_ra`→sys_esp_temp.
- text: oro_kokybe→bedroom_air_quality, koordinates→gps_coords, gsm_operatorius/tinklas, uptime→sys_uptime, patarimai→system_advice, 220v_saltinis→power_source_220, judejimo_aliarmas→alert_movement_sent, oru_prognoze→weather_forecast.

**Išvada:** v32.2 pataisymas tikrai atrakino SSE duomenis (grandinė nebemiega). Reikia tik **gyvo ESP patvirtinimo** (`[SSE BRANCH]` logai + kortelės).

## 2. Trys SSE-nesutampantys (visi nekritiniai)
- `sensor_energija_akum_total_ah` → UNMATCHED. Junctek „Total Ah" — ne pagrindinis rodmuo.
- `sensor_gsm_signalas_dbm_` → UNMATCHED (nes chain'e `gsm_signal` su `!dbm`). GSM signalas **%** veikia; dBm variantas nerodomas svarbiose vietose.
- `text_sensor_gps_fix_statusas` → UNMATCHED. **Bet** app „GPS fix" vietoje (`#gps-fix`) rodo **palydovų skaičių** (`gps_sats_total_sensor`), ne fix tekstą — tad realiai **nerodomas ir nekenkia**.

Jei norima 100% padengimo — pridėti į grandinę (prieš atitinkamas eilutes):
```javascript
else if (rawId.includes('gsm_signal') && rawId.includes('dbm')) sensorCache['gsm_signal_dbm'] = value;  // PRIEŠ pct eilutę
else if (rawId.includes('akum_total_ah') || rawId.includes('total_ah')) sensorCache['junc_total_ah'] = value;
```
> Prieš pridedant — patikrinti, ar `getCache('gsm_signal_dbm')`/`junc_total_ah` realiai rodomi; jei ne — nereikia.

## 3. `binary_sensor` domenas neapdorojamas
Domenų regex'ai: number/switch/(text_sensor|sensor)/text. `binary_sensor-*` (→ `binary_sensor_*`) **nesutampa nė su vienu** → niekada nesaugomas. Firmware turi `energija_isorine_220v`, `energija_keitiklio_220v`, `sistema_rysio_aliarmas`.
- 220V realiai veikia per **text_sensor** `energija_220v_saltinis`, tad kortelė pildosi.
- `sistema_rysio_aliarmas` (ryšio aliarmas) — jei rodomas, liktų tuščias.

Jei reikia — pridėti domeno šaką (po sensor, prieš text):
```javascript
else if (rawId.match(/^binary_sensor[\/\-_]/)) {
    if (rawId.includes('isorine_220v')) sensorCache['shore_power_present'] = value;
    else if (rawId.includes('keitiklio_220v')) sensorCache['inverter_220_active'] = value;
    else if (rawId.includes('rysio_aliarmas')) sensorCache['link_alarm'] = value;
}
```
(Neprivaloma; patikrinti display raktus.)

## 4. Cloud aliarmai — laiko zonos rizika
`_cloudDataAgeMin` parsina `json.timestamp` („YYYY-MM-DD HH:MM:SS") kaip **įrenginio lokalų laiką**. Apps Script formatuoja **lapo TZ**. Jei lapo TZ = įrenginio TZ (abu Lietuva) — gerai. Jei skiriasi — amžius klaidingas → arba visada „pasenę" (aliarmai niekada nesuveiks), arba neigiamas (irgi blokuojama). **Rekomendacija:** patikrinti, kad Google lapo TZ = Europe/Vilnius; arba ateity įtraukti UTC laiką/poslinkį. Šiaip saugiklis „fail-safe" (jei abejotina — nealarmuoja).

Kita cloud logika teisinga: whitelist be vagystės, 20 min riba, „(debesys)" žymė, bendri `alarmFlags` (dedup nekonfliktuoja, nes gyvas ir cloud keliai nesutampa laike).

## 5–6. Kita
- **`[SSE BRANCH]` diagnostika** (index.html ~2507) shipuota v35 — po gyvos verifikacijos **pašalinti** ir perbuild'inti.
- **onopen** — sujungtas, `_sseRetryCount=0` grąžintas ✓.
- **Versijos** — CURRENT_VERSION=versionCode=version.json=35, apk_url→v35.apk, failas yra ✓.
- **Dual-network (v32), TTS, MediaStore, SMS slėpimas, UI mastelis** — teisinga (ankstesni auditai).

---

## Scenarijų imitacija (v35)

1. **ESP „Tik Kemperis" (grynas SSE, be interneto):** dabar (po v32.2) SSE grandinė aktyvi → visi 39 jutikliai turi užsipildyti. Anksčiau (v33) būtų tušti. **Tai pagrindinis pagerėjimas** — planšetė be interneto turėtų rodyti viską.
2. **Auto + LTE (telefonas):** SSE per ESP WiFi (pririšta), internetas per LTE (getSafeConnection). Stabilu (v32).
3. **Offline + cloud aliarmai ON:** kas 60 s cloud fetch → jei duomenys ≤20 min ir virš ribos → balsas „(debesys, prieš X min)". Vagystė netaikoma. (TZ prielaida — žr. #4.)
4. **Kalibravimas:** REST POST → ESP kalibruoja → naujas roll=0 dabar **atkeliauja per veikiančią SSE grandinę** → ekranas atsinaujina (jei ryšys stabilus).
5. **SSE watchdog + reconnect:** `_sseRetryCount=0` po onopen (v32.3) → patikimesnis atsistatymas.

---

## Rekomendacijos (prioritetas)
1. **🟢 Gyvai patvirtinti SSE** su ESP (idealu „Tik Kemperis"): `[SSE BRANCH]` logai + ar užsipildo anksčiau tuščios kortelės. Simuliacija sako, kad taip. Po to — **pašalinti `[SSE BRANCH]` diagnostiką**.
2. **🟡 (Neprivaloma)** pridėti 3 gap'us + binary_sensor domeną, jei tie rodmenys reikalingi (patikrinti display raktus).
3. **🟡** Patikrinti Google lapo TZ (cloud aliarmų šviežumui).
4. Likę atidėti: SSE object_id migracija (deterministinis map — dabar mažiau skubu, nes grandinė veikia), Apps Script auth/valymas.

## Ką patikrinau
- SSE `'state'` handler + **pilna node simuliacija** (42 object_id per grandinę + domenų maršrutas + binary_sensor).
- Cloud aliarmai (TZ parse, whitelist, flags), _fetchSheetsNow.
- onopen, versijos, apk_url, GPS/SOC display raktai.
- Ankstesni: dual-network, TTS, MediaStore, native.
- Git: v35 sinchronizuota, push'inta.
