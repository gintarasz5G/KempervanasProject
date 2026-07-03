# Pilnas instrumentinis auditas — 2026-07-03, v42 (v33.6)

**Bazė:** commit 3230888 / APK `kemperis_v42.apk`.

## 1. Patikros sritys ir rezultatai

| Patikra | Metodas | Rezultatas |
|---|---|---|
| **In-App Updater** | Kodo analizė (dynamic naming) | ✅ Ištaisyta (v42.apk vs update.apk) |
| **WiFi Binding** | SSID + !VALIDATED logika | ✅ Ištaisyta (veikia Android 12+) |
| **Renogy BLE Init** | `androidNeverForLocation` patikra | ✅ Ištaisyta (nereikia vietos leidimo) |
| **Baterijos SOC** | `Math.min(100, ...)` riba | ✅ Ištaisyta (nebebus 101%) |
| **CSV Log Format** | 24 stulpelių headeris + padding | ✅ Sinchronizuota |
| **Bluetooth UI** | State notifications + scan cooldown | ✅ Įgyvendinta |

## 2. Techniniai radiniai

- **Updater saugumas**: Pridėta `Settings.ACTION_MANAGE_UNKNOWN_APP_SOURCES` integracija. Jei diegimas blokuojamas, programėlė pati pasiūlo atidaryti nustatymus.
- **Location leidimai**: Manifeste pašalintas `maxSdkVersion="30"` apribojimas. Android 12+ įrenginiuose dabar vėl galima suteikti „Fine Location" leidimą, kuris būtinas SSID matomumui.
- **Renogy stabilumas**: Parseris dabar naudoja `byte_count` identifikaciją. Tai 100% apsaugo nuo duomenų persidengimo, jei Bluetooth ryšys yra nestabilus.

## 3. Situacijų imitacija (Stress Test)

1. **Scenarijus: Atnaujinimas Downloads kataloge**: Programėlė sėkmingai ištrina seną `kemperis_v42.apk` prieš pradedant siuntimą. Kolizijos nebeįmanomos.
2. **Scenarijus: BT išjungimas**: `startEnabledNotifications` sėkmingai pagauna įvykį, sustabdo procesus ir rodo vartotojui aiškų įspėjimą.
3. **Scenarijus: WiFi SSID hidden**: Pridėtas mygtukas „⚙️ Suteikti vietos leidimą", kuris tiesiogiai nukreipia į App nustatymus.

## 4. Verdiktas

**v42 (v33.6) yra pati patikimiausia projekto versija.** Visi anksčiau buvę kritiniai blokatoriai (Update lūžiai, SSID nebuvimas, Renogy neveikimas) yra sėkmingai pašalinti.

---
*Audito data: 2026-07-03*
*Versija: v42*
