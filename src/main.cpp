#include <mbed.h>
#include <rtos.h>
#include "project_settings.h"
#include "uart_parser.h"
#include "SpiManager.h"
#include "InterruptManager.h"

/* Define serial objects */
Serial mcuSerial(MCUHELPER_UART_TX, MCUHELPER_UART_RX, "mcu", MCUHELPER_BAUD);
Serial debugSerial(DEBUG_UART_TX, DEBUG_UART_RX, "debug", DEBUG_UART_BAUD);

/* Threads and queues for communication */
Thread receiveThread(osPriorityRealtime, 4096);
Queue<McuCommandPacket, MCUHELPER_QUEUE_LEN> cmdQueue;
Thread interruptReceiveThread(osPriorityRealtime, 4096);
Queue<int32_t, INTERRUPT_QUEUE_LEN> intQueue;

/* SPI receive buffer */
uint8_t SpiRxBuffer[MCU_COMMAND_MAX_DATA_LEN];

/* Answer packets for handlers */
McuCommandPacket answerPacket;
McuCommandPacket answerPacketInterrupt;

void SendCmdAnswer(McuCommand cmd, bool ok) {
	answerPacket.command = cmd;
	answerPacket.len = 1;
	answerPacket.data[0] = ok ? MCU_HELPER_ACK : MCU_HELPER_NACK;
	McuHelper_SendPacket(answerPacket);
}

void SendSpiAnswer(const uint8_t* rxData, size_t rxLen) {
	if(rxLen > MCU_COMMAND_MAX_DATA_LEN)
		return;
	answerPacket.command = SpiTransferAnswer;
	answerPacket.len = (uint16_t) rxLen;
	memcpy(answerPacket.data, rxData, rxLen);
	McuHelper_SendPacket(answerPacket);
}

void ReceiverThread() {

	McuCommandPacket* packet;
	while(1) {
		//Get one command from the queue
		osEvent evt = cmdQueue.get();

		if(evt.status != osEventMessage) {
			debugSerial.printf("QUEUE ERROR: Status %d\n", evt.status);
			continue;
		}

		packet = (McuCommandPacket*) evt.value.p;

		if(!packet) {
			debugSerial.printf("NULL pointer from queue\n");
			continue;
		}

		debugSerial.printf("Got packet Cmd %02x Len %d\n",
				(uint8_t)packet->command, (int) packet->len);

		//variables for SPI command
		SpiSetupDataHeader* spiHdr = NULL;
		uint8_t* txData = NULL;
		size_t txDataLen = 0;
		bool cmdOk = false;
		int16_t ss = 0;
		//Variables for Interrupt command
		int16_t intPin = 0;
		uint8_t modeByte;

		switch(packet->command) {
		case SpiSetup:

			//Packet long enough?
			if(packet->len < 7) {
				debugSerial.printf("SpiSetup packet too short: %d bytes\n", (int)packet->len);
				SendCmdAnswer(SpiSetupAnswer, false);
				break;
			}

			spiHdr = (SpiSetupDataHeader*) packet->data;
			cmdOk = SpiManager_Setup(spiHdr->freq, spiHdr->spiMode,
					(int16_t*) &packet->data[5],
					(packet->len - 5) / sizeof(int16_t)
					);
			SendCmdAnswer(SpiSetupAnswer, cmdOk);

			break;
		case SpiTransfer:

			if(packet->len < 2)  {
				debugSerial.printf("Too small for SpiTransfer: %d bytes\n", (int)packet->len);
				SendSpiAnswer(NULL, 0);
				break;
			}

			txData = (uint8_t*) (&packet->data[2]);
			txDataLen = packet->len - 2;
			memcpy(&ss, packet->data, sizeof(int16_t));
			SpiManager_Transfer(
					ss,
					txData,
					txDataLen,
					(uint8_t*)SpiRxBuffer
					);
			SendSpiAnswer((uint8_t*)SpiRxBuffer, txDataLen);

			break;
		case InterruptRegister:

			//Each interrupt register command can register one pin and mode.
			//Check packet length
			if(packet->len != 3) {
				debugSerial.printf("Invalid packet length for interrupt register: %d\n",
						(int)packet->len);
				SendCmdAnswer(InterruptRegisterAnswer, false);
				break;
			}

			//First two bytes are the pin
			memcpy(&intPin, packet->data, sizeof(int16_t));
			//Next is the mode byte
			modeByte = packet->data[2];
			if(modeByte < INTERRUPT_MODE_MIN_VALID || modeByte > INTERRUPT_MODE_MAX_VALID) {
				debugSerial.printf("Invalid mode byte for interrupt register: %02x\n",
						modeByte);
				SendCmdAnswer(InterruptRegisterAnswer, false);
				break;
			}

			cmdOk = InterruptManager_RegisterInterrupt(intPin, (InterruptMode) modeByte);
			SendCmdAnswer(InterruptRegisterAnswer, cmdOk);
			break;
		case Reset:
			SpiManager_Reset();
			InterruptManager_Reset();
			SendCmdAnswer(ResetAnswer, true);
			break;
		default:
			debugSerial.printf("Unknown command %02x\n", (uint8_t) packet->command);
			break;
		}

		McuHelper_ReleasePacket(packet);
	}
}

/* Thread which receives interrupt events and sends them over serial */
void InterruptThread() {
	int32_t interruptEvent = 0;
	while(1) {
		//Get one command from the queue
		osEvent evt = intQueue.get();

		if(evt.status != osEventMessage) {
			debugSerial.printf("QUEUE ERROR: Status %d\n", evt.status);
			continue;
		}

		interruptEvent = evt.value.v;

		//Decode pin and interrupt
		int16_t pin = (int16_t)(interruptEvent & 0xffff);
		bool rising = (interruptEvent >> 16) != 0;

		debugSerial.printf("Interrupt Pin %d event rising: %d\n", (int) pin, (int) rising);

		answerPacketInterrupt.command = InterruptEvent;
		answerPacketInterrupt.len = 3;
		memcpy(answerPacketInterrupt.data, &pin, sizeof(int16_t));
		answerPacketInterrupt.data[2] = rising ? INTERRUPT_EVENT_RISING : INTERRUPT_EVENT_FALLING;
		McuHelper_SendPacket(answerPacketInterrupt);
	}
}

int main() {

	debugSerial.printf("Firmware startup!\n");

	//Register queue for incoming commands
	McuHelper_RegisterQueue(&cmdQueue);

	//Register queue for interrupt events
	InterruptManager_RegisterQueue(&intQueue);

	//Start McuCommand receiver thread
	receiveThread.start(callback(ReceiverThread));

	//Start interrupt receiver thread
	interruptReceiveThread.start(callback(InterruptThread));

	//Hook RX interrupt of serial
	mcuSerial.attach(callback(&McuHelper_ParserFunc), Serial::RxIrq);

	while(1) {
		wait(10.0f);
	}

	return 0;
}
