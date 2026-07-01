# Užduotis: Ištaisyti failų išsaugojimą ir TTS balsą (Kemperis Android app)

**Kontekstas:** v30 APK, testuota Huawei planšetėje. Dvi problemos:
1. „Išsaugoti logą / CSV / GPX" meta klaidą.
2. Balso pranešimai (TTS) planšetėje netariami — girdisi tik pyptelėjimas — nors telefone veikia.

**Bendra taisyklė:** abu pataisymai daromi **tik Java pusėje** (`MainActivity.java`). **Nekeisti** `android/www/index.html`.

---

## Pataisymas 1 — `saveTextFile` argumentų neatitikimas

**Failas:** `android/android/app/src/main/java/lt/kemperis/app/MainActivity.java`, klasė `KemperisFileBridge`, metodas `saveTextFile` (~487 eil.).

**Simptomas:** `Error: /storage/…/Download/terminal_log_….txt/text/plain (No such file or directory)`.

**Priežastis:**
- JS (`android/www/index.html`, 3 vietose: `saveTerminal`, `downloadLocalCSV`, GPX eksportas) kviečia `KemperisFile.saveTextFile(failoVardas, mimeTipas, turinys)`.
- Java signatūra yra `saveTextFile(String folder, String name, String content)` — todėl `mimeTipas` („text/plain") tampa failo vardu su `/`, ir kelias sugadinamas.
- Antra klaida: JS tikrina `result === 'ok'`, o Java grąžina absoliutų kelią, ne `'ok'` — tad sėkmė niekada neaptinkama.

**Sprendimas** (viename taške, be JS keitimo) — priimti `(name, mime, content)`, rašyti tiesiai į Downloads, grąžinti `"ok"`:

```java
@JavascriptInterface
public String saveTextFile(String name, String mime, String content) {
    try {
        File dir = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS);
        if (!dir.exists()) dir.mkdirs();
        File file = new File(dir, name);
        try (FileOutputStream fos = new FileOutputStream(file)) {
            fos.write(content.getBytes(StandardCharsets.UTF_8));
        }
        return "ok";
    } catch (Exception e) { return "Error: " + e.getMessage(); }
}
```

**Priėmimo kriterijai:** visi trys mygtukai („Išsaugoti logą", „Atsisiųsti CSV", GPX) sukuria failą `Download/` aplanke ir terminale rodo žalią „✅ … išsaugotas", ne raudoną klaidą.

---

## Pataisymas 2 — TTS diagnostika + abiejų kalbų (EN ir LT) tikrinimas

**Failas:** tas pats `MainActivity.java`, klasė `KempTtsBridge` (~452 eil.).

**Simptomas:** planšetėje `speak()` tyliai nieko nedaro (`ready=false`), nes trūksta sistemos TTS variklio arba balso duomenų; telefone veikia. Pyptelėjimas girdisi, nes chime nepriklauso nuo TTS.

**Reikalavimai:**

1. `onInit`: jei `status != SUCCESS`, per `webView.evaluateJavascript(...)` pranešti į app terminalą (pvz. „⚠️ TTS variklis nerastas — įdiekite balso variklį").
2. Po `SUCCESS` — patikrinti **abiejų** kalbų prieinamumą su `isLanguageAvailable(...)` ir rezultatą išvesti į terminalą; jei bent vienos trūksta duomenų — pasiūlyti įdiegimą.
3. `applyLang`: tikrinti `setLanguage` grąžinamą kodą konkrečiai kalbai; `LANG_MISSING_DATA` → pranešti + `ACTION_INSTALL_TTS_DATA`; `LANG_NOT_SUPPORTED` → įspėti terminale.
4. `speak()`: jei `!ready`, vietoj tylaus nieko žurnaluoti, kad TTS neparuoštas.

**Pavyzdinis kodas:**

```java
private void reportLangs() {
    int en = tts.isLanguageAvailable(Locale.US);
    int lt = tts.isLanguageAvailable(new Locale("lt","LT"));
    logToWeb("TTS EN: " + langStatus(en) + " | LT: " + langStatus(lt));
    if (en == TextToSpeech.LANG_MISSING_DATA || lt == TextToSpeech.LANG_MISSING_DATA) {
        Intent i = new Intent(TextToSpeech.Engine.ACTION_INSTALL_TTS_DATA);
        i.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        activity.startActivity(i);
    }
}

private String langStatus(int r) {
    switch (r) {
        case TextToSpeech.LANG_AVAILABLE:
        case TextToSpeech.LANG_COUNTRY_AVAILABLE:
        case TextToSpeech.LANG_COUNTRY_VAR_AVAILABLE: return "OK";
        case TextToSpeech.LANG_MISSING_DATA:  return "truksta duomenu";
        case TextToSpeech.LANG_NOT_SUPPORTED: return "nepalaikoma";
        default: return "nezinoma(" + r + ")";
    }
}

private void logToWeb(String msg) {
    final String safe = msg.replace("'", " ");
    activity.runOnUiThread(() ->
        webView.evaluateJavascript("sysLog('[TTS] " + safe + "','warn')", null));
}
```

`onInit` po `ready=true; applyLang("en");` iškviesti `reportLangs();`. `applyLang` papildyti kodo tikrinimu:

```java
private void applyLang(String l) {
    if (!ready) return;
    Locale loc = "lt".equals(l) ? new Locale("lt","LT") : Locale.US;
    int r = tts.setLanguage(loc);
    if (r == TextToSpeech.LANG_MISSING_DATA) {
        logToWeb(l + " truksta balso duomenu");
        Intent i = new Intent(TextToSpeech.Engine.ACTION_INSTALL_TTS_DATA);
        i.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        activity.startActivity(i);
    } else if (r == TextToSpeech.LANG_NOT_SUPPORTED) {
        logToWeb(l + " kalba variklio nepalaikoma");
    }
}
```

**Priėmimo kriterijai:**
- Startuojant terminale matoma abiejų kalbų būsena, pvz. `TTS EN: OK | LT: truksta duomenu`.
- Trūkstant bent vienos kalbos duomenų — automatiškai pasiūlomas įdiegimo langas.
- Įdiegus balsus, tariama ir angliškai, ir lietuviškai (perjungiama per `setLang`).
- Jei variklis kalbos nepalaiko arba jo nėra — aiškus pranešimas terminale (ne tyla).

---

## Užbaigimas (build + release)

1. Nekeisti `android/www/index.html`.
2. Pakelti `versionCode` ir `versionName` į **31** (`android/android/app/build.gradle`).
3. Build: `cd android/android && ./gradlew assembleRelease`.
4. Nukopijuoti `app/build/outputs/apk/release/app-release.apk` → `android/kemperis_v31.apk`, `git add -f`.
5. Commit + push į GitHub.
6. Testuoti Huawei planšetėje: išsaugojimo mygtukai sukuria failus; balsas tariamas EN ir LT (arba pasirodo įdiegimo pasiūlymas).
