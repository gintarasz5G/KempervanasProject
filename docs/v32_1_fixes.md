# v32.1 — užduotis asistentui: sukaupti pataisymai (be SSE migracijos)

Taisyti tik žemiau. **NELIESTI:** SSE object_id atvaizdavimo migracija (sąmoningai atidėta), getSafeConnection/dual-network (v32 jau geras).

---

## 1. `CURRENT_VERSION` → 32 (klaida iš v32)
`MainActivity.java` (~66 eil.): `static final int CURRENT_VERSION = 32;` (dabar 31 — todėl app klaidingai rodo „yra nauja versija v32", nors jau ant v32).

---

## 2. Pašalinti dubliuojančią vagystės kortelę
`www/index.html`, funkcija **`renderAlarmSettings()`** (NE `checkAlarms`), `ALARM_CONFIG.forEach(alarm => {` pradžioje pridėti:
```javascript
if (alarm.id === 'theft_alarm') return; // nerodyti dubliuojančios kortelės — vagystė valdoma per „Vagystės balso tekstai"
```
**Palikti** `theft_alarm` įrašą `ALARM_CONFIG` (jo ieško `checkTheftVoice`). Rezultatas: viršutinė „VAGYSTĖ" kortelė dingsta; lieka „DUJOS – ŽEMAS", „KONDENSATAS" ir apatiniai du vagystės tekstų blokai.

---

## 3. `saveTextFile` — apsauga nuo likusio aplanko (Android ≤9 legacy šaka)
`MainActivity.java`, `KemperisFileBridge.saveTextFile`, **else (legacy) šakoje**, prieš `new FileOutputStream(file)`:
```java
File file = new File(dir, name);
if (file.exists() && file.isDirectory()) {   // sena v30 bug'o liekana — aplankas failo vardu
    File[] kids = file.listFiles();
    if (kids != null) for (File k : kids) k.delete();
    file.delete();
}
try (FileOutputStream fos = new FileOutputStream(file)) { ... }
```
(MediaStore šakos keisti nereikia.) Rezultatas: „Is a directory" klaida nebekartos net jei liko senų aplankų.

---

## 4. TTS EN‑fallback — kai nėra lietuvių balso, skaityti angliškus tekstus
Problema: planšetės Google TTS neturi LT balso → lietuvišką tekstą skaito angliškai („aktaivus"). Sprendimas: jei LT balsas neprieinamas, native perjungia į EN, o JS parenka **EN** tekstą (ne LT).

**4a. `MainActivity.java` `KempTtsBridge`** — pridėti `effectiveLang` lauką ir metodą; `applyLang` daryti LT→EN fallback, kai LT neprieinama:
```java
private String effectiveLang = "en";

private void applyLang(String l) {
    if (!ready) return;
    Locale target;
    if ("lt".equals(l) && isUsable(new Locale("lt","LT"))) {
        target = new Locale("lt","LT");
        effectiveLang = "lt";
    } else {                       // prašoma EN ARBA LT nepalaikoma → anglų
        if (isUsable(Locale.US))      target = Locale.US;
        else if (isUsable(Locale.UK)) target = Locale.UK;
        else                          target = Locale.getDefault();
        effectiveLang = "en";
    }
    int r = tts.setLanguage(target);
    if (r == TextToSpeech.LANG_MISSING_DATA) { logToWeb(l + " truksta balso duomenu"); promptInstall(); }
    else if (r == TextToSpeech.LANG_NOT_SUPPORTED) { logToWeb(l + " kalba nepalaikoma"); }
}

@JavascriptInterface
public String getEffectiveLang() { return effectiveLang; }
```

**4b. `www/index.html`** — kur parenkamas vagystės balso tekstas pagal kalbą (`checkTheftVoice`, `renderTheftTexts`), vietoj `ttsLang` naudoti faktinę variklio kalbą:
```javascript
function effTtsLang() {
    try { if (window.KempTts && window.KempTts.getEffectiveLang) return window.KempTts.getEffectiveLang(); } catch(e){}
    return ttsLang;
}
```
`checkTheftVoice`: `const texts = THEFT_TEXTS[effTtsLang()] || THEFT_TEXTS['en'];` (vietoj `THEFT_TEXTS[ttsLang]`).
`renderTheftTexts`: laukelių pildymui rodyti pagal `ttsLang` (redagavimui), bet **balsui** naudoti `effTtsLang()`.

**Priėmimas:** planšetėje be LT balso vagystės/aliarmų pranešimai tariami **angliškai taisyklingai** (ne „aktaivus"); telefone su LT balsu — lietuviškai.

---

## 5. Zoom‑out — „UI mastelis" valdiklis (pinch‑out neveikia su device‑width)
`www/index.html`. Pridėti Nustatymuose valdiklį, kuris sumažina visą turinį (tilptų daugiau kortelių).

**5a. Nustatymų kortelė** (šalia kitų nustatymų):
```html
<div class="card"><h3>🔍 Ekrano mastelis</h3>
  <div class="input-group"><label>UI mastelis: <span id="ui-scale-label">100</span>%</label>
    <input type="range" id="ui-scale-range" min="60" max="100" step="5" value="100"
           oninput="setUiScale(this.value)"></div>
</div>
```

**5b. JS:**
```javascript
function setUiScale(pct) {
    pct = Math.max(60, Math.min(100, parseInt(pct) || 100));
    document.body.style.zoom = (pct / 100).toString();
    const lbl = document.getElementById('ui-scale-label'); if (lbl) lbl.textContent = pct;
    const rng = document.getElementById('ui-scale-range'); if (rng) rng.value = pct;
    localStorage.setItem('ui_scale', pct);
}
```
`init()` gale pritaikyti išsaugotą: `setUiScale(localStorage.getItem('ui_scale') || 100);`

**Priėmimas:** slankiklis 60–100% sumažina visą UI (tilpsta daugiau kortelių); reikšmė išsaugoma; kortelės, mygtukai, žemėlapis, apatinė juosta lieka tvarkingi.

---

## NEĮTRAUKTA (sąmoningai)
- **SSE object_id migracija** — atidėta (reikia gyvo ESP; rizika tyliai pamesti kortelę).
- **Cloud aliarmai** — tai **funkcija, ne klaida** (aliarmai tyčia neveikia su pasenusiais Google duomenimis). Jei norėsi — atskira užduotis su apsauga nuo pasenusių reikšmių ir dubliavimo. Šioje užduotyje NEDAROMA.

---

## Užbaigimas
1. `versionName` → „32.1", `versionCode` → 33 (kad naujas diegtųsi ir CURRENT_VERSION patikra būtų teisinga; **arba** palikti 32 ir CURRENT_VERSION=32 — svarbiausia, kad `CURRENT_VERSION` == `version.json` version).
   - Jei keli versijas: `MainActivity.CURRENT_VERSION`, `build.gradle versionCode/Name`, `version.json version` — visi suderinti.
2. `cd android/android && ./gradlew assembleRelease`
3. `app-release.apk` → `android/kemperis_v32.apk` (arba v33; jei pervadini — atnaujinti `version.json` apk_url), `git add -f`, commit + push.

## Testavimas
- Atnaujinimų tikrinimas rodo „naujausias" (ne netikrą „nauja versija").
- Aliarmų skiltyje nebėra dubliuojančios „VAGYSTĖ" kortelės.
- Failų išsaugojimas veikia net jei buvo likę senų aplankų.
- Balsas: be LT balso — angliškai; su LT balsu — lietuviškai.
- UI mastelis mažina/talpina korteles; išsaugomas.
