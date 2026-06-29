# CLAUDE.md — Kempervanas ESPHome projektas

Šis failas yra pagrindinė instrukcijų bazė Claude Code agentui.
Perskaityk **viską** prieš pradedant bet kokį veiksmą.

---

## Projekto apžvalga

ESP32-S3 pagrindu veikianti kempervano automatizavimo sistema (VW Crafter 2008).
Pagrindinis kontroleris: **KC868-A16v3.1** (ESP32-S3)
WiFi AP: `Kemperis-Valdymas` / `kemperis123` → `192.168.4.1`

### 🎯 Autoritetingi failai (production deployed)

| Komponentas | Failas | Aplankas |
|---|---|---|
| **ESPHome firmware** | `kempervanas_v22_veikiantis.yaml` | `C:\Users\ginta\KemperosProjektas\ESPHOME\` |
| **Google Apps Script** | `GoogleSheetsScript.js` | `C:\Users\ginta\KemperosProjektas\ESPHOME\` |
| **Android app frontend** | `www/index.html` | `C:\Users\ginta\kempervanas\veikiantisKEMPERIS\KemperisApp\` |
| **Android native bridges** | `MainActivity.java`, `KemperisService.java` | `...\KemperisApp\android\app\src\main\java\lt\kemperis\app\` |
| **Dashboard HTML (sena)** | `kemperis1.html` → `kemperis_dashboard.h` | `C:\Users\ginta\kempervanas\CLAUDE\` (nebenaudojamas — pakeistas KemperisApp) |
| **Dalių sąrašas** | `kempervanas_daliu_sarasas_v2.xlsx` | `C:\Users\ginta\kempervanas\CLAUDE\` |

### ⚠️ Aplankų struktūros įspėjimas

Projektas turi **du paraleliais vystymo aplankus** — nepainioti:

- `C:\Users\ginta\KemperosProjektas\ESPHOME\` — **PRODUKCIJA** (v22 deployed firmware, deployed Apps Script)
- `C:\Users\ginta\kempervanas\CLAUDE\` — **DRAFT/HISTORY** (turi v2–v21 senas iteracijas, ANALIZE.md, ši CLAUDE.md)
- `C:\Users\ginta\kempervanas\veikiantisKEMPERIS\KemperisApp\` — **Android app šaltinis**

> ⚠️ CLAUDE folder'yje YRA `kempervanas.yaml` ir `kempervanas_v21.yaml` — tai **nebėra production**. Visada redaguok v22 KemperosProjektas aplanke.
> ⚠️ `kempervanas veikiantis su gps ir sms.yaml` (CLAUDE/) yra **istorinė referencinė** — neredaguok, tik žiūrėk kaip veikė.

---

## Build & Flash komandos

> **Vykdyk iš** `C:\Users\ginta\KemperosProjektas\ESPHOME\` aplanko.

```powershell
# Validacija (be flash'inimo)
esphome config kempervanas_v22_veikiantis.yaml

# Kompiliacija
esphome compile kempervanas_v22_veikiantis.yaml

# Flash per USB-C
esphome run kempervanas_v22_veikiantis.yaml

# OTA (įrenginys turi būti tinkle)
esphome run kempervanas_v22_veikiantis.yaml --device 192.168.4.1

