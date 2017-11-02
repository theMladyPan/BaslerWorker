# BaslerWorker
v0.0
Very small app made for getting TCP message, capturing image from connected basler camera and then saving it locally.
Compiled and tested mainly under VS2017 x64

Basler worker waits for connection on configured (_defaults.cfg_) port. 
Then after succesfull receiving of regular expression: ^[0-9]{14}[A-Za-z0-9]+.jpg|png$ which means 
8 digits of serial number of camera following by 6 digits of exposure in microseconds 
following by sequence of alphanumeric characters ended by trailing extension of 
desired filename (.jpg or .png), executes grabbing and waits for _TIMEOUT_ amount of time 
for image which is then saved to desired filename.