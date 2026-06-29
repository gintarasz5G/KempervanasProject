@echo off
setlocal enabledelayedexpansion
chcp 65001 >nul
echo ==================================================
echo   KEMPERIS - SMS / leidimu suteikimas per ADB
echo   Paketas: lt.kemperis.app
echo ==================================================
echo.

REM --- 1. Surandame adb.exe ---
set "ADB="
where adb >nul 2>nul && set "ADB=adb"
if not defined ADB if exist "%LOCALAPPDATA%\Android\Sdk\platform-tools\adb.exe" set "ADB=%LOCALAPPDATA%\Android\Sdk\platform-tools\adb.exe"
if not defined ADB if exist "%ANDROID_HOME%\platform-tools\adb.exe" set "ADB=%ANDROID_HOME%\platform-tools\adb.exe"
if not defined ADB if exist "%USERPROFILE%\AppData\Local\Android\Sdk\platform-tools\adb.exe" set "ADB=%USERPROFILE%\AppData\Local\Android\Sdk\platform-tools\adb.exe"

if not defined ADB (
  echo [KLAIDA] adb.exe nerastas.
  echo Idiek "Android SDK Platform-Tools" arba ideki adb i PATH.
  echo Atsisiusti: https://developer.android.com/tools/releases/platform-tools
  pause
  exit /b 1
)
echo Naudojamas adb: %ADB%
echo.

REM --- 2. Tikriname prijungta telefona ---
"%ADB%" start-server >nul 2>nul
echo Prijungti irenginiai:
"%ADB%" devices
echo.
echo Jei matai "unauthorized" - telefone patvirtink USB derinimo (RSA) langa ir paleisk skripta is naujo.
echo.
pause

set "PKG=lt.kemperis.app"

echo.
echo --- Suteikiami leidimai paketui %PKG% ---

for %%P in (
  android.permission.SEND_SMS
  android.permission.RECEIVE_SMS
  android.permission.READ_PHONE_STATE
  android.permission.READ_PHONE_NUMBERS
  android.permission.ACCESS_FINE_LOCATION
  android.permission.ACCESS_COARSE_LOCATION
  android.permission.POST_NOTIFICATIONS
) do (
  echo.
  echo  ^> %%P
  "%ADB%" shell pm grant %PKG% %%P
)

echo.
echo --- Papildomai per appops (SMS hard-restricted atvejui) ---
"%ADB%" shell appops set %PKG% SEND_SMS allow 2>nul
"%ADB%" shell appops set %PKG% RECEIVE_SMS allow 2>nul

echo.
echo --- Patikra (turi rodyti granted=true) ---
"%ADB%" shell dumpsys package %PKG% | findstr /I "SEND_SMS RECEIVE_SMS"

echo.
echo === BAIGTA ===
echo Jei virsuje matai "granted=true" - SMS leidimas suteiktas.
echo Atidaryk Kemperis programele is naujo.
echo.
echo Jei matai klaida "not allowed ... hard restricted" - parasyk man,
echo tada naudosime set-permission-flags metoda.
echo.
pause
