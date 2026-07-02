# Gyvo testo analizė — 2026-07-02 21:31–21:38 (v40)

**Artefaktai:** terminalo logas (499 eil.), dienos CSV (19 eil.), GPX (19 taškų).

## 🎉 Pagrindinė pergalė: SSE GYVAI PATVIRTINTA

Ilgai laukta v32.2 SSE fix'o gyva verifikacija — **veikia**: `[SSE BRANCH]` logai dega,
kortelės pildosi realiomis reikšmėmis: SOC 100 %, 13.26 V, vanduo 58.6 %, dujos 92 %,
GPS koordinatės + palydovai (NAVSTAR 16, BeiDou 9, Galileo 0), GSM teledema/Bitė,
pokrypis, barometrinis aukštis. **Galima vykdyti seną atidėtą darbą: šalinti laikiną
`[SSE BRANCH]` diagnostiką** (ji savo misiją atliko).

Taip pat gyvai patvirtinta: **WiFi bind veikė** (21:33:05 „Kemperis WiFi disconnected"
įvykis įrodo, kad iki tol buvo surišta ir `onLost` kelias veikia), CSV/GPX eksportai
veikia, nauji ren_ stulpeliai rašomi.

## Gyvi `[SSE UNMATCHED]` (galutinis realus sąrašas)

| object_id | Svarba |
|---|---|
| `text_sensor_sistema_modemo_busena` | 🟠 NAUJAS (v35 simuliacija nematė) — Sistema kortelė liks tuščia |
| `text_sensor_sistema_paskutine_klaida` | 🟠 NAUJAS — diagnostikos kortelė liks tuščia |
| `text_sensor_gps_fix_statusas` | 🟡 žinomas, nerodomas — ok |
| `sensor_energija_akum_total_ah` | 🟡 žinomas, ok |
| `sensor_gsm_signalas_dbm_` | 🟡 žinomas (% veikia), ok |

## 🔴 Nauji defektai (v33.5 taisymui)

**D1. CSV header nepapildytas.** `localLogHeader` (index.html 780) liko 17 pavadinimų,
o eilutės dabar 24 laukų — Excel'yje ren_ stulpeliai be antraščių, o R5 padding
lygina su per trumpu header'iu. **Taisymas:** header +
`,Ren SOC,Ren V,Ren A,Ren Temp,DCC Saule W,DCC Gen W,DCC Stadija`.

**D2. SSE šakos:** pridėti `modemo_busena` ir `paskutine_klaida` mapinimą (jei tos
kortelės rodomos UI; jei ne — nedaryti).

**D3. ~~WiFi dingo 21:33~~ — NE KLAIDA (vartotojo patikslinimas):** vartotojas nuėjo nuo
kemperio — ryšys dingo natūraliai, app teisingai perėjo į offline. Vienintelis liekantis
darbas — D4 (fetch spam be backoff, kai ryšio nėra).

**D6 (NAUJAS — Renogy tylos priežastis, rasta kodo analize):** vartotojas BANDĖ Renogy
tab'ą — nepavyko be jokių logų, nors originali Renogy programėlė jungiasi (BT tvarkingas).
Priežastis: `renogy.js` kviečia `BleClient.initialize()` **be parametrų** →
`androidNeverForLocation` default `false` → plugin'as Android 12+ prašo vietos leidimo
runtime → manifest'e `ACCESS_FINE_LOCATION` deklaruotas tik `maxSdkVersion=30` →
leidimas akimirksniu DENIED → `initialize()` lūžta → visos tolesnės klaidos matomos
tik su debug jungikliu → tab'as tyliai miręs.
**Taisymas:** `BleClient.initialize({ androidNeverForLocation: true })` (atitinka
manifest'ą) + Renogy klaidas rodyti UI būsenos eilutėje, ne tik debug loge.

**D4. Config fetch spam:** dingus ryšiui, 17× `num_*` klaidos kartojasi kas ~3 s —
loadAllNumbers retry be backoff. Pridėti: jei SSE offline — nekartoti number fetch'ų.

**D5. Renogy gyvo testo NEBUVO** — jokių `[Renogy]` logų, CSV ren_ reikšmės 0.
Renogy tab'as dar netestuotas su tikrais įrenginiais (scan → priskyrimas → reikšmės).

**A1 (firmware, žinomi):** `junc_hex` INFO flood gyvai matomas (K1 — vis dar shipintas);
`D9 nerasta` — Junctek temperatūros žymeklis neateina (temp kortelė gali būti pasenusi);
`Paskutine klaida: HTTP POST klaida (rysys/serveris)` — Sheets upload buvo lūžęs bent kartą.

**A2 (anomalija):** CSV vakaro eilutėse SOC=101 (firmware riboja iki 100) — išsiaiškinti,
iš kur app'e 101 (getCache šaltinis/užapvalinimas).

## Būsena po testo

| Kas | Būsena |
|---|---|
| SSE gyva verifikacija (laukė nuo v32.2) | ✅ **PATVIRTINTA** |
| WiFi bind (v33.2 fix) | ✅ veikė; D3 — OS lygio stabilumas |
| CSV/GPX eksportai | ✅ veikia (D1 header kosmetika) |
| Renogy BLE gyvas testas | ⏳ dar neatliktas (D5) |
| `[SSE BRANCH]` šalinimas | 🟢 galima daryti dabar |
| P0 sauga | ❌ vis dar atvira |

*Analizė: 2026-07-02 vėlyvas vakaras, pagal vartotojo gyvo testo artefaktus.*
