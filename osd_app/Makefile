# Simple makefile for simple example
PROGRAM=osd_app
PROGRAM_SRC_DIR=. ./i2c ./adxl345
# The -D directives #define constants to enable things in httpd.h
EXTRA_CFLAGS=-DLWIP_HTTPD_CGI=1 -DLWIP_HTTPD_SSI=1 -I./fsdata
EXTRA_COMPONENTS=extras/mbedtls extras/httpd
include /usr/local/esp8266/esp-open-rtos/common.mk

html:
	@echo "Generating fsdata..."
	cd fsdata && ./makefsdata
