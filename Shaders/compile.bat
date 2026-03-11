@echo off
rem Compile HLSL shaders with DXC (SM 6.0) for modern DX12.
rem Produces <name>_vs.cso and <name>_ps.cso next to this batch file.

setlocal enabledelayedexpansion

set "DXC=C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\dxc.exe"

if not exist "!DXC!" (
	echo ERROR: DXC compiler not found at !DXC!
	exit /b 1
)

if not exist "%~dp0" (
	echo ERROR: cannot determine script folder.
	exit /b 1
)

rem Helper: compile a file for VS and PS using DXC (SM 6.0).
rem Usage: call :CompileShader SourceFile BaseName
goto :main

:CompileShader
rem %1 = source file (relative to this script), %2 = base name for output
echo Compiling %~1 with DXC ^(SM 6.0^)...
"!DXC!" -E VS -T vs_6_0 -Fo "%~dp0%~2_vs.cso" "%~dp0%~1" -nologo
if errorlevel 1 echo FAILED: %~1 ^(VS/DXC^)
"!DXC!" -E PS -T ps_6_0 -Fo "%~dp0%~2_ps.cso" "%~dp0%~1" -nologo
if errorlevel 1 echo FAILED: %~1 ^(PS/DXC^)
goto :eof

:main
echo Using DXC: !DXC!

rem Compile the project's shaders.
call :CompileShader NormalMap.hlsl NormalMap
call :CompileShader Default.hlsl Default
call :CompileShader Shadows.hlsl Shadows
call :CompileShader DrawNormals.hlsl DrawNormals
call :CompileShader Ssao.hlsl Ssao
call :CompileShader SsaoBlur.hlsl SsaoBlur

rem Compile raytracing shader library (lib_6_5, no entry point)
echo Compiling Raytracing.hlsl with DXC ^(lib_6_5^)...
"!DXC!" -T lib_6_5 -Fo "%~dp0Raytracing.cso" "%~dp0Raytracing.hlsl" -nologo
if errorlevel 1 echo FAILED: Raytracing.hlsl ^(lib_6_5^)

echo Done.
endlocal

