# v31.2 — užduotis asistentui: sutvarkyti atnaujinimų tikrinimą (native↔JS kontraktas)

**Problema:** v31.1 sutvarkė tinklo maršrutą (`getSafeConnection`), bet atnaujinimų tikrinimas vis tiek meta „Atnaujinimo patikra užtruko", nes native ir JS **nesutampa argumentais**:

- Native `checkUpdate()` siunčia žalią JSON kaip **vieną** argumentą: `onUpdateResult("{...}")`.
- JS `onUpdateResult(type, version, apkUrl, notes)` tikisi **keturių** ir lygina `type === 'update'` / `'latest'`.
- Native pusėje **nėra jokio versijos palyginimo**, tad nė viena JS šaka nesuveikia → mygtukas neatstatomas → 35 s timeout.

**Keisti tik:** `MainActivity.java`, `UpdateBridge.checkUpdate()`. **NELIESTI:** JS (`onUpdateResult` jau teisingas — turi `update`/`latest`/`error` šakas, visos atstato mygtuką), `getSafeConnection` (jau geras), `www/index.html`, firmware, Apps Script.

---

## Pataisymas — `checkUpdate()` perrašyti

`MainActivity.java`, klasė `UpdateBridge`. Pakeisti metodą į:

```java
@JavascriptInterface
public void checkUpdate() {
    new Thread(() -> {
        try {
            URL url = new URL(VERSION_JSON_URL);
            HttpURLConnection conn = getSafeConnection(url);   // v31.1 metodas — palikti
            conn.setConnectTimeout(15000);
            conn.setReadTimeout(15000);
            BufferedReader br = new BufferedReader(new InputStreamReader(conn.getInputStream()));
            StringBuilder sb = new StringBuilder(); String line;
            while ((line = br.readLine()) != null) sb.append(line);
            conn.disconnect();

            JSONObject jo = new JSONObject(sb.toString());
            int remoteVer   = jo.optInt("version", -1);
            String apkUrl   = jo.optString("apk_url", "");
            String notes    = jo.optString("notes", "");
            String type     = (remoteVer > CURRENT_VERSION) ? "update" : "latest";

            final String js = "window.onUpdateResult && window.onUpdateResult("
                + JSONObject.quote(type) + "," + remoteVer + ","
                + JSONObject.quote(apkUrl) + "," + JSONObject.quote(notes) + ")";
            runOnUiThread(() -> getBridge().getWebView().evaluateJavascript(js, null));

        } catch (Exception e) {
            final String msg = (e.getMessage() == null) ? "nezinoma klaida" : e.getMessage();
            final String js = "window.onUpdateResult && window.onUpdateResult("
                + JSONObject.quote("error") + ",0," + JSONObject.quote(msg) + "," + JSONObject.quote("") + ")";
            runOnUiThread(() -> getBridge().getWebView().evaluateJavascript(js, null));
        }
    }).start();
}
```

**Pastabos:**
- `JSONObject` jau importuotas (`org.json.JSONObject`).
- `getSafeConnection`, `CURRENT_VERSION`, `getBridge()` — jau prieinami (nekeisti).
- Klaidos atveju žinutė perduodama **3-iu** argumentu (`apkUrl` pozicija), nes JS daro `errMsg = apkUrl || notes` — tad terminale matysis reali priežastis, ne tyla.
- Timeout'ai (15s+15s) — kad neužstrigtų, jei tinklo nėra.

---

## Laukiamas rezultatas
- `version.json` „version" = 31, `CURRENT_VERSION` = 31 → `remoteVer > CURRENT_VERSION` = false → **„latest"** → app parodys „✅ v31 — naujausias" (vietoj „užtruko").
- Kai ateity `version.json` turės aukštesnę versiją → parodys atnaujinimo langą su „ATSISIŲSTI".
- Nesant interneto → „error" šaka su tikslia priežastimi, mygtukas atsistato iškart (ne po 35 s).

## Verifikacija
- Su internetu paspausti „Tikrinti dabar" → per kelias sek. „v31 — naujausias" (ne „užtruko").
- Be interneto → aiški klaida, mygtukas atsistato.

## Užbaigimas
1. (Neprivaloma) bumpinti versionName į „31.2"; `versionCode` gali likti 31 (tas pats diegiasi ant esamo).
2. `cd android/android && ./gradlew assembleRelease`
3. `app-release.apk` → `android/kemperis_v31.apk`, `git add -f`, commit + push.
