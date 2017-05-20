CC = xtensa-lx106-elf-gcc
CFLAGS = -I. -I/usr/local/esp8266/ESP8266_NONOS_SDK/include -mlongcalls
LDLIBS = -nostdlib -Wl,--start-group -lcrypto -lwpa -lwpa2 -lmain -lnet80211  -llwip -lpp -lphy -lc -Wl,--end-group -lgcc
LDFLAGS = -Teagle.app.v6.ld -L/usr/local/esp8266/ESP8266_NONOS_SDK/lib -L/usr/local/esp8266/ESP8266_NONOS_SDK/ld

blinky-0x00000.bin: blinky
	esptool.py elf2image $^

blinky: blinky.o

blinky.o: blinky.c

flash: blinky-0x00000.bin
	esptool.py write_flash 0 blinky-0x00000.bin 0x10000 blinky-0x10000.bin

clean:
	rm -f blinky blinky.o blinky-0x00000.bin blinky-0x40000.bin
