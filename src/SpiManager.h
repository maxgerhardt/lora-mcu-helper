/*
 * SpiManager.h
 *
 *  Created on: 30.03.2018
 *      Author: Maxi
 */

#ifndef SRC_SPIMANAGER_H_
#define SRC_SPIMANAGER_H_

#include <stdint.h>
#include "project_settings.h"

enum SpiMode {
	Mode0 = 0x00, Mode1 = 0x01, Mode2 = 0x03, Mode3 = 0x04
};

#define SPI_MAX_SLAVE_SELECTS 8

struct SpiSetupDataHeader {
	int32_t freq;			/* frequency */
	SpiMode spiMode;		/* SPI mode */
};

/* Sets up the SPI bus with pins */
bool SpiManager_Setup(int32_t freq, SpiMode mode, const int16_t* slaveSelects, size_t numSlaveSelects);

/* Transfers a given buffer over SPI */
void SpiManager_Transfer(int16_t slaveSelect, const uint8_t* txData, size_t txDataLen, uint8_t* rxData);

/* Reset all pin mappings */
void SpiManager_Reset();

#endif /* SRC_SPIMANAGER_H_ */
