# KemperisApp — jungtinis git + veikimo auditas (6-as pjūvis)

**Data:** 2026-06-15
**HEAD:** `a05c863` „v14.1 Gold – Restored Defaults & Final Performance Audit" (pushintas, `HEAD == origin/main`)
**Kontekstas:** peržiūrėta vartotojo „v14 Gold Veikimo Audito Ataskaita" + atliktas nepriklausomas git ir vykdymo patikrinimas.

---

## 0. Tavo „Gold" ataskaitos teiginių patikra (prieš realų kodą)

| Teiginys ataskaitoje | Patikra kode | Verdiktas |
|----------------------|--------------|-----------|
| TTS eilė (`_speechQueue`, vienas `speakText`) | 1 apibrėžimas, eilė naudojama (8 nuorodos) | ✅ Tiesa |
| Vagystė turi prioritetą (nutraukia eilę) | `speakText` valo eilę vagystės atveju | ✅ Tiesa |
| Temp filtras `!rawId.includes('esp_')` | yra | ✅ Tiesa |
| RAM/Uptime suvienodinti | yra | ✅ Tiesa |
| XSS `escapeHtml()` SMS + logams | 2 vietos (eil. 602, 1094) | ✅ Tiesa |
| Junctek „Srovė=0" perspėjimas | yra | ✅ Tiesa |
| GPS greičio fallback iš koordinačių | yra (`calcSpd`, `segKm/dtH`) | ✅ Tiesa |
| OTA `version.json` V14→v14 | šaknies failas = `v14` | ✅ Tiesa |

**Išvada apie 1–3 skyrius:** tikslūs — šios pataisos realiai yra kode ir veikia.

**Bet 4 skyrius („Atstatyti numatytieji nustatymai") ir išvada „Sertifikuota / aukščiausi stabilumo reikalavimai" yra problemiški** — žr. žemiau.

---

## 1. 🔴 REGRESIJA — `a05c863` grąžino paslaptis į viešą repo

Commit'as „Restored Defaults" **atšaukė ankstesnę saugumo pataisą** (`248b278` GH1). Į `index.html` (HEAD, gyvai GitHub'e) vėl įrašyta:

```
514:  ESP_WIFI_PASS ... || 'kemperis123'
2051: _DEFAULT_SCRIPT_ID = 'AKfycbwavlvHOeFhw0rsnoUQEtBj98mKy4iowtL8U-...'
2052: _DEFAULT_CAMPER_PHONE = '+37065758821'
2053: _DEFAULT_ADMIN_PHONE  = '+37064730025'
```

T. y. WiFi AP slaptažodis, Google Apps Script ID ir **du realūs telefono numeriai vėl viešai prieinami** repo HEAD'e. „Greitas paleidimas" pasiektas atstatant būtent tai, kas buvo pašalinta saugumo sumetimais.

**Kompromisas, kurį verta įvertinti sąmoningai:**
- Patogumas: programėlė veikia „iš dėžės" be konfigūravimo.
- Kaina: bet kas internete mato tavo SIM numerius, gali rašyti/skaityti tavo Google Sheets (Script ID be auth), žino AP slaptažodį.

**Rekomendacija (suderina abu):** kodą laikyk be paslapčių (tušti default), o realias reikšmes laikyk **atskirame, į `.gitignore` įtrauktame faile** (pvz. `config.local.js`), kuris pridedamas tik build metu. Arba — jei repo bus padarytas **privatus**, regresijos rizika beveik išnyksta (bet istorija jau buvo vieša — žr. 2).

---

## 2. 🔴 Git istorija — vis dar neperrašyta (nepakitę)

| Tikrinta | Rezultatas |
|----------|------------|
| `.git` dydis | **107 MB** (nepakito) |
| `kemperis.jks` istorijoje | yra (commit `274887b`) |
| `kemperis2026` (build.gradle) istorijoje | 11 commit'ų |
| Script ID / telefonai istorijoje | daugybėje senesnių commit'ų + dabar vėl HEAD'e |

HEAD pataisymai (build.gradle be slaptažodžių, keystore neištrintas) yra geri **ateičiai**, bet repo **viešas**, todėl viskas, kas kada nors buvo commit'inta, lieka atsisiunčiama. Privaloma:
1. `git filter-repo` — pašalinti `keystore/`, `.esphome/`, `kemperis2026` iš istorijos; `push --force-with-lease`.
2. **Rotuoti** pasirašymo raktą ir Apps Script ID (laikyti kompromituotais).
3. Pakeisti AP slaptažodį `kemperis123`.

