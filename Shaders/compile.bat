@echo off
rem Compile HLSL shaders using DXC (DirectX Shader Compiler) for Shader Model 6.5.
rem Falls back to FXC with SM 5.1 if DXC is not available.
rem Produces <name>_vs.cso and <name>_ps.cso next to this batch file.

setlocal enabledelayedexpansion
set "DXC=C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\dxc.exe"
set "FXC=C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\fxc.exe"

if not exist "%~dp0" (
	echo ERROR: cannot determine script folder.
	exit /b 1
)

rem Detect which compiler is available
set "USE_DXC=0"
if exist "%DXC%" set "USE_DXC=1"

if "%USE_DXC%"=="1" (
	echo Using DXC ^(Shader Model 6.5^)
) else (
	echo DXC not found, falling back to FXC ^(Shader Model 5.1^)
)

rem Helper: compile a file for VS and PS using entrypoints VS and PS.
rem Usage: call :CompileShader SourceFile BaseName
goto :main

:CompileShader
rem %1 = source file (relative to this script), %2 = base name for output
if "%USE_DXC%"=="1" (
	"%DXC%" -nologo -E VS -T vs_6_5 -Fo "%~dp0%~2_vs.cso" "%~dp0%~1"
	if errorlevel 1 echo FAILED: %~1 (VS)
	"%DXC%" -nologo -E PS -T ps_6_5 -Fo "%~dp0%~2_ps.cso" "%~dp0%~1"
	if errorlevel 1 echo FAILED: %~1 (PS)
) else (
	"%FXC%" /nologo /E VS /T vs_5_1 "%~dp0%~1" /Fo "%~dp0%~2_vs.cso"
	if errorlevel 1 echo FAILED: %~1 (VS)
	"%FXC%" /nologo /E PS /T ps_5_1 "%~dp0%~1" /Fo "%~dp0%~2_ps.cso"
	if errorlevel 1 echo FAILED: %~1 (PS)
)
goto :eof

:main
rem Compile the project's shaders. Adjust entry names if you change shader functions.
call :CompileShader NormalMap.hlsl NormalMap
call :CompileShader Default.hlsl Default
call :CompileShader Shadows.hlsl Shadows
call :CompileShader DrawNormals.hlsl DrawNormals
call :CompileShader Ssao.hlsl Ssao
call :CompileShader SsaoBlur.hlsl SsaoBlur

echo Done.
endlocal

