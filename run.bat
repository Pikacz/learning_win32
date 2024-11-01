@echo off
setlocal

if "%~1"=="" (
    echo Usage: %0 configuration
    exit /b 1
)
set "configuration=%~1"

@call "%~dp0build.bat" %configuration%
if ERRORLEVEL 1 (
    exit /b %ERRORLEVEL%
)

if not "%configuration%"=="release" (
    set "exe_name=main_%configuration%"
) else (
    set "exe_name=main"
)

build\%exe_name%.exe

endlocal