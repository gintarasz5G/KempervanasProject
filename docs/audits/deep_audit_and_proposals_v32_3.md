# Kemperis App - Gilusis auditas ir strateginis pasiūlymas (v32.3)

## 1. Lyginamoji analizė su rinkos lyderiais

| Funkcija | Kemperis App (v32.3) | VictronConnect (VRM) | CaraControl | Truma iNet X |
| :--- | :--- | :--- | :--- | :--- |
| **Duomenų gavimas** | **SSE (Gyvai) + Google Sheets** | Real-time Cloud (VRM) | Global LTE-M | Local BT / Cloud (2025) |
| **Pranešimų forma** | **Sintezuotas balsas (TTS)** | Push pranešimai | Garsinė sirena + Push | Tik vizualūs įspėjimai |
| **Tinklo stabilumas** | **Dual-Network (WiFi+LTE)** | Priklauso nuo Cerbo GX | Specializuotas modemas | Dažni BT lūžiai |
| **Valdymas** | Tik monitoringas | Pilnas (Inverteriai/Relės) | Pilnas (Šviesos/Šildymas) | Pilnas (Klimatas) |
| **Unikalumas** | **Lygiavimas + Žvejybos prognozė** | **Saulės prognozė** | **Welcome Home funkcija** | **Klaidų aiškinimas tekstu** |

### Stiprybės
1. **Hibridinis ryšys**: „Dual-Network“ logika užtikrina SSE stabilumą.
2. **Balsas**: TTS įspėjimai fone yra saugesni nei „Push“ pranešimai.
3. **Nišinės integracijos**: „Žvejybos paruoštuko“ algoritmas.

## 2. Techninis veikimo auditas

- **Proaktyvumas**: Programėlė šiuo metu yra pasyvi (rodo, bet nedaro įtakos).
- **Analitika**: Trūksta ilgalaikės vartojimo istorijos (vandens, elektros tendencijų).
- **Stabilumas**: v32.3 pasiekė aukščiausią patikimumo lygį (Regex fix, Dual-network fixes).

## 3. Strateginis Roadmap (v33-v35)

### I. Energijos Prognozė (v33)
- **Solar Readiness**: Analizuoti orų prognozę ir siūlyti energijos taupymo planą.
- **Adaptive Time-to-Empty**: Skaičiuoti akumuliatoriaus veikimo laiką pagal slankųjį vidurkį.

### II. Sumanioji Automatizacija (v34)
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