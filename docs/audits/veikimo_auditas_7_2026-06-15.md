# KemperisApp — veikimo auditas (7-as pjūvis) + 2 klaidų taisymas

**Data:** 2026-06-15
**HEAD prieš taisymą:** `fe076ae` „FINAL: Fixed SOC 0% bug…"
**Pagrindas:** vartotojo terminalo logas (`terminal_log_2026-06-15T18-59-26.txt`) + ekrano nuotrauka + kodo analizė.

---

## Diagnozė (iš logo ir kodo)

### 🔴 Klaida 1 — SOC rodo „0%", nors juosta pilna
Logas (SMS statusas) rodo **SOC:98%**, bet kortelėje — `0%` su pilna žalia juosta.

**Priežastis:** `updateUI()` atnaujino tik SOC **juostos plotį** (`soc-fill`, iš `junc_soc`), bet **niekada nerašė skaičiaus** į `#soc_value` / `#soc_d` — nebuvo `setSensorValue('junc_soc', …)`. Todėl juosta judėjo (≈98%), o tekstas likdavo pradinis HTML „0". Commit'as `fe076ae` apskaičiavo SOC reikšmę, bet pamiršo ją parodyti tekste.

**Taisymas (`www/index.html`, ~1645 eil.):**
```js
setSensorValue('junc_soc', 'soc_value', v=>Math.round(v));
setSensorValue('junc_soc', 'soc_d',     v=>Math.round(v));
```

### 🔴 Klaida 2 — arm/disarm visada blokuojamas (offline)
Logas: `⛔ +disarm blokuota: admin numeris nežinomas (prisijunkite prie ESP32 bent vieną kartą)`. (`+status`, `+lokacija` veikė — jiems patikros nėra.)

**Priežastis (catch-22):** arm/disarm sauga skaitė `sensorCache['txt_admin_number']`, kuris užpildomas **tik prisijungus prie ESP32**. Bet offline SMS reikalingas būtent tada, kai ESP32 **nepasiekiamas** → laukas tuščias → visada blokuojama. Numatytasis admin nr. buvo įrašomas tik į įvesties lauką, ne į cache/localStorage.

**Taisymas (`www/index.html`):**
- Pridėtas `saveAdminNumber()` — admin nr. saugomas ir `localStorage['admin_number']`, ir `sensorCache` (ne tik siunčiamas į ESP).
- Sauga dabar turi atsarginį šaltinį: `sensorCache['txt_admin_number'] || localStorage.getItem('admin_number')`.
- `init()` užsėja `admin_number` iš numatytojo (`+37064730025`), kad veiktų iš karto.
- Admin laukelio `onchange` dabar kviečia `saveAdminNumber(this.value)` (išlieka po perkrovimo, veikia offline).

> Pastaba: jei programėlę leidi telefone, kurio SIM ≠ admin nr., gali įsijungti antroji patikra („šis tel. ≠ admin"). Jei `getOwnPhoneNumber()` grąžina tuščią — leidžiama su įspėjimu.

---

## Patikra
- JS sintaksė po taisymų — **švari** (`node --check`).
- Abu taisymai lokalizuoti `www/index.html`; native sluoksnis ir build nepaliesti.

## Kas dar nepakitę (iš ankstesnių pjūvių)
- Git istorija neperrašyta, pasirašymo raktas/Script ID nerotuoti (saugumas — atskirose ataskaitose).
- Veikimo niuansai: tinklo bind/unbind lenktynės, žemėlapis tik su internetu, `localStorage` rašymas kas SSE žinutę.

---

## ⚠️ BŪTINI žingsniai, kad pataisymai pasiektų telefoną
Pakeistas tik `www/index.html` — įdiegta APK to dar neturi. Reikia:
1. **Perkompiliuoti APK** (`build_and_sync.bat` arba `npx cap sync android` + `gradlew assembleRelease`).
2. **Įdiegti iš naujo** (jei parašas tas pats — atnaujins; jei ne — pašalink ir įdiek).
3. **Paleisti `grant_sms.bat`** dar kartą (po perdiegimo SMS leidimas išsivalo).
4. Atidaryti programėlę → patikrinti: SOC rodo realų % ir „🔓 Disarm" nebeblokuojamas.
