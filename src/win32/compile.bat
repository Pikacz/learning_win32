@echo off

if "%~3"=="" (
    echo Usage: %0 root_dir exe_name configuration
    exit /b 1
)
set "root_dir=%~1"
set "exe_name=%~2"
set "configuration=%~3"
set "build_dir=%root_dir%build\"
set "last_build=%build_dir%%exe_name%_last_build.txt"
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

echo Compiling sources

set "main_file=%~dp0main.cpp"
set "out_exe=%build_dir%%exe_name%.exe"
set "out_obj=%build_dir%%exe_name%.obj"
set "libraties=user32.lib D3D12.lib DXGI.lib"

if exist %out_exe% (
    del %out_exe%
)


set "flags=/W4 /D _UNICODE /D UNICODE /D NOMINMAX"
if "%configuration%"=="terminal" (
    set "flags=%flags% /D TERMINAL_RUN /D DEBUG /D _DEBUG"
    set "libraties=%libraties% dxguid.lib"
) else if "%configuration%"=="debug" (
    set "flags=%flags% /Zi /D DEBUG /D _DEBUG"
    set "libraties=%libraties% dxguid.lib"
) else if not "%configuration%"=="release" (
    echo Unknown configuration "%configuration%". Possible: "terminal", "debug", "release"
    exit /b 1
)

cl %main_file% %flags% /Fo"%out_obj%" /Fe"%out_exe%" %libraties%
if errorlevel 1 (
    exit /b 1
)
%modification_date_exe% %~dp0 > %last_build%

:END
if exist %tmp_file% (
    del %tmp_file%
)
exit /b 0

