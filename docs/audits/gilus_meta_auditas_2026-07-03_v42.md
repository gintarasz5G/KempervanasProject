# Gilus meta-auditas — 2026-07-03 (v42 / v33.6)

**Bazė:** commit 6303e18 (po asistento pataisos `app_v42_auditas_techninis.md`).
**Metodas:** kodo peržiūra + vykdomos imitacijos (Node) + online palyginimas su analogiškais projektais.
**Apimtis:** patikrintas asistento pataisytas auditas, gilus firmware/app/Apps Script kodo auditas, Renogy parserio imitacija, rinkos palyginimas.

> Šis auditas *netikrina* fizinio įrenginio — tik kodą ir jo logiką. „Imitacija" = sintetiniai duomenys leidžiami per realų kodo algoritmą.

---

## 1. Asistento pataisyto audito verifikacija

Asistentas `app_v42_auditas_techninis.md` pataisė gerai — visi ankstesni priekaištai adresuoti:

| Ankstesnis priekaištas | Ar ištaisyta | Patikra |
|---|---|---|
| „24 stulpeliai" prieštaravo 22-laukų firmware kontraktui | ✅ Paaiškinta: 24 = **lokalus app CSV** (su 8 Renogy stulpeliais), firmware/Sheets lieka 22–23 | `index.html:783` header realiai turi 24 laukus; firmware — 22 |
| „byte_count", „100% apsaugo" — overclaim | ✅ Pakeista į „ilgio baitas (`buffer[2]+5`)", „ženkliai sumažina" | `renogy.js:236` atitinka |
| Nebuvo eilučių nuorodų | ✅ Pridėtos (`MainActivity.java:486`, `index.html:1571` ir kt.) | Visos patikrintos — teisingos |
| Ignoruota P0 saugumo skola | ✅ Pridėta aiški pastaba verdikte | — |
| „Pilnas auditas" per platus pavadinimas | ✅ Pervadinta „App v42 techninis auditas" | — |

**Išvada:** pataisytas dokumentas dabar tikslus ir sąžiningas. Vienintelis kosmetinis likutis — lentelės eilutė vis dar sako „24 stulpelių headeris + padding", bet dabar su ⚠️ ir paaiškinimu, tad neklaidina.

Patikrinti eilučių atitikmenys (visi teisingi): `index.html:1571` SOC `Math.min(100,...)`; `index.html:783` lokalus 24-laukų CSV header; `index.html:922–927` R5 padding; `MainActivity.java:486` dinaminis APK vardas + `dest.delete()`; `MainActivity.java:524` `ACTION_MANAGE_UNKNOWN_APP_SOURCES`; `AndroidManifest.xml:24` FINE_LOCATION be `maxSdkVersion`.

---

## 2. Vykdomos imitacijos (Node)

Ištraukiau realias `renogy.js` funkcijas (`crc16`, `u16/s16/u32`, framing, parseris) ir paleidau su sintetiniais Modbus RTU kadrais.

| Testas | Rezultatas |
|---|---|
| CRC16 (Modbus, A001 poly) | ✅ `[0x74,0x5b]` teisingas užklausai `FF 03 13 B2 00 06` |
| Baterijos V/A/Ah kadras (len 0x0C) | ✅ A=12.34, V=13.3, remAh=55, capAh=110 → **SOC=50.0%** (skalės teisingos) |
| Kadro ilgio patikra `buffer[2]+5` | ✅ teisingai atskiria kadrą |
| Du kadrai vienoje notifikacijoje | ⚠️ **parsina tik 1-ą**, 17 baitų lieka bufery (žr. B1) |
| `u32()` su aukštu bitu | ⚠️ **grąžina neigiamą** skaičių ≥2³¹ (žr. B2) |

Registrų užklausų ir `len` atitikimas taip pat teisingas: baterija `0x1388 count 0x22` → atsakas len `0x44`; `0x13B2 count 0x06` → len `0x0C`; DCC `0x0100 count 0x21` → len `0x42`. Visi trys `parseFrame` `len` tikrinimai sutampa.

---

## 3. Gilaus kodo audito radiniai

### 3.1. Latentinės klaidos (app, `renogy.js`)

**B1 — Kadrų surinkimas apdoroja tik vieną kadrą per notifikaciją** (`onData`, eil. 230–255).
Po `dev.buffer.slice(expectedLen)` likutis paliekamas, bet nėra `while` ciklo pakartotinai apdoroti bufery likusius pilnus kadrus. Praktikoje rizika **maža**: BLE notifikacijos fragmentuoja atsaką (≤MTU), o kiekviena užklausa resetina buferį, tad kadrai ateina po gabalą, ne sulipę. Bet jei įrenginys kada nors atsiųs 2 sulipusius kadrus vienoje notifikacijoje, antras „užstrigs" iki kitos notifikacijos. **Taisymas:** `while (dev.buffer.length >= dev.buffer[2]+5)` ciklas.