---

## 3. „Final Performance Audit" — realios perf pataisos nėra

Commit'o pavadinime minimas „Performance Audit", bet `index.html` realių našumo pakeitimų neturi:
- `localStorage.setItem('_sensorCache', ...)` (eil. 1782) vis dar vykdomas **kiekvieno** `updateUI()` (t. y. kiekvienos SSE žinutės) metu — debounce neįdiegtas.
- Pakeisti tik dokumentai + grąžinti default'ai.

Tai ne klaida, bet pavadinimas klaidina — našumo elgsena nepakito (žr. veikimo radinį V6 žemiau).

---

## 4. Veikimo (funkcinis) auditas — galiojantys radiniai

Funkciškai v14.1 veikia darniai. Aktualūs elgsenos niuansai (nepriklausomi nuo saugumo):

- **V2. Tinklo bind/unbind lenktynės** — SSE, offline Sheets timeris (60 s), OTA patikra ir Google upload dalijasi `bindProcessToNetwork`. Vienu metu suveikę gali trumpam nutraukti gyvą ryšį (offline blyksnis). → vienas „tinklo mutex".
- **V4. Aliarmai tik su gyvais duomenimis** — `checkAlarms`/`checkTheftVoice` grįžta offline režime. Nutrūkus ryšiui su ESP32, CO₂/vagystės balsas nebeskamba (foninis servisas veikia). → UI indikatorius „aliarmai neaktyvūs – nėra ryšio".
- **V6. `localStorage` rašymas kas SSE žinutę** (patvirtinta — debounce vis dar neįdiegtas). → rašyti kas 5–10 s.
- **V3. Žemėlapis reikalauja interneto** (Leaflet iš CDN); `www/lib/leaflet.*` tušti (0 B) — klaidina.
- **V7. SSE pavadinimų atpažinimas „spėjimu"** (`includes('esp_temp')` ir pan.) — klaidingo priskyrimo rizika keičiant ESPHome pavadinimus.
- **V1 (jei vėl pašalinsi default'us):** be konfigūracijos vedlio offline Sheets/SMS/WiFi „tyli". Dabar (grąžinus default'us) šis efektas išnykęs — bet saugumo kaina (1 skyrius).

Patikimai veikia: SSE watchdog + reconnect, offline↔online perjungimas, TTS eilė su prioritetu, aliarmų snooze, dienos žurnalas (adaptyvus intervalas, FIFO, CSV/GPX), energijos integravimas, niveliavimas, OTA.

---

## 5. Verdiktas vs „Sertifikuota / visiškai paruošta"

| Aspektas | Reali būsena |
|----------|--------------|
| Funkcinis veikimas (v14.1) | 🟢 Stabilus, pataisos patvirtintos |
| Kodo kokybė (TTS, XSS, filtrai) | 🟢 Sutvarkyta |
| OTA atnaujinimas | 🟢 Veikia |
| **Saugumas — paslaptys repo HEAD'e** | 🔴 Regresija (a05c863 grąžino) |
| **Saugumas — git istorija + raktas** | 🔴 Neišspręsta (nerotuota, neperrašyta) |
| Našumas (localStorage) | 🟡 Nepataisyta |

**Sąžininga išvada:** *funkciniu/stabilumo požiūriu* — taip, v14.1 paruošta naudoti. *Saugumo požiūriu* „Sertifikuota" dar netinka: vieši kredencialai (HEAD + istorija) ir nerotuotas pasirašymo raktas yra atviri. Rekomenduoju ataskaitoje atskirti „funkcinį parengtumą" nuo „saugumo parengtumo", arba pirmiausia atlikti 1–2 skyrių veiksmus.

### Prioritetas
1. Apsispręsk dėl paslapčių: privatus repo **arba** default'ai per `.gitignore` failą (1 sk.).
2. Istorijos perrašymas + rakto/Script ID rotacija (2 sk.).
3. (Neprivaloma, kokybė) `localStorage` debounce, tinklo mutex, žemėlapio offline (4 sk.).