# Logai realiuoju laiku
esphome logs kempervanas_v22_veikiantis.yaml
```

---

## Hardware architektūra

### Magistralės

| Magistralė | Pinai | Įrenginiai |
|-----------|-------|-----------|
| I2C | SDA=GPIO9, SCL=GPIO10 | ADXL345 (0x53), VL53L0X ×1 (0x29), BMP180 (0x77), BME688 (0x76, SDO→GND), PCF8574 ×3 (0x22, 0x24, 0x25), SSD1306 OLED (0x3C, lizdas E) |
| UART0 | TX=GPIO40, RX=GPIO41 | A7670E GSM/GPS (115200 baud) |
| RS485 | TX=GPIO16, RX=GPIO17 | Modbus RTU: Junctek KH-F150A (slave 1, tiesiogiai prie šunto) |
| 1-Wire | GPIO39 | AM2301 temperatūra/drėgmė (atsarginė) |
| GPIO | GPIO47 (DOUT), GPIO48 (CLK) | HX711 (dujų baliono svoris) |
| ADC | GPIO7 | TDS vandens kokybės jutiklis |
| PCF8574 0x22/0x24/0x25 | — | 8 šviesos išėjimai (sviesa_1…sviesa_8) + 8 jungiklių įėjimai |


### Žinomi apribojimai ir kritinės pastabos

- **ADXL345**: Boot metu būtinas I2C write `{0x2D, 0x08}` (measurement mode) prieš nuskaitant duomenis.
- **BME688 adresas**: SDO → GND → adresas 0x76 (ne 0x77, kad nevyktų konfliktas su BMP180 0x77).
- **VL53L0X**: Vienas jutiklis, adresas 0x29. Nėra XSHUT logikos.
- **Junctek KH-F150A**: Jungiamas **tiesiogiai prie šunto laidų** (valdiklio dėžutė sudegė). Slave adresas = 1 (gamyklinis — konflikto su Renogy nebėra, nes Renogy pereina į BT).
- **Renogy DCC50S Bluetooth**: Turi integruotą BT. Reikia BLE client sprendimo — žr. Etapas 1 krok. 6. Alternatyva: atskiras ESP32 su BT → MQTT → KC868.
- **Renogy PRO 100Ah LiFePO4**:Turi integruotą BT. Reikia BLE client sprendimo
- **HX711**: DOUT=GPIO47, CLK=GPIO48 — patikrink konfliktus.
- **A7670E**: 5V, max ~2A. UART TX=GPIO40, RX=GPIO41.
- **RS485**: AUTO-direction per MAX13487EESA — YAML **neturi** turėti `direction_pin`.
- **SMS koduotė**: Tik ASCII (GSM 7-bit) — **jokių lietuviškų raidžių** SMS tekstuose.
- **GPS odometras**: Minimalus greitis = 5 km/h — multipath apsauga.
- **No internet**: Tik WiFi AP, jokio STA/cloud.
- **BMP180 vs BMP085**: ESPHome naudoja `bmp085` komponentą — normalu.
- **220V aptikimas**: Iškeltas į **atskirą ESP32** — PCF8574 0x21 šiame projekte nenaudojamas.
- **UV LED**: Valdomas atskiru krano jungikliu — **neintegruojamas** į šį projektą.
- **8 LED + 8 jungikliai**: OUT1–8 = šviesos (MOSFET sink). DIN1–8 = fiziniai jungikliai (optoisoliuoti, aktyvuojami GND). Kiekvienas jungiklis valdo atitinkamą šviesą. bei vienas virtualus mygtukas isjungti visas sviesas.

---

## Reikalavimai kiekvienam jutikliui

> Tikslinė būsena. Claude Code tikrina ar YAML atitinka — jei ne, taiso ir pagrindžia.

### 1. GPS (A7670E — `+CGNSSINFO`)

**Rodo:**
- Koordinatės (lat/lon, 6 ženklai), aukštis (m), greitis (km/h), kryptis (°)
- GPS / GLONASS / BeiDou palydovų skaičius (naudojami / matomi)
- Fix statusas (text_sensor: "Nėra" / "2D" / "3D")

**Reikalavimai:**
- Atskiri `sensor:` kiekvienam parametrui
- `accuracy_decimals: 0` greičiui, krypčiai, aukščiui; `accuracy_decimals: 6` koordinatėms
- Jei nėra fix — paskutinė žinoma reikšmė arba NaN

### 2. Pokrypis (ADXL345)

**Rodo:**
- Šoninis K/D (°, sveikas, + = dešinė aukščiau)
- Išilginis P/G (°, sveikas, + = priekis aukščiau)
- Domkratų patarimas (text_sensor)

**Reikalavimai:**
- `accuracy_decimals: 0`
- Zero kalibravimas: `button:` → offsets į NVS (`restore_value: true`)
- **Domkratų logika**: `mm = tan(kampas_rad) × 1800` (VW Crafter ratų tarpas 1800mm)
  Pvz.: "Kelti K +42mm" / "Kelti D +18mm" / "Lygu" (< 0.5° = lygu)
- Roll/Pitch su `atan2` iš ADXL345 registrų 0x32

### 3. Dujų svoris (HX711)

**Rodo:** Likutis kg (1 ženklas), likutis %

**Reikalavimai:**
- `number:` tuščio baliono svoris (kg, NVS, numatyta 13.5)
- `number:` pilno baliono svoris (kg, NVS, numatyta 23.5)
- `number:` scale factor kalibravimui (NVS)
- `button:` tare (nustatyti nulį)
- Likutis = `max(0, išmatuotas − tuščias)`, procentai ribojami 0–100%

### 4. Temperatūra ir slėgis (BMP180)

**Rodo:** Temperatūra (°C), slėgis (hPa), aukštis (m), orų prognozė (text)

**Reikalavimai:**
- `number:` temp ofset'as (°C, −10..+10, žingsnis 0.1, NVS)
- Orų prognozė — 7 slėgio reikšmių istorija kas 30 min → 3h tendencija:
  - > +4 hPa/3h → "Labai greitai gerėja"
  - +2..+4 → "Geras"
  - +0.5..+2 → "Gerėja"
  - ±0.5 → "Stabilus"
  - −0.5..−2 → "Kinta"
  - −2..−4 → "Blogas"
  - < −4 hPa/3h → "Smarkiai blogėja"
- Atnaujinimas kas 30 min

### 5. Oro kokybė (BME688)

**Rodo:** Temperatūra (°C), drėgmė (%), slėgis (hPa), VOC (Ω), oro kokybė (text)

**Reikalavimai:**
- I2C adresas: 0x76 (SDO → GND)
- ESPHome komponentas: `bme680` arba `bme68x` — patikrink https://esphome.io/components/sensor/bme680.html
- Jei reikia BSEC — naudoti `bme680_bsec`
- `number:` temp ofset'as (NVS, numatyta −4°C — BME688 šyla nuo savęs)
- Oro kokybė text: "Geras" / "Vidutinis" / "Blogas" pagal VOC reikšmę

### 6. GSM ryšys (A7670E)

**Rodo:** Operatorius, RSSI (dBm), signalas %, registracijos statusas

**Reikalavimai:**
- AT: `AT+COPS?`, `AT+CSQ`, `AT+CREG?` — atnaujinimas kas 60s
- **SMS atsakymas** į "Status?" (ASCII, max 160 simbolių):
  ```
  Kemperis OK
  Bat: {V}V {%}%
  GPS: {lat},{lon} {alt}m
  Tilt: {roll}d {pitch}d
  Gas: {kg}kg H2O:{%}%
  Temp:{C}C {hPa}hPa
  ```

### 7. Vandens bakas (VL53L0X)

**Rodo:** Atstumas cm (raw), lygis %, litrai

**Reikalavimai:**
- `number:` tuščio bako cm (NVS), pilno bako cm (NVS), tūris litrais (NVS)
- `button:` "Nustatyti kaip tuščią" ir "Nustatyti kaip pilną"
- Procentai = `(tuščias − dabartinis) / (tuščias − pilnas) × 100`
- `median` filtras window 5, ribojama 0–100%

### 8. Akumuliatorius (Renogy PRO 100Ah LiFePO4 — Bluetooth)

**Rodo:** Įtampa (V), srovė (A), galia (W), SOC %, Ah, temperatūra (°C), ciklų skaičius

**Reikalavimai:**
- Įrenginys turi integruotą BT (Renogy BT protokolas)
**Reikalavimai:**
- ⚠️ **Reikia tyrimo** (Etapas 1, krok. 6):
  1. Patikrink ar KC868 ESP32-S3 palaiko BLE
  2. Ieškok: https://github.com/cyrils/renogy-bt protokolo
  3. Ieškok: https://github.com/search?q=renogy+esphome+ble
  4. Alternatyva: atskiras ESP32 su BT → MQTT → KC868 per WiFi
  5. Jei nerealus šiame etape — dokumentuoti ir atidėti
  
### 9. Saulės kroviklis (Renogy DCC50S — Bluetooth)

**Rodo:** Saulės įtampa/srovė/galia, akum įtampa, krovimo statusas, režimas, kraunama is saules,kraunama is variklio  

**Reikalavimai:**
- ⚠️ **Reikia tyrimo** (Etapas 1, krok. 6):
  1. Patikrink ar KC868 ESP32-S3 palaiko BLE
  2. Ieškok: https://github.com/cyrils/renogy-bt protokolo
  3. Ieškok: https://github.com/search?q=renogy+esphome+ble
  4. Alternatyva: atskiras ESP32 su BT → MQTT → KC868 per WiFi
  5. Jei nerealus šiame etape — dokumentuoti ir atidėti

### 10. Šviesos ir jungikliai

**Valdymas:** 8 šviesos (sviesa_1…sviesa_8), 8 fiziniai jungikliai (jungiklis_1…jungiklis_8)

**Reikalavimai:**
- Kiekvienas jungiklis_N sinchronizuotas su sviesa_N
- OUT išėjimai: MOSFET sink — `inverted: true` jei reikia
- DIN įėjimai: aktyvuojami GND (KCOM = teigiama pusė)

---

## Dashboard grupavimas (web_server UI)

ESPHome `web_server` rodo sensorius abėcėlės tvarka pagal `name:`.
Grupavimui naudoti **prefiksus su rodyklėmis** — tada grupės rikiuojasi tvarkingai.

**Privaloma naudoti šiuos prefiksus:**

| Prefiksas | Grupė |
|-----------|-------|
| `GPS \|` | Palydovai, koordinatės, aukštis, greitis, kryptis |
| `Pokrypis \|` | Šoninis, išilginis, domkratų patarimas, zero mygtukas |
| `Energija \|` | Akumuliatorius ir saulės kroviklis |
| `Aplinka \|` | BME688 + BMP180 + AM2301 duomenys |
| `Resursai \|` | Vanduo + dujos su kalibravimo mygtukais |
| `GSM \|` | Operatorius, signalas, statusas |
| `Šviesos \|` | sviesa_1…8 ir jungiklis_1…8 |

**Pavyzdžiai:**
```yaml
name: "GPS | Greitis"
name: "GPS | Palydovai GPS"
name: "Pokrypis | Šoninis K/D"
name: "Pokrypis | Domkratų patarimas"
name: "Energija | Akum įtampa"
name: "Aplinka | Temperatūra"
name: "Resursai | Vanduo lygis"
name: "Resursai | Dujos kg"
name: "GSM | Operatorius"
name: "Šviesos | Sviesa 1"
```

---

## Claude Code užduočių sąrašas
> Vykdyk nuosekliai. Kiekvieną žingsnį baigk prieš pradedant kitą.
> **Paieškos taisyklė**: Prieš kiekvieną sprendimą ieškok bent 3 šaltiniuose:
> 1. ESPHome oficiali dokumentacija
> 2. ESPHome GitHub issues/discussions
> 3. Bent vienas bendruomenės šaltinis (forumas, Reddit, kitas GitHub projektas)
> Palygink rastus sprendimus — pasirink patikimiausią ir naujausią.

### ETAPAS 1 — Analizė (nekeisk failų)

```
Perskaityk "kempervanas veikiantis su gps ir sms.yaml" pilnai.
Sudaryk GPIO žemėlapį — visi naudojami pinai. Patikrink konfliktus.
Kiekvieno komponento patikrinimas — kiekvienam įrenginiui iš CLAUDE.md:
a) Ar komponentas apskritai aprašytas YAML? Jei ne — žymėk "TRŪKSTA"
b) Ar ESPHome komponento pavadinimas teisingas? (pvz. bmp085 vs bmp180,
bme680 vs bme68x) — tikrink oficialioje dokumentacijoje
c) Ar I2C adresas sutampa su hardware? Ar nėra adresų konfliktų?
d) Ar GPIO pinai teisingi ir nesikartoja su kitais komponentais?
e) Ar visi privalomi parametrai nurodyti? (update_interval, address, pinai)
f) Ar konfigūracija kompiliuosis be klaidų? — ieškoti panašių klaidų
GitHub issues jei abejoji
g) Ar funkcionalumas atitinka CLAUDE.md reikalavimus? (kalibravimas,
NVS, filtrai, tikslumas) — jei ne, žymėk "NEPILNA"
Tikrintinų komponentų sąrašas (pagal prioritetą):

