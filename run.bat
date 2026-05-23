@echo off
setlocal EnableExtensions
pushd "%~dp0" || exit /b 1

rem MSBuild for .NET Framework can fail if the parent process provides both
rem Path and PATH. Recreate a single canonical Path entry before launching it.
set "ORIGINAL_PATH=%Path%"
set "PATH="
set "Path=%ORIGINAL_PATH%"
set "ORIGINAL_PATH="

set "MSBUILD="
for /f "delims=" %%i in ('where.exe msbuild 2^>nul') do (
  set "MSBUILD=%%i"
  goto :build
)

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" goto :missing

for /f "delims=" %%i in ('"%VSWHERE%" -latest -products * -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe') do (
  set "MSBUILD=%%i"
  goto :build
)

:missing
if exist "bin\Release\MatchstickPuzzle.exe" (
  echo MSBuild was not found. Starting the existing game build.
  start "" "bin\Release\MatchstickPuzzle.exe"
  exit /b 0
)

echo MSBuild was not found. Install Visual Studio 2022 with:
echo - Desktop development with C++
echo - C++/CLI support
echo Then run this file again.
pause
exit /b 1

:build
echo Building the game with "%MSBUILD%"...
"%MSBUILD%" MatchstickPuzzle.sln /p:Configuration=Release /p:Platform=x64 /m
if errorlevel 1 (
  echo Build failed.
  pause
  exit /b 1
)

start "" "bin\Release\MatchstickPuzzle.exe"
exit /b 0
