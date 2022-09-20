@ECHO OFF

PUSHD %~dp0

SET VSWHERE="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
SET CMAKE="%ProgramFiles%\CMake\bin\cmake.exe"

FOR /F "usebackq delims=." %%i IN (`%VSWHERE% -latest -prerelease -property installationVersion`) DO (
		SET VS_VERSION=%%i
)

IF %VS_VERSION% == 17 (
	SET CMAKE_GENERATOR="Visual Studio 17 2022"
) ELSE IF %VS_VERSION% == 16 (
	SET CMAKE_GENERATOR="Visual Studio 16 2019"
) ELSE IF %VS_VERSION% == 15 (
	SET CMAKE_GENERATOR="Visual Studio 15 2017"
) ELSE IF %VS_VERSION% == 14 (
	SET CMAKE_GENERATOR="Visual Studio 14 2015"
) ELSE (
	ECHO.
	ECHO No compatible version of Visual Studio installed
	ECHO.
	PAUSE
	GOTO :Exit
)

SET CMAKE_BINARY_DIR="out"

ECHO CMake Generator: %CMAKE_GENERATOR%
ECHO CMake Out Directory: %CMAKE_BINARY_DIR%
ECHO.

MKDIR %CMAKE_BINARY_DIR% 2>NUL
PUSHD %CMAKE_BINARY_DIR%

%CMAKE% -G %CMAKE_GENERATOR% -A x64 -Wno-dev "%~dp0"

IF %ERRORLEVEL% NEQ 0 (
	PAUSE
) ELSE (
	START VkSamples.sln
)

POPD

:Exit

POPD