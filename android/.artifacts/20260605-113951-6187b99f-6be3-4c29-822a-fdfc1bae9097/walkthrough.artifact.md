# Kemperis App v4.1.1 — Release Versija ir Automatizacija

Šis atnaujinimas orientuotas į programėlės stabilumą, diegimo patogumą ir nuolatinį vystymą.

## Kas pakeista?

### 1. Perėjimas prie Pasirašytos (Release) versijos
- Vietoj laikinos „debug“ versijos dabar naudojama oficiali **Release** versija.
- **Privalumai**: Programėlė veikia sparčiau, Android sistema jos „neužmuša“ fone taip agresyviai, o tai kritiškai svarbu balso pranešimams užgesusiu ekranu.
- Sukonfigūruotas automatinis pasirašymas su saugiu „keystore“ raktu.

### 2. Sinchronizavimo Automatizacija (`build_and_sync.bat`)
- Sukurtas specialus skriptas jūsų kompiuteryje: `C:\Users\ginta\kempervanas\veikiantisKEMPERIS\KemperisApp\build_and_sync.bat`.
- **Ką jis daro?**: Vienu paspaudimu nukopijuoja jūsų atliktus `index.html` pakeitimus į Android projektą, atlieka „npx cap sync“ ir sukompiliuoja galutinį APK failą.
- Dabar jums nebereikės rankiniu būdu rūpintis failų kopijavimu — skriptas viską padarys už jus.

### 3. Versijos Žyma ir Diagnostika
- Sistemos juostoje apačioje pridėta tiksli versijos žyma (pvz., `v4.1.1-build-20260606`). Tai padės išvengti painiavos, ar telefone tikrai įdiegta naujausia kodo versija.
- Dar kartą patikrinta balso sintezės logika — užtikrinta, kad media kanalas būtų valdomas teisingai (rekomenduojame patikrinti, ar jūsų magnetoloje garsas kinta kartu su slankikliu).

## Kaip naudotis?

### Programėlės įrašymas:
1. Atsisiųskite galutinį failą: [KemperisApp_v4.1.1_Release.apk](file:///C:/Users/ginta/AppData/Local/Google/AndroidStudio2026.1.1/projects/android.274930c4/.artifacts/20260605-113951-6187b99f-6be3-4c29-822a-fdfc1bae9097/KemperisApp_v4.1.1_Release.apk)
2. **Ištrinkite seną versiją** (būtina, nes keičiasi pasirašymo raktas iš debug į release).
3. Įsidiekite APK.

### Naujų pakeitimų kūrimas ateityje:
1. Redaguokite `www/index.html`.
2. Paleiskite `build_and_sync.bat` failą.
3. Pasiimkite sugeneruotą APK iš `android\app\build\outputs\apk\release\app-release.apk`.