BMP180 (bmp085) — I2C 0x77, slėgis, temperatūra, prognozė
VL53L0X — I2C 0x29, vandens bakas, XSHUT nereikalingas
HX711 — GPIO47/48, svoris, kalibracija
ADXL345 — I2C 0x53, pokrypis, boot write 0x2D/0x08
BME688 (bme680/bme68x) — I2C 0x76, oro kokybė
A7670E GPS — UART GPIO40/41, CGNSSINFO, palydovai
A7670E GSM — AT+COPS, AT+CSQ, AT+CREG, SMS atsakymas
Renogy PRO LiFePO4 — BLE, tyrimas
Renogy DCC50S — BLE, tyrimas
PCF8574 — tik 0x22/0x24/0x25, 0x21 turi būti PAŠALINTAS
AM2301 — GPIO39, 1-Wire
SSD1306 OLED — I2C 0x3C
8 šviesos OUT1-8 — MOSFET sink
8 jungikliai DIN1-8 — GND aktyvacija


Patikrink RS485 — neturi būti direction_pin.
Renogy BT tyrimas (bent 4 šaltiniai):
a. Ar KC868 ESP32-S3 palaiko BLE?
b. cyrils/renogy-bt protokolas — ar tinka LiFePO4 ir DCC50S?
c. Ieškoti renogy esphome ble GitHub
d. Pasiūlyti sprendimą arba dokumentuoti atidėjimą
Visus radinius išsaugok kaip ANALIZE.md su trimis sekcijomis:

