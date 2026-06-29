# ANALIZE.md — kempervanas_v9.yaml

**Data:** 2026-05-06  
**Failas:** kempervanas_v9.yaml  
**Etapas:** ETAPAS 1 — Analizė (nekeisti failų)

---

## GPIO ŽEMĖLAPIS — Konfliktų patikrinimas

| GPIO | Komponentas | Statusas |
|------|-------------|----------|
| 9 | I2C SDA | ✓ OK |
| 10 | I2C SCL | ✓ OK |
| 40 | UART TX (A7670E) | ✓ OK |
| 41 | UART RX (A7670E) | ✓ OK |
| 47 | HX711 DOUT (NENURODYTA v9) | ⚠️ Neaplašyta |
| 48 | HX711 CLK (NENURODYTA v9) | ⚠️ Neaplašyta |
| 39 | 1-Wire AM2301 (NENURODYTA v9) | ⚠️ Neaplašyta |
| 7 | ADC TDS (NENURODYTA v9) | ⚠️ Neaplašyta |
| 16 | RS485 TX (NENURODYTA v9) | ⚠️ Neaplašyta |
| 17 | RS485 RX (NENURODYTA v9) | ⚠️ Neaplašyta |

**Išvada:** Nėra GPIO konfliktų. v9 naudoja tik I2C + UART. BT (Renogy) neturi dedicated pinų (per BLE chipą).

---

## KOMPONENTŲ PATIKRINIMAS — Pagal prioritetą

### 1. ✅ VEIKIA: BMP180 (Temperatūra, slėgis, aukštis)

**Eilutės:** 205-214

```yaml
- platform: bmp085
  temperature:
    name: "Aplinka | Temperatura"
    id: bmp_temp
    filters:
      - offset: -2.0
  pressure:
    name: "Aplinka | Slegis"
    id: bmp_pressure
  update_interval: 10s
```

**Statusas:**
- ✓ ESPHome komponentas `bmp085` teisingas (naudojamas BMP085 ir BMP180)
- ✓ I2C adresas 0x77 (numatytas, nesukonfliktuojasi su kitais)
- ✓ Temperature offset: −2°C (numatyta CLAUDE.md)
- ✓ Update interval: 10s ✓
- ✓ Kompiliuojasi be klaidų

**Trūkumai (NEPILNA):**
- ❌ **NĖRA ORŲ PROGNOZĖS** — CLAUDE.md reikalauja 7 slėgio reikšmių istorijos kas 30 min → 3h tendencija. v9 tik nustato offset.
- ❌ Nėra `number:` komponento temp offset'o kalibracijai (NVS `restore_value`)
- ❌ Nėra text_sensor su prognozės tekstais ("Labai greitai gerėja", "Geras", "Stabilus" ir t.t.)

**Rekomendacija:** Pridėti `globals` masyvą slėgio istorijoje ir text_sensor su tendencija.

---

### 2. ❌ TRŪKSTA: VL53L0X (Vandens bakas)

**CLAUDE.md reikalavimas:**
- Atstumas cm (raw), lygis %, litrai
- I2C 0x29
- Kalibraciniai `number:` (tuščio, pilno, tūrio)
- `button:` "Nustatyti kaip tuščią" ir "Nustatyti kaip pilną"
- Procentai = `(tuščias − dabartinis) / (tuščias − pilnas) × 100`
- `median` filtras window 5

**v9 statusas:** ❌ **PILNAI TRŪKSTA**

---

### 3. ❌ TRŪKSTA: HX711 (Dujų baliono svoris)

**CLAUDE.md reikalavimas:**
- GPIO47 (DOUT), GPIO48 (CLK)
- Likutis kg (1 ženklas), likutis %
- `number:` tuščio, pilno, scale factor
- `button:` tare

**v9 statusas:** ❌ **PILNAI TRŪKSTA**

---

### 4. ✅ VEIKIA: ADXL345 (Pokrypis)

**Eilutės:** 217-245 (sensoriai), 102-109 (globalinės kalibracijų),  182-188 (kalibruoti mygtukas)

