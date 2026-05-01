@echo off
REM Build plugin for all available Maya devkits
REM Usage: build_all.bat [devkits_dir]
REM Output: dist\exattr-maya-<version>.mll

setlocal enabledelayedexpansion

set SCRIPT_DIR=%~dp0
REM Remove trailing backslash to avoid "path\" quoting issues with CMake
if "%SCRIPT_DIR:~-1%"=="\" set SCRIPT_DIR=%SCRIPT_DIR:~0,-1%

if "%~1"=="" (
    set DEVKIT_DIR=%SCRIPT_DIR%\devkits
) else (
    set DEVKIT_DIR=%~1
)

set DIST_DIR=%SCRIPT_DIR%\dist

echo ============================================
echo Extra Attribute Manager - Multi-Version Build
echo ============================================
echo Devkit Dir: %DEVKIT_DIR%
echo Output Dir: %DIST_DIR%
echo.

REM ===== Setup Visual Studio Environment =====
set VCVARS_FOUND=0
for %%e in (Professional Enterprise Community BuildTools) do (
    for %%y in (2022 2019) do (
        for %%d in ("C:\Program Files\Microsoft Visual Studio\%%y\%%e\VC\Auxiliary\Build\vcvarsall.bat" "C:\Program Files (x86)\Microsoft Visual Studio\%%y\%%e\VC\Auxiliary\Build\vcvarsall.bat" "D:\Program Files\Microsoft Visual Studio\%%y\%%e\VC\Auxiliary\Build\vcvarsall.bat") do (
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
    echo [ERROR] Visual Studio not found.
    exit /b 1
)

call %VCVARS_BAT% x64
if errorlevel 1 (
    echo [ERROR] Failed to setup Visual Studio environment.
    exit /b 1
)
echo [OK] Visual Studio configured.
echo.

REM ===== Check for CMake =====
where cmake >nul 2>&1
if errorlevel 1 (
    echo [ERROR] CMake not found. Install CMake 3.20+.
    exit /b 1
)

REM ===== Build for each available devkit =====
set BUILD_COUNT=0
set FAIL_COUNT=0

for %%v in (2025 2026 2027) do (
    set DEVKIT_BASE=%DEVKIT_DIR%\Maya%%v\devkitBase

    if exist "!DEVKIT_BASE!\include\maya" (
        echo ==================================================
        echo Building for Maya %%v...
        echo ==================================================

        set BUILD_DIR=%SCRIPT_DIR%\build_%%v

        REM Clean build directory
        if exist "!BUILD_DIR!" rmdir /s /q "!BUILD_DIR!"
        mkdir "!BUILD_DIR!"

        REM Configure
        cd "!BUILD_DIR!"
        cmake -G "Visual Studio 17 2022" -A x64 ^
            -DMAYA_DEVKIT_ROOT="!DEVKIT_BASE!" ^
            -DMAYA_VERSION=%%v ^
            "%SCRIPT_DIR%" >nul 2>&1

        if errorlevel 1 (
            echo [ERROR] CMake configuration failed for Maya %%v
            set /a FAIL_COUNT+=1
            cd "%SCRIPT_DIR%"
        ) else (
            set PLUGIN_FILE=exattr-maya-%%v.mll

            REM Build
            cmake --build . --config Release >nul 2>&1
            if errorlevel 1 (
                echo [ERROR] Build failed for Maya %%v
                echo Retrying with verbose output...
                cmake --build . --config Release 2>&1 | findstr /i "error"
                set /a FAIL_COUNT+=1
            ) else (
                REM Copy to dist
                if not exist "%DIST_DIR%" mkdir "%DIST_DIR%"
                copy "Release\!PLUGIN_FILE!" "%DIST_DIR%\!PLUGIN_FILE!" >nul 2>&1
                if not exist "%DIST_DIR%\!PLUGIN_FILE!" (
                    copy "!PLUGIN_FILE!" "%DIST_DIR%\!PLUGIN_FILE!" >nul 2>&1
                )

                if exist "%DIST_DIR%\!PLUGIN_FILE!" (
                    echo [OK] Maya %%v - Built successfully
                    set /a BUILD_COUNT+=1
                ) else (
                    echo [ERROR] Maya %%v - Plugin file not found after build
                    set /a FAIL_COUNT+=1
                )
            )
            cd "%SCRIPT_DIR%"
        )

        REM Cleanup build directory
        if exist "!BUILD_DIR!" rmdir /s /q "!BUILD_DIR!"
        echo.
    )
)

REM Copy MEL scripts to dist
if %BUILD_COUNT% GTR 0 (
    copy "%SCRIPT_DIR%\scripts\*.mel" "%DIST_DIR%\" >nul 2>&1
)

echo ============================================
echo BUILD COMPLETE
echo ============================================
echo Success: %BUILD_COUNT%
echo Failed:  %FAIL_COUNT%
echo.
echo Output:
for %%v in (2025 2026 2027) do (
    if exist "%DIST_DIR%\exattr-maya-%%v.mll" (
        echo   [OK] dist\exattr-maya-%%v.mll
    )
)

REM ===== Carton package staging + zip =====
REM Skip if no .mll built or carton-maya not installed.
if %BUILD_COUNT% EQU 0 goto :skip_carton

python -c "import carton" >nul 2>&1
if errorlevel 1 (
    echo.
    echo [INFO] carton-maya not installed - skipping zip packaging.
    echo        To enable: pip install carton-maya  (or uvx carton-maya)
    goto :skip_carton
)

echo.
echo ============================================
echo Carton Packaging
echo ============================================

set CARTON_SRC=%SCRIPT_DIR%\carton
set CARTON_STAGE=%SCRIPT_DIR%\dist-carton
set CARTON_OUT=%SCRIPT_DIR%\dist-carton-out

if not exist "%CARTON_SRC%\package.json" (
    echo [WARNING] %CARTON_SRC%\package.json not found - skipping carton step.
    goto :skip_carton
)

REM Reset staging
if exist "%CARTON_STAGE%" rmdir /s /q "%CARTON_STAGE%"
mkdir "%CARTON_STAGE%"
mkdir "%CARTON_STAGE%\scripts"

REM Seed manifests from source-controlled carton/
copy "%CARTON_SRC%\package.json" "%CARTON_STAGE%\package.json" >nul
copy "%CARTON_SRC%\exattr-maya.mod" "%CARTON_STAGE%\exattr-maya.mod" >nul

REM Drop each Maya version's .mll under plug-ins/<ver>/exattr-maya.mll
for %%v in (2025 2026 2027) do (
    if exist "%DIST_DIR%\exattr-maya-%%v.mll" (
        if not exist "%CARTON_STAGE%\plug-ins\%%v" mkdir "%CARTON_STAGE%\plug-ins\%%v"
        copy "%DIST_DIR%\exattr-maya-%%v.mll" "%CARTON_STAGE%\plug-ins\%%v\exattr-maya.mll" >nul
    )
)

REM MEL scripts
copy "%SCRIPT_DIR%\scripts\*.mel" "%CARTON_STAGE%\scripts\" >nul 2>&1

REM Lint then pack
python -m carton package lint "%CARTON_STAGE%"
if errorlevel 1 (
    echo [ERROR] Carton lint failed - aborting zip step.
    goto :skip_carton
)

python -m carton package pack "%CARTON_STAGE%" --out "%CARTON_OUT%"
if errorlevel 1 (
    echo [ERROR] Carton pack failed.
    goto :skip_carton
)

echo.
echo Carton zip:
for %%f in ("%CARTON_OUT%\exattr-maya-*.zip") do echo   [OK] %%f

:skip_carton

endlocal
exit /b %FAIL_COUNT%
