@echo off
rem Compile HLSL shaders with explicit entrypoints and targets.
rem Produces <name>_vs.cso and <name>_ps.cso next to this batch file.

setlocal enabledelayedexpansion
set "FXC=C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\fxc.exe"

if not exist "%~dp0" (
	echo ERROR: cannot determine script folder.
	exit /b 1
)

rem Helper: compile a file for VS and PS using entrypoints VS and PS.
rem Usage: call :CompileShader SourceFile BaseName
goto :main

:CompileShader
rem %1 = source file (relative to this script), %2 = base name for output
"%FXC%" /nologo /E VS /T vs_5_0 "%~dp0%~1" /Fo "%~dp0%~2_vs.cso"
if errorlevel 1 echo FAILED: %~1 (VS)
"%FXC%" /nologo /E PS /T ps_5_0 "%~dp0%~1" /Fo "%~dp0%~2_ps.cso"
if errorlevel 1 echo FAILED: %~1 (PS)
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

