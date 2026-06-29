# KemperisApp — git auditas (3-ias pjūvis, gilesnis)

**Data:** 2026-06-15 (vakaras)
**Repozitorija:** https://github.com/gintarasz5G/kemperis-app (vieša)
**Tikslas:** nauji radiniai, kurių nebuvo ankstesnėse dviejose ataskaitose (visa istorija, ne tik HEAD)

> Pastaba: nuo praėjusio audito atsirado 3 nauji commit'ai (`f498ea1`, `52e13c9`, `fee34ab`, 20:34–20:47), tarp jų `52e13c9 "Fix update URL (V14 -> v14)"`. Patikrinau, ar jie realiai išsprendė problemas. **Iš dalies — su esmine klaida (žr. N1).**

---

## N1. 🔴 KRITINIS — atnaujinimas VIS DAR neveiks: pataisa atlikta ne tame faile

Repozitorijoje yra **DU `version.json` failai**, kurių turinys nesutampa:

| Failas | `apk_url` registras | Ar tai skaito programėlė? |
|--------|--------------------|--------------------------|
| `version.json` (šaknyje) | `.../download/`**`V14`**`/...` ❌ | **TAIP** |
| `KemperisApp/version.json` | `.../download/`**`v14`**`/...` ✅ | NE |

Programėlė (`MainActivity.VERSION_JSON_URL`) kreipiasi į
`https://raw.githubusercontent.com/gintarasz5G/kemperis-app/main/version.json` — tai **šaknies** failas.

Commit'as „Fix update URL (V14 → v14)" pataisė **`KemperisApp/version.json`** (kopiją, kurios programėlė neskaito), o **šaknies `version.json` liko su `V14`** (didžiosiomis). Kadangi GitHub release tag yra `v14` (mažosiomis), parsisiuntimo nuoroda ir toliau grąžins **404**.

Patikrinta: `HEAD == origin/main` (0/0) — t. y. ši **klaidinga būsena jau yra gyvai GitHub'e**. Vadinasi, v13 **vis dar neatsinaujins** į v14.

**Taisymas (būtinas, 1 vieta):** pataisyk **šaknies** `version.json`:
```json
"apk_url": "https://github.com/gintarasz5G/kemperis-app/releases/download/v14/kemperis_v14.apk"
```
commit + push. **Papildomai:** atsikratyk dubliavimo — palik vieną `version.json` (šaknyje, nes jį skaito OTA), o `KemperisApp/version.json` ištrink, kad ateityje nereikėtų taisyti dviejų vietų ir nekiltų tokių neatitikimų.

---

## N2. 🟠 AUKŠTAS — „Repository Cleanup" buvo tik kosmetinis (istorija neperrašyta)

Commit'ai `e5931a1` ir `52e13c9` su antrašte „Clean Repo" / „Repository Cleanup" **tik `git rm` pašalino failus iš darbo medžio**, bet jie **toliau pilnai pasiekiami git istorijoje** (ir, kadangi repo viešas — visiems internete).

Ištrinta iš HEAD, bet lieka istorijoje (dalinis sąrašas):
- visos senos `index.html` versijos (`KemperisV3/V4/V5.html`),
- senos ESPHome konfigūracijos `kempervanas_v21*.yaml` + `ESPHome YAML konfigūracija.txt`,
- nuotraukos (`2026-05-17/PXL_*.jpg`, ~8.8 MB), `googlesheets.xlsx`, CSV log'ai,
- `DXF.rar`, `Dežė.zip`.

**Pasekmės:**
1. Repo „pasvėrimas" nesumažėjo — `.git` toliau ~107 MB (ESP firmware build artefaktai istorijoje, žr. ankstesnę ataskaitą GM1).
2. **Visos paslaptys lieka istorijoje** nepriklausomai nuo cleanup: keystore slaptažodis `kemperis2026` (`build.gradle`), AP slaptažodis `kemperis123`, Apps Script ID, telefonų numeriai. `git rm` jų **nepašalina** iš viešo repo.

**Taisymas:** vienintelis būdas realiai pašalinti — **perrašyti istoriją** (`git filter-repo` arba BFG), tada `git push --force-with-lease`. Apjunk su keystore ir `.esphome/` valymu (komandos — ankstesnėje ataskaitoje GM1). Po to GitHub'e dar reikia ištrinti senus release'us/cache, jei juose buvo jautrių asset'ų.

---

## N3. 🟠 VIDUTINIS — ESPHome konfigūracija repo atskleidžia AP slaptažodį; OTA be slaptažodžio

`ESPHome/kempervanas_v14_final.yaml` (sekamas HEAD) turi:
```yaml
wifi / ap:
  ssid: "Kemperis-Valdymas"
  password: "kemperis123"     # AP slaptažodis viešame repo
ota:
  - platform: esphome          # be 'password:' — OTA neautentifikuotas
```
- AP slaptažodis `kemperis123` viešas (sutampa su programėlės įkoduotu — patvirtina ankstesnį M1/GH1).
- `ota:` neturi `password:` — kas yra to paties WiFi tinkle, gali užkrauti firmware į ESP32 be slaptažodžio. Lokaliam AP rizika ribota, bet verta pridėti OTA slaptažodį (per `!secret`).
- Gerai: nerasta `api:` encryption key ar kitų raktų YAML'e.

**Taisymas:** WiFi/OTA slaptažodžius perkelk į ESPHome `secrets.yaml` (su `!secret`), o `secrets.yaml` įtrauk į `.gitignore`. Pakeisk AP slaptažodį į stipresnį.

---

## N4. 🟢 ŽEMI / informacija

- **Commit'ai nepasirašyti** (visi `sig:N`). 12 commit'ų, visi vieno autoriaus `Gintaras <gintarasz@gmail.com>` (tavo paties — ne nutekėjimas). Norint „Verified" žymės — įjunk GPG/SSH commit signing.
- **Ankstesnis indekso „corrupt" įspėjimas — buvo laikinas** (skirtingų git versijų artefaktas). Dabar `git log/show/rev-list` veikia be klaidų; tavo repo sveikas. GL1 atšaukiamas.
- **Release tag'ų registras vis dar nenuoseklus** (`V4/V6/v7/v9/v10/v14`) — tai ir sukėlė N1. Pasirink vieną stilių.

---

## Santrauka — kas pasikeitė nuo praėjusio audito

| Buvęs radinys | Būsena dabar |
|---------------|--------------|
| GC2 (apk_url `V14`≠`v14`) | **VIS DAR neišspręsta** — pataisyta tik dubliuotame faile, ne tame, kurį skaito programėlė (N1) |
| GM1 (107 MB build bloat) | Nepakitę — „cleanup" buvo kosmetinis (N2) |
| GC1 (keystore + slaptažodis) | Nepakitę — `kemperis.jks` vis dar HEAD + istorijoje, `kemperis2026` `build.gradle` |
| GH1 (Script ID/telefonai/WiFi) | Nepakitę — lieka istorijoje (N2) |

## Veiksmų eiliškumas
1. **N1** — pataisyk **šaknies** `version.json` (`V14`→`v14`) + ištrink dublį → OTA pradės veikti (5 min darbo).
2. **GC1 + N2 + GH1** — vienas istorijos perrašymas (`filter-repo`): pašalink `keystore/`, `.esphome/`, senus failus; rotuok keystore raktą ir Apps Script ID; perkelk slaptažodžius į ignoruojamus failus.
3. **N3** — ESPHome slaptažodžiai į `secrets.yaml`; pridėk OTA slaptažodį.