**B2 — `u32()` ženklo bito problema** (eil. 58). `(b1<<24)|...` JS'e grąžina neigiamą reikšmę kai `b1≥0x80` (reikšmė ≥2³¹). remAh/capAh 110 Ah baterijai niekada tokių verčių nepasiekia (110 Ah×1000 = 110000), tad **realiai nepaveikia SOC**. Bet jei kada nors parsintum „Total Ah" gyvavimo skaitiklį — persipildytų į neigiamą. **Taisymas:** `return ((b1<<24)|(b2<<16)|(b3<<8)|b4) >>> 0;`

**B3 (žema/saugumo nit) — `device.name` interpoliuojamas į `onclick` eilutę** (`renderScanResults`, eil. 386–387). BLE įrenginio vardas su `'` ar `"` gali sugadinti HTML/onclick arba injektuoti kodą. Rizika maža (lokalus BLE, savas įrenginys), bet švaresnis sprendimas — `addEventListener` su `dataset`, ne inline `onclick` su string interpoliacija.

**B4 (kosmetinis) — `junc_soc` kešas ribojamas tik `Math.min(100,...)`**, be `Math.max(0,...)` (`index.html:1571`). Rodymo elementai apkarpo `Math.max(0,...)`, tad UI nenukenčia, bet kešo reikšmė teoriškai gali būti neigiama.

### 3.2. Kas kode padaryta gerai

- **Renogy CRC tikrinamas** prieš parsinimą (eil. 246) — blogi kadrai atmetami, ne parsinami.
- **Buferis resetinamas kiekvienai užklausai** (`queryDevice`, eil. 223) — apsauga nuo senų duomenų maišymo.
- **Retry su backoff**: `retryCount<5`, po to 60 s atvėsimo langas (eil. 144, 165) — nesikabina begaliniai bandymai.
- **DCC užklausa 2.5 s ofsetu** nuo baterijos (eil. 162) — vengia BLE radijo konflikto.
- **Firmware 5 CSV eilučių generatoriai nuoseklūs** — visi 22 laukai (patikrinta: 21 kablelis formato eilutėje ties 532/1624/1785/2065/2200). Ankstesnis „22 vs 24" įtarimas buvo `snprintf(row, sizeof(row),` prefikso triukšmas, ne klaida.
- **R5 CSV padding** (`index.html:922`) prideda kablelius eilutės **gale** — saugu, nes formatas visada plėstas pridedant stulpelius gale.

### 3.3. Saugumas (P0 — patvirtinta grep'u)

| Vektorius | Būsena | Įrodymas |
|---|---|---|
| OTA slaptažodis | ❌ Nėra | `ota:` blokas (eil. 45) be `password:` |
| API encryption key | ❌ Nėra | `grep encryption:` = 0 |
| web_server auth | ❌ Nėra | `grep auth:` = 0 |
| Apps Script token | ❌ Nėra | `GoogleSheetsScript.js` grep = 0 |
| SMS siuntėjo autorizacija | ❌ Nevalidijama | `sms_sender` naudojamas tik istorijai |

**Realiausias pavojus — SMS.** Bet kas, žinantis kemperio numerį ir komandų sintaksę (`+arm`/`+disarm`/`+ikelk`), gali valdyti apsaugą ar priversti duomenų siuntimą. WiFi AP-only + fizinis artumas dengia OTA/API/web dalį, bet SMS eina per viešą GSM tinklą. Tai turi būti **Etapas A prioritetas** (kaip jau numatyta CLAUDE.md).

### 3.4. Kodo kokybė (makro)

