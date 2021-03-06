@REM Copyright (c) Microsoft. All rights reserved.
@REM Licensed under the MIT license. See LICENSE file in the project root for full license information.

@setlocal EnableExtensions EnableDelayedExpansion
@echo off

set current-path=%~dp0

rem // remove trailing slash
set current-path=%current-path:~0,-1%

set build-root=%current-path%\..
rem // resolve to fully qualified path
for %%i in ("%build-root%") do set build-root=%%~fi

rem ----------------------------------------------------------------------------
rem -- parse arguments
rem ----------------------------------------------------------------------------

set build-clean=
set build-config=
set build-runtime=

:args-loop
if "%1" equ "" goto args-done
if "%1" equ "-c" goto arg-build-clean
if "%1" equ "--clean" goto arg-build-clean
if "%1" equ "--config" goto arg-build-config
if "%1" equ "--platform" goto arg-build-platform
call :usage && exit /b 1

:arg-build-clean
set "build-clean=--no-incremental"
goto args-continue

:arg-build-config
shift
if "%1" equ "" call :usage && exit /b 1
set "build-config=--configuration %1"
goto args-continue

:arg-build-platform
shift
if "%1" equ "" call :usage && exit /b 1
if "%1" equ "x86" set "build-runtime=--runtime win-x86"
if "%1" equ "x64" set "build-runtime=--runtime win-x64"
goto args-continue

:args-continue
shift
goto args-loop

:args-done

rem ----------------------------------------------------------------------------
rem -- build
rem ----------------------------------------------------------------------------

call dotnet restore ^
    %build-root%\bindings\dotnetcore\dotnet-core-binding ^
    %build-root%\samples\dotnet_core_module_sample\modules

if not !errorlevel!==0 exit /b !errorlevel!

call dotnet build %build-clean% %build-config% %build-runtime% ^
    %build-root%\bindings\dotnetcore\dotnet-core-binding\Microsoft.Azure.Devices.Gateway ^
    %build-root%\bindings\dotnetcore\dotnet-core-binding\E2ETestModule ^
    %build-root%\samples\dotnet_core_module_sample\modules\PrinterModule ^
    %build-root%\samples\dotnet_core_module_sample\modules\SensorModule

if not !errorlevel!==0 exit /b !errorlevel!

rem ----------------------------------------------------------------------------
rem -- test
rem ----------------------------------------------------------------------------

call dotnet test ^
    %build-root%\bindings\dotnetcore\dotnet-core-binding\Microsoft.Azure.Devices.Gateway.Tests

if not !errorlevel!==0 exit /b !errorlevel!

goto :eof

rem ----------------------------------------------------------------------------
rem -- helper routines
rem ----------------------------------------------------------------------------

:usage
echo build_dotnet_core.cmd [options]
echo options:
echo  -c, --clean           delete artifacts from previous build before building
echo  --config ^<value^>      [Debug] build configuration (e.g. Debug, Release)
echo  --platform ^<value^>    [x86] build platform (e.g. x86, x64, ...)
goto :eof
