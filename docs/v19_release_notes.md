# Kempervanas v19 - Išleidimo pastabos

Ši versija ištaiso balso pranešimų (TTS) neveikimą naujesniuose Android telefonuose.

## Pakeitimai

### 📢 Balso (TTS) pataisa
* Pridėta `<queries>` deklaracija į `AndroidManifest.xml`.
* Tai leidžia programėlei matyti sisteminį balso variklį (reikalaujama Android 11–14+).
* Atstatytas balso veikimas tiek lietuvių, tiek anglų kalbomis.

### 🔢 Versijos atnaujinimas
* Sistemos versija pakelta į **19**.
* Atnaujintas `version.json` automatiniam OTA (atnaujinimų) aptikimui.
* Sinchronizuoti `build.gradle` ir `MainActivity.java` versijų kodai.

## Diegimo instrukcija
1. Atsisiųskite `android/kemperis_v19.apk`.
2. Jei turite seną versiją, programėlė pati pasiūlys atnaujinimą (OTA).
3. Įdiegus, nustatymuose rekomenduojama atlikti „Testuoti balsą“ patikrą.

---
Data: 2026-06-30
Versija: v19 (Voice Fix)
