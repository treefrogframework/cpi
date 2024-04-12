@echo OFF

set VCVARSBAT=""
set VSVER=
set VSVER2022=2022
set VSVER2019=2019
set VSVER2017=2017
set VSWHERE="%PROGRAMFILES(X86)%\Microsoft Visual Studio\Installer\vswhere.exe"

if exist %VSWHERE% (
  for /f "usebackq tokens=*" %%i in (`%VSWHERE% -find  **\vcvarsall.bat`) do (
    echo %%i | find "%VSVER2022%" >NUL
    if not ERRORLEVEL 1 (
      set VCVARSBAT="%%i"
      set VSVER=%VSVER2022%
      goto :break
    )
    echo %%i | find "%VSVER2019%" >NUL
    if not ERRORLEVEL 1 (
      set VCVARSBAT="%%i"
      set VSVER=%VSVER2019%
      goto :break
    )
    echo %%i | find "%VSVER2017%" >NUL
    if not ERRORLEVEL 1 (
      set VCVARSBAT="%%i"
      set VSVER=%VSVER2017%
      goto :break
    )
  )
)

:break
if exist %VCVARSBAT% (
  call %VCVARSBAT% amd64 >nul
  echo Set up environment for MSVC %VSVER% usage
)

@rem Run cpi
cpi.exe
