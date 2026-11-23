@echo off
rem Needs MinGW-w64 targeting 32-bit. FFX.exe is a 32-bit process, so a 64-bit DLL will NOT load.
rem   - standalone MinGW-w64 i686 build:  the compiler is  i686-w64-mingw32-gcc
rem   - MSYS2: open the "MINGW32" shell and use plain  gcc
i686-w64-mingw32-gcc -shared -O2 -Wall -o ffx_true_bdl.dll ffx_true_bdl.c -static-libgcc
if errorlevel 1 (echo. & echo BUILD FAILED) else (echo. & echo Built ffx_true_bdl.dll)
pause
