@echo off
setlocal


if "%~1"=="" (
    echo Usage: %0 configuration
    exit /b 1
)
set "configuration=%~1"

if not "%configuration%"=="release" (
    set "exe_name=main_%configuration%"
) else (
    set "exe_name=main"
)


@call "%~dp0scripts\initialize.bat"
if ERRORLEVEL 1 (
    echo Unable to initialize other scripts %ERRORLEVEL%.
    exit /b %ERRORLEVEL%
)
@call "%~dp0src\win32\compile.bat" "%~dp0" "%exe_name%" "%configuration%"
if ERRORLEVEL 1 (
    echo Unable to compile program %configuration% %ERRORLEVEL%.
    exit /b %ERRORLEVEL%
)


@call "%~dp0src\shaders\compile.bat" "%~dp0" "%configuration%"
if ERRORLEVEL 1 (
    echo Unable to compile shaders %ERRORLEVEL%.
    exit /b %ERRORLEVEL%
)

endlocal