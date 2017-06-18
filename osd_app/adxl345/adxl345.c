/*
 * ADXL345 Library
 */

#include <stdio.h>
//#include "espressif/esp_common.h"
//#include "../i2c/i2c.h"
#include "adxl345.h"

int adxl345Init(int scl, int sda) {
  i2c_init(scl,sda);
  printf("i2c_init() complete\n");
  return(0);
}


/**
 * Initialise the i2c bus and scan the i2c bus for the first ADXL345 chip 
 * on the bus.
 * @return the address of the first ADXL345 chip on the bus.
 */
uint8_t findAdxl345(int scl, int sca) {
  uint8_t id;
  uint8_t reg = 0;
  uint8_t i;
  uint8_t byte;
  adxl345Init(scl,sca);
  for (i=0;i<127;i++) {
    if (i2c_slave_read(i, &reg, &id, 1)) {
      // non-zero response = error
    } else {
      printf("Response received from address %d (0x%x) - id=%d (0x%x)id\n",i,i,id,id);
      if ((i==ADXL345_ADDRESS) || (i==ADXL345_ALT_ADDRESS)) {
	printf("Address correct for ADXL345 chip.\n");
	if (id==ADXL345_DEVID) {
	  printf("Device ID Correct for ADXL345 chip - Chip found at address %x - initialising it for measurement\n",i);
	  reg = ADXL345_REG_POWER_CTL;
	  byte = 0x08;
	  if (i2c_slave_write(i, &reg, &byte, 1)) {
	    printf("Error setting measurement Mode\n");
	  } else {
	    printf("Measurement Mode set\n");
	  }
	  return(i);
	} else {
	  printf("Device ID incorrect - not an ADXL345 chip\n");
	}
      } else {
	printf("Address incorrect - not an ADXL345 chip\n");
      }
    }
  }
  return(-1);
}

/* read the X axis acceleration */
uint16_t adxl345_getXAcc(uint8_t devAddr) {
  uint16_t xAcc;
  uint8_t byte;
  uint8_t reg;
  printf("adxl345_getXacc\n");
  reg = ADXL345_REG_DATAX0;
  if (i2c_slave_read(devAddr, &reg, &byte, 1)) {
    printf("Error reading accel\n");
    return -1;
  } else {
    printf("Accel=%d\n",byte);
  }
  xAcc = byte;
  return xAcc;
}
