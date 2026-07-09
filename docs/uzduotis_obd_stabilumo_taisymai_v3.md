# Užduotis: v52 patikrinimas realiu testu

**Statusas:** Visi kodo pakeitimai implementuoti ir sukompiliuoti į `kemperis_v52.apk`
(commit'ai `35081db` → `806ba7b` → `f848ca0` → `9e80572` → `8e50c4b` → `0812446` → `21374fb`).
`cap sync` MD5 patikrintas — SUTAMPA. Versijos (`versionCode`/`CURRENT_VERSION`/`version.json`)
— visos **52**, suvienodintos. `git status` švarus, jokių merge conflict žymeklių kode nėra.

**Vienintelis likęs darbas — REALUS testas su automobiliu ir Renogy baterija** (Žingsniai 1-3,
6a-patikra). Visas kodas jau parašytas, tereikia patvirtinti, kad veikia realiomis sąlygomis.

---

## ✅ Ką jau patikrinau (auditas, nereikia kartoti)

| # | Dalykas | Statusas |
|---|---|---|
| 0 | APK v52, `cap sync` MD5 | ✅ Patvirtinta — sutampa |
| 0 | Versijų suderinamumas (gradle/Java/json) | ✅ Patvirtinta — visur 52 |
| 3a | Renogy log lygis `error`→`warn` su komentaru | ✅ Kodas atitinka specifikaciją |
| 4 | `decodeKwpField`/`decodeKwpBlockResponse` | ✅ Implementuota tiksliai |
| 5 | Service21 UI su `GROUP_FIELD_LABELS` | ✅ Implementuota, atitinka lauko-lygio analizę |
| 6a | `ATRV` bug (buvo `startPolling()` viduje, niekad nesileido) | ✅ Ištaisyta — atskira `startBatteryVoltagePolling()` |
| 6b | DTC mygtukas (`1802FF00`) | ✅ Implementuota su fallback pranešimu |
| 6c | VIN/SW-ID kortelės, debug log jungiklis | ✅ Implementuota, papildomai auto-užpildo VIN/SW-ID iš sweep'o |
| — | `node --check obd.js`/`renogy.js` | ✅ Praeina |

**NIEKAS iš esamų, jau veikiančių elementų nebuvo ištrinta ar pakeista be reikalo** — visi
pakeitimai buvo PRIDĖJIMAI, kaip ir reikalauta.

---

## Žingsnis 1: realus testas — OBD

Su `kemperis_v52.apk`, prijungtu ELM327 adapteriu, paspausti „🚀 Surinkti VISKĄ".

**⚠️ SVARBU:** ankstesniame teste (2026-07-06) `tagState('Dujinu ~1500 RPM')` sutapo su
MODULIŲ SKENAVIMU, o Service21 grupių sweep'as įvyko ~2.5 min VĖLIAU, kai variklis jau buvo
išjungtas — todėl vis dar NETURIME nė vieno Service21 grupės atsakymo, užfiksuoto kol RPM > 0.
**Šiame teste būtina:** PIRMA paspausti `tagState()` mygtuką atitinkančiai būsenai (pvz.
„~1500 RPM"), TADA IŠKART paleisti „3b️⃣ Service 21 blokų sweep" (arba „Surinkti VISKĄ"), KOL
variklis realiai tebesuka tą apsukų skaičių — ne bet kada tame pačiame važiavime.

**Tikrintina:**
- Žurnale NĖRA `0902`/`0904`/`090A` (Mode 09) komandų.
- Žurnale NĖRA `0x700-0x7FF` adresų sekos (modulių skenavimas).
- Jei modulių skenavimas vis dėlto paleidžiamas rankiniu mygtuku „4️⃣" — patikrinti, kad
  `ATCS` komanda pasirodo TIK du kartus (prieš ir po `for` ciklo), NE kas 25 adresus.
- Standartinis Mode 01 PID polling'as (`010C`, `010D` ir t.t.) NEPASILEIDŽIA automatiškai.
- **Nauja:** Service21 sweep'as vykdomas KOL RPM > 0 (žr. aukščiau) — tikrinti, ar
  „Grupė 1"/„Grupė 11"/„Grupė 13" laukai dabar rodo NENULINES reikšmes, atitinkančias
  realų RPM/turbo/purkštukų korekcijos lygį.
- „✅ Patvirtinti duomenys" — baterijos įtampa atsinaujina kas ~10s (ATRV pataisytas).
- „🔍 Tikrinti klaidų kodus" mygtukas — patikrinti, ar Service 0x18 atsako, ar aiškus
  „neatsakė" pranešimas.

**Failas jei problema:** `android/www/obd.js`.

---

## Žingsnis 2: realus testas — Renogy temperatūros jutikliai

Su `kemperis_v52.apk`, prijungus prie Renogy baterijos per BLE, stebėti Renogy kortelę ≥10 min.

**Tikrintina:**
- Temperatūros jutikliai 3 ir 4 NEBERODOMI kaip „0.0°C" (arba paslėpti, arba nerodomi
  `sensorCache` viduje, kai `data[34..35]` registro reikšmė < 3/4).
- Jutikliai 1 ir 2 rodo prasmingas reikšmes (~15-30°C diapazone, ne 0).

**Failas jei problema:** `android/www/renogy.js`, `parseFrame()`, `len === 0x44` šaka.

---

## Žingsnis 3: realus testas — Renogy „Writing characteristic failed"

Su tuo pačiu ≥10 min. testu kaip Žingsnis 2, suskaičiuoti
`grep -c "battery query failed" <naujas terminal_log>`.

Sprendimas jau įgyvendintas (log lygis `warn`, ne `error`) remiantis
[cyrils/renogy-bt issue #80](https://github.com/cyrils/renogy-bt/issues/80) — nustatyta, kad
tai **Renogy PRO baterijos BLE modulio aparatūros ypatumas** (patvirtinta nepriklausomai
Python platformoje su ta pačia baterijų serija), NE mūsų kodo ar plugin'o klaida. Duomenys
visada pasiekia klientą, nepaisant klaidos pranešimo.

**Tikrintina:** klaida nebematoma kaip `'error'` (raudona), bet duomenys toliau atsinaujina
normaliai kas ~10s.

**Jei norėtųsi giliau tirti** (NEBŪTINA, bendra Android BLE gera praktika, ne šios klaidos
priežastis): `android/www/ble-plugin.js` turi VIENĄ GLOBALŲ eiliavimą visiems BLE
įrenginiams — pagal
[Making Android BLE work, part 3](https://medium.com/@martijn.van.welie/making-android-ble-work-part-3-117d3a8aee23)
geriau būtų PER ĮRENGINĮ. Tai galėtų padėti su DCC50S ryšio laiko limito problema, jei ji
kartosis.

**DCC50S ryšio laiko limito problema** (jei kartosis STOVINT vietoje) — pagal
[Renogy oficialų troubleshooting](https://www.renogy.com/troubleshooting/How-to-Solve-the-Connection-Issue-Between-BT-2-Bluetooth-Module-and-DC-DC-MPPT-Battery-Charger):
atjungti BT modulį nuo įkroviklio ~30s, prijungti akumuliatorių, tada vėl prijungti BT
modulį. Tai HARDWARE veiksmas, ne kodo pakeitimas.

---

## Kontekstas: Service 21 duomenų dekodavimas (jau implementuota, Dalis D)

**Formulių šaltinis:** [jazdw/vag-blocks](https://github.com/jazdw/vag-blocks) (GPLv3),
`decodeKwpField()`/`decodeKwpBlockResponse()` — 22 tipo kodai (RPM, °C, mbar, %, mg/stk ir
t.t.), implementuota `obd.js` viduje, naudojama `runBlockSweepInternal()`.

**Lauko lygio patikrinimo lentelė** (patikrinta prieš `docs/logs/obd_log_2026-07-06T16-52-42.txt`,
variklis IŠJUNGTAS testo metu — riboja patikrinimą dinaminėms reikšmėms, žr. Žingsnis 1):

| Grupė | L1 | L2 | L3 | L4 |
|---|---|---|---|---|
| 1 | ✅ RPM=0 | ✅ inj.kiekis=0mg/gūž | ❌ tipas 0x64 nežinomas | ❌ tipas 0x05 nežinomas |
| 2 | ✅ RPM=0 | ✅ pedalas=0% | ✅ Binary (tinka „režimo bitai") | ❌ tipas 0x05 nežinomas |
| 3 | ✅ RPM=0 | ✅ MAF norimas=320mg/gūž | ✅ MAF realus=0mg/gūž | ✅ EGR DC=100.6% |
| 4 | ✅ RPM=0 | ❌ tipas 0x1B nežinomas | ✅ trukmė=0ms | ❌ tipas 0x64 nežinomas |
| 10 | ❌ tipas 0x31 (NE RPM!) | ✅ slėgis=989mbar | ✅ slėgis=989mbar | ✅ DC=0% |
| 11 | ✅ RPM=0 | ✅ slėgis=999.6mbar | ✅ slėgis=989mbar | ✅ DC=4.7% |
| 12 | ✅ Binary (tinka „būsenos bitai") | ✅ trukmė=0s | ❌ tipas 0x06 nežinomas | ❌ tipas 0x05 nežinomas |
| **13** | **✅ korekcija=0** | **✅ korekcija=0** | **✅ korekcija=0** | **✅ korekcija=0** |
| 14 | ✅ korekcija=0 | ✅ Binary (nenaudojama) | ✅ Binary (nenaudojama) | ✅ Binary (nenaudojama) |

**Grupė 13 — stipriausias patvirtinimas:** visi 4 laukai naudoja tipą `0x33` (įpurškimo
korekcija) — TIKSLIAI atitinka „cilindrų balanso" teiginį visuose 4 laukuose vienu metu
(atsitiktinis 4/4 sutapimas mažai tikėtinas).

**Grupė 10 vs 11 — bendruomenės lentelė NETIKSLI:** tik grupė 11 turi RPM pirmame lauke;
grupė 10 turi `mg/stk` tipą pirmame lauke, NE RPM. Grupės 10 UI etiketės NENAUDOJAMOS
(kodas jau tai atspindi — `GROUP_FIELD_LABELS` neturi įrašo grupei 10).

**UI logika** (jau implementuota): grupės 1, 2, 3, 11, 13, 14 rodomos su „(tikėtina: ...)"
etikete; grupės 4, 10, 12 — tik žalias skaičius be pavadinimo.

**Vis dar TIKRA riba:** visos aukščiau esančios reikšmės — statinės (variklis išjungtas).
Dinaminis patvirtinimas (RPM keičiasi realiai, slėgis kyla su turbo) reikalauja Žingsnio 1
testo su veikiančiu varikliu.

**Architektūrinis kontekstas:** 2006-2016 VW Crafter statytas kartu su Mercedes Sprinter.
Automobilis NETURI VAG Gateway modulio; tik variklis, imobilaizeris ir centrinis užraktas
yra VW/VCDS-native — likę moduliai (ABS, oro pagalvės) yra Mercedes aparatūra, VAG metodais
nepasiekiama. Todėl šis darbas apsiriboja TIK variklio Service 21 duomenimis.

---

## Bendros taisyklės

- Po KIEKVIENO kodo pakeitimo: `node --check android/www/obd.js` ir
  `node --check android/www/renogy.js`.
- **NEDARYTI** `git commit`/`push`/naujo APK build'o be aiškaus vartotojo prašymo.
- Versijos numeris (`versionCode`/`CURRENT_VERSION`/`version.json`) turi būti VISADA
  identiškas visose trijose vietose.
- Prieš baigiant bet kurį pakeitimą, kuris keičia `obd.js`/`renogy.js`/`index.html`:
  palyginti `git diff` ir įsitikinti, kad NIEKAS jau veikiantis nebuvo ištrinta/pakeista be
  reikalo — leidžiami TIK PRIDĖJIMAI, nebent aiškiai nurodyta kitaip.
- Žingsnius 1-3 atlikti realiu testu su automobiliu/Renogy baterija PRIEŠ laikant šią
  užduotį pilnai baigta.
