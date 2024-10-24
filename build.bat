@echo off
setlocal

@call "%~dp0scripts\initialize.bat"
if ERRORLEVEL 1 (
    echo Unable to initialize other scripts %ERRORLEVEL%.
    exit /b %ERRORLEVEL%
)
@call "%~dp0src\win32\compile.bat" "%~dp0"
if ERRORLEVEL 1 (
    echo Unable to compile program %ERRORLEVEL%.
    exit /b %ERRORLEVEL%
)
@call "%~dp0src\shaders\compile.bat" "%~dp0"
if ERRORLEVEL 1 (
    echo Unable to compile shaders %ERRORLEVEL%.
    exit /b %ERRORLEVEL%
)

endlocal