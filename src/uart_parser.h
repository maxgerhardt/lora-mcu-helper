/*
 * uart_parser.h
 *
 *  Created on: 30.03.2018
 *      Author: Maxi
 */

#ifndef SRC_UART_PARSER_H_
#define SRC_UART_PARSER_H_

#include <stdint.h>
#include <rtos.h> //For Queue
#include "project_settings.h"

/* Sucess / error values */
#define MCU_HELPER_ACK 0x01
#define MCU_HELPER_NACK 0x02

/* Serial port configuration */
#define MCU_HELPER_RX_BUF_SIZE 1024
#define MCU_COMMAND_MAX_DATA_LEN 512

enum McuCommand {
	SpiSetup = 0x01,
	SpiSetupAnswer = 0x02,
	SpiTransfer = 0x03,
	SpiTransferAnswer = 0x04,
	InterruptRegister = 0x05,
	InterruptRegisterAnswer = 0x06,
	InterruptEvent = 0x07,
	Reset = 0x08,
	ResetAnswer = 0x09,
};

#define MCUCOMMAND_MIN_VALID	0x01
#define MCUCOMMAND_MAX_VALID	0x09

/* Frame format (tag-length-value)
    1      2          <len>     bytes
   cmd |  len     |  data ...
*/
struct McuCommandPacket {
	McuCommand command;
	uint16_t len;
	uint8_t data[MCU_COMMAND_MAX_DATA_LEN];
};

/* Interrupt function for UART RX */
void McuHelper_ParserFunc();

/* Queue where parsed packets will be put into */
void McuHelper_RegisterQueue(Queue<McuCommandPacket, MCUHELPER_QUEUE_LEN>* queue);

/* Sends the packet over serial */
void McuHelper_SendPacket(const McuCommandPacket& packet);

/* Should be called when processing of a received packet is done */
void McuHelper_ReleasePacket(McuCommandPacket* pkt);

#endif /* SRC_UART_PARSER_H_ */