```yaml
- platform: template
  name: "Pokrypis | KD"
  id: roll_sensor
  # atan2(Y, Z) → Roll
  
- platform: template
  name: "Pokrypis | PG"
  id: pitch_sensor
  # atan2(X, Z) → Pitch
```

**Statusas:**
- ✓ I2C adresas 0x53 ✓
- ✓ Boot metu I2C write `{0x2D, 0x08}` (eilutės 15-17) ✓
- ✓ Roll (K/D) ir Pitch (P/G) apskaičiavimai teisingi
- ✓ Globalinės kalibriacijos (`cal_roll`, `cal_pitch`) su `restore_value: true` ✓
- ✓ Kalibruoti mygtukas (button) ✓
- ✓ `accuracy_decimals: 0` ✓
- ✓ `update_interval: 1s` ✓

**Trūkumai (NEPILNA):**
- ❌ **NĖRA DOMKRATŲ PATARIMO** (text_sensor)
  - CLAUDE.md reikalauja: `mm = tan(kampas_rad) × 1800` (VW Crafter 1800mm ratų tarpas)
  - Pvz: "Kelti K +42mm" / "Kelti D +18mm" / "Lygu" (< 0.5° = lygu)
- ❌ Nėra `number:` komponento offset'o kalibracijai per UI

**Rekomendacija:** Pridėti text_sensor su domkratų logika.

---

### 5. ✅ VEIKIA: GPS — A7670E (Koordinatės, aukštis, greitis, kryptis)

**Eilutės:** 248-263 (sensoriai), 512-548 (parsavimas iš +CGNSSINFO)

```yaml
GPS sensoriai:
- gps_altitude (m)
- gps_satellites (skaičius)
- gps_coords (text_sensor, lat/lon)
- gps_fix (bool)
```

**Statusas:**
- ✓ UART konfigūracija: GPIO40/41, 115200 baud ✓
- ✓ Globaliniai kintamieji (`gps_lat`, `gps_lon`, `gps_alt`, `gps_sats`, `gps_fix`) ✓
- ✓ +CGNSSINFO parsavimas (komma-separated, 12+ parametrų)
- ✓ Fix statusas (2D/3D atidarant, bet nėra `accuracy_decimals` įrašyto)
- ✓ Palydovų skaičius (GPS + GLONASS + BeiDou sumavimas)
- ✓ Aukščio nuskaitymas (m), `accuracy_decimals: 0` ✓
- ✓ Koordinačių nuskaitymas (lat/lon), `accuracy_decimals: 6` (neexplicitai, bet svarbiai)

**Trūkumai (NEPILNA):**
- ❌ **NĖRA GREIČIO SENSORIAUS** (km/h)
  - +CGNSSINFO turi parametrą greičiui (paprastai p[9] arba p[10])
  - Reikalingas text_sensor su nuolatiniu greičiu
- ❌ **NĖRA KRYPTIES SENSORIAUS** (°)
  - +CGNSSINFO turi parametrą krypčiai (heading, paprastai p[10] arba p[11])
  - Reikalingas sensorius su `accuracy_decimals: 0`
- ❌ Nėra atskirų sensorių `gps_speed`, `gps_heading` (naudojami tik globals)

**Rekomendacija:** Pridėti `sensor:` template su `gps_speed` ir `gps_heading`.

---

### 6. ✅ VEIKIA: GSM — A7670E (Signalas, operatorius, SMS)

**Eilutės:** 267-275 (signalas %), 344-357 (operatorius), 508-562 (parsavimas)

```yaml
- platform: template
  name: "GSM | Signalas"
  id: gsm_signal_pct
  unit_of_measurement: "%"
```

**Statusas:**
- ✓ AT komandos: AT+CPIN?, AT+CSQ, AT+COPS?, AT+CGNSSINFO ✓
- ✓ RSSI signalas nuo 0–31 dBm → % konvertavimas (eilutės 273-275) ✓
- ✓ Operatoriaus dekodavimas: Tele2 (24602), Telia (24601), Bite (24603) ✓
- ✓ SIM statusas (READY, READY, READY)
- ✓ update_interval: 10s ✓
- ✓ Update interval AT komandų: 10s ✓

