
cd "%~dp0%"
g++ -std=c++17 -I ./include -o ./bin/backup.exe ./src/backup.cpp ./src/client.cpp ./src/server.cpp -lwsock32 -lws2_32

@echo off
SET "RELATIVE_EXE_PATH=bin"
SET "ABSOLUTE_EXE_PATH=%~dp0%RELATIVE_EXE_PATH%"
set "newPath=%ABSOLUTE_EXE_PATH%"
for /f "tokens=2*" %%a in ('reg query "HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Environment" /v Path') do (
    setx PATH "%%b;%newPath%" /M
)
echo PATH updated.

pause
