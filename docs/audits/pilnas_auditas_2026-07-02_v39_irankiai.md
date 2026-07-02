# Pilnas instrumentinis auditas — 2026-07-02, v39 (v33.3)

**Bazė:** commit 01de260 / APK kemperis_v39.apk.
**Skirtumas nuo ankstesnių auditų:** ne tik kodo skaitymas — visos išvados paremtos
**įvykdytais įrankiais**: node unit testais, CRC kryžmine validacija, APK binarine
analize, python statine YAML analize, git integralumo patikra.

## Naudoti įrankiai ir rezultatai

| Patikra | Įrankis | Rezultatas |
|---|---|---|
| JS sintaksė (renogy.js, ble-plugin.js, capacitor.js, GoogleSheetsScript.js) | `node --check` | ✅ visi 4 |
| **Renogy parserio unit testai** (16 testų: signed srovė, V, SOC, 4 celės, temp, visi DCC laukai, stage=MPPT, fragmentacija per 3 notifikacijas, R1 atsparumas pamestam atsakymui) | node arnešas su sintetiniais Modbus kadrais + tikru CRC | ✅ **16/16** |
| CRC16 implementacijos validacija | Kryžminis vektorius iš renogy-bt README (`FF 03 0100 0022` → `D1 F1`) | ✅ atitinka |
| APK turinys == commitintas kodas | `cmp` APK `assets/public/*` ↔ `git show HEAD:*` | ✅ **baitas į baitą** (4 failai) |
| APK manifest BT permisijos | binarinio AXML `strings` | ✅ SCAN/CONNECT/BLUETOOTH/ADMIN/LOCATION |
| Versijų grandinė | python (version.json ↔ build.gradle ↔ APK failas) | ✅ 39/39/„33.3"/v39.apk |
| Firmware GPIO konfliktai | python regex analizė | ✅ 9 pinai, 0 dublikatų |
| Firmware id/name unikalumas | python | ✅ 207 id, 89 name, 0 dublikatų |
| SMS ASCII (be LT raidžių) | python | ✅ |
| Firmware CSV šablonai (4 vietos) | python | ✅ visi 22 laukai, `;` gale |
| index.html DOM id dublikatai | grep/uniq | ✅ nėra |
| JSON failų validumas | python json | ✅ (po atstatymo, žr. žemiau) |
| Git objektų DB | `git show`/`git log` | ✅ veikia |

## 🆕 Nauji radiniai

**R7. `research/renogy-dcdc/renogy.yaml` užkoduotas CRC — KLAIDINGAS.**
Užklausai `FF 03 0100 0021` faile įrašyta `D1 CA`, o teisingas CRC — `91 F0`
(patikrinta implementacija, validuota prieš renogy-bt vektorių). App'ui įtakos NĖRA
(renogy.js CRC skaičiuoja runtime), bet tai tikėtina priežastis, kodėl ankstesnis
ESP-side eksperimentas su Junctek/Renogy tylėjo — įrenginiai ignoruoja blogo CRC
užklausas. Taisyti, jei kada bus grįžtama prie Etapo D 2 žingsnio.

**Vietinės git kopijos korupcija (tęsiasi):** `git fsck` → „bad index file sha1
signature" (indeksas sugadintas); `version.json` worktree buvo nukirstas (unterminated
string) — **atstatytas iš HEAD**. Modelis pasitvirtino: commit'ai daromi kitoje
kopijoje, šis aplankas — veidrodis su sinchronizacijos artefaktais. Vaistai šiai
kopijai: ištrinti `.git/index` ir `.git/index.lock`, tada `git reset` (atstatys indeksą
iš HEAD; darbiniai failai nesikeičia).

**Repo dydis:** `.git` = **143 MB** (14 APK istorijoje). GitHub Releases rekomendacija
lieka galioti.

## Kas patikrinta ankstesniuose šios dienos audituose (nekartota)

WiFi bind logika (checkAndBind + VALIDATED + onCapabilitiesChanged), DCC/baterijos
registrų žemėlapiai prieš renogy-bt šaltinį, plugins.json + DEX klasės APK'e,
saugumo P0 būsena. Žr. `auditas_2026-07-02_vakaras_v38.md`,
`verifikacija_v36_renogy_2026-07-02.md`, `projekto_auditas_2026-07-02_rinkos_palyginimas.md`.

## Limitacijos (sąžiningai)

- `esphome config` nevalidiuota — šioje aplinkoje ESPHome neįdiegtas; firmware tikrintas
  tik statiškai (GPIO/id/name/CSV/SMS). **CI su `esphome config` rekomendacija lieka P1.**
- SSE mapping simuliacija nekartota — SSE grandinės kodas šiandien nekeistas
  (v35 audito rezultatas 39/42 tebegalioja), bet gyva ESP verifikacija vis dar laukia.
- Gyvi BLE/WiFi testai — tik su tikra įranga (checklist v38 audito pabaigoje).

## Verdiktas

**v39 yra geriausios būklės release iki šiol:** parserio matematika įrodyta testais
(ne tik peržiūra), APK garantuotai atitinka commitintą kodą, versijos sinchronizuotos.
Belieka gyvas testas. Atviri prioritetai nepasikeitę: **P0 sauga → CI → gyva SSE
verifikacija → Renogy gyvas testas.**

*Auditas: 2026-07-02 vėlyvas vakaras. Testų artefaktai: /tmp/renogy_test.js (16 testų).*
