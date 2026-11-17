@echo off
setlocal
if not defined SORTER_IDF_PATH set "SORTER_IDF_PATH=C:\esp\v6.0.2\esp-idf"
set "IDF_PATH=%SORTER_IDF_PATH%"
call "%IDF_PATH%\export.bat"
if errorlevel 1 exit /b 1
pushd "%~dp0.."
python "%IDF_PATH%\tools\idf.py" -B build/verify-hardware -D "SDKCONFIG=%CD%\build\sdkconfig.verify-hardware" -D "SDKCONFIG_DEFAULTS=sdkconfig.defaults;config/verify-hardware.defaults" build
set "result=%errorlevel%"
popd
exit /b %result%
