# v31.3 — užduotis asistentui: TTS garso srauto pataisymas (balsas negirdimas)

## Diagnozė (patvirtinta įrenginyje)
- Sistemos „Listen to an example" (Google TTS, en‑GB fully supported) — **kalba puikiai**.
- App **chime** (Web Audio, media srautas) — **girdimas**.
- App native `KempTts.speak()` — **tyla**, `onTtsDone` niekada nesuveikia (baigiasi tik per atsarginį laikmatį).

Išvada: garsiakalbis ir media srautas veikia; tyli **tik app `TextToSpeech`**, nes jis groja ne per media srautą (kai kuriuose Huawei/Android variantuose `TextToSpeech` be aiškių `AudioAttributes` naudoja tylų accessibility/notification srautą). App dar ir kelia **STREAM_MUSIC** iki 100% — bet TTS ten negroja.

## Keisti
`MainActivity.java`, klasė `KempTtsBridge` (1–3 p.) + `www/index.html` viena eilutė `init()` (3 p.). **NELIESTI** kitų failų.

## 1. Pataisymas — nustatyti TTS AudioAttributes į media/speech srautą (pagrindinis — dėl tylos)

Importai (viršuje, jei nėra):
```java
import android.media.AudioAttributes;
import android.media.AudioManager;
```

`KempTtsBridge` konstruktoriuje, `onInit` `SUCCESS` šakoje (po `ready = true;`), pridėti:
```java
AudioAttributes aa = new AudioAttributes.Builder()
        .setUsage(AudioAttributes.USAGE_MEDIA)
        .setContentType(AudioAttributes.CONTENT_TYPE_SPEECH)
        .build();
tts.setAudioAttributes(aa);   // TTS groja per media srautą — tą patį, kur chime ir kur app kelia garsą
```
(`setAudioAttributes` — API 21+, tinka; minSdk 24.)

Papildomai — `speak()` perduoti STREAM_MUSIC per params (belt‑and‑suspenders senesniam elgesiui):
```java
@JavascriptInterface
public void speak(String text) {
    if (ready) {
        android.os.Bundle params = new android.os.Bundle();
        params.putInt(TextToSpeech.Engine.KEY_PARAM_STREAM, AudioManager.STREAM_MUSIC);
        tts.speak(text, TextToSpeech.QUEUE_ADD, params, "uid");
    } else {
        logToWeb("TTS neparuostas - nutylima");
    }
}
```

## Laukiamas rezultatas
- App balso pranešimai girdimi (tas pats media srautas kaip chime ir sistemos pavyzdys).
- `onTtsDone` suveikia normaliai (garsumo atkūrimas ir eilė tikslūs, ne per 3 s atsarginį laikmatį).
- Esamas STREAM_MUSIC garsumo kėlimas dabar veikia teisingai (TTS groja MUSIC sraute).

