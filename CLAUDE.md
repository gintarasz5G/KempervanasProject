# CLAUDE.md â€” Kempervanas ESPHome projektas

Å is failas yra pagrindinÄ— instrukcijÅ³ bazÄ— Claude Code agentui.
Perskaityk **viskÄ…** prieÅ¡ pradedant bet kokÄ¯ veiksmÄ….

---

## Projekto apÅ¾valga

ESP32-S3 pagrindu veikianti kempervano automatizavimo sistema (VW Crafter 2008).
Pagrindinis kontroleris: **KC868-A16v3.1** (ESP32-S3)
WiFi AP: `Kemperis-Valdymas` / `kemperis123` â†’ `192.168.4.1`

### ðŸŽ¯ Autoritetingi failai (production deployed)

| Komponentas | Failas | Aplankas |
|---|---|---|
| **ESPHome firmware** | `kempervanas_v22_veikiantis.yaml` | `C:\Users\ginta\KemperosProjektas\ESPHOME\` |
| **Google Apps Script** | `GoogleSheetsScript.js` | `C:\Users\ginta\KemperosProjektas\ESPHOME\` |
| **Android app frontend** | `www/index.html` | `C:\Users\ginta\kempervanas\veikiantisKEMPERIS\KemperisApp\` |
| **Android native bridges** | `MainActivity.java`, `KemperisService.java` | `...\KemperisApp\android\app\src\main\java\lt\kemperis\app\` |
| **Dashboard HTML (sena)** | `kemperis1.html` â†’ `kemperis_dashboard.h` | `C:\Users\ginta\kempervanas\CLAUDE\` (nebenaudojamas â€” pakeistas KemperisApp) |
| **DaliÅ³ sÄ…raÅ¡as** | `kempervanas_daliu_sarasas_v2.xlsx` | `C:\Users\ginta\kempervanas\CLAUDE\` |

### âš ï¸ AplankÅ³ struktÅ«ros Ä¯spÄ—jimas

Projektas turi **du paraleliais vystymo aplankus** â€” nepainioti:

- `C:\Users\ginta\KemperosProjektas\ESPHOME\` â€” **PRODUKCIJA** (v22 deployed firmware, deployed Apps Script)
- `C:\Users\ginta\kempervanas\CLAUDE\` â€” **DRAFT/HISTORY** (turi v2â€“v21 senas iteracijas, ANALIZE.md, Å¡i CLAUDE.md)
- `C:\Users\ginta\kempervanas\veikiantisKEMPERIS\KemperisApp\` â€” **Android app Å¡altinis**

> âš ï¸ CLAUDE folder'yje YRA `kempervanas.yaml` ir `kempervanas_v21.yaml` â€” tai **nebÄ—ra production**. Visada redaguok v22 KemperosProjektas aplanke.
> âš ï¸ `kempervanas veikiantis su gps ir sms.yaml` (CLAUDE/) yra **istorinÄ— referencinÄ—** â€” neredaguok, tik Å¾iÅ«rÄ—k kaip veikÄ—.

---

## Build & Flash komandos

> **Vykdyk iÅ¡** `C:\Users\ginta\KemperosProjektas\ESPHOME\` aplanko.

```powershell
# Validacija (be flash'inimo)
esphome config kempervanas_v22_veikiantis.yaml

# Kompiliacija
esphome compile kempervanas_v22_veikiantis.yaml

# Flash per USB-C
esphome run kempervanas_v22_veikiantis.yaml

# OTA (Ä¯renginys turi bÅ«ti tinkle)
esphome run kempervanas_v22_veikiantis.yaml --device 192.168.4.1

