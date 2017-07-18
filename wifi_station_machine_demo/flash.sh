#!/bin/sh
esptool.py -p /dev/ttyUSB0 --baud 115200 write_flash -fs 32m -fm qio -ff 40m 0x0 bin/eagle.flash.bin 0x20000 ./bin/eagle.irom0text.bin 