✅ VEIKIA — komponentas teisingai aprašytas
⚠️ NEPILNA — aprašytas bet trūksta funkcionalumo
❌ TRŪKSTA / KLAIDA — nėra arba neteisinga konfigūracija
```

### ETAPAS 2 — Kompiliacija

```
1. esphome config "kempervanas veikiantis su gps ir sms.yaml"
2. Kiekvienai klaidai: priežastis → paieška → pataisa → validacija iš naujo.
3. Kartok kol 0 klaidų.
4. esphome compile — tas pats ciklas.
```

### ETAPAS 3 — Sensorių funkcionalumas (prioritetų tvarka)

- [ ] BMP180 — prognozė, kalibravimas
- [ ] VL53L0X — bakas, kalibravimas
- [ ] HX711 — svoris, kalibravimas
- [ ] ADXL345 — pokrypis, domkratai
- [ ] A7670E GPS — palydovai, koordinatės, fix
- [ ] A7670E GSM — operatorius, signalas, SMS
- [ ] Junctek — Modbus RS485
- [ ] Renogy — BT arba atidėjimas
- [ ] BME688 — temperatūra, drėgmė, VOC
- [ ] 8 šviesos + 8 jungikliai
- [ ] Dashboard grupavimas — visi prefiksai

### ETAPAS 4 — Galutinis patikrinimas

```
1. esphome config — 0 klaidų, 0 įspėjimų
2. esphome compile — sėkminga
3. Atnaujinti ANALIZE.md su pakeitimų sąrašu ir kito etapo planu
```

---

## Ateities etapai (dar nevykdyti)

### Etapas B — HTML Dashboard
Naujų grupių ir sensorių atspindėjimas UI. Neredaguojama dabar.

### Etapas C — MQ dujų jutiklis (ADC A1)
Dujų nuotėkio aliarmas — integruoti kai BMP180/BME688 bus stabilūs.

### Etapas D — Google Sheets logavimas
Per A7670E 4G, kas 15 min. Kritiniai įvykiai iš karto.
30 dienų langas (Apps Script auto-delete).

### Etapas E — OBD2/CAN (atskiras Waveshare ESP32-S3-RS485-CAN)
M-CAN 500kbps: RPM, temp, greitis, turbo.

---

## Kodo konvencijos

- Komentarai YAML — lietuvių kalba
- `name:` — lietuvių kalba su grupės prefiksu (pvz. `"GPS | Greitis"`)
- SMS — tik ASCII
- `restore_value: true` — visi kalibraciniai parametrai
- `accuracy_decimals: 0` — greitis, pokrypis, kryptis, aukštis, %
- `accuracy_decimals: 1` — temperatūra, slėgis, svoris, litrai
- `accuracy_decimals: 2` — įtampa, srovė
- `accuracy_decimals: 6` — GPS koordinatės
- Visi `number:` ir `button:` turi unikalų `id:`
- Nekeisti `web_server:` — SSE veikia

---

## Žinynas
Žinynas ir paieškos šaltiniai

Claude Code: prieš siūlant bet kokį sprendimą — patikrink visus aktualius šaltinius.
Nesiremk tik vienu šaltiniu. Palygink sprendimus ir pasirink geriausią.

ESPHome oficiali dokumentacija

Visi komponentai: https://esphome.io/components/
BMP085/180: https://esphome.io/components/sensor/bmp085.html
BME680/688: https://esphome.io/components/sensor/bme680.html
VL53L0X: https://esphome.io/components/sensor/vl53l0x.html
HX711: https://esphome.io/components/sensor/hx711.html
ADXL345: https://esphome.io/components/sensor/adxl345.html
Modbus controller: https://esphome.io/components/modbus_controller.html
BLE Client: https://esphome.io/components/ble_client.html
UART: https://esphome.io/components/uart.html
Custom komponento rašymas: https://esphome.io/custom/custom_component.html
Lambda / C++ ESPHome: https://esphome.io/guides/automations.html

ESPHome GitHub — klaidų ir sprendimų paieška

Issues: https://github.com/esphome/esphome/issues
Discussions: https://github.com/esphome/esphome/discussions
Pavyzdžių katalogas: https://github.com/esphome/esphome/tree/dev/tests

ESPHome bendruomenė

Forumas: https://community.home-assistant.io/c/esphome/
Discord: https://discord.gg/KhAMKrd (paieška pagal komponentą)
Reddit: https://reddit.com/r/Esphome

Renogy BT protokolas

cyrils/renogy-bt (Python): https://github.com/cyrils/renogy-bt
Protokolo dokumentacija: https://github.com/cyrils/renogy-bt/blob/main/PROTOCOL.md
ESPHome Renogy paieška: https://github.com/search?q=renogy+esphome
ESPHome Renogy BLE: https://github.com/search?q=renogy+ble+esphome
Home Assistant Renogy integracija: https://github.com/search?q=renogy+home+assistant+bluetooth
Renogy BT-2 dongle protokolas: https://community.home-assistant.io/t/renogy-solar-ble

Hardware dokumentacija

KC868-A16v3 schema ir pinout: https://kincony.com/kc868-a16-v3.html
KC868 forum/support: https://www.kincony.com/forum/
A7670E / SIM7670 AT komandos: ieškoti "SIM7670 AT Command Manual PDF"
ADXL345 datasheet: https://www.analog.com/media/en/technical-documentation/data-sheets/ADXL345.pdf
VL53L0X ESPHome GitHub issues: https://github.com/esphome/esphome/issues?q=vl53l0x
BME688 BSEC biblioteka: https://github.com/boschsensortec/BSEC-Arduino-library

Papildomi sprendimų šaltiniai

Stack Overflow ESPHome: https://stackoverflow.com/questions/tagged/esphome
Instructables ESP32 projektai: https://www.instructables.com/search/?q=esp32+esphome
Random Nerd Tutorials: https://randomnerdtutorials.com/?s=esphome
digiblur ESPHome YouTube/blog: https://digiblur.com
ESPHome-Devices.com pavyzdžiai: https://www.esphome-devices.com