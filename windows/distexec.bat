@echo off
cd /d %~dp0
Set MYPATH=%CD%
REM Set PATH="%MYPATH%\..\lib;%PATH%"
Set PLUGIN_PATH="%MYPATH%\share\distexec\plugins"
%MYPATH%\bin\distexec.exe --plugin-path %PLUGIN_PATH% %*
