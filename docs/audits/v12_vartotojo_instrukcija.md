# 🚐 Kemperis App — Naudojimosi Instrukcija

## 1. Pradžia ir Ryšys

### Du veikimo režimai

**📡 Gyvas režimas** — prijungtas prie kemperio WiFi (`Kemperis-Valdymas`):
- Duomenys atsinaujina kiekvieną sekundę per SSE
- Visos funkcijos prieinamos: mygtukai, perjungėjai, GPS
- Antraštėje rodoma: `Prisijungta`

**📵 Offline režimas** — kemperis toli arba WiFi nepasiekiamas:
- App automatiškai per 4G atsisiunčia paskutinius duomenis iš Google Sheets
- Rodomas paskutinis užfiksavimo laikas
- Antraštėje rodoma: `📵 Offline`
- ESP32 mygtukai ir perjungėjai automatiškai išjungiami (pilkėja)
- Kortelės be realaus laiko duomenų prislopinamos

### Mygtukų antraštėje reikšmė
| Mygtukas | Funkcija |
|---|---|
| ⚡ Gyvai | Priverstinis SSE prisijungimas (jei app vis dar rodo Offline) |
| 📵 Offline | Priverstinis perėjimas į offline (Sheets) režimą |
| 🔔 Garsumas | Aliarmų TTS garsumas |

---

## 2. Skirtukų Vadovas

### 📊 Apžvalga
Svarbiausi skaičiai vienoje vietoje:
- **🔋 SOC** — akumuliatoriaus įkrova %. Spalvos: žalia >50%, geltona 25–50%, raudona <25%
- **⚡ 220V** — aktyvus šaltinis: `Išorinis` (shore power) arba `Keitiklis`
- **🔐 Apsauga** — ar įjungta vibracijų/vagystės apsauga
- **🔥 Dujos** — dujų baliono likutis %
- **💧 Vanduo** — vandens bako lygis %
- **Svarbu:** Raudoni skaičiai = aliarmo riba, geltoni = įspėjimo riba

### 📍 GPS ir Žemėlapis
- **Koordinatės** — dabartinė vieta (platuma/ilguma)
- **Greitis, Aukštis, Kryptis** — realiu laiku iš GPS modulio
- **Palydovai** — GPS/GLONASS/BeiDou/Galileo palydovų skaičius
- **🗺️ Rodyti žemėlapyje** — nubraižo šios dienos maršrutą su rodykle
  - Rodyklė sukasi pagal GPS kryptį
  - Kai žemėlapis atidarytas — pozicija atsinaujina kiekvienu SSE eventu

### 🔋 Energija
- **Įkrova (SOC)** — procentai ir Ah likutis
- **Įtampa / Srovė / Galia** — realiu laiku
- **Laikas iki pilno / tuščio** — skaičiuojamas pagal dabartinę srovę
- **220V šaltinis** — Shore power arba Inverter
- **Energijos sekimas** — apyvarta Wh, vid. galia, maks. galia nuo sesijos pradžios

### 🌡️ Aplinka
- **Temperatūra/Drėgmė/Slėgis** — lauke (BME280)
- **Miegamasis** — CO₂ ppm, temp, drėgmė (SCD41)
- **💦 Kondensato rizika** — rodoma kai temp < 8°C IR drėgmė > 65%
- **🌬️ Oro kokybė** — bendras įvertinimas
- **🎣 Žvejybos paruoštukas** — pagal slėgio tendenciją ir temp prognozuoja kibimą

### ⚖️ Lygiavimas
- Interaktyvūs grafikai rodo šoninį ir priekio/galo posvyrį laipsniais
- Mygtukas **🎯 Kalibruoti** — nustato dabartinę padėtį kaip nulinę (lygiaus pagrindo kalibravimas)
- Rodoma kiek cm kurį kampą reikia pakelti domkratu (pagal sukonfigūruotą ašių atstumą)

### 💬 Ryšys (SMS)
Apsaugos komandos per SMS kai kemperis nepasiekiamas per WiFi:

| Mygtukas | SMS komanda | Rezultatas |
|---|---|---|
| 🔒 Arm | `+arm` | Įjungia apsaugą + TTS patvirtinimas |
| 🔓 Disarm | `+disarm` | Išjungia apsaugą + TTS patvirtinimas |
| 📡 Statusas | `+status` | ESP32 siunčia statuso žinutę (tik sysLog, ne TTS) |
| 📍 Lokacija | `+lokacija` | GPS koordinatės žinute |
| 📤 Upload | `+ikelk` | Priverstinis Sheets įkėlimas |

**Pastaba:** TTS skamba TIK po `+arm`/`+disarm` patvirtinimo. `+status` atsakas TTS nesukelia.

### 🔧 Nustatymai
Čia konfigūruojami visi sistemos parametrai:

**Tinklas:**
- ESP32 IP adresas (numatyta `192.168.4.1`)
- WiFi SSID ir slaptažodis
- Kemperio SIM telefono numeris
- Google Apps Script ID

