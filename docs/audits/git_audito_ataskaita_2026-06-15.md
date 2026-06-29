# KemperisApp — git repozitorijos gilus auditas

**Data:** 2026-06-15
**Repozitorija:** https://github.com/gintarasz5G/kemperis-app (vieša)
**Tikrinta:** git istorija, sekami failai, paslaptys, dideli blobai, lokalus vs remote, atnaujinimo grandinė

---

## 0. Repozitorijos faktai

| Rodiklis | Reikšmė |
|----------|---------|
| Matomumas | **Public** (`repository_public: true`) |
| Šakos | tik `main` |
| Commit'ų | 9 |
| `.git` dydis | **107 MB** |
| Darbo medis (be .git) | 448 MB |
| HEAD | `e5931a1` „Repository Cleanup – Only v14 Gold Release" |
| Lokalus `main` vs `origin/main` | sutampa (viskas įkelta) |
| GitHub `main` `version.json` | **version 14** (patvirtinta per GitHub API) |
| Naujausias release | **`v14`** (mažosiomis), su `kemperis_v14.apk` (4.25 MB) |

---

## 1. 🔴 KRITINIAI radiniai

### GC1. Pasirašymo raktas IR jo slaptažodis yra viešame repo
Tai pilnai išnaudojama grandinė — užpuolikui prieinama **viskas**, ko reikia suklastoti atnaujinimą:

