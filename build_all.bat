
@call "%~dp0build.bat" "terminal"
if ERRORLEVEL 1 (
    exit /b %ERRORLEVEL%
)

@call "%~dp0build.bat" "debug"
if ERRORLEVEL 1 (
    exit /b %ERRORLEVEL%
)

@call "%~dp0build.bat" "release"
if ERRORLEVEL 1 (
    exit /b %ERRORLEVEL%
)
