set drive=%~dp0
set drivep=%drive%

mklink  %drivep%\ESP14MES\general.h %drivep%\ESP8266EASTRON\general.h
mklink  %drivep%\ESP14MES\general.ino %drivep%\ESP8266EASTRON\general.ino
mklink  %drivep%\ESP8266AZ7798\general.h %drivep%\ESP8266EASTRON\general.h
mklink  %drivep%\ESP8266AZ7798\general.ino %drivep%\ESP8266EASTRON\general.ino
mklink  %drivep%\ESP8266CO2PM25\general.h %drivep%\ESP8266EASTRON\general.h
mklink  %drivep%\ESP8266CO2PM25\general.ino %drivep%\ESP8266EASTRON\general.ino
pause

