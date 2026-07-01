# v31 — užduotis asistentui (tik keitimai)

Pritaikyta prie **veikiančios versijos (v30)**. Taisyti tik žemiau nurodyta.
**NELIESTI:** firmware YAML, Google Apps Script, PIN logikos, `startSSE`/`SENSOR_ID_MAP`, `BootReceiver.java` (žr. pastabą pabaigoje).
Failai: `MainActivity.java`, `www/index.html` (5 ir 6 p.), `app/build.gradle`, `version.json`.

> **Techninė patikra (2026-07-01, Android dokumentacija):** 1 p. MediaStore — teisinga, Android 10+ leidimų nereikia. 2 p. TTS API (`isLanguageAvailable`, `ACTION_INSTALL_TTS_DATA`, `UtteranceProgressListener`, `utteranceId`) — teisinga. 5 p. zoom — atitinka dokumentuotą Capacitor sprendimą. 6 p. `FEATURE_TELEPHONY`/`getSimState` — teisinga.

---

## 1. `saveTextFile` → MediaStore
`MainActivity.java`, klasė `KemperisFileBridge` (~487 eil.). Dabartinė signatūra `(folder, name, content)` sugadina kelią ir Android 10+ blokuoja tiesioginį rašymą. Pakeisti visą metodą į:

```java
@JavascriptInterface
public String saveTextFile(String name, String mime, String content) {
    try {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            ContentValues cv = new ContentValues();
            cv.put(MediaStore.Downloads.DISPLAY_NAME, name);
            cv.put(MediaStore.Downloads.MIME_TYPE, (mime == null || mime.isEmpty()) ? "text/plain" : mime);
            cv.put(MediaStore.Downloads.RELATIVE_PATH, Environment.DIRECTORY_DOWNLOADS);
            android.net.Uri uri = ctx.getContentResolver().insert(MediaStore.Downloads.EXTERNAL_CONTENT_URI, cv);
            if (uri == null) return "Error: MediaStore insert null";
            try (OutputStream os = ctx.getContentResolver().openOutputStream(uri)) {
                os.write(content.getBytes(StandardCharsets.UTF_8));
            }
            return "ok";
        } else {
            File dir = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS);
            if (!dir.exists()) dir.mkdirs();
            File file = new File(dir, name);
            try (FileOutputStream fos = new FileOutputStream(file)) {
                fos.write(content.getBytes(StandardCharsets.UTF_8));
            }
            return "ok";
        }
    } catch (Exception e) { return "Error: " + e.getMessage(); }
}
```
JS keisti nereikia — `saveTerminal`, `downloadLocalCSV`, `downloadGPX` jau siunčia `(failas, mime, turinys)` ir tikrina `result === 'ok'`.
**Priėmimas:** visi 3 mygtukai kuria failą Downloads ir rodo žalią „✅".

---

## 2. TTS diagnostika (EN+LT) + pabaigos signalas
`MainActivity.java`, klasė `KempTtsBridge` (~452 eil.). Dabar `speak()` tyli, kai nėra variklio/balso, ir `onTtsDone` niekada nekviečiamas. Pakeisti konstruktorių ir `speak`, pridėti pagalbinius metodus:

```java
public KempTtsBridge(Activity a, WebView wv) {
    this.activity = a; this.webView = wv;
    tts = new TextToSpeech(a, status -> {
        if (status == TextToSpeech.SUCCESS) {
            ready = true;
            applyLang("en");
            reportLangs();
            tts.setOnUtteranceProgressListener(new UtteranceProgressListener() {
                @Override public void onStart(String id) {}
                @Override public void onDone(String id) { fireDone(); }
                @Override public void onError(String id) { fireDone(); }
            });
        } else {
            logToWeb("variklis nerastas (status=" + status + ")");
        }
    });
}
@JavascriptInterface
public void speak(String text) {
    if (ready) tts.speak(text, TextToSpeech.QUEUE_ADD, null, "uid");
    else logToWeb("TTS neparuostas - nutylima");
}
@JavascriptInterface
public void setLang(String lang) { applyLang(lang); }
private void applyLang(String l) {
    if (!ready) return;
    Locale loc = "lt".equals(l) ? new Locale("lt","LT") : Locale.US;
    int r = tts.setLanguage(loc);
    if (r == TextToSpeech.LANG_MISSING_DATA) { logToWeb(l + " truksta balso duomenu"); promptInstall(); }
    else if (r == TextToSpeech.LANG_NOT_SUPPORTED) { logToWeb(l + " kalba nepalaikoma"); }
}
private void reportLangs() {
    int en = tts.isLanguageAvailable(Locale.US);
    int lt = tts.isLanguageAvailable(new Locale("lt","LT"));
    logToWeb("TTS EN: " + langStatus(en) + " | LT: " + langStatus(lt));
    if (en == TextToSpeech.LANG_MISSING_DATA || lt == TextToSpeech.LANG_MISSING_DATA) promptInstall();
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
private void promptInstall() {
    try {
        Intent i = new Intent(TextToSpeech.Engine.ACTION_INSTALL_TTS_DATA);
        i.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        activity.startActivity(i);
    } catch (Exception ignored) {}
}
private void fireDone() {
    activity.runOnUiThread(() -> webView.evaluateJavascript("window.onTtsDone && window.onTtsDone()", null));
}
private void logToWeb(String msg) {
    final String safe = msg.replace("'", " ");
    activity.runOnUiThread(() -> webView.evaluateJavascript("sysLog && sysLog('[TTS] " + safe + "','warn')", null));
}
public void shutdown() { if (tts != null) tts.shutdown(); }
```
**Priėmimas:** startuojant terminale, pvz. `TTS EN: OK | LT: truksta duomenu`; trūkstant — diegimo langas; balsas atkuriamas pilnai (`onTtsDone`).

