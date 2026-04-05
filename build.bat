@echo off
REM Extra Attribute Manager - Build Script for Maya Plugin
REM Usage: build.bat [Maya_Install_Path]
REM Example: build.bat "C:\Program Files\Autodesk\Maya2025"
REM          build.bat                (auto-detects latest Maya)

setlocal enabledelayedexpansion

REM ===== Configuration =====
set SCRIPT_DIR=%~dp0
set BUILD_DIR=%SCRIPT_DIR%build
set PLUGIN_BASE_NAME=exattr-maya

REM Get Maya path from command line argument, or auto-detect
if "%~1"=="" (
    echo [INFO] No Maya path specified, auto-detecting...
    set MAYA_ROOT=
    for /f "delims=" %%d in ('dir /b /ad /o-n "C:\Program Files\Autodesk" 2^>nul ^| findstr /r "^Maya[0-9][0-9][0-9][0-9]$"') do (
        if not defined MAYA_ROOT (
            set MAYA_ROOT=C:\Program Files\Autodesk\%%d
        )
    )
    if not defined MAYA_ROOT (
        echo [ERROR] No Maya installation found in C:\Program Files\Autodesk\
        echo Please specify the Maya installation path as argument.
        echo Usage: build.bat [Maya_Install_Path]
        exit /b 1
    )
    echo [INFO] Auto-detected: !MAYA_ROOT!
) else (
    set MAYA_ROOT=%~1
)

REM Extract Maya version number from path
set MAYA_VER=
for /f "tokens=2 delims=Maya" %%v in ("!MAYA_ROOT!") do (
    set MAYA_VER=%%v
)
REM Fallback: extract last 4 digits
if not defined MAYA_VER (
    set MAYA_VER_STR=!MAYA_ROOT!
    set MAYA_VER=!MAYA_VER_STR:~-4!
)

echo [INFO] Maya version detected: %MAYA_VER%

REM Determine Qt version from Maya version (Maya 2022+ = Qt6, earlier = Qt5)
set QT_MAJOR=6
if defined MAYA_VER (
    if !MAYA_VER! LSS 2022 (
        set QT_MAJOR=5
    )
)
set PLUGIN_NAME=%PLUGIN_BASE_NAME%-qt!QT_MAJOR!.mll
echo [INFO] Qt version: Qt!QT_MAJOR!
echo [INFO] Plugin name: !PLUGIN_NAME!

REM Validate Maya path
if not exist "%MAYA_ROOT%" (
    echo [ERROR] Maya installation not found at: %MAYA_ROOT%
    echo Please specify the correct Maya installation path.
    echo Usage: build.bat [Maya_Install_Path]
    exit /b 1
)

echo ============================================
echo Extra Attribute Manager - Build Script
echo ============================================
echo Maya Root: %MAYA_ROOT%
echo Maya Version: %MAYA_VER%
echo Build Dir: %BUILD_DIR%
echo.

REM ===== Setup Visual Studio Environment =====
echo [1/4] Setting up Visual Studio environment...

REM Find Visual Studio environment variable setup script (2022 and 2019)
set VCVARS_FOUND=0

for %%e in (Professional Enterprise Community) do (
    for %%y in (2022 2019) do (
        for %%d in ("C:\Program Files\Microsoft Visual Studio\%%y\%%e\VC\Auxiliary\Build\vcvarsall.bat" "D:\Program Files\Microsoft Visual Studio\%%y\%%e\VC\Auxiliary\Build\vcvarsall.bat") do (
            if exist %%d (
                if !VCVARS_FOUND!==0 (
                    set VCVARS_BAT=%%d
                    set VCVARS_FOUND=1
                )
            )
        )
    )
)

if %VCVARS_FOUND%==0 (
    echo [ERROR] Visual Studio 2019 or 2022 not found.
    echo Please install Visual Studio with C++ development tools.
    exit /b 1
)

echo [INFO] Using: %VCVARS_BAT%
call %VCVARS_BAT% x64
if errorlevel 1 (
    echo [ERROR] Failed to setup Visual Studio environment.
    exit /b 1
)

echo [OK] Visual Studio environment configured.
echo.

REM ===== Check for CMake =====
where cmake >nul 2>&1
if errorlevel 1 (
    echo [ERROR] CMake not found in PATH.
    echo Please install CMake 3.20 or later and add it to your PATH.
    echo Download: https://cmake.org/download/
    exit /b 1
)

REM ===== Prepare Build Directory =====
echo [2/4] Preparing build directory...

if exist "%BUILD_DIR%" (
    echo Cleaning existing build directory...
    rmdir /s /q "%BUILD_DIR%"
)

mkdir "%BUILD_DIR%"
if errorlevel 1 (
    echo [ERROR] Failed to create build directory.
    exit /b 1
)

echo [OK] Build directory ready.
echo.

