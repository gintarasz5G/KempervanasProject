# Pilnas programėlės auditas — 2026-07-01 (v33 / v32.1)

**Apimtis:** Android app (`www/index.html` ~3374 eil., native Java ~724 eil.), firmware SSE sąsaja, Google Apps Script, tinklo logika, veiksmų sekos, scenarijų imitacija.
**Metodai:** kodo analizė, `node` logikos imitacija, ESPHome/Android dokumentacija online.
**Būsena:** po v31–v33 (MediaStore, TTS, pinch-zoom, SMS slėpimas, atnaujinimų tiltas, dual-network v32, v32.1 maintenance).

---

## Santrauka pagal svarbą

| # | Radinys | Svarba | Statusas |
|---|---------|--------|----------|
| 1 | **SSE gyvų reikšmių saugojimas neveikia** (domenų regex `[\/\-]` vs normalizuotas `_`) | 🔴 Kritinė | Reikia gyvo patvirtinimo |
| 2 | De-facto duomenų šaltinis daugeliui jutiklių — **Google cloud**, ne SSE | 🔴 Kritinė | Išvestinė iš #1 |
| 3 | Aliarmai tikrinami tik `!_isOffline` + ant galimai tuščio/numatyto cache | 🟠 Aukšta | Susiję su #1 |
| 4 | Galimai dubliuotas `_sseSource.onopen` (2459 ir 2590) | 🟡 Vidutinė | Patikrinti |
| 5 | Įkomponuoti „secret" default'ai (Script ID, tel. numeriai) shipuotame APK | 🟡 Vidutinė | Priimtina, bet žinoti |
| 6 | Apps Script be autentifikacijos / be 30 d. valymo | 🟡 Vidutinė | Neliesti (vartotojo sprendimas) |
| 7 | Dual-network (v32) — teisingas | 🟢 Gerai | Įgyvendinta |
| 8 | Versijų sinchronizacija (v32.1) — sutvarkyta | 🟢 Gerai | Įgyvendinta |

---

## 1–2. KRITINIS: SSE gyvų reikšmių saugojimas ir tikrasis duomenų šaltinis

### Radinys
`startSSE` `'state'` apdorojiklyje id normalizuojamas:
```javascript
let rawId = (data.name_id || data.id).toLowerCase().replace(/[^a-z0-9]+/g, '_');
```
o domenas atpažįstamas regex'ais, kurie reikalauja `/` arba `-`:
```javascript
if (rawId.match(/^number[\/\-]/)) { ... }
else if (rawId.match(/^(text_sensor|sensor)[\/\-]/)) { ... }  // čia gyvena visas mapinimas
```

**Problema:** normalizacija `-` ir `/` paverčia į `_`, o regex'ai `_` **nepriima**. Tad po normalizacijos `sensor-gps_greitis` → `sensor_gps_greitis`, ir regex **nesutampa** → visa jutiklių saugojimo šaka **niekada neįvyksta**.

**Patvirtinta 3 būdais:**
- Baitų analizė (`od`): klasė tikrai `[\/\-]` (tik `/` ir `-`).
- ESPHome dok.: web_server v3 SSE siunčia `id="sensor-<object_id>"` (brūkšnys) ir `name_id="sensor/<name>"` (pasvirasis) — abu normalizuojasi į `_`.
- `node` imitacija su tikru handler'iu ir visais formatais → `branch = NONE`, `stored = null`.

### Pasekmė (išvestinė)
Jutikliai, kurie **saugomi tik per SSE grandinę**, cache niekada nepatenka. Realiai duomenys ateina tik iš:
- **Google cloud** (`_fetchSheetsNow`): SOC/Ah, vanduo%, dujos%, TDS, BME temp/drėgmė/slėgis, CO2, miegamojo temp/drėgmė, GPS lat/lon, uptime.
- **REST** `loadAllNumbers`/`loadAllTexts`: tik `number`/`text` **įvesties** entitetai (kalibravimo skaičiai, admin nr., APN, Script ID) + `refreshWaterLevel` (vanduo).

**Cloud NEapima** (tad be interneto — tušti): oro kokybė, GPS greitis/aukštis/kryptis/palydovai, ESP temperatūra, laisva RAM, GSM operatorius/signalas, orų prognozė, 220V, apsauga/judėjimas, pokrypis (roll/pitch), vibracija, akum. įtampa/srovė/galia/temp.