**Įrenginiai:**
- Vandens bako kalibravimas (tuščio/pilno atstumas cm)
- Dujų baliono svoriai (tuščias/pilnas kg, tare)
- Pokrypio kalibravimas (ašių atstumai)
- Baterijos talpa (Ah)

**Sistema:**
- TTS kalba (LT/EN)
- TTS garsumas
- UTC laiko poslinkis
- Vibracijos jautrumas

**Apsauga:**
- Admin telefono numeris (SMS apsauga)
- SMS PIN kodas
- GSM APN nustatymai

**ESP32 mygtukai (veikia TIK online):**
- 🎯 Kalibruoti lygiavimą
- 🗑 Išvalyti SMS istoriją ESP32
- 📤 Rankinis Google Sheets siuntimas
- 💧 Vanduo: užfiksuoti tuščią / pilną
- 🔥 Dujos: Tare (sverstuvų nullinimas)
- ⏰ Pažadinti (wake_up)

---

## 3. Aliarmų Sistema

### Aliarmai ir ribos

| Aliarmas | Sąlyga | Snooze grupė |
|---|---|---|
| 💧 Vanduo kritinis | < 5% | water |
| 💧 Vanduo žemas | < 15% | water |
| 🛏️ CO₂ pavojingas | > 2500 ppm | co2 |
| 🛏️ CO₂ padidėjęs | 1800–2500 ppm | co2 |
| 🔋 SOC kritinis | < 15% | soc |
| 🔋 SOC žemas | < 25% | soc |
| 🔥 Dujos žemos | < 10% | gas |
| 🚨 Vagystė | judėjimas be savininko | — |
| 📳 Vibracija | > 0.4 (kai apsauga įjungta) | vibration |
| 💦 Kondensatas | temp < 8°C ir drėgmė > 65% | condensation |

### Snooze (Nutildymas)

Kai aliarmas aktyvus, kortelėje atsiranda juosta su mygtukais:
- **15 min** — nutildyti 15 minučių
- **30 min** — nutildyti 30 minučių
- **60 min (1 val)** — nutildyti 1 valandą
- **✕ Atšaukti** — atšaukti snooze (aliarmas vėl aktyvus)

Snooze laikmatis matomas realiu laiku (atskaita sekundėmis).

Snooze automatiškai išvalomas kai sąlyga atsistato (pvz. vanduo papildytas).

---

## 4. Apsaugos Sistema

### Įjungimas
1. **Per app:** Nustatymai → Apsauga → Toggle perjungėjas
2. **Per SMS:** siųsti `+arm` į kemperio numerį

### Vagystės aptikimas
- ESP32 ADXL345 jutiklis fiksuoja judėjimą
- `alert_movement_sent` = `true` → aktyvuojamas vagystės aliarmas
- **Stovintis kemperis:** „Kemperyje aptiktas judėjimas. Patikrinkite kemperi."
- **Važiuojantis kemperis** (GPS greitis > 3 km/h): „Vyksta vagystė. Kemperis važiuoja."
- Pakartojamas kas 2 minutes kol aliarmas aktyvus

### Vibracijos aliarmas
- Aktyvus tik kai apsauga įjungta
- Suveikia kai `vib_level` > 0.4
- Pakartojamas kas 20s

---

## 5. ESP32 Sveikatos Stebėjimas

Viršuje rodomas geltonas baneris jei:
- **Laisva RAM < 50 KB** → reikia paleisti ESP32 iš naujo
- **ESP32 temperatūra > 70°C** → tikrinti ventiliaciją

TTS kalba tik pirmą kartą: `"Dėmesio. Atmintis baigiasi, liko X kilobaitų."`

---

## 6. App Atnaujinimas

1. Nustatymai → skiltis „Atnaujinimas" → **Tikrinti atnaujinimą**
2. App patikrina `version.json` iš GitHub
3. Jei nauja versija — rodomas modalas su changelog
4. **Atnaujinti** → APK parsiunčiamas per 4G → automatiškai įdiegiamas

**Pastaba:** Prieš atsiuntimą app laikinai atsiriša nuo WiFi (per 4G).

---

## 7. Dažnos Problemos

| Problema | Sprendimas |
|---|---|
| App rodo Offline nors WiFi prisijungęs | Paspausti **⚡ Gyvai** mygtuką antraštėje |
| TTS nekalba | Tikrinti garsumą 🔔, tikrinti Android TTS nustatymus |
| SMS nesiunčiama | Tikrinti kemperio telefono numerį Nustatymuose |
| Žemėlapis neatsidaro | Nėra sukauptų GPS taškų — reikia bent 1 min gyvo ryšio |
| Sheets duomenys seni | Tikrinti Google Apps Script ID Nustatymuose |
| Snooze mygtukai nerodomi | Aliarmas turi būti aktyvus (kortelė raudona/geltona) |
| ESP32 mygtukai pilki | Offline režimas — mygtukai neveikia be WiFi ryšio |
| Aliarmų istorija prarasta | Saugoma localStorage — ištrinama tik „Clear Data" |
