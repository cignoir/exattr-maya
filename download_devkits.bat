@echo off
REM Download Maya devkits for all supported versions (2022-2027)
REM Usage: download_devkits.bat [download_dir]
REM Example: download_devkits.bat F:\maya-devkits

setlocal enabledelayedexpansion

set SCRIPT_DIR=%~dp0

if "%~1"=="" (
    set DEVKIT_DIR=%SCRIPT_DIR%devkits
) else (
    set DEVKIT_DIR=%~1
)

echo ============================================
echo Maya Devkit Downloader
echo ============================================
echo Download Dir: %DEVKIT_DIR%
echo.

if not exist "%DEVKIT_DIR%" mkdir "%DEVKIT_DIR%"

REM Devkit URLs (latest update for each major version)
set "BASE=https://autodesk-adn-transfer.s3-us-west-2.amazonaws.com/ADN+Extranet/M%%26E/Maya"
set "BASE2=https://autodesk-adn-transfer.s3.us-west-2.amazonaws.com/ADN+Extranet/M%%26E/Maya"

set "URL_2022=!BASE!/devkit+2022/Autodesk_Maya_2022_5_Update_DEVKIT_Windows.zip"
set "URL_2023=!BASE!/devkit+2023/Autodesk_Maya_2023_3_Update_DEVKIT_Windows.zip"
set "URL_2024=!BASE2!/devkit+2024/Autodesk_Maya_2024_2_Update_DEVKIT_Windows.zip"
set "URL_2025=!BASE2!/devkit+2025/Autodesk_Maya_2025_3_Update_DEVKIT_Windows.zip"
set "URL_2026=!BASE2!/devkit+2026/Autodesk_Maya_2026_3_Update_DEVKIT_Windows.zip"
set "URL_2027=!BASE2!/devkit+2027/Autodesk_Maya_2027_DEVKIT_Windows.zip"

for %%v in (2022 2023 2024 2025 2026 2027) do (
    echo --------------------------------------------------
    echo [Maya %%v]

    if exist "%DEVKIT_DIR%\Maya%%v\devkitBase\include\maya" (
        echo Already downloaded. Skipping.
    ) else (
        echo Downloading devkit...
        set "DL_URL=!URL_%%v!"

        curl -L --progress-bar -o "%DEVKIT_DIR%\devkit_%%v.zip" "!DL_URL!"
        if errorlevel 1 (
            echo [ERROR] Download failed for Maya %%v
        ) else (
            echo Extracting...
            if not exist "%DEVKIT_DIR%\Maya%%v" mkdir "%DEVKIT_DIR%\Maya%%v"
            python -c "import zipfile; zipfile.ZipFile(r'%DEVKIT_DIR%\devkit_%%v.zip').extractall(r'%DEVKIT_DIR%\Maya%%v')"

            if exist "%DEVKIT_DIR%\Maya%%v\devkitBase\include\maya" (
                REM Extract Qt.zip if present
                if exist "%DEVKIT_DIR%\Maya%%v\devkitBase\Qt.zip" (
                    echo Extracting Qt headers and libraries...
                    python -c "import zipfile; zipfile.ZipFile(r'%DEVKIT_DIR%\Maya%%v\devkitBase\Qt.zip').extractall(r'%DEVKIT_DIR%\Maya%%v\devkitBase')"
                )

                REM Also extract Qt headers zip in include/ if present
                for %%f in ("%DEVKIT_DIR%\Maya%%v\devkitBase\include\qt_*.zip") do (
                    if not exist "%DEVKIT_DIR%\Maya%%v\devkitBase\include\QtWidgets" (
                        echo Extracting Qt headers from %%f...
                        python -c "import zipfile; zipfile.ZipFile(r'%%f').extractall(r'%DEVKIT_DIR%\Maya%%v\devkitBase\include')"
                    )
                )

                echo [OK] Maya %%v devkit ready.
            ) else (
                echo [ERROR] Extraction failed for Maya %%v
            )
            del "%DEVKIT_DIR%\devkit_%%v.zip" 2>nul
        )
    )
    echo.
)

echo ============================================
echo Done!
echo ============================================
echo.
for %%v in (2022 2023 2024 2025 2026 2027) do (
    if exist "%DEVKIT_DIR%\Maya%%v\devkitBase\include\maya" (
        echo   [OK] Maya %%v
    ) else (
        echo   [--] Maya %%v ^(not available^)
    )
)

endlocal
exit /b 0
