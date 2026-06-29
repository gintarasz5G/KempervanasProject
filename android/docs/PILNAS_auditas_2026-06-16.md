# KemperisApp — PILNAS auditas

**Data:** 2026-06-16
**HEAD:** `c171dfb` (pushintas, `HEAD == origin/main`)
**Apimtis:** web app (`www/index.html`), native Android (`MainActivity.java`, `KemperisService.java`, manifestas), build/deploy grandinė, git istorija/saugumas, versijos. Metodas — statinis kodo + git + logų/nuotraukų pjūvis (be realaus įrenginio).

---

## 0. Vykdomoji santrauka

| Sritis | Būsena |
|--------|--------|
| Web app funkcionalumas (kodas) | 🟢 Geras — pataisymai vietoje, be dublikatų |
| Native sluoksnis | 🟡 1 esminis pataisymas padarytas (WiFi su mobiliais duomenimis), **dar necommit'intas** |
| **Build / Deploy** | 🔴 **Pagrindinė kliūtis** — pataisymai kode, bet neįdiegti į telefoną (senas APK) |
| Saugumas / git | 🔴 Nepakitę — paslaptys HEAD + istorijoje, raktas nerotuotas |
| Versijos | 🟠 Nesutampa (versionCode 13 vs version 14) |

**Esmė:** kodas iš esmės tvarkingas, bet **beveik visos problemos, kurias matai telefone, kyla iš vienos vietos — naujas APK nesukompiliuotas/neįdiegtas.** Visi pataisymai (SOC, arm/disarm, WiFi su mobiliais) yra šaltinyje, bet įdiegtas APK senas.

---

## 1. Web app (funkcinis sluoksnis) — `www/index.html`

**Patvirtinti pataisymai (HEAD, švarūs):**
- ✅ TTS eilė — vienas `speakText`, eilė veikia, vagystė turi prioritetą.
- ✅ XSS — `escapeHtml()` SMS terminale ir loguose.
- ✅ Temperatūros filtras (`!esp_`) — 42°C klaida neberodoma lauko temperatūroje.
- ✅ SOC: `setSensorValue('junc_soc','soc_value'/'soc_d')` + Ah→% atsarga (`ah/cap`, 107.6/110≈98%). `json.soc>0` blokas pašalintas.
- ✅ arm/disarm offline: `saveAdminNumber` + `localStorage['admin_number']` atsarga (be dublikatų).
- ✅ Dublikatų funkcijų nėra; JS sintaksė švari.

**Likę veikimo niuansai (ne klaidos, bet verta):**
- Tinklo bind/unbind dalijasi keli procesai (SSE, Sheets timeris, OTA) — galimas trumpas „offline blyksnis". Sprendimas: vienas „tinklo mutex".
- Aliarmai veikia tik su gyvais ESP duomenimis (offline — tyli). Vertas indikatorius „aliarmai neaktyvūs – nėra ryšio".
- Žemėlapis tik su internetu (Leaflet iš CDN); `www/lib/leaflet.*` tušti (0 B) — klaidina.
- `localStorage` rašomas kas SSE žinutę — našumo/baterijos našta; vertas debounce.

---

## 2. Native Android sluoksnis

### 🔴→✅ NAUJAS pataisymas: WiFi neveikė su įjungtais mobiliais duomenimis
**Priežastis:** `registerAutoWifiCallback()` naudojo `NetworkRequest.Builder()...build()`, kuris **numatytai reikalauja `NET_CAPABILITY_INTERNET`**. ESP32 AP interneto neturi → callback'as jo nepagaudavo → procesas nepririšamas prie WiFi → su mobiliais duomenimis srautas eina per mobilųjį tinklą, ne į 192.168.4.1.
**Pataisymas (padarytas, darbo medyje):** pridėta `.removeCapability(NET_CAPABILITY_INTERNET)`. Dabar callback'as pagauna ESP WiFi, pririša procesą, ir ESP pasiekiamas su mobiliais duomenimis. Tai taip pat įjungia esamus `rebindNetwork()/unbindNetwork()` (jie veikia tik kai `boundNetwork` nustatytas).
**⚠️ Necommit'inta ir neįtraukta į jokį APK** — reikia commit + build.

**Kita native (gerai):**
- `KemperisService` — foreground `specialUse`, START_STICKY; Android 14 atitiktis.
- Tiltai: SMS, TTS (UtteranceProgressListener), Volume, Update (DownloadManager), File (MediaStore), WiFi. `onDestroy` švarus.
- Leidimai: SMS/Phone/Location/Notifications prašomi runtime. Sideload riboja SMS (Play Protect) — sprendžiama per ADB `grant_sms.bat`.

