#!/usr/bin/python
#
# Simple script to echo the ttyUSB0 serial port to the console at 74880 baud.
# Based on http://www.esp8266.com/viewtopic.php?p=33650.
# I found it really really hard to do using standard tools like cu...
# By doing ./monitory.py and resetting the esp8226 (while it is connected
# via USB) you can see the boot up messages and anything you 'printf'
# to stdout.
#
import sys
from serial import Serial

dev = Serial("/dev/ttyUSB0", 74880)
while True:
    c = dev.read(1)
    sys.stdout.write(c)


