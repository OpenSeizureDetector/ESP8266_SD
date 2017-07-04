/*
 * OpenSeizureDetector - ESP8266 Version.
 *
 * ESP8266_SD - a simple accelerometer based seizure detector that runs on 
 * an ESP8266 system on chip, with ADXL345 accelerometer attached by i2c 
 * interface.
 *
 * See http://openseizuredetector.org for more information.
 *
 * Copyright Graham Jones, 2015, 2016, 2017
 *
 * This file is part of ESP8266_SD.
 *
 * ESP8266_SD is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * ESP8266_SD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with ESP8266_SD.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "fcntl.h"
#include "unistd.h"
#include "spiffs.h"
#include "esp_spiffs.h"

#include "osd_app.h"

void settings_init() {
  uint32_t total, used;
  APP_LOG(APP_LOG_LEVEL_INFO, "settings_init()");
  esp_spiffs_init();
  if (esp_spiffs_mount() != SPIFFS_OK) {
    printf("Error mount SPIFFS\n");
  }

  SPIFFS_info(&fs, &total, &used);
  printf("Total: %d bytes, used: %d bytes\n", total, used);
  read_settings();
  save_settings();
  read_settings();

}


void read_settings() {
  APP_LOG(APP_LOG_LEVEL_INFO, "read_settings()");
  
  const int buf_size = 0xFF;
  uint8_t buf[buf_size];
  
  spiffs_file fd = SPIFFS_open(&fs, "other.txt", SPIFFS_RDONLY, 0);
  if (fd < 0) {
    printf("Error opening file\n");
    return;
  }
  
  int read_bytes = SPIFFS_read(&fs, fd, buf, buf_size);
  printf("Read %d bytes\n", read_bytes);
  
  buf[read_bytes] = '\0';    // zero terminate string
  printf("Data: %s\n", buf);
  
  SPIFFS_close(&fs, fd);
}


void save_settings() {
  APP_LOG(APP_LOG_LEVEL_INFO, "save_settings()");

  uint8_t buf[] = "Example data, written by ESP8266";
  
  int fd = open("other.txt", O_WRONLY|O_CREAT, 0);
  if (fd < 0) {
    printf("Error opening file\n");
    return;
  }
  
  int written = write(fd, buf, sizeof(buf));
  printf("Written %d bytes\n", written);
  
  close(fd);
}


