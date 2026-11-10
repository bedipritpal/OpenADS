@echo off
setlocal EnableExtensions
if "%~1"=="" exit /b 2
if not "%~2"=="32" if not "%~2"=="64" exit /b 2
set "HB=%~f1"
set "KIT=%~dp0"
set "BITS=%~2"
if not exist "%HB%\include\hbapi.h" exit /b 3
if not exist "%HB%\contrib\rddads\ads1.c" exit /b 3
if not exist "%KIT%openace%BITS%.lib" exit /b 3
where cl >nul 2>&1
if errorlevel 1 exit /b 4
where lib >nul 2>&1
if errorlevel 1 exit /b 4
for /f "delims=" %%x in ('git -C "%HB%" status --porcelain') do (
 echo Harbour source is dirty. Use a clean dedicated checkout.
 exit /b 5
)
git -C "%HB%" apply --check "%KIT%patches\harbour\rddads-pending-fields.patch"
if errorlevel 1 exit /b 6
git -C "%HB%" apply "%KIT%patches\harbour\rddads-pending-fields.patch"
if errorlevel 1 exit /b 6
set "OUT=%KIT%harbour-lib%BITS%"
if not exist "%OUT%" mkdir "%OUT%"
for %%u in (ads1 adsfunc adsmgmnt adsx) do (
 cl /nologo /c /O2 /MD /I"%HB%\include" /I"%HB%\contrib\rddads" /I"%KIT%include" /Fo"%OUT%\%%u.obj" "%HB%\contrib\rddads\%%u.c"
 if errorlevel 1 exit /b 7
)
cl /nologo /c /O2 /MD /I"%HB%\include" /I"%KIT%include" /Fo"%OUT%\oads_hb.obj" "%KIT%contrib\oads_hb\oads_hb.c"
if errorlevel 1 exit /b 9
lib /nologo /OUT:"%OUT%\rddads.lib" "%OUT%\ads1.obj" "%OUT%\adsfunc.obj" "%OUT%\adsmgmnt.obj" "%OUT%\adsx.obj" "%OUT%\oads_hb.obj"
if errorlevel 1 exit /b 9
echo Built RDD and OADS wrapper libraries in %OUT%.
echo Link these with this kit's matching openace%BITS%.lib and your Harbour VM.
exit /b 0
