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
#include "jsmn.h"
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

  // initialise settings structure with default values.
  sdS.samplePeriod = SAMPLE_PERIOD_DEFAULT;
  sdS.sampleFreq = SAMPLE_FREQ_DEFAULT;
  sdS.freqCutoff = FREQ_CUTOFF_DEFAULT;
  sdS.dataUpdatePeriod = DATA_UPDATE_PERIOD_DEFAULT;
  
  readSdSettings();
  saveSdSettings();
  readSdSettings();

  if (sdS.samplePeriod == SAMPLE_PERIOD_DEFAULT)
    printf("Success - sample period read correctly!\n");


  readWifiSettings();
  //saveWifiSettings();
}


/**
 * readSdettings() - read the settings struct sdS from file
 * in binary format.
 */
void readSdSettings() {
  APP_LOG(APP_LOG_LEVEL_INFO, "readSdSettings()");
  spiffs_file fd = SPIFFS_open(&fs, ANALYSIS_SETTINGS_FNAME, SPIFFS_RDONLY, 0);
  if (fd < 0) {
    printf("Error opening file %s\n",ANALYSIS_SETTINGS_FNAME);
    return;
  }  
  int read_bytes = SPIFFS_read(&fs, fd, &sdS, sizeof(sdS));
  printf("Read %d bytes from file %s into settings structure\n", read_bytes,ANALYSIS_SETTINGS_FNAME);  
  SPIFFS_close(&fs, fd);
}

/**
 * saveSdSettings() - write the settings struct sdS to file
 * in binary format.
 */
void saveSdSettings() {
  APP_LOG(APP_LOG_LEVEL_INFO, "save_settings()");

  int fd = open(ANALYSIS_SETTINGS_FNAME, O_WRONLY|O_CREAT, 0);
  if (fd < 0) {
    printf("Error opening file %s\n",ANALYSIS_SETTINGS_FNAME);
    return;
  }
  int written = write(fd, &sdS, sizeof(sdS));
  printf("Written %d bytes to file %s\n", written,ANALYSIS_SETTINGS_FNAME);
  close(fd);
}

/**
 * readWifiettings() - read the settings struct wifiS from file
 * in binary format.
 */
void readWifiSettings() {
  char buf[256];
  APP_LOG(APP_LOG_LEVEL_INFO, "readWifiSettings()");
  spiffs_file fd = SPIFFS_open(&fs, WIFI_SETTINGS_FNAME, SPIFFS_RDONLY, 0);
  if (fd < 0) {
    printf("Error opening file %s - using default values\n",WIFI_SETTINGS_FNAME);
    strncpy(wifiS.ssid,SSID_DEFAULT,sizeof(wifiS.ssid));
    strncpy(wifiS.passwd,PASSWD_DEFAULT,sizeof(wifiS.ssid));
    strncpy(wifiS.serverIp,OSD_SERVER_IP_DEFAULT,sizeof(wifiS.ssid));
  } else {  
    int read_bytes = SPIFFS_read(&fs, fd, &wifiS, sizeof(wifiS));
    printf("Read %d bytes from file %s into wifi settings structure\n",
	   read_bytes,WIFI_SETTINGS_FNAME);  
    SPIFFS_close(&fs, fd);
  }
  wifiSettingsToString(buf,sizeof(buf));
  printf("settings=%s\n",buf);
}

/**
 * saveWifiSettings() - write the settings struct wifiS to file
 * in binary format.
 */
void saveWifiSettings() {
  APP_LOG(APP_LOG_LEVEL_INFO, "saveWifiSettings()");

  int fd = open(WIFI_SETTINGS_FNAME, O_WRONLY|O_CREAT, 0);
  if (fd < 0) {
    printf("Error opening file %s\n",WIFI_SETTINGS_FNAME);
    return;
  }
  int written = write(fd, &wifiS, sizeof(wifiS));
  printf("Written %d bytes to file %s\n", written, WIFI_SETTINGS_FNAME);
  close(fd);
}


/* write the wifi settings as a JSON string to buffer buf of length len
 */
void wifiSettingsToString(char* buf, int len) {
  printf("wifiSettingsToString - len=%d\n",len);
  snprintf(buf,len,"{'ssid':'%s','passwd':'%s','serverip':'%s'}",
	   wifiS.ssid, wifiS.passwd, wifiS.serverIp);
}