- `index.html` — **3604 eilutės viename faile** (monolitas). Sunku prižiūrėti; `renogy.js` atskyrimas — geras pirmas modularizacijos žingsnis, verta tęsti (SSE, CSV, UI į atskirus modulius).
- **Nėra automatinių testų** — visa verifikacija rankinė/imitacinė. Renogy parseris ir CSV kontraktas — puikūs kandidatai unit-testams (jau įrodyta, kad juos galima izoliuoti ir paleisti Node'e).
- **0 TODO/FIXME/HACK žymų** — arba švaru, arba skola neregistruojama kode (skola gyvena `docs/audits/`, kas irgi ok).
- Komentarai LT, `name:` LT su prefiksais — atitinka konvencijas.

---

## 4. Palyginimas su analogiškais projektais

| Projektas | Apimtis | Kaip lyginasi |
|---|---|---|
| [gianfrdp/esphome-junctek_khf](https://github.com/gianfrdp/esphome-junctek_khf) | Junctek per UART → Home Assistant | Vienas jutiklis, reikia HA. Šis projektas skaito Junctek per **BLE su savu parseriu** ir neprivalo HA. |
| [mavenius/renogy-bt-esphome](https://github.com/mavenius/renogy-bt-esphome) | Renogy BLE → HA | Analogiška Renogy dalis, bet šio projekto Renogy skaitomas **app'e (planšetės BT)**, ne ESP — mažina ESP RAM riziką. |
| [syssi/esphome-jk-bms](https://github.com/syssi/esphome-jk-bms) | JK-BMS BLE/UART | Brandus vieno komponento projektas su testais/bendruomene; šis — platesnis, bet mažiau brandus. |
| [Vanomation](https://van-automation.com/) | Vandens jutiklis + Renogy → MQTT/HA | Panaši filosofija, bet HA-centrinė; šis — **autonominis (be HA/Pi)**. |
| [Victron Cerbo GX](https://www.victronenergy.com/communication-centres/cerbo-gx) | Komercinis, VRM debesis, GPS geofencing, GX LTE | Nupoliruotas, patikimas, bet ~€160–500+ su celiuliariniu ryšiu ir uždaras Victron ekosistemai. |

**Esminis skirtumas:** dauguma DIY analogų yra **vieno integravimo komponentai, priklausomi nuo Home Assistant / Raspberry Pi**. Šis projektas — **autonominė, offline-first (WiFi AP), 4G-jungta (SMS + Google Sheets) sistema su vagystės apsauga ir sava Android app**. Komercinis Victron nupoliruotas ir patikimesnis, bet nedengia šio projekto jutiklių įvairovės (dujų svoris, lazerinis vandens lygis, CO2, pokrypis, TDS) ir kainuoja kartotinai daugiau.

---

## 5. Stipriosios ir silpnosios pusės

### 💪 Stipriosios
1. **Jutiklių breadth** — energija, GPS, pokrypis, dujų svoris, vandens lygis+TDS, CO2, aplinka, GSM. Lenkia daugumą DIY ir dengia sritis, kurių Victron neturi.
2. **Autonomija** — veikia be interneto/HA/Pi; 4G tik išeitiniam (Sheets/SMS). Reta ir vertinga van-living kontekste.
3. **Vagystės apsauga** — ADXL pokrypis + GSM trianguliacija (CLBS) + GPS sekimas + SMS. Funkcijų rinkinys lygiuojasi su komerciniais.
4. **Robustiškas jutiklių apdorojimas** — median+EMA filtrai, spike atmetimas, BLE watchdog, modem semaforas, RAM sargas.
5. **Verifikuota matematika** — Renogy parseris ir SOC skaičiavimai imitacijoje davė teisingus rezultatus.

### ⚠️ Silpnosios
1. **Saugumas (P0)** — nė vienas kanalas neautentifikuotas; SMS komandos iš bet kokio siuntėjo. Dominuojanti skola.
2. **Prižiūrimumas** — 3600 eilučių monolitinis `index.html`; nėra automatinių testų.
3. **Latentinės parserio klaidos** — B1 (multi-frame), B2 (u32 ženklas) — dabar nekenksmingos, bet spąstai ateičiai.
4. **Bendruomenė/branda** — vieno-žmogaus projektas be testų vs brandūs komponentai (syssi) su CI ir vartotojų baze.
5. **SMS overclaim rizika dokumentacijoje** — istorija rodo polinkį į „100%/patikimiausia" formuluotes; naujausias auditas tai ištaisė, verta išlaikyti discipliną.

---

## 6. Prioritetų rekomendacijos

1. **Etapas A (saugumas) — pirmas.** SMS siuntėjo whitelist (`txt_admin_number` palyginimas *prieš* vykdant komandą); `ota.password`; `api.encryption.key`; Apps Script bendras token'as. Mažiausiai pastangų, didžiausia rizikos redukcija.
2. **B1/B2 taisymai** — 5 minučių `renogy.js` pataisos (while-ciklas + `>>> 0`).
3. **Unit-testai Renogy parseriui ir CSV kontraktui** — jau įrodyta, kad izoliuojami; užfiksuotų 22-laukų kontraktą ir skalės regresijas.
4. **Palaipsnė `index.html` modularizacija** — SSE/CSV/UI į atskirus `.js` (kaip `renogy.js`).
5. **B3/B4** — kosmetiniai, kai bus laiko.

---
*Auditą atliko: Claude (Cowork) · 2026-07-03 · imitacijos: Node.js, sintetiniai Modbus kadrai*
