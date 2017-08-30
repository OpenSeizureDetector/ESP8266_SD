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


/* These undefines prevent SYLT-FFT using assembler code */
#undef __ARMCC_VERSION
#undef __arm__
#include "SYLT-FFT/fft.h"

#include "osd_app.h"


/* GLOBAL VARIABLES */
// Initialise the Analysis Data Structure An_Data (anD)
An_Data anD;

fft_complex_t *fftData;   // spectrum calculated by FFT



/*************************************************************
 * Data Analysis
 *************************************************************/

/*********************************************
 * Returns the magnitude of a complex number
 * (well, actually magnitude^2 to save having to do
 * a square root.
 */
int getMagnitude(fft_complex_t c) {
  int mag;
  mag = c.r*c.r + c.i*c.i;
  return mag;
}

int getAmpl(int nBin) {
  return fftData[nBin].r;
}


/***********************************************
 * Analyse spectrum and set alarm condition if
 * appropriate.
 * This routine assumes it is called every second to check the 
 * spectrum for an alarm state.
 */
int alarm_check() {
  bool inAlarm;
  int i;
  if (debug) APP_LOG(APP_LOG_LEVEL_DEBUG,"Alarm Check anD.nMin=%d, nMax=%d",anD.nMin,anD.nMax);

  inAlarm = false;
  anD.alarmRoi = 0;
  if (sdS.sdMode == SD_MODE_FFT) {
    inAlarm = (anD.roiPower>sdS.alarmThresh) && (anD.roiRatio>sdS.alarmRatioThresh);
  }
  // Check each of the multiple ROIs - any one being in alarm state is an alarm.
  else if (sdS.sdMode == SD_MODE_FFT_MULTI_ROI) {
    if (anD.roiPower>sdS.alarmThresh) {
      for (i=0;i<=3;i++) {
	if (anD.roiRatios[i]>sdS.alarmRatioThresh) {
	  inAlarm = true;
	  anD.alarmRoi = i;
	  if (debug) APP_LOG(APP_LOG_LEVEL_DEBUG,"doAnalysis() - alarm in ROI %d", anD.alarmRoi);
	}
      }
    }
  }
  
  if (inAlarm) {
    anD.alarmCount+=sdS.samplePeriod;
    if (anD.alarmCount>sdS.alarmTime) {
      anD.alarmState = 2;
    } else if (anD.alarmCount>sdS.warnTime) {
      anD.alarmState = 1;
    }
  } else {
    anD.alarmState = 0;
    anD.alarmCount = 0;
  }
  if (debug) APP_LOG(APP_LOG_LEVEL_DEBUG,"alarmState = %d, alarmCount=%d",anD.alarmState,anD.alarmCount);

  return(anD.alarmState);
}


/**
 * accel_handler():  Called whenever accelerometer data is available.
 * Add data to circular buffer accData[] and increments accDataPos to show
 * the position of the latest data in the buffer.
 */
void accel_handler(ADXL345_IVector *data, uint32_t num_samples) {
  int i;

  //if (debug) APP_LOG(APP_LOG_LEVEL_DEBUG,"accel_handler(): num_samples=%d",num_samples);
  if (sdS.sdMode==SD_MODE_RAW) {
    if (debug) APP_LOG(APP_LOG_LEVEL_DEBUG,"num_samples=%d",num_samples);
    //sendRawData(data,num_samples);
  } else {
    // Add the new data to the accData buffer
    for (i=0;i<(int)num_samples;i++) {
      // Wrap around the buffer if necessary
      if (anD.accDataPos>=anD.nSamp) { 
	anD.accDataPos = 0;
	anD.accDataFull = 1;
	// a separate task looks for anD.accDataFull being true, and does
	// the analysis if this is the case.
	//do_analysis();
	break;
      }
      // add good data to the accData array
      anD.accData[anD.accDataPos] =
	abs(data[i].XAxis)
	+ abs(data[i].YAxis)
	+ abs(data[i].ZAxis);
	anD.accDataPos++;
    }
    anD.latestAccelData = data[num_samples-1];
  }
}

/****************************************************************
 * Simple threshold analysis to chech for fall.
 * Called from clock_tick_handler()
 */