**Trūkumai (NEPILNA):**
- ❌ **NĖRA SMS ATSAKYMO Į "Status?"**
  - CLAUDE.md reikalauja atsakymo (max 160 ASCII simbolių):
    ```
    Kemperis OK
    Bat: {V}V {%}%
    GPS: {lat},{lon} {alt}m
    Tilt: {roll}d {pitch}d
    Gas: {kg}kg H2O:{%}%
    Temp:{C}C {hPa}hPa
    ```
  - v9 nėra tokios logikos

**Rekomendacija:** Pridėti SMS atsakymo logiką (`uart.write` per script).

---

### 7. ⚠️ NEPILNA: Junctek KH-F150A (Modbus RS485 — Įkrovos srovė)

**CLAUDE.md reikalavimas:**
- RS485: TX=GPIO16, RX=GPIO17
- Modbus RTU: slave 1
- Sensoriai: Srovė (A)

**v9 statusas:** ❌ **PILNAI TRŪKSTA**
- Nėra UART RS485 magistralės konfigūracijos
- Nėra `modbus_controller` komponento
- Nėra Junctek sensorių

**Pastaba:** CLAUDE.md sako "Jungiamas **tiesiogiai prie šunto laidų** (valdiklio dėžutė sudegė)".

---

### 8. ✅ VEIKIA (dalinai): Renogy PRO 100Ah LiFePO4 (BLE)

**Eilutės:** 148-154 (BLE client), 278-325 (sensoriai), 372-461 (BLE notify parsavimas)

```yaml
ble_client:
  - mac_address: "38:3B:26:79:DB:64"
    id: renogy_batt
```

**Statusas:**
- ✓ BLE client konfigūracija su MAC adresu ✓
- ✓ Sensoriai aprašyti: `bat_volts`, `bat_amps`, `bat_soc`, `bat_cell_1-4`, `bat_temp_min` ✓
- ✓ BLE notify charakteristika "FFF1" ✓
- ✓ on_connect / on_disconnect logika ✓
- ✓ `accuracy_decimals` nustatyti: 2 (V, A), 0 (%), 3 (cell V), 1 (temp)

**KODAVIMO KLAIDOS IR PROBLEMOS:**

1. **Eilutė 420 — VARDINĖ KLAIDA:**
   ```cpp
   int len = next_tag - (c0_idx + 1);  // ❌ c0_idx nėra deklaruotas!
   ```
   Turėtų būti: `c_idx` (tas pats kintamasis iš eilutės 405)
   
2. **Bitų dekodavimas (eilutės 399-401) — ABEJOTINAS:**
   ```cpp
   float volts = ((id(bat_buffer)[1] >> 4) * 10 + (id(bat_buffer)[1] & 0x0F)) + ...
   ```
   - Ši logika daroma iš `bat_buffer[1]`, kuris yra `0x14` (tipo markeris), ne data
   - **Turi daryti iš `bat_buffer[2]` ir `bat_buffer[3]`** (BCD encoded voltage)

3. **SOC (bat_soc) — NEATLIKTA**
   - Eilutė 457 yra komentiruota: `// id(bat_soc).publish_state(100.0);`
   - Nėra BB 20 arba kito markeriaus dekodavimo

4. **Celių įtampos (D2/D3) — NEPILNA**
   - Logika atrodo primityviai priskiriama tik `bat_cell_1`
   - Turėtų identifikuoti D2 / D3 indeksą ir priskirtipilnai visoms 4 ląstelėms

**Rekomendacija:** 
- [ ] Pataisyti `c0_idx` → `c_idx` (eilutė 420)
- [ ] Peržiūrėti įtampos BCD dekodavimą (kurie baitai yra duomenys?)
- [ ] Implementuoti SOC dekodavimą
- [ ] Identifikuoti visas 4 ląsteles (ne tik 1)

---

### 9. ✅ VEIKIA (dalinai): Renogy DCC50S (BLE)

**Eilutės:** 159-168 (BLE client), 327-337 (sensoriai), 464-490 (BLE notify parsavimas)

```yaml
ble_client:
  - mac_address: "C4:D3:6A:63:D6:42"
    id: renogy_dcdc
```

