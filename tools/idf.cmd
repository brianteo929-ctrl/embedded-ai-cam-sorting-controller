@echo off
setlocal
if not defined SORTER_IDF_PATH set "SORTER_IDF_PATH=C:\esp\v6.0.2\esp-idf"
set "IDF_PATH=%SORTER_IDF_PATH%"
if not exist "%IDF_PATH%\export.bat" (
  echo Set SORTER_IDF_PATH to your ESP-IDF 6.0.2 installation.
  exit /b 1
)
call "%IDF_PATH%\export.bat"
if errorlevel 1 exit /b 1
pushd "%~dp0.."
if not exist build mkdir build
python "%IDF_PATH%\tools\idf.py" -B build/sorter -D "SDKCONFIG=%CD%\build\sdkconfig.sorter" %*
set "result=%errorlevel%"
popd
exit /b %result%
