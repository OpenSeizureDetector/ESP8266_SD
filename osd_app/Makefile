# Simple makefile for simple example
PROGRAM=osd_app
PROGRAM_SRC_DIR=. ./i2c ./adxl345
EXTRA_CFLAGS=-I./fsdata
EXTRA_COMPONENTS=extras/mbedtls extras/httpd
include /usr/local/esp8266/esp-open-rtos/common.mk

html:
	@echo "Generating fsdata..."
	cd fsdata && ./makefsdata
