@IF NOT DEFINED ORG_PATH (
    @SET "ORG_PATH=%PATH%"
)


@IF NOT DEFINED IS_SET_UP (
    @REM Setup shader compiler
    @SET "PATH=C:\Program Files (x86)\Windows Kits\10\bin\10.0.22621.0\x64;%ORG_PATH%"

    @REM Setup compiler for x64
    @"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

    @SET IS_SET_UP=1
)



