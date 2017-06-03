/*
 * OpenSeizureDetector - ESP8266 Version.
 * Based on ESPRESSIF sample project (MIT Licensed)
 */

/*
 * ESPRSSIF MIT License
 *
 * Copyright (c) 2015 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
#include <esp_common.h>
#include <i2c_master.h>
#include "esp_common.h"
#include "osd_app.h"

#define DEV_ADDR 0x1D
#define REG_ADDR 0x32


/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
*******************************************************************************/
uint32 user_rf_cal_sector_set(void)
{
    flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;
        case FLASH_SIZE_64M_MAP_1024_1024:
            rf_cal_sec = 2048 - 5;
            break;
        case FLASH_SIZE_128M_MAP_1024_1024:
            rf_cal_sec = 4096 - 5;
            break;
        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}

/***************************************************/
// i2c functions 
void read_reg(int *result, uint8_t reg_addr) {
	uint8_t ack = 1;
	i2c_master_start();
	//ack = i2c_master_writeByte((DEV_ADDR << 1) + 0);
	i2c_master_writeByte((DEV_ADDR << 1) + 0);
	if (!ack) {
		printf("addr not ack when tx write command \n");
		i2c_master_stop();
	}
	//ack = i2c_master_writeByte(reg_addr);
	i2c_master_writeByte(reg_addr);
	if (!ack) {
		printf("register addr not ack \n");
		i2c_master_stop();
	}
	os_delay_us(40000);
	i2c_master_stop();
	i2c_master_start();
	//ack = i2c_master_writeByte((DEV_ADDR << 1) + 1);
        i2c_master_writeByte((DEV_ADDR << 1) + 1);
	if (!ack) {
		printf("read device addr not ack when tx write command \n");
		i2c_master_stop();
	}
	os_delay_us(40000);
	int res = i2c_master_readByte();
	i2c_master_stop();
	*result = res;
	printf("reading %x succeed\n", res);
}

void read_acc_reg(int32 client_sock, uint8_t reg_addr) {
	uint8_t i, ack = 1; 
	short x, y, z;
	char buf[100];
	short temp[6];
	i2c_master_start();
	//ack = i2c_master_writeByte((DEV_ADDR << 1) + 0);
	i2c_master_writeByte((DEV_ADDR << 1) + 0);
	if (!ack) {
		printf("addr not ack when tx write command \n");
		i2c_master_stop();
	}
	//ack = i2c_master_writeByte(reg_addr);
	i2c_master_writeByte(reg_addr);
	if (!ack) {
		printf("register addr not ack \n");
		i2c_master_stop();
	}
	os_delay_us(40000);
	i2c_master_stop();
	i2c_master_start();
	//ack = i2c_master_writeByte((DEV_ADDR << 1) + 1);
	i2c_master_writeByte((DEV_ADDR << 1) + 1);
	if (!ack) {
		printf("read device add not ack\n");
		i2c_master_stop();
	}
	os_delay_us(40000);
	for (i = 0; i < 6; ++i) {
		temp[i] = i2c_master_readByte();
		if (i == 5) {
			i2c_set_ack(false);
		} else {
			i2c_set_ack(true);
		}
	}
	i2c_master_stop();
	x = (temp[1] << 8 | temp[0]);
	y = (temp[3] << 8 | temp[2]);
	z = (temp[5] << 8 | temp[4]);
	printf("x:%d y:%d z:%d \n", x, y, z);   
	sprintf(buf, "x:%d y:%d z:%d \n", x, y, z);
	write(client_sock, buf, strlen(buf));
}

void write_to(int32 client_sock, uint8_t reg_addr, uint8_t val) {
	uint8_t ack=1;
	i2c_master_start();
	//ack = i2c_master_writeByte((DEV_ADDR << 1) + 0);
	i2c_master_writeByte((DEV_ADDR << 1) + 0);
	if (!ack) {
		printf("addr not ack when tx write command \n");
		i2c_master_stop();
	}	
	os_delay_us(40000);
	//ack = i2c_master_writeByte(reg_addr);
	i2c_master_writeByte(reg_addr);
	if (!ack) {
		printf("reg addr not ack when tx write command \n");
		i2c_master_stop();
	}	
	os_delay_us(40000);
	//ack = i2c_master_writeByte(val);
        i2c_master_writeByte(val);
	if (!ack) {
		printf("val not ack when tx write command \n");
		i2c_master_stop();
	}
	i2c_master_stop();
}

void adxl_init() {
  printf("adxl_init");
  int response;
  i2c_master_init();
  // read device id.
  read_reg(&response, 0x00);
  printf("device: %x\n", response);
}




LOCAL void ICACHE_FLASH_ATTR LEDBlinkTask(void *pvParam) {
  while(1) {
    vTaskDelay(1000/portTICK_RATE_MS);
    printf("Blink...\n");
    if (GPIO_REG_READ(GPIO_OUT_ADDRESS) & BIT2)
      {
	// set gpio low
	gpio_output_set(0,BIT2,BIT2,0);
      }
    else
      {
	// set gpio high
	gpio_output_set(BIT2,0,BIT2,0);
      }
  }
}




/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void user_init(void)
{
  printf("*****************************************\n");
  printf("* OpenSeizureDetector - ESP8266 Version *\n");
  printf("SDK version:%s\n", system_get_sdk_version());
  printf("*****************************************\n\n");


  adxl_init();
  
   // Config pin as GPIO12
  PIN_FUNC_SELECT (PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12);

  xTaskCreate(LEDBlinkTask,(signed char *)"Blink",256,NULL,2,NULL);


}

