# Kempervanas v22 - Išsamios diagnostikos atnaujinimas

Ši versija skirta giliam klaidų ieškojimui (ypač dėl balso problemų) ir sistemos stabilumui.

## Naujovės

### 🔍 Išsamus žurnalas (Full Verbose Logging)
* Nustatymuose pridėtas jungiklis **„Išsamūs veiksmai (Debug)“**.
* Jį įjungus, „Logų“ skiltyje matysite **kiekvieną** programėlės žingsnį:
    - SSE duomenų gavimą iš ESP32.
    - Balso eilės (`SpeechQueue`) būsenas.
    - Ryšio su Android native sluoksniu užklausas.
    - Atnaujinimų tikrinimo detales.
* Pridėtas mygtukas **„Išvalyti žurnalą“**, kad būtų patogiau stebėti naujus įvykius.

### 📢 Balso ir Garso stabilumas
* Programėlė dabar priverstinai bando atkurti garso kontekstą (`AudioContext`) kiekvieno vartotojo paspaudimo metu.
* Patobulinta `KempTts` jungtis Android pusėje su papildomu klaidų registravimu.

### 🔢 Versijos atnaujinimas
* Versija pakelta į **22**.
* Atnaujintas `version.json` metaduomenų failas.

---
Data: 2026-06-30
Versija: v22 (Verbose Logging & Diagnostics)
