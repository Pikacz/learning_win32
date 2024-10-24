@echo off

if "%~1"=="" (
    echo Usage: %0 root_dir
    exit /b 1
)
set "root_dir=%~1"
set "build_dir=%root_dir%build\shaders\"
set "last_build=%build_dir%last_build.txt"
set "tmp_file=%build_dir%tmp.txt"
set "modification_date_exe=%root_dir%scripts\bin\modification_date.exe"

if not exist %build_dir% (
    mkdir %build_dir%
    goto BUILD
)

if not exist %last_build% (
    goto BUILD
)

%modification_date_exe% %~dp0 > %tmp_file%
if errorlevel 1 (
    echo Unable to check date of the changes
    exit /b 1
)
fc %tmp_file%  %last_build%  >nul
if errorlevel 1 (
    goto BUILD
) else (
    goto END
)

:BUILD
%modification_date_exe% %~dp0 > %last_build%

echo Compiling shaders
REM TODO


:END
if exist %tmp_file% (
    del %tmp_file%
)
exit /b 0

