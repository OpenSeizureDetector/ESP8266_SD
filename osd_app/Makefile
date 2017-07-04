# Simple makefile for simple example
PROGRAM=osd_app
PROGRAM_SRC_DIR=. ./i2c ./adxl345
# The -D directives #define constants to enable things in httpd.h
EXTRA_CFLAGS=-DLWIP_HTTPD_CGI=1 -DLWIP_HTTPD_SSI=1 -I./fsdata


# Add any modules from the 'extras' folder in esp-open-rtos here.
EXTRA_COMPONENTS=extras/mbedtls extras/httpd extras/spiffs

# spiffs configuration
FLASH_SIZE = 32
SPIFFS_BASE_ADDR = 0x200000
SPIFFS_SIZE = 0x010000

# SPIFFS_SINGLETON = 0  # for run-time configuration


include /usr/local/esp8266/esp-open-rtos/common.mk

html:
	@echo "Generating fsdata..."
	cd fsdata && ./makefsdata
