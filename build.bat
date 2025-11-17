@echo off
REM Build script for Windows using Visual Studio Command Prompt

cl /EHsc /O2 /W3 /D_UNICODE /DUNICODE sphere_gouraud.cpp /link user32.lib gdi32.lib comctl32.lib /SUBSYSTEM:WINDOWS /OUT:sphere_gouraud.exe

echo Build complete!
pause
