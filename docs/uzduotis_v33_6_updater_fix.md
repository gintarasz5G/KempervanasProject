# Užduotis v33.6 (v42): in-app updater taisymas + location leidimo grąžinimas

**Data:** 2026-07-02 (papildyta 07-03) · **Bazė:** v41
**Failai:** `MainActivity.java`, `AndroidManifest.xml` (tik location eilutės), diagnostikos tekstas.

## Problema (vartotojo pastebėta, priežastis rasta kode)

`downloadAndInstall()` (~483–513 eil.) naudoja fiksuotą vardą `kemperis_update.apk`:
1. DownloadManager, radęs egzistuojantį failą, naują saugo kaip `kemperis_update-1.apk`,
   `-2`… — o `installApk()` visada atidaro SENĄ `kemperis_update.apk` → instaliatorius
   gauna jau įdiegtą versiją → „nepasileidžia automatiškai". Pirmas atnaujinimas veikė,
   visi kiti — ne.
2. Vartotojas mato beprasmį vardą „update" vietoj versijos.

## Taisymas (viena vieta)

```java
@JavascriptInterface
public void downloadAndInstall(String apkUrl) {
    try {
        // Vardas iš URL (pvz. kemperis_v42.apk) — ir informatyvu, ir be kolizijų tarp versijų
        String fileName = apkUrl.substring(apkUrl.lastIndexOf('/') + 1);
        if (fileName.isEmpty() || !fileName.endsWith(".apk")) fileName = "kemperis_update.apk";
        final File dest = new File(
            Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS), fileName);
        if (dest.exists()) dest.delete();   // apsauga nuo -1/-2 sufiksų TAI PAČIAI versijai

        DownloadManager.Request request = new DownloadManager.Request(Uri.parse(apkUrl));
        ...
        request.setDestinationInExternalPublicDir(Environment.DIRECTORY_DOWNLOADS, fileName);
        ...
        // receiver'yje:
        if (st == DownloadManager.STATUS_SUCCESSFUL) installApk(dest);
```

Papildomai (privaloma):
1. `installApk()` apvilkti `try/catch` su `sendNativeLog("Install error: " + e.getMessage())` —
   dabar exception receiver'yje miršta tyliai.
2. Prieš `startActivity` patikrinti `getPackageManager().canRequestPackageInstalls()`
   (API 26+): jei `false` — `sendNativeLog` paaiškinimas + atidaryti
   `Settings.ACTION_MANAGE_UNKNOWN_APP_SOURCES` su `package:` URI, kad vartotojas
   vienu paspaudimu leistų diegimą.
3. (Nebūtina, bet švaru) Po sėkmingo install intent'o — ištrinti senus
   `kemperis_update*.apk` ir senesnių versijų `kemperis_v*.apk` iš Downloads.

## 2 dalis: location leidimo grąžinimas (SSID matomumui)

**Problema (vartotojo pastebėta 07-03):** manifest'e `ACCESS_FINE_LOCATION` su
`maxSdkVersion="30"` → Android 12+ leidimas NEdeklaruotas → jo neįmanoma suteikti net
nustatymuose → `getCurrentSSID()` visada „Nežinomas (įjunkite vietovę)" (patarimas
klaidinantis) → WiFi bind veikia tik per `!VALIDATED` fallback, SSID kelias miręs.

**Taisymas:**
1. Manifest: `ACCESS_FINE_LOCATION` ir `ACCESS_COARSE_LOCATION` — **pašalinti
   `maxSdkVersion="30"`** (deklaruoti visoms versijoms). BLUETOOTH_SCAN su
   `neverForLocation` NELIESTI — jie nepriklausomi: BLE ir toliau nereikalaus location,
   o SSID skaitymui location bus prieinamas.
2. `requestNecessaryPermissions()` jau prašo FINE_LOCATION — patikrinti, kad dialogas
   pasirodo Android 12+ po įdiegimo (jei vartotojas anksčiau 2× atmetė — Android
   neberodys; tada diagnostikos kortelėje „Nežinomas" atveju pridėti mygtuką, atidarantį
   app nustatymus: `Settings.ACTION_APPLICATION_DETAILS_SETTINGS`).
3. Diagnostikos tekstą patikslinti: „Nežinomas (reikia vietos leidimo IR įjungtos
   vietovės)" — nes Android 9+ SSID reikalauja abiejų.

## ⚠️ Testavimo niuansas
v42 atsisiuntimą vykdys DAR SENAS v41 updater kodas (su „update" vardu ir kolizija) —
tad v42 diegimui vartotojui reikės: prieš spaudžiant „Atnaujinti" ištrinti iš planšetės
Downloads visus `kemperis_update*.apk`, o jei instaliatorius neatsidarys — įdiegti
ranka iš Downloads. NAUJAS updater kodas pilnai pasitikrins tik su v42→v43 atnaujinimu.

## Priėmimo kriterijai
1. Atsisiųstas failas Downloads kataloge vadinasi `kemperis_v42.apk` (iš apk_url).
2. Baigus siųsti, instaliatorius atsidaro automatiškai (net kai Downloads jau buvo
   senų update failų).
3. Jei „Install unknown apps" neduotas — loge aiški žinutė ir atsidaro nustatymai.
4. Location: Android 12+ nustatymuose Kemperis app turi Location leidimo eilutę;
   suteikus leidimą ir įjungus vietovę, prie kemperio AP kortelė rodo „Kemperis-Valdymas"
   (ne „Nežinomas"), bind per SSID kelią.
5. Renogy BLE po pakeitimo veikia kaip veikė (neverForLocation kelias nepakitęs).
6. Versijų grandinė: 42/42/42/„33.6"/kemperis_v42.apk; `git diff --stat` be nukirstų failų.

## NEDARYTI
renogy.js, firmware — neliesti. BLUETOOTH_SCAN `neverForLocation` neliesti.
FileProvider/file_paths.xml neliesti (patikrinta: `external-path path="."` dengia
public Downloads — jis veikia).