## Verifikacija
1. Įdiegti, paleisti, sukelti balso pranešimą (arba testą „Voice notifications are active").
2. Turi girdėtis balsas; terminale po `KempTts.speak` pabaiga turi ateiti greičiau nei 3 s (per `onDone`, ne atsarginį laikmatį).

## 2. Pataisymas — `applyLang` rinktis PRIEINAMĄ kalbą (ne hardkodintą en‑US)

Priežastis: įrenginy pilnai palaikoma **en‑GB**, bet app hardkodina `Locale.US` — jei en‑US duomenų nėra, kalba nepavyksta. Perrašyti `applyLang` ir pridėti pagalbinį `isUsable`:

```java
private boolean isUsable(Locale loc) {
    int r = tts.isLanguageAvailable(loc);
    return r == TextToSpeech.LANG_AVAILABLE
        || r == TextToSpeech.LANG_COUNTRY_AVAILABLE
        || r == TextToSpeech.LANG_COUNTRY_VAR_AVAILABLE;
}
private void applyLang(String l) {
    if (!ready) return;
    Locale target;
    if ("lt".equals(l)) {
        target = new Locale("lt", "LT");
    } else {
        // rinktis prieinamą anglų variantą: US → GB → įrenginio numatytą
        if (isUsable(Locale.US))       target = Locale.US;
        else if (isUsable(Locale.UK))  target = Locale.UK;
        else                           target = Locale.getDefault();
    }
    int r = tts.setLanguage(target);
    if (r == TextToSpeech.LANG_MISSING_DATA) {
        // anglų atveju – atsarga į en‑GB
        if (!"lt".equals(l) && isUsable(Locale.UK)) { tts.setLanguage(Locale.UK); return; }
        logToWeb(l + " truksta balso duomenu"); promptInstall();
    } else if (r == TextToSpeech.LANG_NOT_SUPPORTED) {
        logToWeb(l + " kalba nepalaikoma");
    }
}
```

## 3. Pataisymas — pritaikyti IŠSAUGOTĄ kalbą starte (dingsta „skaičiai angliškai")

Priežastis: paleidžiant native lieka anglų (`applyLang("en")` konstruktoriuje), o išsaugota LT niekada nepritaikoma, nes `init()` tik nustato meniu reikšmę.

**3a. `MainActivity.java` — `KempTtsBridge`:** pridėti laukiančios kalbos lauką ir pritaikyti ją `onInit`:
```java
private String pendingLang = null;

@JavascriptInterface
public void setLang(String lang) {
    if (ready) applyLang(lang);
    else pendingLang = lang;   // pritaikysim, kai variklis pasiruoš
}
```
Konstruktoriaus `onInit` `SUCCESS` šakoje vietoj `applyLang("en");` naudoti:
```java
applyLang(pendingLang != null ? pendingLang : "en");
```

**3b. `www/index.html` — `init()`** (~2363 eil., ten kur `langSel.value = ttsLang;`), pridėti vieną eilutę, kad išsaugota kalba nueitų į native:
```javascript
if (window.KempTts && window.KempTts.setLang) { try { window.KempTts.setLang(ttsLang); } catch(e){} }
```
(Native pusėje, jei variklis dar neparuoštas, kalba įsimenama `pendingLang` ir pritaikoma `onInit`.)

**Priėmimas (2+3):** startuojant native TTS kalba atitinka app nustatymą (LT arba EN); jei pasirinkta EN — naudojamas prieinamas variantas (en‑GB), ne tuščias en‑US; skaičiai skaitomi pasirinkta kalba.

## 4. Pataisymas — pritildyti kitą garsą (muziką/radiją) kol kalba (audio focus + ducking)

Tikslas: prieš kalbant paprašyti laikinos garso pirmenybės (kiti grotuvai **pritildomi**), o pabaigus — grąžinti (muzika atsistato).

`MainActivity.java`, klasė `KempTtsBridge`. Pridėti lauką ir du pagalbinius metodus:

```java
private android.media.AudioFocusRequest audioFocusReq = null;

private void requestDuck() {
    try {
        AudioManager am = (AudioManager) activity.getSystemService(Context.AUDIO_SERVICE);
        if (am == null) return;
        AudioAttributes aa = new AudioAttributes.Builder()
                .setUsage(AudioAttributes.USAGE_MEDIA)
                .setContentType(AudioAttributes.CONTENT_TYPE_SPEECH)
                .build();
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            audioFocusReq = new android.media.AudioFocusRequest.Builder(
                    AudioManager.AUDIOFOCUS_GAIN_TRANSIENT_MAY_DUCK)   // pritildo kitus; pilnam stabdymui – AUDIOFOCUS_GAIN_TRANSIENT
                    .setAudioAttributes(aa)
                    .build();
            am.requestAudioFocus(audioFocusReq);
        } else {
            am.requestAudioFocus(null, AudioManager.STREAM_MUSIC,
                    AudioManager.AUDIOFOCUS_GAIN_TRANSIENT_MAY_DUCK);
        }
    } catch (Exception ignored) {}
}

private void abandonDuck() {
    try {
        AudioManager am = (AudioManager) activity.getSystemService(Context.AUDIO_SERVICE);
        if (am == null) return;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            if (audioFocusReq != null) { am.abandonAudioFocusRequest(audioFocusReq); audioFocusReq = null; }
        } else {
            am.abandonAudioFocus(null);
        }
    } catch (Exception ignored) {}
}
```

Kviesti:
- `speak()` pradžioje (kai `ready`, prieš `tts.speak(...)`): `requestDuck();`
- `fireDone()` viduje (po kalbos pabaigos, `onDone`/`onError`): `abandonDuck();`

**Pastaba:** tarp kelių eilių iš eilės muzika gali trumpam atsileisti (~0.3 s pauzė eilėje). Jei nepatiks — focus galima laikyti visą eilę (prašyti prie pirmos, atiduoti kai eilė tuščia); bet paprastam variantui užtenka per‑frazę.

**Priėmimas:** grojant muzikai/radijui (to paties įrenginio grotuve) app pranešimo metu garsas pritildomas, po pranešimo — grįžta.

---

## Užbaigimas
1. `versionName` → „31.3" (`versionCode` gali likti 31).
2. `cd android/android && ./gradlew assembleRelease`
3. `app-release.apk` → `android/kemperis_v31.apk`, `git add -f`, commit + push.
