/*
 * SpiManager.cpp
 *
 *  Created on: 30.03.2018
 *      Author: Maxi
 */
#include "SpiManager.h"
#include <map>

std::map<int16_t, DigitalOut*> spiPinMap;

//PinName mosi, PinName miso, PinName sclk
SPI spiObj(SPI_MOSI, SPI_MISO, SPI_SCK);

/* Sets up the SPI bus with pins */
bool SpiManager_Setup(int32_t freq, SpiMode mode, const int16_t* slaveSelects, size_t numSlaveSelects) {

	spiObj.frequency((int) freq);
	//SpiMode matches the description of the parameter signature
	spiObj.format(8, (int) mode);

	for(size_t i=0; i < numSlaveSelects; i++) {
		int16_t ss = slaveSelects[i];

		//Is this pin already in the map?!
		if(spiPinMap.count(ss) != 0) {
			//Not found
			debugSerial.printf("SS=%d already initialized\n",(int)ss);
			continue;
		}

		debugSerial.printf("Initializing SPI SS=%d\n", (int) ss);
		spiPinMap[ss]
				  = new DigitalOut((PinName) ss);
	}
	return true;
}

/* Transfers a given buffer over SPI */
void SpiManager_Transfer(int16_t slaveSelect, const uint8_t* txData, size_t txDataLen, uint8_t* rxData) {

	//Check if slave select pin is currently mapped
	if(spiPinMap.count(slaveSelect) == 0) {
		//Not found
		debugSerial.printf("Requested SPI transfer for SS=%d, not mapped\n",(int)slaveSelect);
		return;
	}

	spiObj.lock();

	//Set SPI slave select accordingly
	//(should actually honor SPI mode settings, but okay for now)
	DigitalOut* ss = spiPinMap[slaveSelect];
	*ss = 0;

	int written = spiObj.write((const char*) txData, (int) txDataLen, (char*) rxData, (int) txDataLen);
	if(written != (int)txDataLen) {
		debugSerial.printf("WARNING: SPI transfer called wrote only %d of %d bytes\n", written, (int) txDataLen);
	}

	*ss = 1;
	spiObj.unlock();
}

void SpiManager_Reset() {
	for(std::map<int16_t, DigitalOut*>::iterator it = spiPinMap.begin(); it != spiPinMap.end(); it++) {
		int16_t pin = it->first;
		DigitalOut* gpioPin = it->second;
		delete gpioPin;
		debugSerial.printf("Unregistering SPI SS pin %d\n", (int)pin);
	}
	spiPinMap.clear();
}

