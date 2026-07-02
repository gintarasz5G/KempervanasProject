# Užduotis: ESP WiFi bind pataisymas (v35+ „nepersijungia į ESP/Auto")

**Data:** 2026-07-02 · **Simptomas:** app nepersijungia į ESP režimą, Auto režime
neprisiriša prie kemperio WiFi, duomenys neatsinaujina.

## Diagnozė (kodo analizė)

Tinklo prisirišimo logika — `MainActivity.registerAutoWifiCallback()` (~227–259 eil.):
callback'as bind'ina procesą prie WiFi tinklo **tik jei** tinklas neturi
`NET_CAPABILITY_INTERNET`:

```java
if (caps.hasTransport(TRANSPORT_WIFI) && !caps.hasCapability(NET_CAPABILITY_INTERNET)) {
    boundNetwork = network;
    cm.bindProcessToNetwork(network);
```

**Problema:** daugelyje Android versijų nevaliduotas tinklas (ESP AP be interneto)
`NET_CAPABILITY_INTERNET` **TURI** (capability reiškia „deklaruoja internetą") — jam
trūksta tik `NET_CAPABILITY_VALIDATED`. Tokiu atveju sąlyga niekada nesuveikia →
`boundNetwork` lieka `null` → `rebindNetwork()` yra no-op → SSE eina per mobilų tinklą,
kur `192.168.4.1` nepasiekiamas → „neatnaujina duomenų". „ESP Only" mygtukas tik
kviečia `startSSE()`, kuris be bind'o tyliai fail'ina (watchdog amžinai retry'ina).

Papildomai patikrinta: `_netOrchestrationLock` NE kaltas (`netUnbindForInternet()`
niekur nebekviečiamas, `startSSE()` pradžioje `netRebindEsp()` atrakina);
`autoBindPaused` — dead code (niekur nenustatomas).

**Antra galima priežastis (patikrinti planšetėje):** Android OS lygiu iš viso
neprisijungęs prie `Kemperis-Valdymas` — Android meta no-internet WiFi, jei
prisijungiant nepasirinkta „likti prisijungus". App pats prie WiFi nesijungia.

## Gyvo logo patvirtinimas (2026-07-02 13:03, planšetė)

Vartotojo terminalo logas patvirtina diagnozę ir prideda radinį:

- `12:11:53 Neprisijungta prie Kemperis-Valdymas — bandoma prisijungti...` — planšetė
  **OS lygiu neprisijungusi** prie AP per visą sesiją; „Pririšta prie Kemperio AP"
  (bind Toast/log) nepasirodo NĖ KARTO.
- `rebindNetwork calling...` kartojasi dešimtis kartų — no-op, nes `boundNetwork == null`.
- SSE klaidos #1–#3 + watchdog ciklas; Sheets fallback ir update check veikia
  (internetas per mobiliuosius yra).

**Naujas radinys — `suggestWifi` nepakankamas:** app bando jungtis per
`WifiNetworkSuggestion` API, bet Android'e suggestion (a) pirmą kartą reikalauja
vartotojo patvirtinimo per sisteminį pranešimą, (b) prie AP be interneto jungiasi
nenoriai arba visai nesijungia, (c) nieko negrąžina — todėl „bandoma prisijungti"
atrodo kaip veiksmas, bet realiai nieko neįvyksta. **Papildomas taisymas:** kai SSID
nesutampa, vietoj tylaus suggestWifi rodyti aiškią UI juostą „Prisijunkite prie
Kemperis-Valdymas WiFi" su mygtuku, atidarančiu WiFi nustatymus
(`Settings.ACTION_WIFI_SETTINGS` per native bridge), ir instrukcija pasirinkti
„Likti prisijungus" kai Android klaus apie internetą.

## Greitas patikrinimas vartotojui (prieš kodo taisymą)

1. Planšetės WiFi nustatymuose prisijungti prie `Kemperis-Valdymas` ir, kai Android
   klaus „šis tinklas neturi interneto", pasirinkti **„Likti prisijungus"**.
2. **Laikinai išjungti mobiliuosius duomenis** — tada WiFi tampa default maršrutu ir
   viskas turi veikti BE bind'o. Jei veikia — diagnozė patvirtinta (bind sąlygos bug'as).

## Taisymas (MainActivity.java)

1. `onAvailable` sąlygą pakeisti į patikimą:
   ```java
   boolean isEsp = false;
   if (caps.hasTransport(NetworkCapabilities.TRANSPORT_WIFI)) {
       // 1) pagal SSID (patikimiausia; getCurrentSSID jau yra, FINE_LOCATION prašoma)
       String ssid = /* WifiManager SSID, be kabučių */;
       if ("Kemperis-Valdymas".equals(ssid)) isEsp = true;
       // 2) fallback: nevaliduotas WiFi (SSID gali būti <unknown ssid> be location)
       else if (!caps.hasCapability(NetworkCapabilities.NET_CAPABILITY_VALIDATED)) isEsp = true;
   }
   if (isEsp) { boundNetwork = network; cm.bindProcessToNetwork(network); ... }
   ```
   SSID lyginti iš `txt` konstantos arba SharedPreferences (ne hardcode dviejose vietose).
2. `onCapabilitiesChanged` override taip pat reikalingas: VALIDATED būsena gali
   pasikeisti PO `onAvailable` — perbind'inti ten pat.
3. UI diagnostika (index.html Tinklo kortelė): rodyti `getCurrentSSID()` ir bind
   būseną („Pririšta prie AP: taip/ne") — kad kitą kartą matytųsi iš karto.
   „ESP Only" pasirinkus, kai bind'o nėra — aiškus pranešimas
   „Planšetė neprijungta prie Kemperis-Valdymas WiFi", ne tyla.
4. Smoke-test scenarijai: (a) mobilieji ON + WiFi AP prijungtas → SSE gyvas;
   (b) mobilieji OFF → SSE gyvas; (c) WiFi atjungtas → Auto perjungia į cloud,
   grįžus — atgal (Toast „Pririšta prie Kemperio AP" turi pasirodyti).

## Apimtis

Tik MainActivity.java + maža index.html UI diagnostika. Versija pagal grandinę
(kartu su Renogy capacitor.js taisymu — žr.
`docs/audits/verifikacija_v36_renogy_2026-07-02.md` viršų).
