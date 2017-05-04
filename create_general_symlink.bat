set drive=%~dp0
set drivep=%drive%

mklink  %drivep%\ESP14MES\general.h %drivep%\ESP8266EASTRON\general.h
mklink  %drivep%\ESP14MES\general.ino %drivep%\ESP8266EASTRON\general.ino
