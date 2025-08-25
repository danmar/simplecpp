@ECHO off
REM Script to run AStyle on the sources
REM The version check in this script is used to avoid commit battles
REM between different developers that use different astyle versions as
REM different versions might have different output (this has happened in
REM the past).

REM Get the directory of the script
SET SCRIPT_DIR=%~dp0

REM Change to that directory
CD /d %SCRIPT_DIR%

REM To require a newer astyle version, update ASTYLE_VERSION below.
REM ASTYLE_VERSION_STR is then constructed to match the beginning of the
REM version string reported by "astyle --version".
SET ASTYLE_VERSION="3.4.13"
SET ASTYLE_VERSION_STR="Artistic Style Version %ASTYLE_VERSION%"

REM Prefer system astyle; else try uvx; else pipx
SET "ASTYLE=astyle"
where astyle >nul 2>nul
IF %errorlevel% neq 0 (
  where uvx >nul 2>nul
  IF %errorlevel% eq 0 (
    SET "ASTYLE=uvx --quiet astyle==%ASTYLE_VERSION%"
  ) ELSE (
    where pipx >nul 2>nul
    IF %errorlevel% eq 0 (
      SET "ASTYLE=pipx run --quiet astyle==%ASTYLE_VERSION%"
    ) ELSE (
      ECHO ERROR: Neither astyle, uvx, nor pipx found in PATH.
      GOTO EXIT_ERROR
    )
  )
)

SET DETECTED_VERSION_STR=""
FOR /F "tokens=*" %%i IN ('%ASTYLE% --version') DO SET DETECTED_VERSION_STR=%%i
ECHO %DETECTED_VERSION_STR% | FINDSTR /B /C:%ASTYLE_VERSION_STR% > nul && (
    ECHO "%DETECTED_VERSION_STR%" matches %ASTYLE_VERSION_STR%
) || (
    ECHO You should use: %ASTYLE_VERSION_STR%
    ECHO Detected: "%DETECTED_VERSION_STR%"
    GOTO EXIT_ERROR
)

REM Run astyle with the project config
%ASTYLE% --project *.cpp
%ASTYLE% --project *.h
GOTO :EOF

:EXIT_ERROR
EXIT /b 1
