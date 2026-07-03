# Užduotis v43 (v33.7): Renogy kortelės pataisa + smulkmenų sutvarkymas

**Data:** 2026-07-03 · **Bazė:** v42 / v33.6 (commit 6303e18)
**Kontekstas:** įkelto gyvo logo (`2026-07-03 12:48`) analizė + auditai
`docs/audits/gilus_meta_auditas_2026-07-03_v42.md`, `app_v42_auditas_techninis.md`,
`gyvo_testo_analize_2026-07-02.md`.
**Apimtis:** tik `android/www/index.html` + `android/www/renogy.js` + versijos.
**Firmware, SSE branduolys, Sheets, Apps Script — NELIESTI.**
**Saugumas (P0) — SĄMONINGAI ATIDĖTAS į Etapą A. Šioje užduotyje jo NEDARYTI.**

> Ši užduotis PERRAŠYTA po pakartotinio kodo audito + online faktų patikros
> (žr. „Verifikacija" gale). Nuo pirmo juodraščio pasikeitė: pašalintas klaidingas
> „updater DNS spam" punktas (tai ne defektas), patikslinti eilučių numeriai ir P1 pagrindas.

---

## 🔴🔴 STATUSAS — auditoriaus patikra 2026-07-03 (PO asistento pataisymų)

Asistentas jau pritaikė daugumą logikos pataisymų, **bet PALIKO ABU failus NUKIRSTUS**
(worktree korupcija — būtent tai, apie ką įspėja skyrius žemiau). **Programėlė šiuo metu
neveiks** (JS `SyntaxError`). Prieš viską — atkurti failų galus.

### P0 — KRITINIS: nukirsti failai (privaloma pataisyti PIRMA)

| Failas | Kur nutrūksta | Kas prarasta | Įrodymas |
|---|---|---|---|
| `android/www/renogy.js` | eil. 416 `    function forget(type` | `forget()` kūnas, `return { init, … getState }` eksportas, IIFE `})();`, auto-init blokas | `node --check` → `SyntaxError: Unexpected end of input` |
| `android/www/index.html` | gale `…>&#x2715; Uždaryti</butto` | `</button></div>`, `<div id="map">…</div>`, `</body></html>` | `tail` rodo nutrūkusį žodį, nėra `</html>` |

**Pasekmė:** `renogy.js` nebeparsinamas → `RenogyBLE` tampa `undefined` → `index.html`
kviečia `RenogyBLE.getState()`/`.init()` (7 vietose, tarp jų kas SSE atnaujinimą) → lūžta
**visa programėlė**, ne tik Renogy.

**Atkūrimas (žinomas-teisingas galas):**
- `renogy.js` — atstatyti pilną `forget(type)` funkciją, `return {…}` eksporto objektą ir
  auto-init bloką (`if (document.readyState==='loading') … RenogyBLE.init()`), kaip buvo
  prieš pataisymus. **Aukščiau esančių gerų pataisų (P1/P2/P3) NELIESTI.**
- `index.html` — atstatyti nukirstą `</button></div>`, `<div id="map" …></div>`,
  `</body>`, `</html>` uodegą.
- Privaloma po atkūrimo: `node --check android/www/renogy.js` praeina; `index.html`
  baigiasi `</html>`.

### Kas jau PRITAIKYTA teisingai (auditoriaus verifikuota — nekeisti)

| Punktas | Būsena | Vieta |
|---|---|---|
| P1 — `window.sensorCache = sensorCache;` | ✅ pritaikyta | `index.html:722` |
| P2.1 — `onData` `while` ciklas (multi-frame) | ✅ pritaikyta | `renogy.js` onData |
| P2.2 — `u32() … >>> 0` | ✅ pritaikyta | `renogy.js:59` |
| P3 — `pollCycle` skip priežasčių logai (`lastSkipReason`) | ✅ pritaikyta | `renogy.js` pollCycle |
| P3 — `connect failed` → `updateUI()` | ✅ pritaikyta | `renogy.js` connectAndNotify |
| P4.1 — `junc_soc` `Math.max(0,…)` (fallback + display) | ✅ pritaikyta | `index.html:1571, 1916–1917` |

### Kas dar LIKO

1. **Atkurti abu nukirstus failus (P0 aukščiau).**
2. **Versijos bump nepadarytas:** `version.json` vis dar `"version": 42`. Reikia → 43,
   `versionName "33.7"`, APK `kemperis_v43.apk`, `apk_url` sinchronas (žr. „Versijos ir build").
3. **APK neperbuildintas** (`kemperis_v42.apk` ištrintas darbiniame medyje, naujo nėra).
4. P4.2/P4.3 — sąlyginiai/žemi (žr. žemiau), gali likti.

> Po atkūrimo **privaloma** `git diff --stat` patikra: jei kurio failo diff'as baigiasi
> vien ištrynimais gale — failas vėl nukirstas, necommitinti (tai kartojasi jau nebe pirmą
> kartą — verta išsiaiškinti įrankio/redaktoriaus priežastį, kodėl uodega dingsta).

---

## ⚠️ PRIEŠ PRADEDANT (privaloma — dėl anksčiau buvusio worktree sugadinimo)

1. `git status` švarus; jokio `.git/index.lock`.
2. Patikrinti, kad failai NENUKIRSTI: `index.html` baigiasi `</html>`, `renogy.js` —
   `RenogyBLE.init()` auto-init bloku gale.
3. Po darbų, PRIEŠ commit: `git diff --stat` — jei kurio failo diff'as vien ištrynimai
   gale, failas nukirstas, necommitinti.

---

## 🔴 P1 — PAGRINDINIS BLOKATORIUS: Renogy duomenys rašomi į `undefined`

**Tai tikroji priežastis, kodėl Renogy kortelė nerodo duomenų — net kai BLE prisijungia.**
Auditai v42 to nepagavo (pagavo tik jau ištaisytą init/`androidNeverForLocation`).

### Diagnozė (patikrinta kode + online)

- `renogy.js` VISAS 17 reikšmių rašo į **`window.sensorCache`** (eil. 266–306), pvz.
  `window.sensorCache['ren_v'] = …`, `window.sensorCache['dcc_stage'] = …`.
- `index.html` cache'as apibrėžtas kaip **`let sensorCache`** (eil. **723**):
  ```js
  let sensorCache = (function(){ try { return JSON.parse(localStorage.getItem('_sensorCache') || '{}'); } catch(e){ return {}; } })();
  ```
- Visas kitas `index.html` kodas naudoja **bare `sensorCache[...]`** (lokalų `let`) —
  patikrinta ~40 vietų (891, 1002, 1436 `getCache`, 1554, 1570, 1915, 2296, 2767 ir kt.).
  **Niekur nėra `window.sensorCache`** — nei rašymo, nei skaitymo.
- **Faktas (MDN, patvirtinta online):** klasikiniame `<script>` `let`/`const`
  aukščiausio lygio deklaracijos **NEsukuria savybės ant global objekto (`window`)** —
  tik `var` ir `function` deklaracijos sukuria. Todėl **`window.sensorCache === undefined`**.
  (Pagrindinis `<script>` eil. 683 yra klasikinis — be `type="module"`.)

**Dvi pasekmės, abi mirtinos Renogy kortelei:**
1. Atėjus Renogy paketui, `window.sensorCache['ren_v'] = …` meta
   **`TypeError: Cannot set properties of undefined`** → `parseFrame` sugriūva kiekvieną
   kartą, `dev.lastSync` nenustatomas, kortelė nepasiatnaujina.
2. Net jei nemestų — UI skaito per `getSensorValue(id)` (eil. 2133) → grąžina **lokalų**
   `sensorCache[id]`. Renogy reikšmės ten niekada nepatenka.

> Palyginimui, kodėl BLE plugino jungtis vis tiek veikia: `ble-plugin.js:1` naudoja
> `var capacitorCommunityBluetoothLe` (`var` → `window` ✅), o `sysLog`/`updateUI`/
> `isDebugEnabled` — `function` deklaracijos (irgi `window` ✅). Tik `sensorCache` yra
> `let` — vienintelis nutrūkęs saitas, ir būtent jo reikia Renogy duomenims.

### Taisymas (1 eilutė, `index.html`, iškart po 723 eil.)

```js
let sensorCache = (function(){ try { return JSON.parse(localStorage.getItem('_sensorCache') || '{}'); } catch(e){ return {}; } })();
window.sensorCache = sensorCache;   // P1: renogy.js rašo į window.sensorCache — turi rodyti į tą patį objektą, kurį skaito UI
```

`sensorCache` niekur neperpriskiriamas (tik pildomas raktais), tad vienos aliaso eilutės
pakanka: `window.sensorCache` ir lokalus `sensorCache` bus tas pats objektas, ir Renogy
`ren_*`/`dcc_*` reikšmės pateks ten, kur skaito `getSensorValue`, kortelės ir CSV logeris.

> **Kodėl aliasas, o ne bare `sensorCache` renogy.js'e:** teoriškai `renogy.js` galėtų
> naudoti bare `sensorCache` (klasikinių skriptų global lexical env dalinasi tarp failų),
> bet tai priklausytų nuo užkrovimo tvarkos ir būtų miglota. Aliasas `index.html`'e —
> vienareikšmis ir vietinis. Rink aliasą.

---

## 🟠 P2 — Renogy parserio latentinės klaidos (meta-audito B1, B2 — patikrintos, dar atviros)

### P2.1 — `onData` apdoroja tik 1 kadrą per notifikaciją (B1, `renogy.js` eil. 230–255)
Po `dev.buffer = dev.buffer.slice(expectedLen)` (eil. 239) likutis paliekamas, bet nėra
ciklo pakartotinai apdoroti bufery likusius pilnus kadrus. Rizika maža (kiekviena užklausa
resetina buferį), bet 2 sulipę kadrai vienoje notifikacijoje → antras užstrigtų.
**Taisymas — `while` ciklas:**
```js
function onData(type, data) {
    const dev = STATE.devices[type];
    dev.buffer.push(...data);
    while (dev.buffer.length >= 5 && dev.buffer.length >= dev.buffer[2] + 5) {
        const expectedLen = dev.buffer[2] + 5;
        const frame = dev.buffer.slice(0, expectedLen);
        dev.buffer = dev.buffer.slice(expectedLen);
        // ... esamas CRC tikrinimas + parseFrame + lastSync + updateUI (nekeisti logikos) ...
    }
}
```
`while` sąlyga `length >= 5` apsaugo nuo `dev.buffer[2]` skaitymo per anksti.

### P2.2 — `u32()` ženklo bitas (B2, `renogy.js` eil. **58**)
```js
// buvo:
function u32(b1, b2, b3, b4) { return (b1 << 24) | (b2 << 16) | (b3 << 8) | b4; }
// turi būti:
function u32(b1, b2, b3, b4) { return ((b1 << 24) | (b2 << 16) | (b3 << 8) | b4) >>> 0; }
```
110 Ah baterijai realiai nepaveikia SOC (110000 « 2³¹), bet apsaugo nuo neigiamų verčių,
jei kada parsinsi „Total Ah" gyvavimo skaitiklį.

---

## 🟠 P3 — Renogy tyli be pėdsakų: rodyti priežastį loge ir UI

`pollCycle` (`renogy.js` eil. 134–174) **tyliai** išeina (be įrašo), kai:
- `!STATE.enabled` (eil. 135) — modulis OFF;
- `!BleClient || !isEnabled()` (eil. 137) — BT išjungtas;
- `STATE.devices.battery.id === null` / `dcc.id === null` (eil. 142/163) — įrenginys nepriskirtas.

Būtent todėl įkeltame loge **nėra nė vienos `[Renogy]` eilutės**, nors debug įjungtas —
atrodo „tab'as miręs be pėdsakų", ir neįmanoma diagnozuoti nuotoliniu būdu.

**Taisymas (du dalykai):**
1. Į tuos tylius return'us pridėti `debug(...)` įrašus (pvz. `'pollCycle skip: modulis OFF'`,
   `'pollCycle skip: BT isjungtas'`, `'pollCycle skip: nepriskirtas irenginys'`).
   **Loginti tik kai būsena pasikeičia**, ne kas 5 s (kad neužplūstų) — laikyti paskutinę
   priežastį kintamajame ir loginti tik pasikeitus.
2. `connectAndNotify` klaidas (`connect failed`, eil. 191–195) ir „miega/nerandama"
   būseną atspindėti UI statuso eilutėse `#ren-bat-sync` / `#ren-dcc-sync`
   (jos jau egzistuoja, `index.html` eil. ~2078–2089). DCC50S be krovimo šaltinio miega —
   tai NE klaida (žr. `uzduotis_renogy_ble_tab.md` pasala #6); po N nesėkmių rodyti
   „Nerandama (miega?)", ne tylą ir ne raudoną klaidą.

---

## 🟡 P4 — Smulkios app pataisos (patikrinta, kad dar atviros)

### P4.1 — `junc_soc` be apatinės ribos + retkarčiais matomas SOC=101 (B4 + audito A2)
Fallback skaičiavimas (`index.html` eil. **1570**) neturi apatinės ribos:
```js
// buvo:  sensorCache['junc_soc'] = Math.min(100, Math.round((ah / cap) * 100));
// turi būti:
sensorCache['junc_soc'] = Math.max(0, Math.min(100, Math.round((ah / cap) * 100)));
```
Papildomai — kad padengtume audito A2 (kartais CSV rodė SOC=101, nors firmware riboja
iki 100; tikslus šaltinis nepatvirtintas — gali būti `json.soc` iš debesies, eil. 2296,
arba užapvalinimas), **apkarpyti rodymą** ties `setSensorValue('junc_soc', …)` (eil. 1915–1917)
formatteryje: `v => Math.max(0, Math.min(100, Math.round(v)))`. Kosmetika, bet SOC niekada
neberodys <0 ar >100. (Šaknies A2 priežasties negaudyti — jei po apkarpymo 101 dingsta, gana.)

### P4.2 (žema, sąlyginė) — `modemo_busena` / `paskutine_klaida` lieka `[SSE UNMATCHED]`
Logas rodo šiuos du kaip UNMATCHED. Patikrinta: UI **nėra** kortelių šiems tekstams
(grep `index.html` — nė vieno elemento). Pagal `gyvo_testo_analize` D2 principą
(„jei kortelių nėra — nedaryti") — **numatytai palikti**. Jei nori matyti modemo būseną /
paskutinę klaidą Sistemos/diagnostikos kortelėje — tada pridėti UI elementą + SSE mapinimą
(prie else-if grandinės ~eil. 2767, prieš `[SSE UNMATCHED]` else). Sprendžia vartotojas.

### P4.3 (žema, robustiškumas — NE saugumo P0) — vardo interpoliacija į `onclick`
`renderScanResults` (`renogy.js` eil. 386–387) BLE įrenginio vardą deda tiesiai į
`onclick="RenogyBLE.assign('…','…','…')"` string. Vardas su `'` ar `"` sugadina mygtuką
(priskyrimas neveiks — funkcinė, ne saugumo problema). Švariau — `addEventListener` +
`dataset`, ne inline string. Neprivaloma šiam ratui; jei imsi — patikrink, kad
„Baterija"/„DCC50S" mygtukai vis dar veikia.

> **PASTABA — kas NĖRA defektas (patikrinta, kad juodrašty buvo klaidingai):**
> Loge matomos `doCheckUpdate` → `Unable to resolve host raw.githubusercontent.com`
> eilutės NĖRA klaida. `doCheckUpdate` (eil. 2889) kviečiamas TIK rankiniu mygtuku
> (`index.html:572 onclick`), auto-patikra (eil. 2680) yra vienkartinis `setTimeout`.
> Tai vartotojas spaudė „Tikrinti dabar" būdamas prisijungęs prie ESP AP be interneto —
> GitHub DNS natūraliai neišsisprendžia. Jokio taisymo nereikia.

---

## 🚫 NEDARYTI

- **Saugumo (P0) NELIESTI** — SMS siuntėjo autorizacija, `ota.password`,
  `api.encryption.key`, web_server auth, Apps Script token — visa tai **Etapui A**.
- **Firmware (`kempervanas.yaml`) NELIESTI.** (Loge matomi `junc_hex` INFO flood ir
  `D9 nerasta` — Junctek šunto firmware pusės, atskiro sprendimo klausimai; NE šioje
  užduotyje.)
- Nekeisti Renogy užklausų (`0x1388/0x22`, `0x13B2/0x06`, `0x0100/0x21`) ir jau
  patikrintų DCC/baterijos offsetų/mastelių — teisingi (v33.3 + meta-audito Node imitacija).
- Nekeisti `androidNeverForLocation: true` (`renogy.js` eil. 68) — patikrinta online:
  atitinka plugino API ir manifest'ą (jau ištaisyta v42).
- Nekeisti lokalaus CSV 24-laukų header'io (jau ištaisyta v42, eil. 783) ir SSE branduolio.

---

## Versijos ir build

`version.json "version": 43` == `versionCode 43` == APK `android/kemperis_v43.apk` ==
`apk_url`; `versionName "33.7"`; `notes` — trumpas changelog (Renogy kortelės duomenų
saitas → veikia; parserio multi-frame + u32; Renogy diagnostikos matomumas; SOC riba).
`npx cap sync android` nereikia (native nesikeičia) — užtenka `www` kopijos į
`android/android/app/src/main/assets/public/` + Gradle release build.

---

## Priėmimo kriterijai

1. **P1:** priskyrus Renogy bateriją/DCC50S ir prisijungus, kortelė rodo realias reikšmes
   (V/A/SOC/temp, DCC W). WebView konsolėje **nėra** `Cannot set properties of undefined`.
   Patikra: `window.sensorCache.__t = 1; getSensorValue('__t') === 1` grąžina `true`
   (t.y. tas pats objektas).
2. **P2:** du sulipę kadrai viename buferyje apdorojami abu (Node imitacija kaip meta-audite);
   `u32()` su aukštu bitu grąžina teigiamą.
3. **P3:** be priskirto įrenginio arba išjungus BT — debug loge aiški `pollCycle` priežastis
   (be flood'o); `#ren-bat-sync`/`#ren-dcc-sync` rodo prasmingą būseną, ne tylą.
4. **P4.1:** SOC niekada nerodo <0 ar >100 nei UI, nei CSV.
5. Kiti tab'ai be regresijų (ypač SSE gyvi duomenys ir CSV/GPX eksportai).
6. Versijų grandinė sinchronizuota; `git diff --stat` be nukirstų failų; commit + push.
7. Trumpas įrašas `docs/audits/` (kas pataisyta; ar Renogy kortelė gyvai parodė duomenis).

---

## Verifikacija (kaip ši užduotis patikrinta)

**Prieš kodą (grep + skaitymas):**
- `let sensorCache` eil. 723; nė vieno `window.sensorCache` `index.html`'e; 17 rašymų į
  `window.sensorCache` `renogy.js`'e (266–306) — P1 patvirtinta.
- `u32` eil. 58; `onData` vienas kadras eil. 236–239; `pollCycle` tylūs return'ai 135/137/142;
  `junc_soc` fallback 1570; `renderScanResults` onclick 386–387 — visi patvirtinti.
- Išbraukta kaip jau ištaisyta v42: CSV 24-laukų header (783), `androidNeverForLocation` (68),
  `[SSE BRANCH]` diagnostika (0 radinių), `loadAllNumbers` offline guard (2164/2176).
- Išbraukta kaip NE defektas: `doCheckUpdate` „DNS spam" (rankinis mygtukas, ne auto-loop).

**Online (faktų patikra):**
- [MDN — `let`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Statements/let):
  aukščiausio lygio `let`/`const` klasikiniame skripte nesukuria savybės ant global objekto
  (`window`); tik `var`/`function` sukuria → `window.sensorCache` yra `undefined`. Pagrindžia P1.
- [capacitor-community/bluetooth-le README](https://github.com/capacitor-community/bluetooth-le/blob/main/README.md):
  `BleClient.initialize({ androidNeverForLocation: true })` ir `startNotifications(deviceId,
  service, characteristic, callback)` atitinka esamą `renogy.js` naudojimą — nekeisti.

---
*Užduotį paruošė ir perverifikavo: Claude (Cowork) · 2026-07-03 · pagrindas: gyvo logo analizė,
kartotinis kodo auditas, v42 auditai, MDN + plugino dokumentacija*