**Tai tiksliai atitinka pastebėtą elgesį:** planšetėje **be interneto** veikė būtent cloud-jutikliai (akum, vanduo, dujos, temp, CO2), o „---" rodė SSE-tik jutikliai (oro kokybė, GPS detalės, ESP/RAM, GSM, pokrypis). Telefone su LTE veikė daugiau, nes cloud pildė daugiau kortelių.

> **Prieštaravimas, kurį BŪTINA patikrinti gyvai:** telefone matėsi ir kai kurie SSE-tik jutikliai (oro kokybė „Puiki", pokrypis) — o pagal statinę analizę jie neturėtų būti užpildyti. Tad arba yra kelias, kurio statiškai nematyti, arba tai buvo trumpo momento/likutinės reikšmės. **Statinė analizė 3× rodo, kad SSE saugojimas neveikia — bet tai reikia patvirtinti realiame įrenginyje.**

### Privaloma verifikacija (pigi, 1 eilutė)
Į `'state'` handler'io jutiklių šaką (arba iškart po regex) pridėti skaitiklį/logą:
```javascript
else if (rawId.match(/^(text_sensor|sensor)[\/\-]/)) {
    debugLog('[SSE STORED] ' + rawId);   // jei loge NIEKADA nepasirodo — šaka mirusi
    ...
}
```
Su įjungtu ESP ir debug: jei `[SSE STORED]` **nepasirodo** — patvirtinta, kad SSE saugojimas neveikia (ir visi „gyvi" jutikliai realiai ateina iš cloud).

### Jei patvirtinta — sprendimas
Regex'us pataisyti, kad priimtų ir `_`:
```javascript
if (rawId.match(/^number[\/\-_]/)) { ... }
else if (rawId.match(/^(text_sensor|sensor)[\/\-_]/)) {
    const stripped = keyId.replace(/^(text_sensor|sensor)[\/\-_]/, '');
    ...
}
```
**⚠️ Atsargiai:** tai „įjungtų" visą iki šiol miegojusią mapinimo grandinę — kartu su jos trapumu ir `SENSOR_ID_MAP` neatitikimais (žr. atidėtą SSE object_id migraciją). Todėl daryti **kartu** su SSE mapinimo peržiūra ir gyvo ESP testu, kad nė vienas jutiklis „nedingtų".

---

## 3. Aliarmų logika

- `checkAlarms(cache)` iškart grįžta, jei `_isOffline` (komentaras „tik gyvi duomenys"). Tas pats 5 s cikle.
- `_isOffline=false` tampa tik per SSE `onopen`. Bet jei SSE saugojimas neveikia (#1), cache lieka be gyvų reikšmių → `getFromCache(c,'water_pct',100)` grąžina **numatytą 100** → aliarmas nesuveiktų realioje situacijoje.
- Cloud duomenys (`_fetchSheetsNow`) veikia esant `_isOffline=true` → tuo metu `checkAlarms` **praleidžiamas**. Todėl aliarmai su cloud duomenimis neveikia by-design (aptarta).
- **Vagystės balsas** (`checkTheftVoice`) — atskira grandinė; naudoja `alert_movement_sent`, kuris irgi SSE-tik → priklauso nuo #1.

**Išvada:** aliarmų teisingumas tiesiogiai priklauso nuo #1. Pirma išspręsti SSE saugojimą, tada validuoti aliarmus.

---

## 4. Galimai dubliuotas SSE `onopen`
Rasti du `_sseSource.onopen = ...` priskyrimai (~2459 ir ~2590). Antras perrašo pirmą. Patikrinti, ar tai sąmoninga (skirtingos funkcijos) ar dubliavimas — jei dubliavimas, palikti vieną.

---

## 5–6. Saugumas / debesis

**5. Įkomponuoti default'ai** (`init`): `_DEFAULT_SCRIPT_ID`, `_DEFAULT_CAMPER_PHONE (+3706…)`, `_DEFAULT_ADMIN_PHONE (+3706…)` — įrašyti shipuotame APK, nuskaitomi iš failo. Priimtina asmeniniam projektui, bet žinoti: bet kas su APK juos mato. Admin nr. naudojamas arm/disarm apsaugai.

**6. Apps Script** — `doGet`/`doPost` be token'o (bet kas su URL rašo/skaito); nėra 30 d. valymo (mėnesių lapai kaupiasi). Vartotojas prašė neliesti — tik pažymima.

---

## 7–8. Kas veikia gerai
- **Dual-network (v32):** procesas laikomas pririštas prie ESP WiFi (SSE), internetas per native `getSafeConnection` (LTE). Orkestracijos blaškymasis pašalintas. Teisinga architektūra.
- **Versijų sinchronizacija (v32.1):** `CURRENT_VERSION`=`versionCode`=`version.json`=33. Atnaujinimų tikrinimas veikia.
- **MediaStore / legacy failai:** išsaugojimas veikia (su aplankų-liekanų apsauga v32.1).
- **TTS:** media srautas, ducking, EN-fallback, startup kalbos sync — įgyvendinta.
- **SMS slėpimas, pinch-zoom, UI mastelis:** įgyvendinta.

---

## Scenarijų imitacija (veiksmų sekos)

1. **Startas su ESP WiFi + LTE (telefonas):** init → offline → cloud fetch (LTE) užpildo dalį kortelių → SSE onopen → „LIVE ESP", `_isOffline=false`. Jutikliai: cloud-mapinti ✓; SSE-tik — priklauso nuo #1.
2. **Startas be interneto (planšetė, tik ESP WiFi):** cloud fetch krenta („Unable to resolve host"), SSE onopen → „LIVE ESP". Cloud-tik kortelės tuščios; SSE-tik — priklauso nuo #1. **Atitinka pastebėtą planšetės elgesį.**
3. **Kalibravimas „nulis":** `sendButton('btn_adxl_zero')` → REST POST (veikia per ESP WiFi) → ESP kalibruoja (patvirtinta loge „Kalibracija baigta"). Ekranas atsinaujina tik jei nauja roll=0/pitch=0 **atkeliauja ir įsisaugo** → priklauso nuo #1 (ir ryšio stabilumo).
4. **Atnaujinimų tikrinimas:** native `checkUpdate` per `getSafeConnection` (LTE) → JSON parse → `onUpdateResult(type,ver,url,notes)` → „naujausias" (versijos sutampa). ✓
5. **Google upload:** native `httpPost` per LTE, be unbind. ✓ (priklauso nuo Apps Script deployment).
6. **SSE watchdog:** 60 s tyla → reconnect; po `MAX_SSE_RETRIES` → offline + cloud. Veikia, bet be interneto reconnect gali cikluoti (matyta loguose).
7. **TTS be LT balso:** applyLang LT→EN fallback → `getEffectiveLang()='en'` → JS parenka EN tekstą. ✓

---

## Rekomendacijos (prioritetas)

1. **🔴 Patvirtinti SSE saugojimą** (radinys #1) — pridėti `[SSE STORED]` logą, testuoti su ESP. Jei nesaugo — tai svarbiausia app klaida (paaiškina tuščias korteles, aliarmų nepatikimumą, kalibravimo neatsinaujinimą). Taisyti regex + kartu peržiūrėti SSE mapinimą (atidėta migracija) su gyvo ESP verifikacija.
2. **🟠 Po #1 — validuoti aliarmus** su realiais duomenimis (vanduo/CO2/dujos/vagystė).
3. **🟡 Patikrinti dublį `onopen`** (#4).
4. Likę atidėti darbai atmintyje: vagystės kortelės dedup (padaryta v32.1), cloud aliarmai, saveTextFile (padaryta), zoom-out (padaryta).

---

## Ką patikrinau
- Native: MainActivity (getSafeConnection, doHttp, TTS, files, SMS, update, autostart), build.gradle, version.json.
- Frontend: init seka, startSSE 'state' handler (+ node imitacija), _fetchSheetsNow, loadAllNumbers/Texts, visos fetch užklausos, checkAlarms/checkTheftVoice, setOfflineMode, dual-network JS.
- Online: ESPHome web_server v3 /events formatas; Android FGS/MediaStore/TTS/AudioFocus (ankstesni auditai).
- Git: `main`=`origin/main`, v33 sinchronizuota.
