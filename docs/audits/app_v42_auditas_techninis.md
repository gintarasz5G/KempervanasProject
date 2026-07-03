# App v42 (v33.6) techninis auditas — 2026-07-03

**Bazė:** commit 3230888 / APK `kemperis_v42.apk`.

## 1. Patikros sritys ir rezultatai

| Patikra | Metodas / Kodas | Rezultatas |
|---|---|---|
| **In-App Updater** | Dinaminis vardas (`apkUrl` substring) | ✅ Ištaisyta (`MainActivity.java:486`) |
| **WiFi Binding** | SSID + !VALIDATED logika | ✅ Ištaisyta (`MainActivity.java:240-254`) |
| **Renogy BLE Init** | `androidNeverForLocation` | ✅ Ištaisyta (`renogy.js:68`) |
| **Baterijos SOC** | `Math.min(100, ...)` | ✅ Ištaisyta (`index.html:1571`) |
| **CSV Log Format** | 24 stulpelių headeris + padding | ⚠️ Lokalus app formatas (`index.html:780, 922-927`) |
| **Bluetooth UI** | State notifications + scan cooldown | ✅ Įgyvendinta (`renogy.js:71-76, 271-274`) |

## 2. Techniniai radiniai

- **Updater saugumas**: Pridėta `Settings.ACTION_MANAGE_UNKNOWN_APP_SOURCES` integracija (`MainActivity.java:524-531`). Jei diegimas blokuojamas, programėlė pasiūlo atidaryti nustatymus.
- **Location leidimai**: Manifeste pašalintas `maxSdkVersion="30"` apribojimas `ACCESS_FINE_LOCATION` leidimui (`AndroidManifest.xml:24`). Tai užtikrina SSID matomumą Android 12+ įrenginiuose.
- **Renogy stabilumas**: Framing'as dabar remiasi ilgio baitu (`dev.buffer[2] + 5`). Tai ženkliai sumažina duomenų persidengimo riziką (`renogy.js:236-240`).
- **CSV suderinamumas**: Lokalus žurnalas naudoja 24 stulpelius, kad apimtų Renogy duomenis. Firmware/Sheets kontraktas išlieka 22-23 stulpeliai; app automatiškai papildo (padding) senas eilutes eksporto metu.

## 3. Scenarijų analizė

1. **Atnaujinimas su esamu failu**: Programėlė ištrina seną APK prieš pradedant siuntimą, išvengiant `-1` sufiksų kolizijos (`MainActivity.java:490`).
2. **Bluetooth būsenos kaita**: `startEnabledNotifications` sėkmingai valdo polling'ą priklausomai nuo adapterio būsenos (`renogy.js:71`).
3. **Paslėptas SSID**: Diagnostikos kortelėje pridėtas mygtukas nukreipia tiesiai į App nustatymus leidimui suteikti (`index.html:366`).

## 4. Verdiktas

v42 (v33.6) yra **stabiliausia programėlės (App) versija** iki šiol. Visi anksčiau nustatyti kritiniai blokatoriai (Update lūžiai, SSID nebuvimas, Renogy inicializacija) sėkmingai pašalinti.

⚠️ **Pastaba**: Nors funkcinis stabilumas pasiektas, išlieka **P0 saugumo skola** (autorizacijos trūkumas SMS/API lygiu, OTA slaptažodžiai), kurią būtina spręsti sekančiuose etapuose.

---
*Audito data: 2026-07-03*
*Versija: v42*
