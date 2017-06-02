#!/bin/sh

#	$(ESPTOOL) --port $(ESPPORT) write_flash $(FW_FILE_1_ADDR) $(FW_FILE_1) $(FW_FILE_2_ADDR) $(FW_FILE_2)

esptool.py write_flash 0x00000 bin/eagle.flash.bin 0x20000 bin/eagle.irom0text.bin