# Logai realiuoju laiku
esphome logs kempervanas_v22_veikiantis.yaml
```

---

## Hardware architektÅ«ra

### MagistralÄ—s

| MagistralÄ— | Pinai | Ä®renginiai |
|-----------|-------|-----------|
| I2C | SDA=GPIO9, SCL=GPIO10 | ADXL345 (0x53), VL53L0X Ã—1 (0x29), BMP180 (0x77), BME688 (0x76, SDOâ†’GND), PCF8574 Ã—3 (0x22, 0x24, 0x25), SSD1306 OLED (0x3C, lizdas E) |
| UART0 | TX=GPIO40, RX=GPIO41 | A7670E GSM/GPS (115200 baud) |
| RS485 | TX=GPIO16, RX=GPIO17 | Modbus RTU: Junctek KH-F150A (slave 1, tiesiogiai prie Å¡unto) |
| 1-Wire | GPIO39 | AM2301 temperatÅ«ra/drÄ—gmÄ— (atsarginÄ—) |
| GPIO | GPIO47 (DOUT), GPIO48 (CLK) | HX711 (dujÅ³ baliono svoris) |
| ADC | GPIO7 | TDS vandens kokybÄ—s jutiklis |
| PCF8574 0x22/0x24/0x25 | â€” | 8 Å¡viesos iÅ¡Ä—jimai (sviesa_1â€¦sviesa_8) + 8 jungikliÅ³ Ä¯Ä—jimai |


### Å½inomi apribojimai ir kritinÄ—s pastabos

- **ADXL345**: Boot metu bÅ«tinas I2C write `{0x2D, 0x08}` (measurement mode) prieÅ¡ nuskaitant duomenis.
- **BME688 adresas**: SDO â†’ GND â†’ adresas 0x76 (ne 0x77, kad nevyktÅ³ konfliktas su BMP180 0x77).
- **VL53L0X**: Vienas jutiklis, adresas 0x29. NÄ—ra XSHUT logikos.
- **Junctek KH-F150A**: Jungiamas **tiesiogiai prie Å¡unto laidÅ³** (valdiklio dÄ—Å¾utÄ— sudegÄ—). Slave adresas = 1 (gamyklinis â€” konflikto su Renogy nebÄ—ra, nes Renogy pereina Ä¯ BT).
- **Renogy DCC50S Bluetooth**: Turi integruotÄ… BT. Reikia BLE client sprendimo â€” Å¾r. Etapas 1 krok. 6. Alternatyva: atskiras ESP32 su BT â†’ MQTT â†’ KC868.
- **Renogy PRO 100Ah LiFePO4**:Turi integruotÄ… BT. Reikia BLE client sprendimo
- **HX711**: DOUT=GPIO47, CLK=GPIO48 â€” patikrink konfliktus.
- **A7670E**: 5V, max ~2A. UART TX=GPIO40, RX=GPIO41.
- **RS485**: AUTO-direction per MAX13487EESA â€” YAML **neturi** turÄ—ti `direction_pin`.
- **SMS koduotÄ—**: Tik ASCII (GSM 7-bit) â€” **jokiÅ³ lietuviÅ¡kÅ³ raidÅ¾iÅ³** SMS tekstuose.
- **GPS odometras**: Minimalus greitis = 5 km/h â€” multipath apsauga.
- **No internet**: Tik WiFi AP, jokio STA/cloud.
- **BMP180 vs BMP085**: ESPHome naudoja `bmp085` komponentÄ… â€” normalu.
- **220V aptikimas**: IÅ¡keltas Ä¯ **atskirÄ… ESP32** â€” PCF8574 0x21 Å¡iame projekte nenaudojamas.
- **UV LED**: Valdomas atskiru krano jungikliu â€” **neintegruojamas** Ä¯ Å¡Ä¯ projektÄ….
- **8 LED + 8 jungikliai**: OUT1â€“8 = Å¡viesos (MOSFET sink). DIN1â€“8 = fiziniai jungikliai (optoisoliuoti, aktyvuojami GND). Kiekvienas jungiklis valdo atitinkamÄ… Å¡viesÄ…. bei vienas virtualus mygtukas isjungti visas sviesas.

---

## Reikalavimai kiekvienam jutikliui

> TikslinÄ— bÅ«sena. Claude Code tikrina ar YAML atitinka â€” jei ne, taiso ir pagrindÅ¾ia.

### 1. GPS (A7670E â€” `+CGNSSINFO`)

**Rodo:**
- KoordinatÄ—s (lat/lon, 6 Å¾enklai), aukÅ¡tis (m), greitis (km/h), kryptis (Â°)
- GPS / GLONASS / BeiDou palydovÅ³ skaiÄius (naudojami / matomi)
- Fix statusas (text_sensor: "NÄ—ra" / "2D" / "3D")

**Reikalavimai:**
- Atskiri `sensor:` kiekvienam parametrui
- `accuracy_decimals: 0` greiÄiui, krypÄiai, aukÅ¡Äiui; `accuracy_decimals: 6` koordinatÄ—ms
- Jei nÄ—ra fix â€” paskutinÄ— Å¾inoma reikÅ¡mÄ— arba NaN

### 2. Pokrypis (ADXL345)

**Rodo:**
- Å oninis K/D (Â°, sveikas, + = deÅ¡inÄ— aukÅ¡Äiau)
- IÅ¡ilginis P/G (Â°, sveikas, + = priekis aukÅ¡Äiau)
- DomkratÅ³ patarimas (text_sensor)

**Reikalavimai:**
- `accuracy_decimals: 0`
- Zero kalibravimas: `button:` â†’ offsets Ä¯ NVS (`restore_value: true`)
- **DomkratÅ³ logika**: `mm = tan(kampas_rad) Ã— 1800` (VW Crafter ratÅ³ tarpas 1800mm)
  Pvz.: "Kelti K +42mm" / "Kelti D +18mm" / "Lygu" (< 0.5Â° = lygu)
- Roll/Pitch su `atan2` iÅ¡ ADXL345 registrÅ³ 0x32

### 3. DujÅ³ svoris (HX711)

**Rodo:** Likutis kg (1 Å¾enklas), likutis %

**Reikalavimai:**
- `number:` tuÅ¡Äio baliono svoris (kg, NVS, numatyta 13.5)
- `number:` pilno baliono svoris (kg, NVS, numatyta 23.5)
- `number:` scale factor kalibravimui (NVS)
- `button:` tare (nustatyti nulÄ¯)
- Likutis = `max(0, iÅ¡matuotas âˆ’ tuÅ¡Äias)`, procentai ribojami 0â€“100%

### 4. TemperatÅ«ra ir slÄ—gis (BMP180)

**Rodo:** TemperatÅ«ra (Â°C), slÄ—gis (hPa), aukÅ¡tis (m), orÅ³ prognozÄ— (text)

**Reikalavimai:**
- `number:` temp ofset'as (Â°C, âˆ’10..+10, Å¾ingsnis 0.1, NVS)
- OrÅ³ prognozÄ— â€” 7 slÄ—gio reikÅ¡miÅ³ istorija kas 30 min â†’ 3h tendencija:
  - > +4 hPa/3h â†’ "Labai greitai gerÄ—ja"
  - +2..+4 â†’ "Geras"
  - +0.5..+2 â†’ "GerÄ—ja"
  - Â±0.5 â†’ "Stabilus"
  - âˆ’0.5..âˆ’2 â†’ "Kinta"
  - âˆ’2..âˆ’4 â†’ "Blogas"
  - < âˆ’4 hPa/3h â†’ "Smarkiai blogÄ—ja"
- Atnaujinimas kas 30 min

### 5. Oro kokybÄ— (BME688)

**Rodo:** TemperatÅ«ra (Â°C), drÄ—gmÄ— (%), slÄ—gis (hPa), VOC (Î©), oro kokybÄ— (text)

**Reikalavimai:**
- I2C adresas: 0x76 (SDO â†’ GND)
- ESPHome komponentas: `bme680` arba `bme68x` â€” patikrink https://esphome.io/components/sensor/bme680.html
- Jei reikia BSEC â€” naudoti `bme680_bsec`
- `number:` temp ofset'as (NVS, numatyta âˆ’4Â°C â€” BME688 Å¡yla nuo savÄ™s)
- Oro kokybÄ— text: "Geras" / "Vidutinis" / "Blogas" pagal VOC reikÅ¡mÄ™

### 6. GSM ryÅ¡ys (A7670E)

**Rodo:** Operatorius, RSSI (dBm), signalas %, registracijos statusas

**Reikalavimai:**
- AT: `AT+COPS?`, `AT+CSQ`, `AT+CREG?` â€” atnaujinimas kas 60s
- **SMS atsakymas** Ä¯ "Status?" (ASCII, max 160 simboliÅ³):
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
- `number:` tuÅ¡Äio bako cm (NVS), pilno bako cm (NVS), tÅ«ris litrais (NVS)
- `button:` "Nustatyti kaip tuÅ¡ÄiÄ…" ir "Nustatyti kaip pilnÄ…"
- Procentai = `(tuÅ¡Äias âˆ’ dabartinis) / (tuÅ¡Äias âˆ’ pilnas) Ã— 100`
- `median` filtras window 5, ribojama 0â€“100%

### 8. Akumuliatorius (Renogy PRO 100Ah LiFePO4 â€” Bluetooth)

**Rodo:** Ä®tampa (V), srovÄ— (A), galia (W), SOC %, Ah, temperatÅ«ra (Â°C), ciklÅ³ skaiÄius

**Reikalavimai:**
- Ä®renginys turi integruotÄ… BT (Renogy BT protokolas)
**Reikalavimai:**
- âš ï¸ **Reikia tyrimo** (Etapas 1, krok. 6):
  1. Patikrink ar KC868 ESP32-S3 palaiko BLE
  2. IeÅ¡kok: https://github.com/cyrils/renogy-bt protokolo
  3. IeÅ¡kok: https://github.com/search?q=renogy+esphome+ble
  4. Alternatyva: atskiras ESP32 su BT â†’ MQTT â†’ KC868 per WiFi
  5. Jei nerealus Å¡iame etape â€” dokumentuoti ir atidÄ—ti
  
### 9. SaulÄ—s kroviklis (Renogy DCC50S â€” Bluetooth)

**Rodo:** SaulÄ—s Ä¯tampa/srovÄ—/galia, akum Ä¯tampa, krovimo statusas, reÅ¾imas, kraunama is saules,kraunama is variklio  

**Reikalavimai:**
- âš ï¸ **Reikia tyrimo** (Etapas 1, krok. 6):
  1. Patikrink ar KC868 ESP32-S3 palaiko BLE
  2. IeÅ¡kok: https://github.com/cyrils/renogy-bt protokolo
  3. IeÅ¡kok: https://github.com/search?q=renogy+esphome+ble
  4. Alternatyva: atskiras ESP32 su BT â†’ MQTT â†’ KC868 per WiFi
  5. Jei nerealus Å¡iame etape â€” dokumentuoti ir atidÄ—ti

### 10. Å viesos ir jungikliai

**Valdymas:** 8 Å¡viesos (sviesa_1â€¦sviesa_8), 8 fiziniai jungikliai (jungiklis_1â€¦jungiklis_8)

**Reikalavimai:**
- Kiekvienas jungiklis_N sinchronizuotas su sviesa_N
- OUT iÅ¡Ä—jimai: MOSFET sink â€” `inverted: true` jei reikia
- DIN Ä¯Ä—jimai: aktyvuojami GND (KCOM = teigiama pusÄ—)

---

## Dashboard grupavimas (web_server UI)

ESPHome `web_server` rodo sensorius abÄ—cÄ—lÄ—s tvarka pagal `name:`.
Grupavimui naudoti **prefiksus su rodyklÄ—mis** â€” tada grupÄ—s rikiuojasi tvarkingai.

**Privaloma naudoti Å¡iuos prefiksus:**

| Prefiksas | GrupÄ— |
|-----------|-------|
| `GPS \|` | Palydovai, koordinatÄ—s, aukÅ¡tis, greitis, kryptis |
| `Pokrypis \|` | Å oninis, iÅ¡ilginis, domkratÅ³ patarimas, zero mygtukas |
| `Energija \|` | Akumuliatorius ir saulÄ—s kroviklis |
| `Aplinka \|` | BME688 + BMP180 + AM2301 duomenys |
| `Resursai \|` | Vanduo + dujos su kalibravimo mygtukais |
| `GSM \|` | Operatorius, signalas, statusas |
| `Å viesos \|` | sviesa_1â€¦8 ir jungiklis_1â€¦8 |

**PavyzdÅ¾iai:**
```yaml
name: "GPS | Greitis"
name: "GPS | Palydovai GPS"
name: "Pokrypis | Å oninis K/D"
name: "Pokrypis | DomkratÅ³ patarimas"
name: "Energija | Akum Ä¯tampa"
name: "Aplinka | TemperatÅ«ra"
name: "Resursai | Vanduo lygis"
name: "Resursai | Dujos kg"
name: "GSM | Operatorius"
name: "Å viesos | Sviesa 1"
```

---

## Claude Code uÅ¾duoÄiÅ³ sÄ…raÅ¡as
> Vykdyk nuosekliai. KiekvienÄ… Å¾ingsnÄ¯ baigk prieÅ¡ pradedant kitÄ….
> **PaieÅ¡kos taisyklÄ—**: PrieÅ¡ kiekvienÄ… sprendimÄ… ieÅ¡kok bent 3 Å¡altiniuose:
> 1. ESPHome oficiali dokumentacija
> 2. ESPHome GitHub issues/discussions
> 3. Bent vienas bendruomenÄ—s Å¡altinis (forumas, Reddit, kitas GitHub projektas)
> Palygink rastus sprendimus â€” pasirink patikimiausiÄ… ir naujausiÄ….

### ETAPAS 1 â€” AnalizÄ— (nekeisk failÅ³)

```
Perskaityk "kempervanas veikiantis su gps ir sms.yaml" pilnai.
Sudaryk GPIO Å¾emÄ—lapÄ¯ â€” visi naudojami pinai. Patikrink konfliktus.
Kiekvieno komponento patikrinimas â€” kiekvienam Ä¯renginiui iÅ¡ CLAUDE.md:
a) Ar komponentas apskritai apraÅ¡ytas YAML? Jei ne â€” Å¾ymÄ—k "TRÅªKSTA"
b) Ar ESPHome komponento pavadinimas teisingas? (pvz. bmp085 vs bmp180,
bme680 vs bme68x) â€” tikrink oficialioje dokumentacijoje
c) Ar I2C adresas sutampa su hardware? Ar nÄ—ra adresÅ³ konfliktÅ³?
d) Ar GPIO pinai teisingi ir nesikartoja su kitais komponentais?
e) Ar visi privalomi parametrai nurodyti? (update_interval, address, pinai)
f) Ar konfigÅ«racija kompiliuosis be klaidÅ³? â€” ieÅ¡koti panaÅ¡iÅ³ klaidÅ³
GitHub issues jei abejoji
g) Ar funkcionalumas atitinka CLAUDE.md reikalavimus? (kalibravimas,
NVS, filtrai, tikslumas) â€” jei ne, Å¾ymÄ—k "NEPILNA"
TikrintinÅ³ komponentÅ³ sÄ…raÅ¡as (pagal prioritetÄ…):

