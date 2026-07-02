# Kemperis App - Gilusis auditas ir strateginis pasiūlymas (v32.3)

## 1. Lyginamoji analizė su rinkos lyderiais

| Funkcija | Kemperis App (v32.3) | SmartyVan | VictronConnect (VRM) | CaraControl | Truma iNet X |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Duomenų gavimas** | **SSE (Gyvai) + Sheets** | Real-time (HA/Nabu Casa) | Real-time Cloud (VRM) | Global LTE-M | Local BT / Cloud (2025) |
| **Pranešimų forma** | **Sintezuotas balsas (TTS)** | Push pranešimai | Push pranešimai | Garsinė sirena + Push | Tik vizualūs įspėjimai |
| **Tinklo stabilumas** | **Dual-Network (WiFi+LTE)** | Starlink + LTE-M | Priklauso nuo Cerbo GX | Specializuotas modemas | Dažni BT lūžiai |
| **Valdymas** | Tik monitoringas | Pilnas (Inverteriai/Relės/AC) | Pilnas (Inverteriai/Relės) | Pilnas (Šviesos/Šildymas) | Pilnas (Klimatas) |
| **Unikalumas** | **Lygiavimas + Žvejyba** | **Starlink + HA integration** | **Saulės prognozė** | **Welcome Home funkcija** | **Klaidų aiškinimas tekstu** |

### Konkurentų įžvalgos
1.  **SmartyVan (smartyvan.com)**: Tai itin galingas atviro kodo projektas, pastatytas ant **Home Assistant** bazės.
    *   **Stiprybė**: „Zero-watt“ filosofija (latched relės), Starlink automatizacija (įjungia tik kai reikia taupant bateriją) ir OBD-II integracija (rodo kuro lygį, padangų slėgį tiesiai app'se).
    *   **Trūkumas**: Labai aukštas įėjimo barjeras vartotojui (reikalauja konfigūravimo) ir priklausomybė nuo „Nabu Casa“ debesų prenumeratos nuotoliniam valdymui.

### „Kemperis App“ pranašumas
Nors **SmartyVan** siūlo neįtikėtiną automatizacijos gylį, **Kemperis App** išlaiko pranašumą **informavimo greičiu**. SmartyVan pasitiki „Push“ pranešimais, kuriuos telefonas gali nutildyti (DND rėžimas), tuo tarpu Kemperio **Native TTS** balsas „prasimuša“ per bet kokį foninį triukšmą ar vartotojo užmaršumą.

## 2. Techninis veikimo auditas

- **Proaktyvumas**: Programėlė šiuo metu yra pasyvi (rodo, bet nedaro įtakos).
- **Analitika**: Trūksta ilgalaikės vartojimo istorijos (vandens, elektros tendencijų).
- **Stabilumas**: v32.3 pasiekė aukščiausią patikimumo lygį (Regex fix, Dual-network fixes).

## 3. Strateginis Roadmap (v33-v35)

### I. Energijos Prognozė (v33) - *SmartyVan/Victron lygis*
- **Solar Readiness**: Analizuoti orų prognozę ir siūlyti energijos taupymo planą.
- **Adaptive Time-to-Empty**: Skaičiuoti akumuliatoriaus veikimo laiką pagal slankųjį vidurkį.

### II. Sumanioji Automatizacija (v34)
- **Starlink Sync (Idėja iš SmartyVan)**: Jei baterija < 20%, automatiškai siųsti komandą į ESP relę, kuri išjungia Starlink (kuris kemperyje vartoja ~60-100W).
- **Rėžimas „Saugus išvykimas“**: Vieno mygtuko patikra prieš kelionę.
- **Loginis variklis**: „Jei < 5°C viduje, įjungti šildymą“.

### III. Geofencing & Security Plus (v35)
- **Virtuali tvora**: Nuolatinis lokacijos transliavimas, jei kemperis pajuda be leidimo.
- **Welcome Home**: Automatinis šviesų įjungimas aptikus telefono Bluetooth.

### IV. UX: „Naktinis sargas“ (Blackout Mode)
- Visiškas ekrano pritemdymas nakčiai, paliekant tik garsinius aliarmus.

---
*Audito data: 2026-07-01*
*Versija: v32.3*