---

## 3. 🔴 Build / Deploy — pagrindinė kliūtis

Tai dažniausia visų „neveikia" priežastis šioje istorijoje:

- Pataisymai (SOC, arm/disarm) **commit'inti** į kodą, bet GitHub `v14` release APK įkeltas **16:59**, o pataisymai — **vėliau** (≥20:xx, 22:36). → GitHub APK **senas**.
- Logų/nuotraukų įrodymas: telefone įtampa/srovė/galia/Ah rodomi teisingai, **tik SOC=0%** — tiksliai senojo kodo požymis (soc_value tekstas nebuvo rašomas). → **įdiegtas senas APK**.
- Build išvesties aplanke (`android/app/build/outputs/apk/release/`) naujo `app-release.apk` **nėra**.
- WiFi pataisymas (2 sk.) — **native**, todėl be perkompiliavimo niekaip nepasieks telefono.

**Vienas build apima viską:** SOC rodymą, arm/disarm offline, WiFi su mobiliais duomenimis.

---

## 4. 🔴 Saugumas ir git — nepakitę (kritinė skola)

| Tikrinta | Būsena |
|----------|--------|
| `kemperis.jks` istorijoje | yra (1 blob, commit `274887b`) |
| `kemperis2026` (build.gradle) istorijoje | **13 commit'ų** |
| Paslaptys HEAD `index.html` | WiFi `kemperis123`, Apps Script ID, telefonai — **yra** |
| `.git` dydis | **107 MB** (build artefaktai istorijoje) |
| Istorijos perrašymas / rakto rotacija | **neatlikta** |
| Repo matomumas | **viešas** |

Kol repo viešas: bet kas turi pasirašymo raktą + slaptažodį (`kemperis2026`) → gali pasirašyti kenkėjišką „atnaujinimą"; Apps Script ID atviras (rašymas/skaitymas į tavo Sheets); SIM numeriai vieši.
**Veiksmai:** `git filter-repo` (keystore, `kemperis2026`, `.esphome/`) + `push --force-with-lease`; rotuoti pasirašymo raktą ir Apps Script ID; paslaptis iš HEAD į `.gitignore` failą — **arba** repo → privatus.

---

## 5. 🟠 Versijos
`version.json`=14, `CURRENT_VERSION`=14, bet `build.gradle` **versionCode 13 / versionName "13"**. Sutvarkyk į 14 (arba 15, jei leisi naują OTA). Pastaba: jei naujas APK liks „v14", OTA jo nepasiūlys esamiems (14 > 14 netiesa) — naujam leidimui kelk į **v15**.

---

## 6. Diagnostika iš 2026-06-16 logų/nuotraukų
- ESP web UI (192.168.4.1) rodo SOC 93%, Ah 101.8 — **duomenys ESP'e yra**. App rodo 0% → senas APK.
- Junctek „D9 nerasta" kartojasi: shuntas siunčia C0 (įtampa) dažnai, D9 (srovė+SOC+Ah) retai → SOC/srovė atnaujinami su pertrūkiais. **Hardware/BLE patikimumas** (ne app).
- OTA „Unable to resolve host raw.githubusercontent.com" — nes ESP WiFi be interneto; tikrinti atnaujinimus galima tik su internetu.

---

## 7. Prioritetinis veiksmų sąrašas

**Dabar (kad telefonas pagaliau gautų pataisymus):**
1. `git commit` native WiFi pataisymą (`MainActivity.java` — `removeCapability`).
2. `build.gradle` → `versionCode 14`, `versionName "14"` (ar 15).
3. `build_and_sync.bat` → naujas `app-release.apk`.
4. Įdiek telefone (jei „App not installed" — pašalink seną, įdiek naują) → `grant_sms.bat`.
5. Patikrink su **įjungtais** mobiliais duomenimis: prisijungia prie ESP, SOC ~93%/98%, arm/disarm veikia.

**Kai būsi pasiruošęs (saugumas):**
6. Istorijos perrašymas + rakto/Script ID rotacija **arba** repo → privatus.

**Kokybė (neprivaloma):**
7. `localStorage` debounce; tinklo mutex; „aliarmai neaktyvūs" indikatorius; žemėlapio offline / tuščių `lib/` valymas.

> Bendras verdiktas: **kodas brandus, bet neišbandytas tikrame įrenginyje su naujausiu build'u.** Kol nesukompiliuosi ir neįdiegsi naujo APK, visi pataisymai lieka tik šaltinyje. Saugumo skola (viešas raktas + paslaptys) — atskira, bet svarbi.