BMP180 (bmp085) â€” I2C 0x77, slÄ—gis, temperatÅ«ra, prognozÄ—
VL53L0X â€” I2C 0x29, vandens bakas, XSHUT nereikalingas
HX711 â€” GPIO47/48, svoris, kalibracija
ADXL345 â€” I2C 0x53, pokrypis, boot write 0x2D/0x08
BME688 (bme680/bme68x) â€” I2C 0x76, oro kokybÄ—
A7670E GPS â€” UART GPIO40/41, CGNSSINFO, palydovai
A7670E GSM â€” AT+COPS, AT+CSQ, AT+CREG, SMS atsakymas
Renogy PRO LiFePO4 â€” BLE, tyrimas
Renogy DCC50S â€” BLE, tyrimas
PCF8574 â€” tik 0x22/0x24/0x25, 0x21 turi bÅ«ti PAÅ ALINTAS
AM2301 â€” GPIO39, 1-Wire
SSD1306 OLED â€” I2C 0x3C
8 Å¡viesos OUT1-8 â€” MOSFET sink
8 jungikliai DIN1-8 â€” GND aktyvacija


Patikrink RS485 â€” neturi bÅ«ti direction_pin.
Renogy BT tyrimas (bent 4 Å¡altiniai):
a. Ar KC868 ESP32-S3 palaiko BLE?
b. cyrils/renogy-bt protokolas â€” ar tinka LiFePO4 ir DCC50S?
c. IeÅ¡koti renogy esphome ble GitHub
d. PasiÅ«lyti sprendimÄ… arba dokumentuoti atidÄ—jimÄ…
Visus radinius iÅ¡saugok kaip ANALIZE.md su trimis sekcijomis:

âœ… VEIKIA â€” komponentas teisingai apraÅ¡ytas
âš ï¸ NEPILNA â€” apraÅ¡ytas bet trÅ«ksta funkcionalumo
âŒ TRÅªKSTA / KLAIDA â€” nÄ—ra arba neteisinga konfigÅ«racija
```

### ETAPAS 2 â€” Kompiliacija

```
1. esphome config "kempervanas veikiantis su gps ir sms.yaml"
2. Kiekvienai klaidai: prieÅ¾astis â†’ paieÅ¡ka â†’ pataisa â†’ validacija iÅ¡ naujo.
3. Kartok kol 0 klaidÅ³.
4. esphome compile â€” tas pats ciklas.
```

### ETAPAS 3 â€” SensoriÅ³ funkcionalumas (prioritetÅ³ tvarka)

- [ ] BMP180 â€” prognozÄ—, kalibravimas
- [ ] VL53L0X â€” bakas, kalibravimas
- [ ] HX711 â€” svoris, kalibravimas
- [ ] ADXL345 â€” pokrypis, domkratai
- [ ] A7670E GPS â€” palydovai, koordinatÄ—s, fix
- [ ] A7670E GSM â€” operatorius, signalas, SMS
- [ ] Junctek â€” Modbus RS485
- [ ] Renogy â€” BT arba atidÄ—jimas
- [ ] BME688 â€” temperatÅ«ra, drÄ—gmÄ—, VOC
- [ ] 8 Å¡viesos + 8 jungikliai
- [ ] Dashboard grupavimas â€” visi prefiksai

### ETAPAS 4 â€” Galutinis patikrinimas

```
1. esphome config â€” 0 klaidÅ³, 0 Ä¯spÄ—jimÅ³
2. esphome compile â€” sÄ—kminga
3. Atnaujinti ANALIZE.md su pakeitimÅ³ sÄ…raÅ¡u ir kito etapo planu
```

---

## Ateities etapai (dar nevykdyti)

### Etapas B â€” HTML Dashboard
NaujÅ³ grupiÅ³ ir sensoriÅ³ atspindÄ—jimas UI. Neredaguojama dabar.

### Etapas C â€” MQ dujÅ³ jutiklis (ADC A1)
DujÅ³ nuotÄ—kio aliarmas â€” integruoti kai BMP180/BME688 bus stabilÅ«s.

### Etapas D â€” Google Sheets logavimas
Per A7670E 4G, kas 15 min. Kritiniai Ä¯vykiai iÅ¡ karto.
30 dienÅ³ langas (Apps Script auto-delete).

### Etapas E â€” OBD2/CAN (atskiras Waveshare ESP32-S3-RS485-CAN)
M-CAN 500kbps: RPM, temp, greitis, turbo.

---

## Kodo konvencijos

- Komentarai YAML â€” lietuviÅ³ kalba
- `name:` â€” lietuviÅ³ kalba su grupÄ—s prefiksu (pvz. `"GPS | Greitis"`)
- SMS â€” tik ASCII
- `restore_value: true` â€” visi kalibraciniai parametrai
- `accuracy_decimals: 0` â€” greitis, pokrypis, kryptis, aukÅ¡tis, %
- `accuracy_decimals: 1` â€” temperatÅ«ra, slÄ—gis, svoris, litrai
- `accuracy_decimals: 2` â€” Ä¯tampa, srovÄ—
- `accuracy_decimals: 6` â€” GPS koordinatÄ—s
- Visi `number:` ir `button:` turi unikalÅ³ `id:`
- Nekeisti `web_server:` â€” SSE veikia

---

## Å½inynas
Å½inynas ir paieÅ¡kos Å¡altiniai

Claude Code: prieÅ¡ siÅ«lant bet kokÄ¯ sprendimÄ… â€” patikrink visus aktualius Å¡altinius.
Nesiremk tik vienu Å¡altiniu. Palygink sprendimus ir pasirink geriausiÄ….

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
Custom komponento raÅ¡ymas: https://esphome.io/custom/custom_component.html
Lambda / C++ ESPHome: https://esphome.io/guides/automations.html

ESPHome GitHub â€” klaidÅ³ ir sprendimÅ³ paieÅ¡ka

Issues: https://github.com/esphome/esphome/issues
Discussions: https://github.com/esphome/esphome/discussions
PavyzdÅ¾iÅ³ katalogas: https://github.com/esphome/esphome/tree/dev/tests

ESPHome bendruomenÄ—

Forumas: https://community.home-assistant.io/c/esphome/
Discord: https://discord.gg/KhAMKrd (paieÅ¡ka pagal komponentÄ…)
Reddit: https://reddit.com/r/Esphome

Renogy BT protokolas

cyrils/renogy-bt (Python): https://github.com/cyrils/renogy-bt
Protokolo dokumentacija: https://github.com/cyrils/renogy-bt/blob/main/PROTOCOL.md
ESPHome Renogy paieÅ¡ka: https://github.com/search?q=renogy+esphome
ESPHome Renogy BLE: https://github.com/search?q=renogy+ble+esphome
Home Assistant Renogy integracija: https://github.com/search?q=renogy+home+assistant+bluetooth
Renogy BT-2 dongle protokolas: https://community.home-assistant.io/t/renogy-solar-ble

Hardware dokumentacija

KC868-A16v3 schema ir pinout: https://kincony.com/kc868-a16-v3.html
KC868 forum/support: https://www.kincony.com/forum/
A7670E / SIM7670 AT komandos: ieÅ¡koti "SIM7670 AT Command Manual PDF"
ADXL345 datasheet: https://www.analog.com/media/en/technical-documentation/data-sheets/ADXL345.pdf
VL53L0X ESPHome GitHub issues: https://github.com/esphome/esphome/issues?q=vl53l0x
BME688 BSEC biblioteka: https://github.com/boschsensortec/BSEC-Arduino-library

Papildomi sprendimÅ³ Å¡altiniai

Stack Overflow ESPHome: https://stackoverflow.com/questions/tagged/esphome
Instructables ESP32 projektai: https://www.instructables.com/search/?q=esp32+esphome
Random Nerd Tutorials: https://randomnerdtutorials.com/?s=esphome
digiblur ESPHome YouTube/blog: https://digiblur.com
ESPHome-Devices.com pavyzdÅ¾iai: https://www.esphome-devices.com
