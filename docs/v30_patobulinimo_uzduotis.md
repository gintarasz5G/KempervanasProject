# v30 — Patobulinimo užduotis (po audito)

Pagrindinė v30 užduotis (tinklo režimai, WifiLock, autostart, šviežumo juosta) jau
įgyvendinta kode ir commit'inta į git (`0085b7f`). Šis dokumentas — tik likę
neuždaryti punktai iš audito. **Neliesk esamos jutiklių atpažinimo grandinės,
aliarmų ir balso logikos.**

## 1. 🔴 KRITIŠKA: kemperis_v30.apk trūksta repo

`version.json` jau nurodo `apk_url` → `android/kemperis_v30.apk`, bet failo
repo'e nėra (`git ls-files | grep apk` — paskutinis yra `kemperis_v29.apk`).
Bet kurio įrenginio atnaujinimo patikra šiuo metu gaus 404.

Veiksmai:
```
cd android
npx cap sync
cd android && ./gradlew assembleRelease
# rezultatas: android/android/app/build/outputs/apk/release/app-release.apk
cp android/app/build/outputs/apk/release/app-release.apk ../kemperis_v30.apk
cd ..
git add -f kemperis_v30.apk
git commit -m "v30: release APK"
git push
```
Patvirtink:
- `aapt dump badging android/kemperis_v30.apk | grep versionCode` → turi būti 30
- `unzip -p android/kemperis_v30.apk assets/public/index.html | md5sum` turi
  sutapti su `md5sum android/www/index.html` (jei nesutampa — `npx cap sync`
  nebuvo paleistas prieš build'ą, arba build naudojo seną cache).

## 2. 🕒 Šviežumo juostoje trūksta absoliučios laiko žymos

`updateFreshnessBar()` (`android/www/index.html`, ~eilutė 2211) šiuo metu rodo
tik santykinį laiką ("Xs ago" / "X min ago") ir šaltinio žymą (Live/Cloud/Stale).
Pridėk absoliučią laiko žymą greta santykinės, pvz.:

```js
const dt = new Date(lastSyncTime);
const abs = lastSyncTime > 0 ? dt.toLocaleString('lt-LT') : '--';
// rodyti: "2026-07-01 14:32:07 (prieš 12s)"
```

Naudok esamą `lastSyncTime` kintamąjį (jau nustatomas ir `live`, ir `cloud`
šaltiniams) — papildomos logikos sekimui nereikia, tik UI teksto pakeitimas.

## 3. Patikrinti debounce realiame ribos zonos scenarijuje

Esamas 5s slenkstis (`_sseSource.onerror` / SSE watchdog) tikriausiai tinka, bet
paprašyta patikrinti realiomis sąlygomis — jei mirga tarp online/offline WiFi
ribos zonoje, padidink iki 8–10s. Nekeisk, jei realus testas rodo, kad 5s stabilu.

## 4. (Neprivaloma, kosmetika) Trumpesni režimų pavadinimai

`network_mode_select` (index.html ~419-423 eil.) — jei nori, pakeisk:
- „🔌 Tik ESP32 WiFi" → „🔌 Tik Kemperis"
- „☁️ Tik Cloud (Google Sheets)" → „☁️ Tik Debesys"

## Testavimas prieš baigiant (iš originalios užduoties)

1. Telefonas (Automatinis, 4G): SSE nekrenta įkeliant į Google; nueini → debesis
   su laiko žyma; grįžti → gyvi duomenys per 1–2s.
2. Huawei T5 (Tik ESP, Android 8): SSE stabilus ~valandą; atsijungus 🔴 juosta su
   paskutinių duomenų laiku; jokio interneto aktyvumo; app nelūžta.
3. Ekraną užgesinti → WifiLock laiko SSE gyvą.
4. Tik debesis: rodo paskutinę Sheets būklę su laiko žyma; ESP mygtukai pilki.
5. Autostart ON → atsidaro gamintojo nustatymai; leidus + perkrovus → app pati
   pasileidžia.

Kai baigsi — parašyk „tikrink", patikrinsiu: APK versiją/MD5, absoliučią laiko
žymą, ir kad `git status` švarus + push'inta.
