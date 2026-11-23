@echo off
setlocal
rem Builds ffx_true_bdl.dll as a 32-bit DLL (FFX.exe is a 32-bit process).
rem Works from a normal prompt or a double-click: it finds Visual Studio itself.

rem Already inside a Visual Studio prompt? Make sure it is the 32-bit (x86) one.
where cl >nul 2>nul
if errorlevel 1 goto findvs
if not defined VSCMD_ARG_TGT_ARCH goto build
if /i "%VSCMD_ARG_TGT_ARCH%"=="x86" goto build
echo This prompt targets %VSCMD_ARG_TGT_ARCH%. Use "x86 Native Tools Command Prompt for VS" instead.
goto end

:findvs
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" goto novs
for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSROOT=%%i"
if not defined VSROOT goto novs
echo Using Visual Studio at: %VSROOT%
call "%VSROOT%\VC\Auxiliary\Build\vcvars32.bat" >nul
where cl >nul 2>nul
if errorlevel 1 goto novs

:build
cl /nologo /LD /O2 /MT /W3 ffx_true_bdl.c /Fe:ffx_true_bdl.dll
if errorlevel 1 goto failed
echo.
echo Built ffx_true_bdl.dll
dumpbin /headers ffx_true_bdl.dll | findstr /i "machine"
echo (it must say 14C machine (x86))
goto end

:novs
echo.
echo Could not find the C++ compiler. In the Visual Studio Installer choose
echo Modify, tick "Desktop development with C++", install, then run this again.
goto end

:failed
echo.
echo BUILD FAILED

:end
pause
