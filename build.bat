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
set REPO_DIR=%~dp0
set BUILD_DIR=%REPO_DIR%cpp\build
set DIST_DIR=%REPO_DIR%dist
set DLL_SRC=%BUILD_DIR%\Release\MarkerTimebar.dll
set MENU_SRC=%REPO_DIR%menu.py
set PY_SRC=%REPO_DIR%python\marker_timebar.py

:: ─── Check for cmake ─────────────────────────────────────────────────────────
where cmake >nul 2>&1
if errorlevel 1 (
    echo [ERROR] cmake not found on PATH.
    echo         Open a Developer Command Prompt for VS 2022 and try again.
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
echo [1/5] Configuring...
cmake -B "%BUILD_DIR%" -A x64 ^
  -DNUKE_ROOT="%NUKE_ROOT%" ^
  -DQt5_ROOT="%QT_ROOT%" ^
  -DPYTHON_LIB_DIR="%PYTHON_LIB_DIR%" ^
  -DPYTHON_INCLUDE="%PYTHON_INCLUDE%" ^
  -S "%REPO_DIR%cpp" ^
  --no-warn-unused-cli > "%BUILD_DIR%_configure.log" 2>&1

if errorlevel 1 (
    echo [ERROR] Configure failed. See %BUILD_DIR%_configure.log
    pause
    exit /b 1
)
echo        OK

:: ─── Build ───────────────────────────────────────────────────────────────────
echo [2/5] Building...
cmake --build "%BUILD_DIR%" --config Release > "%BUILD_DIR%_build.log" 2>&1

if errorlevel 1 (
    echo [ERROR] Build failed. See %BUILD_DIR%_build.log
    pause
    exit /b 1
)
echo        OK

:: ─── Copy to dist ────────────────────────────────────────────────────────────
echo [3/5] Copying to dist\...
if not exist "%DIST_DIR%" mkdir "%DIST_DIR%"
copy /Y "%DLL_SRC%"  "%DIST_DIR%\MarkerTimebar.dll"   > nul
copy /Y "%MENU_SRC%" "%DIST_DIR%\menu.py"              > nul
copy /Y "%PY_SRC%"   "%DIST_DIR%\marker_timebar.py"    > nul
echo        dist\MarkerTimebar.dll
echo        dist\menu.py
echo        dist\marker_timebar.py

:: ─── Install to .nuke ────────────────────────────────────────────────────────
echo [4/5] Copying files to %NUKE_PLUGIN_DIR%\...
if not exist "%NUKE_PLUGIN_DIR%" mkdir "%NUKE_PLUGIN_DIR%"
copy /Y "%DLL_SRC%"  "%NUKE_PLUGIN_DIR%\MarkerTimebar.dll"  > nul
copy /Y "%PY_SRC%"   "%NUKE_PLUGIN_DIR%\marker_timebar.py"  > nul
echo        %NUKE_PLUGIN_DIR%\MarkerTimebar.dll
echo        %NUKE_PLUGIN_DIR%\marker_timebar.py

:: ─── Handle menu.py — NEVER overwrite, always append or skip ─────────────────
echo [5/5] Checking menu.py...
set NUKE_MENU=%NUKE_PLUGIN_DIR%\menu.py
if exist "%NUKE_MENU%" (
    findstr /C:"MarkerTimebar" "%NUKE_MENU%" > nul 2>&1
    if errorlevel 1 (
        echo        Appending Marker Timebar block to existing menu.py...
        echo. >> "%NUKE_MENU%"
        echo # ---- Marker Timebar (auto-added by build.bat) ---- >> "%NUKE_MENU%"
        type "%MENU_SRC%" >> "%NUKE_MENU%"
        echo        Done — your existing menu.py was NOT replaced.
    ) else (
        echo        menu.py already contains MarkerTimebar ^(skipped — not modified^)
    )
) else (
    copy /Y "%MENU_SRC%" "%NUKE_MENU%" > nul
    echo        %NUKE_PLUGIN_DIR%\menu.py ^(created fresh — no existing file found^)
)

echo.
echo ============================================================
echo  Done! Files installed to %NUKE_PLUGIN_DIR%\
echo.
echo  To use the C++ version ^(DWM blur^):  USE_CPP = True  in menu.py
echo  To use the Python version:           USE_CPP = False in menu.py
echo.
echo  Restart Nuke 14.1 and press Alt+M to show the overlay.
echo ============================================================
echo.
pause