- **Keystore failas:** `KemperisApp/keystore/kemperis.jks` — sekamas dabar ir yra istorijoje (commit `274887b` „v6").
- **Slaptažodžiai atviru tekstu** `android/app/build.gradle`:
  ```
  storeFile     ../../keystore/kemperis.jks
  storePassword "kemperis2026"
  keyAlias      "kemperis_alias"
  keyPassword   "kemperis2026"
  ```

**Pasekmė:** bet kas gali parsisiųsti raktą iš viešo repo, atrakinti jį žinomu slaptažodžiu ir pasirašyti **kenkėjišką APK tuo pačiu parašu**. Kadangi programėlė pati parsisiunčia ir diegia atnaujinimus iš GitHub (`KemperisUpdate.downloadAndInstall`), o Android tikrina tik parašo sutapimą, toks APK būtų priimtas kaip teisėtas atnaujinimas. Tai aukščiausios rizikos radinys visame projekte.

**Taisymas (svarbi tvarka ir įspėjimas):**
1. **Laikyk šį raktą ir slaptažodį kompromituotais.** Sugeneruok **naują** keystore.
2. Iškelk raktą ir slaptažodžius iš repo: `build.gradle` skaityk reikšmes iš `keystore.properties` (kuris yra `.gitignore`), raktą laikyk **tik lokaliai / CI secret'uose**.
3. Perrašyk git istoriją, kad pašalintum `.jks` iš visų commit'ų (žr. GM1 — apjunk su build artefaktų valymu).
4. **Įspėjimas:** pakeitus pasirašymo raktą, esami įdiegti įrenginiai **negalės auto-atsinaujinti** į nauju raktu pasirašytą APK (parašų neatitikimas) — vartotojai turės **rankiniu būdu perdiegti** vieną kartą. Tai būtina kaina už saugumą.

---

### GC2. Atnaujinimas neveikia dėl tag'o registro neatitikimo (`V14` ≠ `v14`)
**Tai pagrindinė priežastis, kodėl v13 neatsinaujina į v14.** (Pataisau ankstesnį atsakymą: `version.json` GitHub'e JAU yra 14, o `v14` release su APK JAU egzistuoja — ankstesnis „version 10" buvo iš seno raw CDN kešo.)

Realios dvi problemos:

1. **Registro neatitikimas (persistentinis, tikrasis blokatorius):**
   - `version.json` → `apk_url` = `.../releases/download/`**`V14`**`/kemperis_v14.apk` (DIDŽIOSIOMIS)
   - Realus release tag = **`v14`** (mažosiomis); tikras failas: `.../releases/download/`**`v14`**`/kemperis_v14.apk`
   - GitHub release URL **skiria didžiąsias/mažąsias raides** → `V14` grąžina **404**. Versijos patikra pavyksta, bet **parsisiuntimas žlunga**.

2. **raw.githubusercontent CDN kešas (laikina):** `raw...main/version.json` keletą minučių po push gali rodyti seną reikšmę. Jei vartotojas matė „naujausia versija", tai dėl šio kešo (rodė 10 > 13 = netiesa).

**Taisymas (1 eilutė):** suvienodink registrą. Lengviausia — `version.json` `apk_url` pakeisk `V14` → `v14`, commit + push:
```json
{ "version": 14,
  "apk_url": "https://github.com/gintarasz5G/kemperis-app/releases/download/v14/kemperis_v14.apk",
  "notes": "v14 - klaidu taisymas" }
```
Ateičiai: laikykis vieno registro stiliaus visiems tag'ams (dabar maišyta: `V4`, `V6`, `v7`, `v9`, `v10`, `v14`).

---

## 2. 🟡 AUKŠTI radiniai

### GH1. Asmens duomenys ir kredencialai commit'inti į viešą repo
`www/index.html` (HEAD, viešai prieinama) yra:
- Google Apps Script ID: `AKfycbwavlvHOeFhw0rsnoUQEtBj98mKy4iowtL8U-...` (eil. 2051)
- Kemperio SIM nr. `+37065758821` (eil. 2052), administratoriaus nr. `+37064730025` (eil. 2053)
- WiFi slaptažodis `kemperis123` (eil. 514)

**Rizika:** Apss Script galas (`doGet`/`doPost`) neturi autentifikacijos — bet kas su šiuo ID gali skaityti ir rašyti į tavo Sheets. Telefono numeriai = vieši asmens duomenys.

**Taisymas:** laikyk Script ID kompromituotu — perdiek Apps Script (naujas `/exec` ID) ir pridėk token tikrinimą; numerius/WiFi pašalink iš kodo (pildomi pirmo paleidimo formoje). Senas reikšmes — pašalink ir iš istorijos kartu su GM1.

---

## 3. 🟠 VIDUTINIAI radiniai

### GM1. Istoriją užkemša build artefaktai (107 MB `.git`)
Didžiausi blobai istorijoje — visi yra **ESP firmware build išvestys**, kurių niekada nereikia versijuoti:

| Dydis | Failas |
|-------|--------|
| 25 MB | `.esphome/build/.../firmware.elf` |
| 20 MB | `.esphome/build/.../.sconsign313.dblite` |
| 16 MB | `.esphome/build/.../firmware.map` |
| 13 MB | `.esphome/build/.../build.ninja` |
| 10 MB | `.esphome/build/.../libbt.a` |
| 8.8 MB | `2026-05-17/PXL_20260513_122139958.jpg` |

**Taisymas:** sukurk `.gitignore` (šaknyje):
```
.esphome/
**/build/
**/.gradle/
node_modules/
*.apk
keystore/
keystore.properties
```
Tada vienu istorijos perrašymu pašalink ir artefaktus, ir keystore (GC1):
```
git rm -r --cached .esphome KemperisApp/keystore
# istorijai (atsargi operacija – pasidaryk kopiją):
git filter-repo --invert-paths \
  --path KemperisApp/keystore/kemperis.jks \
  --path-glob '.esphome/*'
git push --force-with-lease origin main
```
(arba BFG Repo-Cleaner). Po to repo „pasvers" kelis MB vietoj 107 MB.

### GM2. Release tag'ų registras nenuoseklus
`V4 / V6 / v7 / v9 / v10 / v14` — maišyti. Tai tiesiogiai sukėlė GC2. Pasirink vieną konvenciją (rekomenduoju mažąsias `v14`) ir jos laikykis tiek tag'uose, tiek `apk_url`.

---

## 4. 🟢 ŽEMI / higiena

- **GL1. Lokalaus git indekso įspėjimas.** Audito metu sandbox git rodė `index uses ... extension ... index file corrupt`. Greičiausiai tai skirtingų git versijų (Windows vs Linux) artefaktas skaitant indeksą, o ne tavo realaus repo problema. **Patikrink lokaliai:** `git status` ir `git fsck`. Jei tikrai sugadintas — `del .git\index` ir `git reset` atstato.
- **GL2. Ne-UTF8 failo pavadinimas** repozitorijoje (`ignoring E��u extension`) — verta surasti ir pervadinti/pašalinti.
- **GL3. APK vientisumo tikrinimas.** GitHub release dabar pateikia `sha256` asset'ui (`1e90d3...`). Programėlė galėtų tikrinti šią sumą prieš diegimą (įdėk į `version.json`) — papildoma apsauga net ir po GC1 sutvarkymo.

---

## 5. Prioritetų santrauka

| Nr. | Radinys | Prioritetas | Pastangos |
|-----|---------|-------------|-----------|
| GC1 | Keystore + slaptažodis viešame repo | 🔴 Kritinis | Vidutinė (raktas + istorija + reinstall) |
| GC2 | `apk_url` `V14`≠`v14` → atnaujinimas 404 | 🔴 Kritinis funkcijai | **Maža (1 eilutė)** |
| GH1 | Script ID / telefonai / WiFi viešai | 🔴 Aukštas | Vidutinė |
| GM1 | 107 MB build artefaktai istorijoje | 🟠 Vidutinis | Vidutinė (history rewrite) |
| GM2 | Tag'ų registras nenuoseklus | 🟠 Vidutinis | Maža |
| GL1–GL3 | Indeksas, failo vardas, APK sha | 🟢 Žemas | Maža |

**Greičiausias laimėjimas:** GC2 — pakeisk `V14`→`v14` faile `version.json`, commit + push, ir atnaujinimas iš v13 pradės veikti per kelias minutes.
**Skubiausias saugumui:** GC1 — kol raktas+slaptažodis viešame repo, bet kas gali pasirašyti kenkėjišką „atnaujinimą".