void check_fall() {
  int i,j;
  int minAcc, maxAcc;

  int fallWindowSamp = (sdS.fallWindow*sdS.sampleFreq)/1000; // Convert ms to samples.
  APP_LOG(APP_LOG_LEVEL_DEBUG,"check_fall() - fallWindowSamp=%d",
	  fallWindowSamp);
  // Move window through sample buffer, checking for fall.
  anD.fallDetected = 0;
  for (i=0;i<anD.nSamp-fallWindowSamp;i++) {  // i = window start point
    // Find max and min acceleration within window.
    minAcc = anD.accData[i];
    maxAcc = anD.accData[i];
    for (j=0;j<fallWindowSamp;j++) {  // j = position within window
      if (anD.accData[i+j]<minAcc) minAcc = anD.accData[i+j];
      if (anD.accData[i+j]>maxAcc) maxAcc = anD.accData[i+j];
    }
    if ((minAcc<sdS.fallThreshMin) && (maxAcc>sdS.fallThreshMax)) {
      if (debug) APP_LOG(APP_LOG_LEVEL_DEBUG,"check_fall() - minAcc=%d, maxAcc=%d",
	      minAcc,maxAcc);
      if (debug) APP_LOG(APP_LOG_LEVEL_DEBUG,"check_fall() - ****FALL DETECTED****");
      anD.fallDetected = 1;
      return;
    }
  }
  //if (debug) APP_LOG(APP_LOG_LEVEL_DEBUG,"check_fall() - minAcc=%d, maxAcc=%d",
  //	  minAcc,maxAcc);

}


/****************************************************************
 * Carry out analysis of acceleration time series to check for seizures
 * Called from accel_handler when buffer is full.
 */
void do_analysis() {
  int i,n;
  APP_LOG(APP_LOG_LEVEL_DEBUG,"do_analysis");
  // Calculate the frequency resolution of the output spectrum.
  // Stored as an integer which is 1000 x the frequency resolution in Hz.

  if (debug) APP_LOG(APP_LOG_LEVEL_DEBUG,"do_analysis");
  anD.freqRes = (int)(1000*sdS.sampleFreq/anD.nSamp);
  if (debug) APP_LOG(APP_LOG_LEVEL_DEBUG,"T=%d ms, anD.freqRes=%d Hz/(1000 bins)",
		     1000*anD.nSamp/sdS.sampleFreq,anD.freqRes);
  // Set the frequency bounds for the analysis in fft output bin numbers.
  anD.nMin = (int)(1000*sdS.alarmFreqMin/anD.freqRes);
  anD.nMax = (int)(1000*sdS.alarmFreqMax/anD.freqRes);

  // Set frequency bounds for multi-ROI mode
  // ROI 0 is the whole ROI
  anD.nMins[0] = anD.nMin;
  anD.nMaxs[0] = anD.nMax;

  // ROI 1 is the lower half
  anD.nMins[1] = anD.nMin;
  anD.nMaxs[1] = (anD.nMin+anD.nMax)/2;

  // ROI 2 is the upper half
  anD.nMins[2] = (anD.nMin+anD.nMax)/2;
  anD.nMaxs[2] = anD.nMax;

  // ROI 3 is the middle half
  anD.nMins[3] = anD.nMin + (anD.nMax-anD.nMin)/4;
  anD.nMaxs[3] = anD.nMax - (anD.nMax-anD.nMin)/4;
  
  // Calculate the bin number of the cutoff frequency
  anD.nFreqCutoff = (int)(1000*sdS.freqCutoff/anD.freqRes);  

  if (debug) APP_LOG(APP_LOG_LEVEL_DEBUG,"do_analysis():  anD.nMin=%d, nMax=%d, anD.nFreqCutoff=%d, anD.fftBits=%d, anD.nSamp=%d",
		     anD.nMin,anD.nMax,anD.nFreqCutoff,anD.fftBits,anD.nSamp);

  if (debug) for (i=0;i<=3;i++) {
      APP_LOG(APP_LOG_LEVEL_DEBUG,"do_analysis(): anD.nMins[%d]=%d, nMaxs[%d]=%d",
	      i,anD.nMins[i],i,anD.nMaxs[i]);
    }

  // Do the FFT conversion from time to frequency domain.
  // The output is stored in accData.  fftData is a pointer to accData.
  fft_fftr((fft_complex_t *)anD.accData,anD.fftBits);


  // Ignore position zero though (DC component)
  if (debug) APP_LOG(APP_LOG_LEVEL_DEBUG,"Calculating anD.specPower - anD.nSamp=%d",anD.nSamp);
  anD.specPower = 0;
  for (i=1;i<anD.nSamp/2;i++) {
    // Find absolute value of the imaginary fft output.
    if (i<=anD.nFreqCutoff) {
      anD.specPower = anD.specPower + getMagnitude(fftData[i]);
    } else {
      //if (debug) APP_LOG(APP_LOG_LEVEL_DEBUG,"i = %d, zeroing data above cutoff frequency",i);
      fftData[i].r = 0;
    }
    // fftResults is used by UI to display spectrum.
    anD.fftResults[i] = getMagnitude(fftData[i]);
  }
  // specPower is average power per bin for whole spectrum.
  anD.specPower = anD.specPower/(anD.nSamp/2);
  if (debug) APP_LOG(APP_LOG_LEVEL_DEBUG,"specPower=%d",anD.specPower);

  // calculate spectrum power in the region of interest
  anD.roiPower = 0;
  for (int i=anD.nMin;i<anD.nMax;i++) {
    anD.roiPower = anD.roiPower + getMagnitude(fftData[i]);
  }
  // roiPower is average power per bin within ROI.
  anD.roiPower = anD.roiPower/(anD.nMax-anD.nMin);
  if (debug) APP_LOG(APP_LOG_LEVEL_DEBUG,"specPower=%ld, roiPower=%ld",
		     anD.specPower, anD.roiPower);
  if  (anD.specPower!=0) {
    anD.roiRatio = 10 * anD.roiPower/anD.specPower;
  } else {
    if (debug) APP_LOG(APP_LOG_LEVEL_DEBUG,"WARNING: specPower=%ld, setting ratio to zero",
		       anD.specPower);
    anD.roiRatio = 0;
  }

  // calculate spectrum power in each of the regions of interest
  // for multi-ROI mode.
  anD.roiPowers[0] = anD.roiPower;  
  for (n=1;n<=3;n++) {
    anD.roiPowers[n]=0;
    for (int i=anD.nMins[n];i<anD.nMaxs[n];i++) {
      anD.roiPowers[n] = anD.roiPowers[n] + getMagnitude(fftData[i]);
    }
    // roiPower is average power per bin within ROI.
    anD.roiPowers[n] = anD.roiPowers[n]/(anD.nMaxs[n]-anD.nMins[n]);
    if (anD.specPower!=0)
      anD.roiRatios[n] = 10 * anD.roiPowers[n]/anD.specPower;
    else
      anD.roiRatios[n] = 0;
    if (debug) APP_LOG(APP_LOG_LEVEL_DEBUG,"roiPower[%d]=%d",n,anD.roiPowers[n]);
  }
  

  // Calculate the simplified spectrum - power in 1Hz bins.
  for (int ifreq=0;ifreq<10;ifreq++) {
    int binMin = 1 + 1000*ifreq/anD.freqRes;    // add 1 to loose dc component
    int binMax = 1 + 1000*(ifreq+1)/anD.freqRes;
    anD.simpleSpec[ifreq]=0;
    for (int ibin=binMin;ibin<binMax;ibin++) {
      anD.simpleSpec[ifreq] = anD.simpleSpec[ifreq] + getMagnitude(fftData[ibin]);
    }
    anD.simpleSpec[ifreq] = anD.simpleSpec[ifreq] / (binMax-binMin);
  }

  alarm_check();
  
  /* Start collecting new buffer of data */
  /* FIXME = it would be best to make this a rolling buffer and analyse it
  * more frequently.
  */
  anD.accDataPos = 0;
  anD.accDataFull = 0;
}

