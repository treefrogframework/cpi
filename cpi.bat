@echo off

set CWD=%~dp0
set VCVARSBAT=""
set VSVER=2022 2019 2017
set VSWHERE="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

where cl.exe >NUL 2>&1
if ERRORLEVEL 1 (
  if exist %VSWHERE% (
    for %%v in (%VSVER%) do (
      for /f "usebackq tokens=*" %%i in (`%VSWHERE% -find **\vcvarsall.bat`) do (
        echo %%i | find "%%v" >NUL
        if not ERRORLEVEL 1 (
            set VCVARSBAT="%%i"
            set VSVER=%%v
            goto :break
        )
      )
    )
  )

:break
  if exist %VCVARSBAT% (
    echo Setting up environment for MSVC %VSVER% usage
    call %VCVARSBAT% amd64 >NUL 2>&1
  )
)

cd /D %CWD%
cpi.exe
