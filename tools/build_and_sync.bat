@echo off
echo ==================================================
echo   KEMPERIS APP - SINCHRONIZAVIMAS IR KOMPILIAVIMAS
echo ==================================================
echo.

echo [1/3] Sinchronizuojami Web failai i Android projekta...
:: Copy index.html to assets/public
copy "www\index.html" "android\app\src\main\assets\public\index.html" /Y

echo [2/3] Vykdomas npx cap sync...
:: Call npx cap sync to ensure Capacitor is aware of changes
call npx cap sync android

echo [3/3] Kompiliuojama Pasirasyta RELEASE APK versija...
cd android
call gradlew assembleRelease

echo.
echo === PROCESAS BAIGTAS ===
echo Failas: android\app\build\outputs\apk\release\app-release.apk
echo.
pause
