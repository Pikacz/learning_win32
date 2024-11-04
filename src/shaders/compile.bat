@echo off

if "%~2"=="" (
    echo Usage: %0 root_dir configuration
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

set "vertex_shader_name=VertexShader"
set "pixel_shader_name=PixelShader"
set "flags="

if "%configuration%"=="terminal" (
    set "flags_vertex=%flags% /Zi -Fd %build_dir%%vertex_shader_name%.pdb"
    set "flags_pixel=%flags% /Zi -Fd %build_dir%%pixel_shader_name%.pdb"
) else if "%configuration%"=="debug" (
    set "flags_vertex=%flags% /Zi -Fd %build_dir%%vertex_shader_name%.pdb"
    set "flags_pixel=%flags% /Zi -Fd %build_dir%%pixel_shader_name%.pdb"
) else if "%configuration%"=="release" (
    set "flags_vertex=%flags%"
    set "flags_pixel=%flags%"
) else (
    echo Unknown configuration "%configuration%". Possible: "terminal", "debug", "release"
    exit /b 1
)

dxc -T vs_6_0 -E main -Fo %build_dir%%vertex_shader_name%.cso %~dp0%vertex_shader_name%.hlsl %flags%
if errorlevel 1 (
    exit /b 1
)
dxc -T ps_6_0 -E main -Fo %build_dir%%pixel_shader_name%.cso %~dp0%pixel_shader_name%.hlsl %flags%
if errorlevel 1 (
    exit /b 1
)


%modification_date_exe% %~dp0 > %last_build%


:END
if exist %tmp_file% (
    del %tmp_file%
)
exit /b 0