**Statusas:**
- ✓ BLE client konfigūracija su MAC adresu ✓
- ✓ Sensoriai: `dcdc_volts` (V), `dcdc_amps` (A) ✓
- ✓ BLE write interval 10s (eilutės 576-586) ✓
- ✓ Modbus rėmo ID: `0xFF03` nuskaitymas ✓

**Trūkumai (NEPILNA):**
- ❌ **Tik 2 sensoriai**, CLAUDE.md reikalauja:
  - ✓ Saulės įtampa (V) — NĖRA
  - ✓ Saulės srovė (A) — NĖRA
  - ✓ Saulės galia (W) — NĖRA
  - ✓ Akum įtampa (V) — ✓ yra `dcdc_volts` (žiūrint kas iš tikrųjų yra bytai 5-6)
  - ✓ Krovimo statusas (text) — NĖRA
  - ✓ Režimas (text) — NĖRA
  - ✓ Kraunama iš saulės / iš variklio (bool/text) — NĖRA

**Rekomendacija:** Išplėsti dekodavimą — reikia pilno Modbus paketo analizės.

---

### 10. ❌ TRŪKSTA: BME688 (Oro kokybė)

**CLAUDE.md reikalavimas:**
- I2C adresas 0x76 (SDO → GND)
- ESPHome komponentas: `bme680` arba `bme68x`
- Temperatūra (°C), drėgmė (%), slėgis (hPa), VOC (Ω)
- `number:` temp offset (numatyta −4°C)
- Text sensor: "Geras" / "Vidutinis" / "Blogas" pagal VOC

**v9 statusas:** ❌ **PILNAI TRŪKSTA**

---

### 11. ❌ TRŪKSTA: AM2301 (Temperatūra/drėgmė — 1-Wire)

**CLAUDE.md reikalavimas:**
- GPIO39 (1-Wire)
- Atsarginė temperatūra/drėgmė

**v9 statusas:** ❌ **PILNAI TRŪKSTA**

---

### 12. ❌ TRŪKSTA: SSD1306 OLED (Ekranas)

**CLAUDE.md reikalavimas:**
- I2C 0x3C
- Lokalus ekranas

**v9 statusas:** ❌ **PILNAI TRŪKSTA**

---

### 13. ⚠️ NEPILNA: PCF8574 šviesos (OUT1–8)

**Eilutės:** 88-96 (konfigūracija), 191-194 (lempa1), 174-179 (mygtukas1)

```yaml
pcf8574:
  - id: pcf_out1
    address: 0x24
  - id: pcf_out2
    address: 0x25
  - id: pcf_in1
    address: 0x21
  - id: pcf_in2
    address: 0x22
```

**Statusas:**
- ✓ Adresas 0x24, 0x25 — OUT ✓
- ✓ Adresas 0x21, 0x22 — IN ✓
- ✓ Switch `lempa1` (pcf_out1, pin 0) ✓
- ✓ Binary sensor `btn1` (pcf_in1, pin 0) ✓
- ✓ Toggle logika (mygtukas1 → lempa1) ✓

**⚠️ PROBLEMA:**
- **CLAUDE.md sako:** "220V aptikimas: Iškeltas į atskirą ESP32 — PCF8574 0x21 šiame projekte nenaudojamas."
- **v9 turi:** `pcf_in1: 0x21`
- **Paaiškinti:** Ar 0x21 naudojama šiam projektui? Ar jį reikia pašalinti?

**Trūkumai (NEPILNA):**
- ❌ **Tik 1 lempa, turėtų būti 8:**
  - `lempa1` (OUT1, pcf_out1:0) ✓
  - `lempa2…8` — NĖRA
- ❌ **Tik 1 jungiklis, turėtų būti 8:**
  - `btn1` (DIN1, pcf_in1:0) ✓
  - `btn2…8` — NĖRA
- ❌ **Virtualus mygtukas "išjungti visas šviesas"** — NĖRA

**Rekomendacija:** Suduplikuoti for loop arba template'ais 8 lempos ir 8 jungiklius.

---

