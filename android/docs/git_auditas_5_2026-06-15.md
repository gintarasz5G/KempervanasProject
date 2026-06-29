# KemperisApp — git auditas (5-as pjūvis, po „HARDENED" commit'o)

**Data:** 2026-06-15 (naktis)
**Naujas commit'as:** `248b278` „HARDENED: Removed plain-text passwords and untracked keystore" (pushintas, `HEAD == origin/main`).

---

## ✅ KĄ IŠTAISĖ `248b278` (HEAD dabar švarus)

| Buvęs radinys | Būsena HEAD'e | Įrodymas |
|---------------|---------------|----------|
| GC1 — slaptažodžiai `build.gradle` | ✅ Pašalinti | Dabar `storePassword keystoreProperties['storePassword']` ir t. t. — jokių `kemperis2026`. |
| GC1 — `kemperis.jks` sekamas | ✅ Išregistruotas | `git show 248b278`: `kemperis.jks Bin 2792 → 0 bytes` (pašalintas iš HEAD). |
| GH1 — paslaptys `index.html` | ✅ Pašalintos | `kemperis123` → `''`, Script ID → `''`, abu telefonai → `''`. |

Tai teisingi, geri pakeitimai — **naujas kodas nutekėjimų nebeturi**, o `keystore.properties` + `.gitignore` užtikrina, kad ateityje nepatektų.

---

## ❌ KAS DAR LIEKA — pašalinimas HEAD'e ≠ paslapties panaikinimas

> Esminis dalykas: kadangi repo **viešas**, paslapties pašalinimas tik iš HEAD jos **nepanaikina** — ji ir toliau atsisiunčiama iš istorijos. Tai patvirtinta.

### 🔴 1. Istorija VIS DAR neperrašyta
- `.git` tebėra **107 MB**.
- Commit'as `274887b` egzistuoja → **istorija neperrašyta**.
- `kemperis.jks` blobas vis dar pasiekiamas istorijoje (1 kopija).
- `kemperis2026` (`build.gradle`) randamas **10-yje senesnių commit'ų** (`92200b8` … `c8c51e3`).
- Apps Script ID ir telefonai randami senesnių commit'ų `index.html`.

**Privaloma** (vienkartinis perrašymas + jėgos push):
```bash
git filter-repo --invert-paths \
  --path KemperisApp/keystore/kemperis.jks \
  --path-glob '.esphome/*' \
  --path-glob '2026-05-17/*'
# build.gradle slaptažodžiams istorijoje pašalinti:
git filter-repo --replace-text <(echo 'kemperis2026==>REMOVED')
git push --force-with-lease origin main
```

### 🔴 2. Rotacija (nes buvo viešai prieinama)
Net ir perrašius istoriją, šios paslaptys jau buvo viešos — laikyk jas kompromituotomis:
- **Sugeneruok naują pasirašymo raktą** (naujas slaptažodis). ⚠️ Esami įrenginiai turės vieną kartą rankiniu būdu perdiegti.
- **Perdiek Apps Script** su nauju `/exec` ID; pridėk token tikrinimą.
- Pakeisk WiFi AP slaptažodį `kemperis123`.

### 🟠 3. Vienas HEAD'e likęs slaptažodis — ESPHome YAML
`ESPHome/kempervanas_v14_final.yaml` (eil. 35): `password: "kemperis123"` — **dar nepašalintas net iš HEAD**. Perkelk į ESPHome `secrets.yaml` (su `!secret`) ir įtrauk `secrets.yaml` į `.gitignore`.

---

## Santrauka

| Sritis | Būsena |
|--------|--------|
| Funkcija (OTA atnaujinimas) | ✅ Sutvarkyta (ankstesnis pjūvis) |
| Kodo kokybė (TTS eilė, XSS) | ✅ Sutvarkyta |
| Naujo kodo higiena (HEAD paslaptys) | ✅ Beveik — liko ESPHome YAML AP slaptažodis |
| **Viešas paslapčių nutekėjimas (istorija)** | ❌ **Neišspręsta** — istorija neperrašyta, raktas/Script ID nerotuoti |

**Liko 3 darbai:** (1) ESPHome YAML slaptažodį į `secrets.yaml`; (2) perrašyk istoriją (`filter-repo`) — keystore, `.esphome/`, `kemperis2026`; (3) rotuok pasirašymo raktą ir Apps Script ID. Po to repo bus realiai saugus, o `.git` sumažės nuo 107 MB iki kelių MB.
