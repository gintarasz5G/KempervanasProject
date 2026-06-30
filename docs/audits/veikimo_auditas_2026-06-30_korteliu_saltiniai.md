# Veikimo auditas — kortelių reikšmių šaltiniai (app ↔ firmware)

**Data:** 2026-06-30
**Apimtis:** ar kiekviena `android/www/index.html` kortelėje rodoma reikšmė atvaizduojama
teisingai ir iš teisingo ESP šaltinio.
**Failai:** `android/www/index.html` + `firmware/kempervanas.yaml`
**Metodas:** statinis kryžminis auditas; ESPHome sluginimas patvirtintas prieš realų source kodą
(`.esphome/.../core/helpers.h` → `to_sanitized_char`/`to_snake_case`).

> Šis auditas papildo ir pataiso `veikimo_auditas_2026-06-29_SSE_mapping.md`.

---

## 0. Patvirtintas slug modelis

`to_sanitized_char()` neleistinus simbolius **PAKEIČIA `_`** (ne pašalina), `-` paliekamas.
→ `" | "` → `"___"` (trigubas pabraukimas), kiekvienas LT baitas (`ū`,`į`) → `_`.
Pvz. `"Energija | Akum itampa"` → `energija___akum_itampa`. App fallback grandinės su `___`
yra teisingos. Object_id kyla iš `name:`, NE iš C++ `id:` → `SENSOR_ID_MAP` sensoriams faktiškai
negyvas, viską padengia substring grandinė (`rawId.includes(...)`).

---

## 1. Rezultatas

**38/38 rodomų reikšmių turi teisingą šaltinį ir teisingai prijungtos.** Pagrindiniai rodikliai —
akumuliatorius (SOC/V/A/W/Ah/temp/laikas), vanduo, dujos, BME688 (temp/drėgmė/slėgis/tendencija/
aukštis/prognozė), pokrypis, vibracija, GPS (koordinatės/greitis/aukštis/kryptis/palydovai),
GSM (operatorius/tinklas/roaming/signalas%), miegamasis (CO2/temp/drėgmė/tendencija/kokybė),
220V šaltinis, patarimai, Google buferis, sistema (RAM/ESP temp/uptime), SMS, kalibravimo `number`.
Paros energija (Wh/avg/max) ir kelionės statistika — skaičiuojama app'e iš `junc_power`/GPS srauto.

---

## 2. ❌ PATAISA: §2.1 iš 2026-06-29 audito — FALSE POSITIVE

Birž. 29 auditas skelbė KRITINĘ klaidą: esą akum. temperatūra užteršia aplinkos `bme_temp`,
nes eil. 2209 yra atskiras `if`.

**Tai neteisinga.** Patikrinta `android/www/index.html:2197–2268`: tai **vienas vientisas
`if / else-if`** blokas. Eil. 2209 prasideda `else if`. Todėl:
- `"Energija | Akum temperatura"` (`energija___akum_temperatura`) → pagaunama ties eil. **2205**
  (`includes('akum_temp')` → `junc_temp`) → `else-if` grandinė **sustoja**, 2209 nepasiekiamas.
- `"Aplinka | Temperatura"` (`aplinka___temperatura`) → praeina 2205, pagaunama 2209 → `bme_temp`. ✓

**Užteršimo nėra.** „Block A / Block B dvi nepriklausomos grandinės" — klaidinga prielaida.

---

## 3. ❌ TIKRA KLAIDA (birž. 29 audite praleista)

### K1. `refreshWaterLevel()` — neteisingas REST kelias
- **Kur:** `android/www/index.html:1112`
- `fetch(.../sensor/water_level_pct)` — ESPHome REST naudoja **object_id** (`resursai___vanduo_lygis__`),
  ne C++ `id:`. → 404. Mygtukas „💧 Vanduo lygis 🔄" tyliai nepavyksta (`catch` → tik sysLog).
- **Poveikis mažas:** SSE kelias vandens kortelę vis tiek atnaujina; lūžta tik rankinis refresh.
- **Taisymas:** `fetch(.../sensor/resursai___vanduo_lygis__)`.

---

## 4. ⚠️ Dizaino / nepanaudoti šaltiniai (rodo teisingai, bet netiesiogiai)

- **P1.** `binary_sensor` SSE iš viso neapdorojami (handleris tik `sensor/text_sensor/number/text/switch`).
  → `comm_alarm` („Sistema | Rysio aliarmas") **niekur nerodomas**; `shore_power_present` /
  `inverter_220_active` ignoruojami. 220V kortelė išvedama iš text `power_source_220` (veikia).
- **P2.** GSM dBm skaičiuojamas app'e iš `%` (`-113 + pct*0.63` ≈ firmware `-113 + raw*2.0`) — realus
  `gsm_signal_dbm` jutiklis nenaudojamas. Klaidos nedaug.
- **P3.** Nenaudojami (UNMATCHED, UI neskaito): `modem_state_sensor`, `last_error_sensor`,
  `gps_fix_status`, `junc_ah_total`, `gsm_signal_dbm`, `junc_raw_hex` (vidinis).

---

## 5. 🔶 Rizikos

- **R1.** Visa app↔ESP sąsaja remiasi substring grandine + ESPHome „pakeisk simbolį `_`" semantika
  (trigubas `___`). Jei ESPHome grįžtų prie „pašalink" elgsenos (dvigubas `__`) — daug reikšmių
  tyliai dingtų. **Pinink ESPHome versiją.** Tas pats — pervadinus bet kurį LT `name:`.
- **R2.** `SENSOR_ID_MAP` — negyvas kodas sensoriams (object_id ≠ C++ id). Klaidina prižiūrėtoją.

---

## 6. Išvada

- Šaltinių kontraktas **teisingas** — visi rodomi jutikliai prijungti gerai.
- **1 reali klaida:** K1 (`refreshWaterLevel` REST 404) — tik rankinis mygtukas.
- Birž. 29 audito „kritinė §2.1" — **paneigta** (false positive).
- Kita — nepanaudoti sensoriai ir latentinės rizikos.

> Junctek `junc_*` realų užpildymą lemia BLE hardware — šio audito apimtis yra app↔firmware
> kontraktas, ne BLE veikimas.