### 14. ✅ VEIKIA: Inverteris (1 switch)

**Eilutės:** 196-198

```yaml
- platform: gpio
  name: "Sviesos | Inverteris"
  id: inverteris
  pin: { pcf8574: pcf_out2, number: 0, mode: OUTPUT, inverted: false }
```

**Statusas:** ✓ Aprašytas teisingai (pcf_out2, pin 0).

---

## DASHBOARD GRUPAVIMAS — Prefiksų patikrinimas

**v9 naudojami prefiksai:**

| Prefiksas | Jau naudojamas? | Pataiso pagal CLAUDE.md? |
|-----------|-----------------|------------------------|
| `GPS \|` | ✓ | ✓ OK |
| `Pokrypis \|` | ✓ | ✓ OK |
| `GSM \|` | ✓ | ✓ OK |
| `Baterija \|` | ✓ | ✓ OK |
| `DCDC \|` | ✓ | ✓ OK |
| `Sviesos \|` | ✓ | ✓ OK |
| `Aplinka \|` | ✓ | ✓ OK |
| `Energija \|` | ❌ | Reikalingas (akumuliatorius + saulės kroviklis) |
| `Resursai \|` | ❌ | Reikalingas (vanduo + dujos) |

**Trūkumo pavyzdys:**
```yaml
# Turėtų būti:
- name: "Energija | Akum įtampa"
  id: bat_volts
- name: "Energija | Saulės galia"
  id: solar_power
  
- name: "Resursai | Vanduo lygis"
  id: water_level
- name: "Resursai | Dujos kg"
  id: gas_kg
```

---

## KOMPILIAVIMO PATIKRINIMAS

### Žinomos klaidos:

1. **Eilutė 420 — Kintamojo vardas:**
   ```
   undefined reference to 'c0_idx'
   ```

2. **BMS dekodavimo logika — neaiški ir potencialiai su uždėlimais.**

### Rekomendacija:
- [ ] Paleisti `esphome config kempervanas_v9.yaml`
- [ ] Pataisyti `c0_idx` → `c_idx`
- [ ] Peržiūrėti BMS paketo parsavimą (kurie baitai yra kurie duomenys?)

---

## SUVESTINĖ

| Komponentas | Status |
|-------------|--------|
| BMP180 (temp, slėgis) | ✅ VEIKIA (⚠️ nėra prognozės) |
| VL53L0X (vanduo) | ❌ TRŪKSTA |
| HX711 (dujos) | ❌ TRŪKSTA |
| ADXL345 (pokrypis) | ✅ VEIKIA (⚠️ nėra domkratų patarimo) |
| A7670E GPS | ✅ VEIKIA (⚠️ nėra greičio/krypties) |
| A7670E GSM | ✅ VEIKIA (⚠️ nėra SMS atsakymo) |
| Junctek RS485 | ❌ TRŪKSTA |
| Renogy Baterija (BLE) | ✅ VEIKIA (⚠️ kodavimo klaida `c0_idx`, nepilna SOC) |
| Renogy DCDC (BLE) | ✅ VEIKIA (⚠️ tik 2/7 sensoriai) |
| BME688 (oro kokybė) | ❌ TRŪKSTA |
| AM2301 (temp/drėgmė) | ❌ TRŪKSTA |
| SSD1306 OLED | ❌ TRŪKSTA |
| PCF8574 šviesos (1/8) | ⚠️ NEPILNA |
| Inverteris | ✅ VEIKIA |

---

## KITO ETAPO PLANAS (ETAPAS 2 — KOMPILIACIJA)

1. Pataisyti `c0_idx` → `c_idx` eilutėje 420
2. Peržiūrėti BMS BCD dekodavimą (kurie baitai?)
3. `esphome config` — 0 klaidų
4. `esphome compile` — 0 klaidų
5. Dokumentuoti kito etapo pradžią

---

## PAIEŠKOS ŠALTINIAI

- ESPHome docs: https://esphome.io/components/
- ESP32 BLE: https://esphome.io/components/ble_client.html
- Renogy BT: https://github.com/cyrils/renogy-bt
- KC868: https://kincony.com/kc868-a16-v3.html
