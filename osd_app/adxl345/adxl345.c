/*
 * ADXL345 Library
 */

#include <stdio.h>
//#include "espressif/esp_common.h"
//#include "../i2c/i2c.h"
#include "adxl345.h"

uint8_t ADXL345_devAddr = 0xff;  // 7 bit i2c address range so this is off scale to
                         //    show  it is not initialised yet.
//uint8_t ADXL345_SCL = 0xff;
//uint8_t ADXL345_SDA = 0xff;


/* Reads a single byte from the ADXL345 at address devAddr, register regAddr
 * @return: the byte read, or -1 on error.
 */
uint8_t i2c_readRegister8(uint8_t regAddr) {
  uint8_t byte;
  if (i2c_slave_read(ADXL345_devAddr, &regAddr, &byte, 1)) {
    // non-zero response = error
    //printf("i2c_readRegister8(0x%x, 0x%x) - Error\n",devAddr,regAddr);
    return -1;
  } else {
    return byte;
  }
}

/* Reads a 2 byte word from the ADXL345 at address devAddr, register regAddr
 * @return: the word read, or -1 on error.
 */
uint16_t i2c_readRegister16(uint8_t regAddr) {
  uint16_t data;
  if (i2c_slave_read(ADXL345_devAddr, &regAddr, (uint8_t*)&data, 2)) {
    // non-zero response = error
    //printf("i2c_readRegister8(0x%x, 0x%x) - Error\n",devAddr,regAddr);
    return -1;
  } else {
    return data;
  }
}


/* Writes a single byte t the ADXL345 at address devAddr, register regAddr
 * @return: 0 on success, or -1 on error.
 */
uint8_t i2c_writeRegister8(uint8_t regAddr, uint8_t byte) {
  if (i2c_slave_write(ADXL345_devAddr, &regAddr, &byte, 1)) {
    // non-zero response = error
    //printf("i2c_writeRegister8(0x%x, 0x%x) - Error\n",devAddr,regAddr);
    return -1;
  } else {
    return 0;
  }
}


/**
 * Scan the i2c bus for the first ADXL345 chip on the bus.
 * @return the address of the first ADXL345 chip on the bus.
 * on Exit ADXL345_devAddr global variable is set to id of device.
 */
uint8_t ADXL345_findDevice() {
  uint8_t id;
  uint8_t i;
  for (i=0;i<127;i++) {
    ADXL345_devAddr = i;
    id = i2c_readRegister8(ADXL345_REG_DEVID);
    if (id!=0xff) {
      printf("Response received from address %d (0x%x) - id=%d (0x%x)\n",i,i,id,id);
      if ((i==ADXL345_ADDRESS) || (i==ADXL345_ALT_ADDRESS)) {
	printf("Address correct for ADXL345 chip.\n");
	if (id==ADXL345_DEVID) {
	  printf("Device ID correct for ADXL345\n");
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



uint8_t ADXL345_init(int scl, int sda) {
  printf("ADXL345_init(%d,%d)\n",scl,sda);
  i2c_init(scl,sda);
  ADXL345_devAddr = ADXL345_findDevice();

  printf("ADXL345 found at address %x - initialising it for measurement\n",
	 ADXL345_devAddr);
  
  if (i2c_writeRegister8(ADXL345_REG_POWER_CTL,0x08)) {
    printf("Error setting measurement Mode\n");
  } else {
    printf("Measurement Mode set\n");
  }
  
  printf("ADXL345_init() complete\n");
  return(ADXL345_devAddr);
}



/* read the X axis acceleration */
uint16_t ADXL345_getXAcc() {
  uint16_t xAcc;
  printf("adxl345_getXacc\n");
  if ((xAcc = i2c_readRegister8(ADXL345_REG_DATAX0))==0xff) {
    printf("Error reading accel\n");
    return -1;
  } else {
    printf("Accel=%d\n",xAcc);
  }
  return xAcc;
}


// Read raw values
ADXL345_Vector ADXL345_readRaw(void)
{
  ADXL345_Vector r;

  r.XAxis = i2c_readRegister16(ADXL345_REG_DATAX0);
  r.YAxis = i2c_readRegister16(ADXL345_REG_DATAY0);
  r.ZAxis = i2c_readRegister16(ADXL345_REG_DATAZ0);
  return r;
}
