@echo off
rem Compile HLSL shaders with DXC (SM 6.5) for DX12 Ultimate, with FXC (SM 5.1) fallback.
rem Produces <name>_vs.cso and <name>_ps.cso next to this batch file.

setlocal enabledelayedexpansion

rem Try to locate DXC first (ships with Windows SDK 10.0.20348+)
set "DXC="
for /f "delims=" %%d in ('dir /s /b "C:\Program Files (x86)\Windows Kits\10\bin\*dxc.exe" 2^>nul ^| findstr /i x64') do (
	set "DXC=%%d"
)

set "FXC=C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\fxc.exe"

if not exist "%~dp0" (
	echo ERROR: cannot determine script folder.
	exit /b 1
)

rem Helper: compile a file for VS and PS using DXC (SM 6.5) or FXC (SM 5.1) fallback.
rem Usage: call :CompileShader SourceFile BaseName
goto :main

:CompileShader
rem %1 = source file (relative to this script), %2 = base name for output
if defined DXC (
	echo Compiling %~1 with DXC ^(SM 6.5^)...
	"%DXC%" -E VS -T vs_6_5 -Fo "%~dp0%~2_vs.cso" "%~dp0%~1" -nologo
	if errorlevel 1 echo FAILED: %~1 (VS/DXC)
	"%DXC%" -E PS -T ps_6_5 -Fo "%~dp0%~2_ps.cso" "%~dp0%~1" -nologo
	if errorlevel 1 echo FAILED: %~1 (PS/DXC)
) else (
	echo Compiling %~1 with FXC ^(SM 5.1 fallback^)...
	"%FXC%" /nologo /E VS /T vs_5_1 "%~dp0%~1" /Fo "%~dp0%~2_vs.cso"
	if errorlevel 1 echo FAILED: %~1 (VS/FXC)
	"%FXC%" /nologo /E PS /T ps_5_1 "%~dp0%~1" /Fo "%~dp0%~2_ps.cso"
	if errorlevel 1 echo FAILED: %~1 (PS/FXC)
)
goto :eof

:main
if defined DXC (
	echo Using DXC: %DXC%
) else (
	echo DXC not found, falling back to FXC: %FXC%
)

rem Compile the project's shaders.
call :CompileShader NormalMap.hlsl NormalMap
call :CompileShader Default.hlsl Default
call :CompileShader Shadows.hlsl Shadows
call :CompileShader DrawNormals.hlsl DrawNormals
call :CompileShader Ssao.hlsl Ssao
call :CompileShader SsaoBlur.hlsl SsaoBlur

echo Done.
endlocal

