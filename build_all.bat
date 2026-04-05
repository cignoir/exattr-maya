@echo off
REM Build plugin for all available Maya devkits
REM Usage: build_all.bat [devkits_dir]
REM Output: dist\Maya<version>\exattr-maya.mll

setlocal enabledelayedexpansion

set SCRIPT_DIR=%~dp0

if "%~1"=="" (
    set DEVKIT_DIR=%SCRIPT_DIR%devkits
) else (
    set DEVKIT_DIR=%~1
)

set DIST_DIR=%SCRIPT_DIR%dist
set PLUGIN_NAME=exattr-maya.mll

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

for %%v in (2022 2023 2024 2025 2026 2027) do (
    set DEVKIT_BASE=%DEVKIT_DIR%\Maya%%v\devkitBase

    if exist "!DEVKIT_BASE!\include\maya" (
        echo ==================================================
        echo Building for Maya %%v...
        echo ==================================================

        set BUILD_DIR=%SCRIPT_DIR%build_%%v

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
            REM Build
            cmake --build . --config Release >nul 2>&1
            if errorlevel 1 (
                echo [ERROR] Build failed for Maya %%v
                echo Retrying with verbose output...
                cmake --build . --config Release 2>&1 | findstr /i "error"
                set /a FAIL_COUNT+=1
            ) else (
                REM Copy to dist
                if not exist "%DIST_DIR%\Maya%%v" mkdir "%DIST_DIR%\Maya%%v"
                copy "Release\%PLUGIN_NAME%" "%DIST_DIR%\Maya%%v\%PLUGIN_NAME%" >nul 2>&1
                if not exist "%DIST_DIR%\Maya%%v\%PLUGIN_NAME%" (
                    copy "%PLUGIN_NAME%" "%DIST_DIR%\Maya%%v\%PLUGIN_NAME%" >nul 2>&1
                )

                if exist "%DIST_DIR%\Maya%%v\%PLUGIN_NAME%" (
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
    for %%v in (2022 2023 2024 2025 2026 2027) do (
        if exist "%DIST_DIR%\Maya%%v\%PLUGIN_NAME%" (
            copy "%SCRIPT_DIR%scripts\*.mel" "%DIST_DIR%\Maya%%v\" >nul 2>&1
        )
    )
)

echo ============================================
echo BUILD COMPLETE
echo ============================================
echo Success: %BUILD_COUNT%
echo Failed:  %FAIL_COUNT%
echo.
echo Output:
for %%v in (2022 2023 2024 2025 2026 2027) do (
    if exist "%DIST_DIR%\Maya%%v\%PLUGIN_NAME%" (
        echo   [OK] %DIST_DIR%\Maya%%v\%PLUGIN_NAME%
    )
)

endlocal
exit /b %FAIL_COUNT%
