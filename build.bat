@echo off
REM Extra Attribute Manager - Build Script for Maya 2025 Plugin
REM Usage: build.bat [Maya2025_Install_Path]
REM Example: build.bat "C:\Program Files\Autodesk\Maya2025"

setlocal enabledelayedexpansion

REM ===== Configuration =====
set SCRIPT_DIR=%~dp0
set BUILD_DIR=%SCRIPT_DIR%build
set PLUGIN_NAME=exattr-maya.mll

REM Default installation path for Maya 2025
set DEFAULT_MAYA_ROOT=C:\Program Files\Autodesk\Maya2025

REM Get Maya path from command line argument, or use default
if "%~1"=="" (
    set MAYA_ROOT=%DEFAULT_MAYA_ROOT%
) else (
    set MAYA_ROOT=%~1
)

REM Validate Maya path
if not exist "%MAYA_ROOT%" (
    echo [ERROR] Maya installation not found at: %MAYA_ROOT%
    echo Please specify the correct Maya 2025 installation path.
    echo Usage: build.bat [Maya2025_Install_Path]
    exit /b 1
)

echo ============================================
echo Extra Attribute Manager - Build Script
echo ============================================
echo Maya Root: %MAYA_ROOT%
echo Build Dir: %BUILD_DIR%
echo.

REM Check moved after VS setup

REM ===== Setup Visual Studio Environment =====
echo [1/4] Setting up Visual Studio environment...

REM Find Visual Studio 2022 environment variable setup script
set VS2022_VCVARS="C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat"
if not exist %VS2022_VCVARS% (
    set VS2022_VCVARS="D:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat"
)
if not exist %VS2022_VCVARS% (
    set VS2022_VCVARS="D:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
)
if not exist %VS2022_VCVARS% (
    set VS2022_VCVARS="C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat"
)
if not exist %VS2022_VCVARS% (
    set VS2022_VCVARS="C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat"
)
if not exist %VS2022_VCVARS% (
    set VS2022_VCVARS="C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
)
if not exist %VS2022_VCVARS% (
    echo [ERROR] Visual Studio 2022 not found.
    echo Please install Visual Studio 2022 with C++ development tools.
    exit /b 1
)

REM Setup MSVC environment
call %VS2022_VCVARS% x64
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
    
    set ZIP_PATH="%MAYA_ROOT%\include\qt_6.5.3_vc14-include.zip"
    set LOCAL_QT_INC="%SCRIPT_DIR%qt_headers"
    
    if exist !ZIP_PATH! (
        echo [INFO] Found Qt headers zip.
        
        REM Check if we need to extract
        if not exist "!LOCAL_QT_INC!\QtWidgets" (
            echo [INFO] Extracting Qt headers to !LOCAL_QT_INC!...
            if not exist "!LOCAL_QT_INC!" mkdir "!LOCAL_QT_INC!"
            tar -xf !ZIP_PATH! -C "!LOCAL_QT_INC!"
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

REM ===== Generate Visual Studio Project with CMake =====
echo [3/4] Generating Visual Studio solution with CMake...

cd "%BUILD_DIR%"

cmake -G "Visual Studio 17 2022" -A x64 ^
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
    set MAYA_SCRIPTS_DIR=%USERPROFILE%\Documents\maya\2025\scripts
    if not exist "%MAYA_SCRIPTS_DIR%" (
        mkdir "%MAYA_SCRIPTS_DIR%"
    )
    copy "%SCRIPT_DIR%scripts\addExtraAttrMenu.mel" "%MAYA_SCRIPTS_DIR%\" >nul
    if errorlevel 1 (
        echo [WARNING] Failed to copy MEL scripts
    ) else (
        echo MEL scripts copied to: %MAYA_SCRIPTS_DIR%
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
