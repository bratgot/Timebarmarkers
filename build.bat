@echo off
setlocal EnableDelayedExpansion

echo ============================================================
echo  Marker Timebar - Build ^& Install
echo ============================================================
echo.

:: ─── Configurable paths ─────────────────────────────────────────────────────
set NUKE_ROOT=C:\Program Files\Nuke14.1v8
set QT_ROOT=C:\Qt\5.15.2\msvc2019_64
set PYTHON_LIB_DIR=%LOCALAPPDATA%\Programs\Python\Python39\libs
set PYTHON_INCLUDE=%LOCALAPPDATA%\Programs\Python\Python39\include
set NUKE_PLUGIN_DIR=%USERPROFILE%\.nuke

:: ─── Derived paths ───────────────────────────────────────────────────────────
set BUILD_DIR=%~dp0cpp\build
set DIST_DIR=%~dp0dist
set DLL_SRC=%BUILD_DIR%\Release\MarkerTimebar.dll
set MENU_SRC=%~dp0dist\menu.py

:: ─── Check for cmake ─────────────────────────────────────────────────────────
where cmake >nul 2>&1
if errorlevel 1 (
    echo [ERROR] cmake not found on PATH.
    echo         Open a Developer PowerShell for VS 2022 and try again.
    pause
    exit /b 1
)

:: ─── Check paths exist ───────────────────────────────────────────────────────
if not exist "%NUKE_ROOT%" (
    echo [ERROR] Nuke not found at: %NUKE_ROOT%
    echo         Edit NUKE_ROOT at the top of this file.
    pause
    exit /b 1
)
if not exist "%QT_ROOT%" (
    echo [ERROR] Qt not found at: %QT_ROOT%
    echo         Edit QT_ROOT at the top of this file.
    pause
    exit /b 1
)
if not exist "%PYTHON_LIB_DIR%" (
    echo [ERROR] Python 3.9 libs not found at: %PYTHON_LIB_DIR%
    echo         Edit PYTHON_LIB_DIR at the top of this file.
    pause
    exit /b 1
)

:: ─── Configure ───────────────────────────────────────────────────────────────
echo [1/4] Configuring...
cmake -B "%BUILD_DIR%" -A x64 ^
  -DNUKE_ROOT="%NUKE_ROOT%" ^
  -DQt5_ROOT="%QT_ROOT%" ^
  -DPYTHON_LIB_DIR="%PYTHON_LIB_DIR%" ^
  -DPYTHON_INCLUDE="%PYTHON_INCLUDE%" ^
  -S "%~dp0cpp" ^
  --no-warn-unused-cli > "%BUILD_DIR%_configure.log" 2>&1

if errorlevel 1 (
    echo [ERROR] Configure failed. See %BUILD_DIR%_configure.log
    pause
    exit /b 1
)
echo        OK

:: ─── Build ───────────────────────────────────────────────────────────────────
echo [2/4] Building...
cmake --build "%BUILD_DIR%" --config Release > "%BUILD_DIR%_build.log" 2>&1

if errorlevel 1 (
    echo [ERROR] Build failed. See %BUILD_DIR%_build.log
    pause
    exit /b 1
)
echo        OK

:: ─── Copy to dist ────────────────────────────────────────────────────────────
echo [3/4] Copying to dist\...
if not exist "%DIST_DIR%" mkdir "%DIST_DIR%"
copy /Y "%DLL_SRC%" "%DIST_DIR%\MarkerTimebar.dll" > nul
copy /Y "%MENU_SRC%" "%DIST_DIR%\menu.py" > nul
echo        dist\MarkerTimebar.dll
echo        dist\menu.py

:: ─── Copy to .nuke ───────────────────────────────────────────────────────────
echo [4/4] Installing to %NUKE_PLUGIN_DIR%\...
if not exist "%NUKE_PLUGIN_DIR%" mkdir "%NUKE_PLUGIN_DIR%"
copy /Y "%DLL_SRC%" "%NUKE_PLUGIN_DIR%\MarkerTimebar.dll" > nul

:: Handle menu.py — merge if already exists, otherwise copy fresh
set NUKE_MENU=%NUKE_PLUGIN_DIR%\menu.py
if exist "%NUKE_MENU%" (
    findstr /C:"MarkerTimebar" "%NUKE_MENU%" > nul 2>&1
    if errorlevel 1 (
        echo.
        echo [NOTE] You already have a menu.py in %NUKE_PLUGIN_DIR%
        echo        It does NOT contain MarkerTimebar entries.
        echo        Adding MarkerTimebar block to the end of your menu.py...
        echo. >> "%NUKE_MENU%"
        echo. >> "%NUKE_MENU%"
        type "%MENU_SRC%" >> "%NUKE_MENU%"
        echo        Merged into existing menu.py
    ) else (
        echo        menu.py already contains MarkerTimebar ^(skipped^)
    )
) else (
    copy /Y "%MENU_SRC%" "%NUKE_MENU%" > nul
    echo        %NUKE_PLUGIN_DIR%\menu.py ^(new^)
)

echo.
echo ============================================================
echo  Build complete!
echo  Restart Nuke 14.1 and press Alt+M to show the overlay.
echo ============================================================
echo.
pause
