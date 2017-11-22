# BaslerWorker

## Info
Very small app made for getting TCP message, capturing image from connected basler camera and then saving it locally.
Compiled and tested mainly under:

OS | Win10 pro
--------- | ---------
Visual studio | VS2017
architecture | x64
C++ | v11

## Program flow
Basler worker waits for connection on configured (_defaults.cfg_) port.
Then after succesfull receiving of regular expression: ^[0-9]{14}[A-Za-z0-9]+.jpg|png$ which means
8 digits of serial number of camera following by 6 digits of exposure in microseconds
following by sequence of alphanumeric characters ended by trailing extension of
desired filename (.jpg or .png), executes grabbing and waits for _TIMEOUT_ amount of time
for image which is then saved to desired filename.

## Latest Release
[v1.4](https://github.com/theMladyPan/BaslerWorker/releases/tag/v1.4)

## From commit
e684b706c80909cc25eb5df08fba103b3ffec81e
