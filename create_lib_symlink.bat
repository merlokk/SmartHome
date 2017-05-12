set drive=%~dp0
set drivep=%drive%

mklink /D %drivep%\..\libraries\lib %drivep%\lib
pause
