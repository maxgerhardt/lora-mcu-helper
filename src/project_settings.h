/*
 * project_settings.h
 *
 *  Created on: 30.03.2018
 *      Author: Maxi
 */

#ifndef SRC_PROJECT_SETTINGS_H_
#define SRC_PROJECT_SETTINGS_H_

#include <mbed.h>
extern mbed::Serial mcuSerial;
extern mbed::Serial debugSerial;

/* Defines general project settings and Pinouts */
#include <PinNames.h>

/* UART pins for the communication. just route through the debug probe. */
#define MCUHELPER_UART_RX USBRX
#define MCUHELPER_UART_TX USBTX
#define MCUHELPER_BAUD 115200

#define MCUHELPER_QUEUE_LEN 5
#define INTERRUPT_QUEUE_LEN 5
#define INTERRUPT_MAX_PINS (10)

#define DEBUG_UART_RX PC_11
#define DEBUG_UART_TX PC_10
#define DEBUG_UART_BAUD 115200

#endif /* SRC_PROJECT_SETTINGS_H_ */
