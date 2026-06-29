# KemperisApp — git + veikimo auditas (8-as pjūvis)

**Data:** 2026-06-15
**HEAD:** `a6fbe68` „Claude Fixes: SOC numeric display & Admin number persistence" (pushintas, `HEAD == origin/main`)
**Prieš auditą perskaityta:** `docs/veikimo_auditas_7_2026-06-15.md`

---

## 1. 🟡 NAUJAS radinys — dublikatai (sukurti suliejant taisymus)

Praeitą kartą **tą pačią** klaidą taisėme abu — tu rankiniu būdu IR aš. Commit'e `a6fbe68` abu taisymai įsiliejo, todėl atsirado dublikatai (tas pats defektų tipas kaip senasis dvigubas `speakText`):

- **Dvi `function saveAdminNumber(v)`** — viena ~1197 eil. (tavo, su `val` + sysLog pranešimu), kita ~1870 eil. (mano, su `t`). Vėlesnė nustelbdavo ankstesnę.
- **Dvigubas admin_number sėjimas `init()`** — eilutėse ~2076 ir ~2086.

**Poveikis:** funkciškai nekenkė (abi versijos identiškos), bet tai negyvas/painus kodas.

**✅ Ištaisyta dabar:** palikau tavo `saveAdminNumber` (su geresniu pranešimu), pašalinau savo dublikatą ir dvigubą sėjimą. Liko po 1. JS sintaksė patikrinta — švari.

> Rekomendacija ateičiai: kai prašai manęs taisyti, **netaisyk tuo pačiu metu rankiniu būdu** to paties — kitaip gaunami dublikatai. Arba pasakyk, kad jau pataisei.

---

## 2. ✅ Patvirtinta — praeiti taisymai veikia HEAD'e
- **SOC tekstas:** `setSensorValue('junc_soc','soc_value'…)` ir `'soc_d'` yra (1646–1647 eil.). Skaičius dabar rodys realų %.
- **arm/disarm offline:** sauga turi atsarginį `localStorage['admin_number']`; `init()` užsėja numatytąjį `+37064730025`. Po naujo build'o veiks offline.
- Kiti rodikliai (vanduo, dujos, GSM) turi ir teksto, ir juostos atnaujinimą — SOC buvo vienintelis praleistas atvejis.

---

## 3. 🟠 NAUJAS — versijų nesutapimas (verta sutvarkyti prieš leidžiant)
| Šaltinis | Reikšmė |
|----------|---------|
| `version.json` (OTA) | **14** |
| `MainActivity.CURRENT_VERSION` | **14** |
| `build.gradle versionCode` | **13** ❌ |
| `build.gradle versionName` | **"13"** ❌ |

OTA aptikimas veikia (lygina JSON `version` su `CURRENT_VERSION`, abu 14), bet pats APK turi Android `versionCode 13`. Pasekmės: sistemos nustatymuose programėlė rodoma kaip „13"; jei kada nors remsiesi `versionCode` — naujesnė versija neatpažinta.
**Taisymas (`build.gradle`):** `versionCode 14`, `versionName "14"` — suderink su likusiais.

---

## 4. 🔴 Saugumas / git — NEPAKITĘ (kritinė skola lieka)
| Tikrinta | Būsena |
|----------|--------|
| `.git` dydis | **107 MB** (build artefaktai istorijoje) |
| `kemperis.jks` istorijoje | yra (1 blob, commit `274887b`) |
| `kemperis2026` istorijoje | **13 commit'ų** `build.gradle` |
| Paslaptys HEAD `index.html` | **vėl yra** (a05c863 „Restored Defaults"): WiFi `kemperis123`, Script ID, telefonai |
| Istorijos perrašymas / rakto rotacija | **neatlikta** |

Tai nepasikeitė nuo 5–6 pjūvio. Kol repo viešas, šios paslaptys (HEAD + istorija) ir nerotuotas pasirašymo raktas lieka atviri. Veiksmai (ankstesnėse ataskaitose): `git filter-repo`, rakto + Script ID rotacija, paslaptys į `.gitignore` failą arba repo → privatus.

---

## 5. Veikimo niuansai — nepakitę (iš ankstesnių pjūvių)
- Tinklo bind/unbind lenktynės (vienas „tinklo mutex" išspręstų).
- Aliarmai veikia tik su gyvais ESP duomenimis (offline — tyli; vertas indikatorius).
- Žemėlapis tik su internetu; tušti `lib/leaflet.*`.
- `localStorage` rašymas kas SSE žinutę (debounce neįdiegtas).

---

## Santrauka ir žingsniai
**Kodo kokybė:** dublikatai sutvarkyti; SOC + arm/disarm taisymai vietoje ✅.
**Liko padaryti:**
1. `build.gradle` → `versionCode 14`, `versionName "14"`.
2. **Perkompiliuoti APK + įdiegti iš naujo + paleisti `grant_sms.bat`** (kitaip telefone pakeitimų nebus).
3. Saugumas (kai būsi pasiruošęs): istorijos perrašymas + rakto/Script ID rotacija arba repo → privatus.

> Dėmesio: dabar `www/index.html` turi nekomitintų pakeitimų (dublikatų valymas). Po build'o nepamiršk `git commit`.
