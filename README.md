# Probe-for-I2C-SPI

Send and receive hex bytes to any SPI or I2C device using Arduino Nano directly from terminal program. With this sketch (program), you can rapidly try out communications with the target device as compared to: Edit-Compile-Upload-Run-Repeat cycle.  All devices, no matter how complex internally, are still just 'bytes in' and 'bytes out'.

All commands can be in upper or lower case

M - switch modes between SPI and I2C.

/ - create START condition (I2C) or /CS (SPI).

; - create STOP (I2C) or deselect (SPI). (done automatically at end of command line)

.nnn - read nnn hex bytes and display

R - reverse bit sense for non-standard devices

Wnnn - wait nnn milliseconds

S - scan all I2C addresses

XX - valid hex bytes sent by default must be 2 digits each, spaces optional between bytes



Examples:

SPI>/030000.8   read 1st 8 bytes from SPI EEPROM

SPI>/06;/020050 11 22 33 44  enable write, then write 4 hex bytes to location 0x50

I2C>/a00140 12 34 56    write 3 bytes to location 0x140

I2C>/a0000/a1.16       read 1st 16 bytes of EEPROM

I2C>S                   scan all 256 addresses for valid addresses

