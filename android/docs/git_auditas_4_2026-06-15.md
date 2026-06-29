# KemperisApp — git auditas (4-as pjūvis, po pataisymų)

**Data:** 2026-06-15 (vėlus vakaras)
**Repozitorija:** https://github.com/gintarasz5G/kemperis-app (vieša)
**Naujausi commit'ai nuo praėjusio audito:** `fee34ab` „Security Hardening & Update Fix", `c8c51e3` „CRITICAL FIX: Update URL corrected (V14 → v14) and duplicate removed". Abu **pushinti** (`HEAD == origin/main`).

---

## ✅ PATVIRTINTA IŠTAISYTA

| Radinys | Būsena | Įrodymas |
|---------|--------|----------|
| **N1 — OTA atnaujinimas** | ✅ Išspręsta | Šaknies `version.json` → `apk_url` dabar `.../download/`**`v14`**`/...`; dublis `KemperisApp/version.json` pašalintas. v13 turėtų atsinaujinti (palaukus raw CDN kešo). |
| **H2 — dvigubas `speakText`** | ✅ Išspręsta | HEAD liko **1** apibrėžimas (eil. 2480), naudojama eilės (`_processSpeechQueue`) versija → aliarmai dabar rikiuojami, nebemetami. |
| **H3 — XSS per innerHTML** | ✅ Išspręsta | `sysLog` (eil. 602) ir SMS terminalas (eil. 1094) dabar naudoja `escapeHtml(msg)`. |
| **Prevencija ateičiai** | ✅ Pridėta | Šaknies `.gitignore` turi `.esphome/`, `*.apk`, `*.jks`, `keystore.properties`. |

Tai realiai sutvarko funkcinę atnaujinimo problemą (pagrindinį tavo klausimą) ir du kodo kokybės/saugos defektus. Gerai padaryta.

---

## ❌ DAR NEIŠSPRĘSTA (saugumas) — „Security Hardening" buvo nepilnas

Commit'as `fee34ab "Security Hardening"` realiai pridėjo daugiausia `.idea/` IDE failus, audito dokumentą ir **nebaigtą** keystore.properties karkasą. Esminės paslapčių problemos liko:

### 🔴 GC1 — keystore ir slaptažodis vis dar viešame repo (nepakitę)
1. **`build.gradle` vis dar turi įrašytus slaptažodžius atviru tekstu:**
   ```
   storePassword "kemperis2026"   // eil. 15
   keyPassword   "kemperis2026"   // eil. 17
   ```
   Pridėtas `keystoreProperties` įkėlimo blokas (eil. 1–4), **bet jis nenaudojamas** — `signingConfig` toliau ima literalus. Migracija pradėta, bet nebaigta → slaptažodžiai liko.
2. **`KemperisApp/keystore/kemperis.jks` vis dar sekamas** (HEAD) **ir istorijoje** (commit `274887b`).
   - Svarbu: `.gitignore` su `*.jks` **NEpašalina jau sekamo failo** — git ignoravimas galioja tik nesekamiems failams. Failą reikia atskirai išregistruoti.

**Užbaigimo žingsniai:**
```bash
# 1. build.gradle — pakeisk literalus į properties:
#   storePassword keystoreProperties['storePassword']
#   keyPassword   keystoreProperties['keyPassword']
#   storeFile     file(keystoreProperties['storeFile'])
# 2. sukurk keystore.properties (jis jau .gitignore) su realiomis reikšmemis
# 3. išregistruok keystore iš sekimo:
git rm --cached KemperisApp/keystore/kemperis.jks
git commit -m "Remove keystore + passwords from tracking"
```

### 🔴 Istorija vis dar neperrašyta
`.git` tebėra **107 MB**. Visi seni failai ir paslaptys lieka viešai pasiekiami istorijoje, nepriklausomai nuo HEAD:
- `kemperis.jks` (commit `274887b`),
- `kemperis2026` (build.gradle istorijoje),
- `kemperis123`, Apps Script ID, telefonai (index.html / YAML istorijoje).

`git rm` ir `.gitignore` jų **nepašalina**. Būtinas vienkartinis istorijos perrašymas:
```bash
git filter-repo --invert-paths \
  --path KemperisApp/keystore/kemperis.jks \
  --path-glob '.esphome/*' \
  --path-glob '2026-05-17/*'
git push --force-with-lease origin main
```
**Po to būtina rotuoti:** sugeneruoti naują pasirašymo raktą (su nauju slaptažodžiu) ir naują Apps Script `/exec` ID. Įspėjimas (kaip ir anksčiau): pakeitus pasirašymo raktą, esami įrenginiai turės vieną kartą rankiniu būdu perdiegti.

### 🟠 GH1 — paslaptys index.html (HEAD, nepakitę)
WiFi slaptažodis `kemperis123` (eil. 514), Apps Script ID (eil. 2051), telefonai `+37065758821` / `+37064730025` (eil. 2052–2053) vis dar kode.

### 🟠 N3 — ESPHome YAML AP slaptažodis (HEAD, nepakitę)
`ESPHome/kempervanas_v14_final.yaml`: `password: "kemperis123"`. Perkelti į `secrets.yaml` (su `!secret`), kurį įtraukti į `.gitignore`.

---

## Santrauka

**Funkcinė dalis ir kodo kokybė — sutvarkyta** ✅ (OTA atnaujinimas, TTS eilė, XSS escape, prevencinis `.gitignore`).

**Saugumo dalis — vis dar atvira** ❌. „Security Hardening" commit'as nepašalino nei keystore, nei slaptažodžių; `.gitignore` saugo tik nuo *būsimų* nutekėjimų, bet jau viešai esančios paslaptys lieka.

### Likę darbai (prioriteto tvarka)
1. **`build.gradle`** — baik keystore.properties migraciją (pašalink literalus `kemperis2026`).
2. **`git rm --cached`** keystore.jks + commit.
3. **Istorijos perrašymas** (`filter-repo`) — pašalink keystore, `.esphome/`, senus failus.
4. **Rotuok** pasirašymo raktą ir Apps Script ID (nes buvo viešai).
5. **index.html / YAML** — iškelk WiFi/Script ID/telefonus iš kodo; ESPHome slaptažodžius į `secrets.yaml`.
