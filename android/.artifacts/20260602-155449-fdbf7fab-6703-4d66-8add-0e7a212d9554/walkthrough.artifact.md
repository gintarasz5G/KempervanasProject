# Kemperio programėlės fono paslaugų sutvarkymas (Android 14+)

Sėkmingai atnaujinau `AndroidManifest.xml` failą, kad programėlė galėtų stabiliai veikti fone naujausiuose Android įrenginiuose, stebėti ESP32 jutiklius ir teikti balso pranešimus.

## Kas buvo atlikta:
1.  **Android 14+ suderinamumas**: Pridėtas `foregroundServiceType="specialUse"` paslaugos tipas, kuris yra būtinas Android 14+ versijoms fono darbams.
2.  **Pateisinimo deklaracija**: Pridėta `PROPERTY_SPECIAL_USE_FGS_TYPE` savybė su anglišku paaiškinimu sistemai, kodėl reikalinga ši paslauga (saugumo monitoringas ir TTS).
3.  **Teisių sutvarkymas**:
    *   Pridėtas `FOREGROUND_SERVICE_SPECIAL_USE`.
    *   Pridėtas `POST_NOTIFICATIONS` (reikalingas Android 13+ matomam pranešimui rodyti).
    *   Leidimai perkelti į manifest viršų pagal standartus.
4.  **Klaidų valymas**: Pašalinti „Empty body“ ir eiliškumo įspėjimai.

## Patikra:
*   **Build**: Projektas sėkmingai sukompiliuotas naudojant `gradle assembleDebug`. Tai patvirtina, kad manifestas yra sintaksiškai teisingas.

## Tolesni veiksmai vartotojui:
1.  **Išvalykite projektą**: Android Studio meniu: `Build > Clean Project`, tada `Build > Rebuild Project`.
2.  **Įdiekite į telefoną**: Prijunkite telefoną ir spauskite **Run**.
3.  **Baterijos nustatymai**: Telefone eikite į `Settings > Apps > Kemperis > Battery` ir pasirinkite **Unrestricted**. Tai kritiškai svarbu, kad Android neuždarytų SSE ryšio fone.

[AndroidManifest.xml](file:///C:/Users/ginta/kempervanas/veikiantisKEMPERIS/KemperisApp/android/app/src/main/AndroidManifest.xml)
