# Kemperis → Android .apk su Capacitor

Šis gidas paverčia `KemperisV5.html` į tikrą Android programėlę su **native TTS** (balsas pritildo radiją) ir **veikimu užgesusiu ekranu**.

## Ko reikia kompiuteryje

- **Node.js** (LTS) — https://nodejs.org
- **Android Studio** (su Android SDK) — https://developer.android.com/studio
- **JDK 17** (ateina su Android Studio)

---

## 1. Sukurti projektą

Atidaryk terminalą (PowerShell) tuščiame aplanke ir paleisk:

```bash
mkdir KemperisApp && cd KemperisApp
npm init -y
npm install @capacitor/core @capacitor/cli @capacitor/android
npx cap init Kemperis lt.kemperis.app --web-dir=www
```

## 2. Įdėti HTML (web turinį)

Capacitor paleidžia `www/index.html`. Todėl:

```bash
mkdir www
```

- Nukopijuok šio aplanko `KemperisV5.html` į `www/` ir **pervadink į `index.html`**.

(Galutinis kelias: `KemperisApp/www/index.html`)

## 3. Pakeisti capacitor.config

Pakeisk sugeneruotą `capacitor.config.ts` šio aplanko versija (su `cleartext: true` ir `androidScheme: 'http'`). Tai išsprendžia HTTP ryšį su ESP (`192.168.x.x`) be „mixed content" klaidų.

## 4. Įdiegti įskiepius

```bash
# Native Text-to-Speech (balsas, audio focus / ducking)
npm install @capacitor-community/text-to-speech

# Background mode — kad balsas/SSE veiktų UŽGESUSIU ekranu (foreground service + wakelock)
npm install cordova-plugin-background-mode cordova-plugin-device
```

> `cordova-plugin-background-mode` yra Cordova įskiepis, bet Capacitor jį palaiko. HTML jau turi jo inicijavimo kodą (`deviceready` + `disableWebViewOptimizations()`).

## 5. Pridėti Android platformą ir sinchronizuoti

```bash
npx cap add android
npx cap sync
```

## 6. Leidimai (AndroidManifest.xml)

Failas: `android/app/src/main/AndroidManifest.xml`

Į `<manifest>` viduje, prieš `<application>`, pridėk:

```xml
<uses-permission android:name="android.permission.FOREGROUND_SERVICE" />
<uses-permission android:name="android.permission.WAKE_LOCK" />
<uses-permission android:name="android.permission.INTERNET" />
<uses-permission android:name="android.permission.ACCESS_NETWORK_STATE" />
```

`android:usesCleartextTraffic="true"` Capacitor jau įdeda per `cleartext: true`, bet patikrink, kad `<application ... android:usesCleartextTraffic="true">` yra.

## 7. Atidaryti Android Studio ir sukurti APK

```bash
npx cap open android
```

Android Studio: **Build → Build Bundle(s) / APK(s) → Build APK(s)**.
Gausi `app-debug.apk` (`android/app/build/outputs/apk/debug/`). Įsidiek į planšetę.

Pasirašytam (release) APK: **Build → Generate Signed Bundle / APK**.

---

## 8. (Pasirinktinai, bet rekomenduojama) Garso valdymas — native snippetas

Kad veiktų funkcija „**pakelti garsą iki X% prieš pranešimą ir grąžinti po jo**", programai reikia pasiekti Android `AudioManager`. Tai padaro mažas kodas `MainActivity`. Be jo programa vis tiek veiks, tik garso lygio nekeis (naudos esamą sistemos media garsą).

Failas: `android/app/src/main/java/lt/kemperis/app/MainActivity.java`
(kelias atitinka `appId` — `lt/kemperis/app`)

Pakeisk failą šiuo turiniu:

```java
package lt.kemperis.app;

import android.content.Context;
import android.media.AudioManager;
import android.os.Bundle;
import android.webkit.JavascriptInterface;
import com.getcapacitor.BridgeActivity;

public class MainActivity extends BridgeActivity {
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // JS pasiekia per window.KemperisVol.getVolumePct() / setVolumePct(p)
        this.getBridge().getWebView().addJavascriptInterface(new VolBridge(this), "KemperisVol");
    }

    public static class VolBridge {
        private final Context ctx;
        VolBridge(Context c) { ctx = c; }

        @JavascriptInterface
        public int getVolumePct() {
            AudioManager am = (AudioManager) ctx.getSystemService(Context.AUDIO_SERVICE);
            int max = am.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
            int cur = am.getStreamVolume(AudioManager.STREAM_MUSIC);
            return max > 0 ? Math.round(cur * 100f / max) : 0;
        }

        @JavascriptInterface
        public void setVolumePct(int pct) {
            AudioManager am = (AudioManager) ctx.getSystemService(Context.AUDIO_SERVICE);
            int max = am.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
            int v = Math.round(max * Math.max(0, Math.min(100, pct)) / 100f);
            am.setStreamVolume(AudioManager.STREAM_MUSIC, v, 0);
        }
    }
}
```

Po šito: **Build → Build APK(s)** iš naujo.

> **Kaip tai veikia kartu:** prieš pranešimą programa įsimena esamą media garsą, pakelia iki tavo nustatyto % (Nustatymai → 🔊 Garsumas), paleidžia pyptelėjimą + balsą, o `category: 'playback'` per TTS įskiepį paprašo *audio focus* — kitų programų (Spotify, navigacijos) garsas pritildomas kalbėjimo metu ir grąžinamas po jo. Pabaigus, media garsas grąžinamas į buvusį lygį.
>
> **Ko negali:** jei muzika groja iš FM/AM aparatinės magnetolos (ne Android programos), programa jos pritildyti negali — tą valdo tik pati magnetola.

---

## SVARBU, kad veiktų patikimai

1. **Lietuvių balsas.** Jei TTS kalba LT — telefone įdiek lietuvių balsą:
   *Nustatymai → Pritaikymas / Accessibility → Text-to-speech → Google → kalbos → Lietuvių (parsisiųsti).*
   Programos „Nustatymai → Balso pranešimų kalba" leidžia perjungti LT/EN (numatyta **EN**).

2. **Battery optimization IŠJUNGTI.** Kad app neužmigtų fone:
   *Nustatymai → Programos → Kemperis → Baterija → „Neoptimizuoti" / „Unrestricted".*
   Be šito Android gali nutildyti balsą po kurio laiko ekranui užgesus.

3. **ESP IP.** Programos „Nustatymai → ESP32 IP adresas" — įvesk dabartinį ESP IP (AP režimu `192.168.4.1`, arba kitą jei ESP prisijungęs prie maršrutizatoriaus).

4. **PIN.** „Nustatymai" ir „Aliarmai" skiltys apsaugotos PIN (numatyta `0000`). Pakeisk per „Prieigos apsauga (PIN)".

---

## Atnaujinus HTML ateityje

```bash
# perkopijuok naują index.html į www/, tada:
npx cap sync
npx cap open android   # ir vėl Build APK
```
