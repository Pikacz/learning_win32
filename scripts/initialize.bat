@echo off

set "modification_date_exe=%~dp0bin\modification_date.exe"
set "modification_date_src=%~dp0src\modification_date.c"

if not exist "%~dp0bin" (
    mkdir "%~dp0bin"
)

if not exist "%modification_date_exe%" (
    cl /EHsc "/Fe%modification_date_exe%" "%modification_date_src%" /Fo"%~dp0modification_date.obj"
    del "%~dp0modification_date.obj"
)
