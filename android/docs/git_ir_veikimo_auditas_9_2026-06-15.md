# KemperisApp — git + veikimo auditas (9-as pjūvis)

**Data:** 2026-06-15
**HEAD:** `f9e87ac` „ULTIMATE MASTER BUILD – Global Logic & Security Certification" (pushintas, `HEAD == origin/main`)

---

## 1. Ką realiai pakeitė `f9e87ac`

| Failas | Pakeitimas |
|--------|-----------|
| `www/index.html` | −10 eil. = mano dublikatų (`saveAdminNumber`, dvigubas sėjimas) valymas |
| `docs/git_ir_veikimo_auditas_8…md` | +72 eil. (audito dokumentas) |

**Išvada:** commit'as **kosmetinis** — įtraukė dublikatų valymą + dokumentą. Pavadinime esanti „Security Certification" **neatitinka turinio** — jokio saugumo darbo neatlikta (žr. 3 sk.).

---

## 2. ✅ Kodo (veikimo) sveikata — gera
- **Dublikatų funkcijų NĖRA** (patikrinta visa `index.html`) — seni `speakText` ir `saveAdminNumber` dublikatai išspręsti.
- **JS sintaksė švari** (`node --check`).
- `window.onSmsReceived` / `onUpdateResult` — po 1; `onTtsDone` priskiriamas dinamiškai (norma).
- SOC tekstas (`soc_value`/`soc_d`) ir arm/disarm offline atsarga (`localStorage['admin_number']`) — vietoje.

---

## 3. 🔴 Saugumas — NEPAKITĘ (nepaisant „Security Certification" pavadinimo)

| Tikrinta | Būsena |
|----------|--------|
| `kemperis.jks` istorijoje | yra (1 blob, commit `274887b`) |
| `kemperis2026` (build.gradle) istorijoje | **13 commit'ų** |
| Paslaptys HEAD `index.html` | WiFi `kemperis123`, Script ID, telefonai — **vis dar yra** |
| `.git` dydis | **107 MB** (istorija neperrašyta) |
| Pasirašymo raktas / Script ID rotacija | **neatlikta** |

Realiai „sertifikuoti" saugumą reikštų: `git filter-repo` (pašalinti keystore + `kemperis2026` + `.esphome/` iš istorijos), rakto ir Apps Script ID rotacija, paslaptys iš HEAD į `.gitignore` failą — **arba** repo padaryti privatų. Kol to nėra, viešas repo atskleidžia kredencialus ir nerotuotą pasirašymo raktą.

---

## 4. 🟠 Versijų nesutapimas — NEPAKITĘ
`version.json` = 14, `CURRENT_VERSION` = 14, bet `build.gradle` **`versionCode 13` / `versionName "13"`**. Sutvarkyk į 14, kad sistemoje rodytų teisingą versiją.

---

## 5. Veikimo niuansai — nepakitę (iš ankstesnių pjūvių)
Tinklo bind/unbind lenktynės; aliarmai tik su gyvais ESP duomenimis; žemėlapis tik su internetu (tušti `lib/leaflet.*`); `localStorage` rašymas kas SSE žinutę.

---

## Verdiktas
**Kodo kokybė / stabilumas:** 🟢 gera — be dublikatų, taisymai vietoje.
**Saugumas:** 🔴 „Certification" pavadinimas pralenkia tikrovę — nieko nepasikeitė nuo 5-o pjūsio.

### Liko 2 konkretūs darbai
1. `build.gradle` → `versionCode 14`, `versionName "14"`.
2. Saugumas: istorijos perrašymas + rakto/Script ID rotacija **arba** repo → privatus.

> Funkcionalumui (SOC, arm/disarm) — nepamiršk perkompiliuoti APK + įdiegti iš naujo + `grant_sms.bat`, kitaip telefone pakeitimų nebus.
