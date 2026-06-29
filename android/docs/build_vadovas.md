# 🔨 Kompiliavimo ir Atnaujinimo Vadovas

## Reikalavimai

- Android Studio (Ladybug arba naujesnė)
- Node.js + npm
- Capacitor CLI: `npm install -g @capacitor/cli`
- Java 17 (Android Studio sudėtyje)

---

## 1. Failų Sinchronizacija (PRIVALOMA prieš kompiliavimą)

Po kiekvieno `www/index.html` pakeitimo būtina sinchronizuoti į Android assets:

```bash
# Rankiniu būdu (paprasčiausias kelias):
copy /Y "KemperisApp\www\index.html" "KemperisApp\android\app\src\main\assets\public\index.html"

# Arba per Capacitor:
cd KemperisApp
npx cap sync android
```

**Patikrinimas (MD5 turi sutapti):**
```bash
# Windows PowerShell:
Get-FileHash "www\index.html" -Algorithm MD5
Get-FileHash "android\app\src\main\assets\public\index.html" -Algorithm MD5
```

---

## 2. Versijos Pakeitimas

### build.gradle (`android/app/build.gradle`)
```gradle
android {
    defaultConfig {
        versionCode 13        // ← Padidinti 1
        versionName "13"      // ← Atnaujinti
    }
}
```

### version.json (šakninė aplanko versija)
```json
{
  "version": 13,
  "apk_url": "https://github.com/gintarasz5G/kemperis-app/releases/download/v13/kemperis_v13.apk",
  "notes": "v13 Atnaujinimas:\n- ✅ Pakeitimas 1\n- ✅ Pakeitimas 2"
}
```

**SVARBU:** `version.json` turi būti pasiekiamas per GitHub (arba kitą serverį) — app jį parsisiunčia tikrindama atnaujinimus.

---

## 3. APK Kompiliavimas

### Per Android Studio (rekomenduojama):
1. Atidaryti `KemperisApp/android` aplanką Android Studio
2. **Build → Build Bundle(s) / APK(s) → Build APK(s)**
3. APK atsiras: `android/app/build/outputs/apk/debug/app-debug.apk`

### Per komandinę eilutę:
```bash
cd KemperisApp/android
./gradlew assembleDebug
# APK: app/build/outputs/apk/debug/app-debug.apk
```

### Release APK (pasirašytas):
```bash
./gradlew assembleRelease
# Reikia keystore failo (signing config)
```

---

## 4. APK Publikavimas GitHub Releases

1. Pervadinti APK: `kemperis_v13.apk`
2. GitHub → Releases → Draft new release
3. Tag: `v13`
4. Įkelti APK failą
5. Publish release
6. **Atnaujinti `version.json`** su nauju `apk_url`

---

## 5. APK Įdiegimas į Telefoną

### Per USB (development):
```bash
adb install -r kemperis_v13.apk
```

### Per app (self-update):
- App parsisiunčia iš GitHub Releases URL
- Automatiškai pasiūlo įdiegti
- Reikia leisti „Install from unknown sources"

### Rankiniu būdu:
- Nukopijuoti APK į telefoną
- Atidaryti per File Manager

---

## 6. Google Apps Script Atnaujinimas

Jei keičiamas `GoogleSheetsScript.js`:

1. Atidaryti [script.google.com](https://script.google.com)
2. Atidaryti projektą `KemperisScript`
3. Kopijuoti naują kodą
4. **Deploy → Manage deployments → New deployment**
5. Type: Web app, Execute as: Me, Who has access: Anyone
6. Nukopijuoti naują URL
7. Atnaujinti `app_script_id` app Nustatymuose

---

## 7. Dažnos Klaidos Kompiliuojant

| Klaida | Sprendimas |
|---|---|
| `JAVA_HOME not set` | Android Studio → Settings → SDK → JDK path |
| `Manifest merger failed` | Tikrinti `AndroidManifest.xml` versijų konfliktus |
| `Resource not found` | `npx cap sync android` prieš kompiliavimą |
| `Duplicate class` | `./gradlew clean` tada `assembleDebug` |
| index.html nerodo pakeitimų | Patikrinti MD5 — ar sinchronizuota į assets/public/ |
