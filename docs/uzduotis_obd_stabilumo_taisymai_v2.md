# Užduotis: KRITINIŲ bug'ų taisymas po v48 audito

**Kontekstas:** `docs/uzduotis_obd_stabilumo_taisymai.md` (Taisymai 1-8) buvo implementuoti
commit'e `f14089f` (v35.1/v48). Auditas prieš tikrą kodą (ne tik commit pavadinimus) rado
**2 kritinius bug'us**, kurie daro flagship funkciją neveikiančią ir gali užšaldyti ryšį.
Taip pat vienas dokumentacijos nuoseklumo taisymas.

---

## Bug #1 (KRITIŠKAS): `runFullCollection()` savęs blokuoja per `scanBusy`

**Failas:** `android/www/obd.js`

**Problema:** `runFullCollection()` nustato `STATE.scanBusy = true`, tada kviečia
`await runIdentity()`, `await runProtocolProbe()`, `await runDidScan()`,
`await runModuleScan()`. Kiekviena iš šių funkcijų PATI pradžioje tikrina
`if (STATE.scanBusy) return;` — kadangi vėliava JAU `true` (tėvinės funkcijos nustatyta),
**visos keturios funkcijos iškart grįžta nieko neveikdamos**. „Surinkti VISKĄ" mygtukas
realiai nieko nesurenka — tik parodo pradžios/pabaigos pranešimus ir išsaugo tuščią žurnalą.

**Sprendimas:** taikyti TĄ PATĮ pattern'ą, kuris jau teisingai panaudotas
`runBlockSweep()`/`runBlockSweepInternal()` poroje — atskirti kiekvieną funkciją į
„public" (su `scanBusy` apsauga) ir „Internal" (be apsaugos, tikrasis darbas) versijas.
`runFullCollection()` turi kviesti INTERNAL versijas (nes JI PATI jau laiko `scanBusy` vėliavą).

Konkretus refaktoringas — kiekvienai iš 4 funkcijų:

```js
async function runIdentity() {
    if (STATE.scanBusy) return;
    STATE.scanBusy = true;
    renderAssigned();
    try {
        await runIdentityInternal();
    } finally {
        STATE.scanBusy = false;
        renderAssigned();
    }
}

async function runIdentityInternal() {
    obdLog('=== ECU IDENTITETAS (Mode 09) ===', 'info');
    const vin = await sendCmd('0902', 3000);
    logDecoded('VIN', vin, 3);
    const calid = await sendCmd('0904', 3000);
    logDecoded('Calibration ID', calid, 3);
    const ecuname = await sendCmd('090A', 3000);
    logDecoded('ECU Name', ecuname, 3);
}
```

Analogiškai `runProtocolProbeInternal()`, `runDidScanInternal()` (dėmesio: viduje jau
kviečia `runBlockSweepInternal()`, tai palikti nepakitę), `runModuleScanInternal()`
(dėmesio: `runModuleScan()` public versija turi TOLIAU turėti `window.confirm(...)` patikrą
PRIEŠ nustatant `scanBusy`, kad patvirtinimo dialogas rodytųsi net kai kviečiama per
`runFullCollection`).

Tada `runFullCollection()` pakeisti į:
```js
async function runFullCollection() {
    if (STATE.scanBusy || !STATE.connected) { obdLog('Nėra ryšio arba skenavimas jau vyksta.', 'error'); return; }
    if (!window.confirm('Ar automobilis STOVI? Vyks pilnas skenavimas su modulių adresavimu.')) return;
    STATE.scanBusy = true;
    renderAssigned();
    try {
        obdLog('########## PRADEDAMAS PILNAS DUOMENŲ RINKIMAS ##########', 'success');
        stopPolling();
        await runIdentityInternal();
        await runProtocolProbeInternal();
        await runDidScanInternal();
        await runModuleScanInternal(); // BE savo window.confirm(), nes jau patvirtinta aukščiau
        await sendCmd('1001', 1000);
        obdLog('########## RINKIMAS BAIGTAS - žurnalas išsaugomas automatiškai ##########', 'success');
        saveObdLog();
    } finally {
        STATE.scanBusy = false;
        renderAssigned();
        startPolling();
    }
}
```
(Perkėlus `window.confirm()` į `runFullCollection()` pradžią — IR palikus jį `runModuleScan()`
public versijoje atskiram paleidimui — bet `runModuleScanInternal()` PATI confirm'o neturi,
kad `runFullCollection()` nerodytų dviejų patvirtinimo dialogų iš eilės.)

**Priėmimo kriterijus:** paspaudus „Surinkti VISKĄ", žurnale realiai matosi ECU identiteto,
protokolo zondo, DID skenavimo IR modulių skenavimo rezultatai (ne tuščias/beveik tuščias
žurnalas), o kitiems 5 mygtukams paspaudus atskirai — jie irgi toliau veikia kaip anksčiau.

---

## Bug #2 (KRITIŠKAS): Auto-ATZ atsigavimas užšaldo ryšį (deadlock)

**Failas:** `android/www/obd.js`, `handleAutoRecovery()` ir timeout callback'as `pumpQueue()` viduje