---

## 3. APK diegimo statuso patikra
`MainActivity.java`, `UpdateBridge`, `downloadReceiver.onReceive` (~431 eil.). Dabar diegia netikrindamas statuso. Pakeisti `onReceive` į:

```java
@Override
public void onReceive(Context context, Intent intent) {
    long id = intent.getLongExtra(DownloadManager.EXTRA_DOWNLOAD_ID, -1);
    if (id != downloadId) return;
    DownloadManager dm = (DownloadManager) ctx.getSystemService(Context.DOWNLOAD_SERVICE);
    DownloadManager.Query q = new DownloadManager.Query().setFilterById(id);
    try (android.database.Cursor c = dm.query(q)) {
        if (c != null && c.moveToFirst()) {
            int st = c.getInt(c.getColumnIndex(DownloadManager.COLUMN_STATUS));
            if (st == DownloadManager.STATUS_SUCCESSFUL) {
                installApk(new File(Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS), "kemperis_update.apk"));
            } else {
                sendNativeLog("❌ Atsisiuntimas nepavyko (status=" + st + ")");
            }
        }
    }
}
```
**Priėmimas:** nepavykus atsisiųsti — pranešimas, diegimas neinicijuojamas su senu failu.

---

## 4. Versija → 31
- `app/build.gradle` → `defaultConfig`: `versionCode 31`, `versionName "31"` (dabar 30, 25–26 eil.).
- `MainActivity.java` → `CURRENT_VERSION = 31` (dabar 30, ~65 eil.).
- `version.json` → `"version": 31` ir `apk_url` → `kemperis_v31.apk`.

---

## 5. Pinch-to-zoom (dviejų pirštų didinimas)

**5.1 `www/index.html` — `<head>`**, viewport meta:
```html
<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=2.0, minimum-scale=0.5, user-scalable=yes">
```

**5.2 `MainActivity.java` — `onCreate()`**, prie ESAMO `WebSettings settings` bloko (116–117 eil.; NEDEKLARUOTI `settings` iš naujo):
```java
settings.setSupportZoom(true);
settings.setBuiltInZoomControls(true);
settings.setDisplayZoomControls(false);
settings.setLoadWithOverviewMode(true);
settings.setUseWideViewPort(true);
```

**5.3 `www/index.html` — CSS**:
```css
body { touch-action: manipulation; }
input, textarea, select { font-size: 16px; }
```
**Priėmimas:** zoom ~50–200%, sklandus, be +/- mygtukų; kortelės, mygtukai, žemėlapis normalūs.

---

## 6. Paslėpti SMS mygtukus įrenginiuose be SMS galimybės
WiFi-only planšetėje (be SIM/modemo) nerodyti SMS mygtukų (Statusas/Lokacija/Arm/Disarm/Upload) ir SMS skirtuko.

**6.1 `MainActivity.java` — `SmsBridge`**, pridėti (importai `PackageManager`, `TelephonyManager` jau yra):
```java
@JavascriptInterface
public boolean canSendSms() {
    PackageManager pm = ctx.getPackageManager();
    if (!pm.hasSystemFeature(PackageManager.FEATURE_TELEPHONY)) return false;
    TelephonyManager tm = (TelephonyManager) ctx.getSystemService(Context.TELEPHONY_SERVICE);
    return tm != null && tm.getSimState() == TelephonyManager.SIM_STATE_READY;
}
```

**6.2 `www/index.html`** — SMS mygtukų du grid'us `#offline-banner` viduje (~184–192 eil.) apvynioti į vieną konteinerį (offline statusas ir „⚡ Gyvai" lieka lauke):
```html
<div id="offline-sms-controls">
  <!-- esami du <div ...grid...> su Statusas/Lokacija ir Arm/Disarm/Upload -->
</div>
```

**6.3 `www/index.html` — `init()`** (~2311 eil.), po bridge'ų patikros pridėti:
```javascript
const smsOk = !!(window.KemperisSms && window.KemperisSms.canSendSms && window.KemperisSms.canSendSms());
if (!smsOk) {
    const c = document.getElementById('offline-sms-controls'); if (c) c.style.display = 'none';
    const smsTab = document.getElementById('sms'); if (smsTab) smsTab.remove();
    const opt = document.querySelector('#tab-select option[value="sms"]'); if (opt) opt.remove();
    const i = SWIPE_TABS.indexOf('sms'); if (i !== -1) SWIPE_TABS.splice(i, 1);
}
```
(`updateSmsTerminal` jau turi `if (!term) return`, tad #sms pašalinimas saugus.)
**Priėmimas:** planšetėje be SIM SMS mygtukai/skirtukas nematomi, offline statusas rodomas; telefone su SIM — kaip anksčiau.

---

## Autostart — patikrinta, NEKEISTI
Veikiančioje versijoje `BootReceiver.java` paleidžia `MainActivity` (Activity) — tai teisinga, nes app logika (SSE, aliarmai, TTS) gyvena WebView'e. Huawei įrenginyje veikia su autostart baltuoju sąrašu (app jį padeda įjungti per `openAutostartSettings`). Paleisti tik `KemperisService` **netiktų** (jame nėra WebView). Todėl `BootReceiver` **palikti kaip yra**.

---

## Užbaigimas
1. `cd android/android && ./gradlew assembleRelease`
2. `app/build/outputs/apk/release/app-release.apk` → `android/kemperis_v31.apk`, `git add -f`
3. Commit + push
4. Testas planšetėje: išsaugojimas kuria failus; balsas EN+LT; pinch-to-zoom veikia; SMS mygtukai/skirtukas paslėpti (be SIM).