REM ===== Check for Qt Headers =====
set MAYA_QT_INCLUDE_ARG=
if not exist "%MAYA_ROOT%\include\QtWidgets" (
    echo [INFO] Qt headers not found in default location. Checking for zip...

    REM Try both Qt5 and Qt6 zip patterns
    set ZIP_FOUND=0
    for %%f in ("%MAYA_ROOT%\include\qt_*.zip") do (
        if !ZIP_FOUND!==0 (
            set ZIP_PATH=%%f
            set ZIP_FOUND=1
        )
    )

    set LOCAL_QT_INC="%SCRIPT_DIR%qt_headers"

    if !ZIP_FOUND!==1 (
        echo [INFO] Found Qt headers zip: !ZIP_PATH!

        REM Check if we need to extract
        if not exist "!LOCAL_QT_INC!\QtWidgets" (
            echo [INFO] Extracting Qt headers to !LOCAL_QT_INC!...
            if not exist "!LOCAL_QT_INC!" mkdir "!LOCAL_QT_INC!"
            REM Try tar first, fall back to Python for zip files
            tar -xf !ZIP_PATH! -C "!LOCAL_QT_INC!" 2>nul
            if not exist "!LOCAL_QT_INC!\QtWidgets" (
                python -c "import zipfile; zipfile.ZipFile(r'!ZIP_PATH!').extractall(r'!LOCAL_QT_INC!')" 2>nul
            )
        ) else (
            echo [INFO] Qt headers already extracted in !LOCAL_QT_INC!.
        )

        set MAYA_QT_INCLUDE_ARG=-DMAYA_QT_INCLUDE_DIR="!LOCAL_QT_INC!"
        echo [INFO] Using local Qt headers.
    ) else (
        echo [WARNING] Qt headers not found and zip not found. Build may fail.
    )
)
echo.

REM ===== Determine CMake Generator =====
set CMAKE_GEN="Visual Studio 17 2022"
REM Check if VS2022 cl.exe is available, otherwise try 2019
where cl >nul 2>&1
if errorlevel 1 (
    set CMAKE_GEN="Visual Studio 16 2019"
)

REM ===== Generate Visual Studio Project with CMake =====
echo [3/4] Generating Visual Studio solution with CMake...

cd "%BUILD_DIR%"

cmake -G %CMAKE_GEN% -A x64 ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DMAYA_ROOT="%MAYA_ROOT%" ^
    %MAYA_QT_INCLUDE_ARG% ^
    ..

if errorlevel 1 (
    echo [ERROR] CMake configuration failed.
    cd "%SCRIPT_DIR%"
    exit /b 1
)

echo [OK] CMake configuration complete.
echo.

REM ===== Build with MSBuild =====
echo [4/4] Building plugin with MSBuild...

msbuild ExtraAttrManager.sln /p:Configuration=Release /p:Platform=x64 /m /v:minimal
if errorlevel 1 (
    echo [ERROR] Build failed.
    cd "%SCRIPT_DIR%"
    exit /b 1
)

echo [OK] Build complete.
echo.

cd "%SCRIPT_DIR%"

REM ===== Verify Build Results =====
if exist "%BUILD_DIR%\Release\%PLUGIN_NAME%" (
    copy "%BUILD_DIR%\Release\%PLUGIN_NAME%" "%BUILD_DIR%\%PLUGIN_NAME%" >nul
)

if exist "%BUILD_DIR%\%PLUGIN_NAME%" (
    echo ============================================
    echo BUILD SUCCESS!
    echo ============================================
    echo Plugin: %BUILD_DIR%\%PLUGIN_NAME%
    echo.
    echo Copying MEL scripts...
    set MAYA_SCRIPTS_DIR=%USERPROFILE%\Documents\maya\%MAYA_VER%\scripts
    if not exist "!MAYA_SCRIPTS_DIR!" (
        mkdir "!MAYA_SCRIPTS_DIR!"
    )
    copy "%SCRIPT_DIR%scripts\addExtraAttrMenu.mel" "!MAYA_SCRIPTS_DIR!\" >nul
    if errorlevel 1 (
        echo [WARNING] Failed to copy MEL scripts
    ) else (
        echo MEL scripts copied to: !MAYA_SCRIPTS_DIR!
    )
    echo.

    REM Copy plugin to MAYA_PLUG_IN_PATH
    if defined MAYA_PLUG_IN_PATH (
        echo Copying plugin to MAYA_PLUG_IN_PATH...
        copy "%BUILD_DIR%\%PLUGIN_NAME%" "%MAYA_PLUG_IN_PATH%\%PLUGIN_NAME%" >nul
        if errorlevel 1 (
            echo [WARNING] Failed to copy plugin to MAYA_PLUG_IN_PATH: %MAYA_PLUG_IN_PATH%
        ) else (
            echo Plugin copied to: %MAYA_PLUG_IN_PATH%
        )
    ) else (
        echo [INFO] MAYA_PLUG_IN_PATH not set. Please copy manually to your plug-ins directory.
    )
    echo.
    echo To use this plugin in Maya:
    echo 1. Make sure %PLUGIN_NAME% is in your Maya plug-ins directory
    echo    or MAYA_PLUG_IN_PATH is set correctly
    echo.
    echo 2. In Maya, go to: Windows ^> Settings/Preferences ^> Plug-in Manager
    echo    Or use MEL command: loadPlugin "%PLUGIN_NAME%"
    echo.
    echo 3. Menu will appear at: Windows ^> General Editors ^> Extra Attribute Editor...
    echo    Or run: exAttrEditor -ui
    echo ============================================
) else (
    echo [ERROR] Plugin file not found after build.
    exit /b 1
)

endlocal
exit /b 0
