# v32.2 — užduotis asistentui: SSE gyvų reikšmių saugojimo pataisymas (regex + teisinga šakų tvarka)

## Problema
`startSSE` `'state'` handler'yje id normalizuojamas `.replace(/[^a-z0-9]+/g,'_')`, bet domenų regex'ai reikalauja `/` arba `-` (`[\/\-]`). Po normalizacijos jie nesutampa → **jutiklių saugojimo šaka niekada neįvyksta** (patvirtinta 3× audite). Todėl gyvi jutikliai realiai ateina tik iš cloud.

## ⚠️ Kritinė detalė (NEpraleisti)
Vien pridėti `_` NEUŽTENKA. „text_sensor" prasideda „text", tad su `/^text[\/\-_]/` **text branšas pagautų ir `text_sensor_*`** (oro kokybė, GPS koordinatės, GSM, uptime, patarimai, 220V, apsauga, SMS) ir juos sulaužytų. **Būtina PAKEISTI ŠAKŲ TVARKĄ:** `(text_sensor|sensor)` tikrinti PRIEŠ `text`.

Teisinga tvarka: **number → switch → (text_sensor|sensor) → text**.
(Node patvirtino: taip text_sensor→sensor šaka, sensor→sensor, `text_admin_number`→text.)

## Keisti — `www/index.html`, `startSSE` domenų blokas (~2500 eil.)

### Dabartinė struktūra (blogai — text prieš sensor, be `_`)
```javascript
if (rawId.match(/^number[\/\-]/)) {
    sensorCache[keyId.substring(7)] = value;
} else if (rawId.match(/^text[\/\-]/)) {          // <-- pagaus text_sensor!
    sensorCache[keyId.substring(5)] = value;
} else if (rawId.match(/^switch[\/\-]/)) {
    const _swKey = keyId.substring(7);
    sensorCache[_swKey] = value;
    if (data.name_id) _entityRestNames['sw:' + _swKey] = data.name_id.replace(/^switch\//i, '');
} else if (rawId.match(/^(text_sensor|sensor)[\/\-]/)) {
    const stripped = keyId.replace(/^(text_sensor|sensor)[\/\-]/, '');
    if (SENSOR_ID_MAP[stripped] !== undefined) { sensorCache[SENSOR_ID_MAP[stripped]] = value; }
    else if (rawId.includes('junc_soc') || ...) ...   // ilga includes grandinė (NEKEISTI)
    ...
    else { debugLog('[SSE UNMATCHED] ' + rawId + '=' + value, 'warn'); }
}
```

### Nauja struktūra (teisinga — `_` pridėta IR tvarka pakeista)
```javascript
if (rawId.match(/^number[\/\-_]/)) {
    sensorCache[keyId.substring(7)] = value;
} else if (rawId.match(/^switch[\/\-_]/)) {
    const _swKey = keyId.substring(7);
    sensorCache[_swKey] = value;
    if (data.name_id) _entityRestNames['sw:' + _swKey] = data.name_id.replace(/^switch\//i, '');
} else if (rawId.match(/^(text_sensor|sensor)[\/\-_]/)) {
    debugLog('[SSE BRANCH] ' + rawId, 'success');   // laikina diagnostika (žr. žemiau)
    const stripped = keyId.replace(/^(text_sensor|sensor)[\/\-_]/, '');
    if (SENSOR_ID_MAP[stripped] !== undefined) { sensorCache[SENSOR_ID_MAP[stripped]] = value; }
    else if (rawId.includes('junc_soc') || ...) ...   // ilga includes grandinė — PALIKTI KAIP YRA
    ...
    else { debugLog('[SSE UNMATCHED] ' + rawId + '=' + value, 'warn'); }
} else if (rawId.match(/^text[\/\-_]/)) {           // <-- PERKELTA Į PABAIGĄ, po sensor
    sensorCache[keyId.substring(5)] = value;
}
```

**Konkretūs veiksmai:**
1. Visuose 4 regex'uose `[\/\-]` → `[\/\-_]` (number, switch, text_sensor|sensor, text).
2. **Perkelti** `text` branšą (`sensorCache[keyId.substring(5)] = value;`) iš 2-os pozicijos į **paskutinę** (po viso `(text_sensor|sensor)` bloko).
3. `stripped` regex: `keyId.replace(/^(text_sensor|sensor)[\/\-_]/, '')`.
4. Vidinę `includes()` grandinę ir `SENSOR_ID_MAP` **NEKEISTI** (tik įjungiam ją).

## Diagnostika (laikina, verifikacijai)
`(text_sensor|sensor)` branšo pradžioje pridėti `debugLog('[SSE BRANCH] ' + rawId, 'success');`.
Su įjungtu ESP ir debug: turi pasirodyti daug `[SSE BRANCH] sensor_...` eilučių. Jei matai `[SSE UNMATCHED]` — užsirašyk tuos id (jie liks tušti; spręsim atskirai). **Kai patvirtinta, kad veikia — šią diagnostikos eilutę pašalinti.**

## Verifikacija (BŪTINA su gyvu ESP)
1. Prisijungti prie ESP (idealu — „Tik Kemperis" režimu, kad be cloud, grynas SSE testas).
2. Patikrinti, kad užsipildo **anksčiau tušti** SSE-tik kortelės: oro kokybė, GPS greitis/aukštis/kryptis/palydovai, ESP temp, RAM, GSM operatorius/signalas, pokrypis (roll/pitch), vibracija, 220V, apsauga, orų prognozė, akum. įtampa/srovė/galia.
3. Patikrinti, kad **tekstiniai** rodo teisingai (oro kokybė „Puiki/…", GPS koordinatės, GSM tinklas, uptime, patarimai) — tai įrodo, kad text_sensor NEnukeliavo į blogą šaką.
4. Loge peržiūrėti `[SSE UNMATCHED]` — jei yra, tie jutikliai reikalauja mapinimo (atskira SSE migracijos užduotis).
5. Kalibravus pokrypį — ekranas turi atsinaujinti į ~0 (nes dabar gyvos reikšmės atkeliauja).

## NEKEISTI
- `SENSOR_ID_MAP` turinys, `includes()` grandinė (pilna SSE mapinimo migracija — atidėta; šioje užduotyje tik įjungiam esamą grandinę).
- Native Java, dual-network.

## Užbaigimas
1. Versijos: `CURRENT_VERSION` = `versionCode` = `version.json version` (visi vienodi, pvz. 34); `versionName` „32.2".
2. `cd android/android && ./gradlew assembleRelease`
3. APK → `android/kemperis_v34.apk` (atnaujinti `version.json` apk_url), `git add -f`, commit + push.

> Po verifikacijos (2 punktas) — jei nauji jutikliai užsipildė, tai pats svarbiausias projekto stabilumo pataisymas. Nepamiršti pašalinti laikinos `[SSE BRANCH]` diagnostikos ir perbuild'inti.