void analysis_init() {
  int nsInit;  // initial number of samples per period, before rounding
  int i,ns;
  // Zero all data arrays:
  for (i = 0; i<NSAMP_MAX; i++) {
    anD.accData[i] = 0;
  }

  // Initialise analysis of accelerometer data.
  // get number of samples per period, and round up to a power of 2
  nsInit = sdS.samplePeriod * sdS.sampleFreq;
  if (debug) APP_LOG(APP_LOG_LEVEL_DEBUG, "sdS.samplePeriod=%d, sdS.sampleFreq=%d - nsInit=%d",
	  sdS.samplePeriod,sdS.sampleFreq,nsInit);

  for (i=0;i<1000;i++) {
    ns = 2<<i;
      if (debug) APP_LOG(APP_LOG_LEVEL_DEBUG, "i=%d  ns=%d nsInit = %d",
	    i,ns,nsInit);
    if (ns >= nsInit) {
      anD.nSamp = ns;
      anD.fftBits = i;
      break;
    }
  }
  if (debug) APP_LOG(APP_LOG_LEVEL_DEBUG, "nSamp rounded up to %d",
		     anD.nSamp);

  
  /* Subscribe to acceleration data service */
  //if (debug) APP_LOG(APP_LOG_LEVEL_DEBUG,"Analysis Init:  Subcribing to acceleration data at frequency %d Hz",sdS.sampleFreq);
  //accel_data_service_subscribe(25,accel_handler);
  // Choose update rate
  //accel_service_set_sampling_rate(sdS.sampleFreq);

  fftData = (fft_complex_t*)anD.accData;
}

