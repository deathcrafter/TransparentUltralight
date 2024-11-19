@echo off
setlocal enabledelayedexpansion

if "%~1"=="" (
    echo Usage: %~nx0 source_directory output_directory
    exit /b 1
)

if "%~2"=="" (
    echo Usage: %~nx0 source_directory output_directory 
    exit /b 1
)


set "source_dir=%~1"
set "output_dir=%~2"

if not exist "%output_dir%" mkdir "%output_dir%" /p >nul 2>&1

for /d %%d in (%source_dir%\*) do (
    set "appname=%%~nxd"
    if exist "%%d\build" (
        @echo Moving %%d\build to %output_dir%\!appname!
        if exist "%output_dir%\!appname!" rmdir /s /q "%output_dir%\!appname!" >nul 2>&1
        xcopy "%%d\build" "%output_dir%\!appname!" /E /I /Y /Q >nul 2>&1
    )
)

endlocal