**Problema:** kai `consecutiveTimeouts >= 3`, nustatoma `STATE.flushing = true` PRIEŠ
kviečiant `await handleAutoRecovery()`. `handleAutoRecovery()` savo viduje kviečia
`await initSequence(true)`, kuri naudoja `sendCmd()`/`pumpQueue()` mechanizmą. Bet
`pumpQueue()` PIRMOJI eilutė yra `if (STATE.pending || STATE.queue.length === 0 || STATE.flushing) return;`
— kadangi `STATE.flushing` **lieka `true`** per VISĄ `handleAutoRecovery()` vykdymą,
`pumpQueue()` niekada neapdoros atsigavimo sekos komandų. Jų `Promise` niekada neišsisprendžia,
`initSequence(true)` amžinai kabo ties pirmu `await sendCmd(...)`, `handleAutoRecovery()`
niekada negrįžta, `flushing` niekad neatstatomas — **ryšys visam laikui „pakimba" būtent
tame scenarijuje, kuriam ši apsauga buvo kurta** (pasikartojantys timeout'ai prieš
atsijungimą — realiai stebėta žurnaluose).

**Sprendimas:** `handleAutoRecovery()` viduje reikia atstatyti `STATE.flushing = false`
PRIEŠ kviečiant `initSequence(true)`, kad eilė galėtų normaliai apdoroti atsigavimo
sekos komandas:

```js
async function handleAutoRecovery() {
    obdLog('⚠️ Pasikartojantys timeoutai — bandoma perkrauti adapterį (ATZ)...', 'error');
    STATE.autoResetCount++;
    const bridge = getBridge();
    if (bridge) bridge.send('ATZ');
    await new Promise(r => setTimeout(r, 1500));
    STATE.flushing = false; // <-- BŪTINA: leisti eilei apdoroti initSequence komandas žemiau
    await initSequence(true);
    STATE.consecutiveTimeouts = 0;
}
```

Ir timeout callback'e (`pumpQueue()` viduje) po `await handleAutoRecovery();` esantis
`STATE.flushing = false;` gali likti (bus jau `false`, nekenkia — tiesiog perteklinis).

**Priėmimo kriterijus:** dirbtinai sukėlus 3+ timeout'us iš eilės (pvz. atjungus adapterį
skenavimo metu ir vėl prijungus), programa turi automatiškai atsigauti ir tęsti darbą,
NE amžinai kaboti/reikalauti perkrauti app'ą.

---

## Taisymas (ne kritiškas): dokumentacijos nuoseklumas

Commit `ebab596` perrašė `docs/uzduotis_obd2_elm327_tab.md` §1a/§3 su VW TP 2.0 handshake
aprašymu ir pašalino aprašytą auto-poravimo (`§1a`) bei skenavimo (`startDiscovery`/`pair`)
funkcionalumą — **nors tikras Java kodas (`KemperisObdBridge`) šių metodų NEPRARADO**
(patikrinta: `MainActivity.java` diff'e f14089f commit'e pakito tik `CURRENT_VERSION`).
Tai reiškia dokumentacija dabar **neatitinka realaus kodo**.

**VW TP 2.0 teiginys jau paneigtas** (žr. `docs/uzduotis_obd_stabilumo_taisymai.md` pradžią) —
turime realių Service 21 duomenų iš `0x7E0` BE jokio TP 2.0 handshake. Prašau:
1. `docs/uzduotis_obd2_elm327_tab.md` §1a ir §3 **grąžinti** prie versijos PRIEŠ `ebab596`
   commit'ą (naudoti `git show bdc353d:docs/uzduotis_obd2_elm327_tab.md` kaip atskaitos tašką
   toms sekcijoms) — t.y. su auto-poravimu ir BE TP 2.0 handshake.
2. Pakopa A lentelė (§3) — `ebab596` pakeitė patikrintus/žinomus PID (`2C`/`2D`/`7A`/`7C`) į
   neverifikuotus grupės numerius, pateiktus kaip faktą („Group 068 = Soot grams"). Grąžinti
   originalią standartinių SAE PID lentelę; neverifikuoti Service 21 grupių numeriai (0x44,
   0x43, 0x0D ir pan., pateikti kaip „faktas") **neturi būti dokumentuojami kaip patvirtinti**
   — jei norima juos paminėti, žymėti aiškiai kaip neverifikuotus kandidatus, atskirai nuo
   REALIAI patvirtintų grupių 0x50 (SW ID), 0x51 (VIN), 0x52 (kito ID string'o), kurios jau
   užfiksuotos `docs/uzduotis_obd_stabilumo_taisymai.md` Taisymo 5 lentelėje.

**Nekeisti** — Java kodo (`KemperisObdBridge`), nes ten viskas jau teisingai veikia.

---

## Bendri reikalavimai

- Po pakeitimų: `node --check android/www/obd.js`.
- Rankiniu būdu peržiūrėti (arba paprašyti manęs peržiūrėti) galutinį `runFullCollection()`,
  kad įsitikintume, jog nebeliko jokio kito atvejo, kur guard'as blokuoja savo paties vidinį
  kvietimą.
- Versijos bump: v48 → **v49**, su changelog paminint šiuos 2 kritinius taisymus.
- **NEDARYTI** `git commit`/`push`/APK build be aiškaus vartotojo prašymo.
