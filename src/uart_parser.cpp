/**
 * @author Maximilian Gerhardt
 * */
#include <mbed.h>
#include <rtos.h>
#include "uart_parser.h"
#include "project_settings.h"

enum McuHelper_ParserState {
	ReadCmd,
	ReadLen1,
	ReadLen2,
	ReadData
};
McuCommandPacket* currentPacket = NULL;
McuHelper_ParserState currentParserState = ReadCmd;

MemoryPool<McuCommandPacket, MCUHELPER_QUEUE_LEN> mcuPacketPool;
Queue<McuCommandPacket, MCUHELPER_QUEUE_LEN>* packetQueue;
uint16_t readPacketBytes = 0;

/* Mutex for UART interface */
Mutex uartMutex;

/* Sends packet into queue */
void McuHelper_EmitPacket() {
	if(packetQueue != NULL) {
		/*debugSerial.printf("PACKET EMIT CMD %02x len %d\n",
				(uint8_t)currentPacket->command,
				(int)currentPacket->len);*/

		packetQueue->put(currentPacket);
		currentPacket = NULL;
	}
}

void McuHelper_ReleasePacket(McuCommandPacket* pkt) {
	mcuPacketPool.free(pkt);
}

void McuHelper_Reset() {
	currentParserState = ReadCmd;
	readPacketBytes = 0;
}

void McuHelper_ParserCore(uint8_t data) {
	//We need to parse data. do we have a current packet?
	if(!currentPacket) {
		currentPacket = mcuPacketPool.alloc();
		//Still no data?
		if(!currentPacket) {
			debugSerial.printf("FAILED TO ALLOC MCU PACKET\n");
			return;
		}
	}

	//which state are we in?
	switch(currentParserState) {
	case ReadCmd:

		//Check plausibility of command
		if(data < MCUCOMMAND_MIN_VALID || data > MCUCOMMAND_MAX_VALID) {
			debugSerial.printf("IGNORING INVALID COMMAND BYTE %02x\n", data);
			McuHelper_Reset();
			return;
		}
		//We made sure it's valid. store it.
		currentPacket->command = (McuCommand) data;
		currentParserState = ReadLen1;
		break;

	case ReadLen1:
		currentPacket->len = data;
		currentParserState = ReadLen2;
		break;
	case ReadLen2:
		currentPacket->len |= (data << 8);

		//Check plausibility of command
		if(currentPacket->len > MCU_COMMAND_MAX_DATA_LEN) {
			debugSerial.printf("OVERLONG PACKET LENGTH %d\n", (int) currentPacket->len);
			McuHelper_Reset();
		}

		//No additional data? Emit one packet.
		if(currentPacket->len == 0) {
			McuHelper_EmitPacket();
			McuHelper_Reset();
		} else {
			//Else, read all data bytes, then emit.
			readPacketBytes = 0;
			currentParserState = ReadData;
		}

		break;
	case ReadData:

		//Read data into buffer, increase read counter
		currentPacket->data[readPacketBytes++] = data;

		//All packet bytes read?
		if(currentPacket->len == readPacketBytes) {
			McuHelper_EmitPacket();
			McuHelper_Reset();
		}

		break;
	}
}

void McuHelper_ParserFunc() {
	//Read all available bytes
	while(mcuSerial.readable()) {
		uint8_t data = (uint8_t) mcuSerial.getc();
		//Feed into parser core byte-wise
		McuHelper_ParserCore(data);
	}
}

/* Queue where parsed packets will be put into */
void McuHelper_RegisterQueue(Queue<McuCommandPacket, MCUHELPER_QUEUE_LEN>* queue) {
	packetQueue = queue;
}

/* Sends the packet over serial */
void McuHelper_SendPacket(const McuCommandPacket& packet) {
	uartMutex.lock();

	debugSerial.printf("Sending packet cmd %02x len %d\n",
			(uint8_t)packet.command, packet.len);

	mcuSerial.putc((uint8_t) packet.command);
	mcuSerial.putc((uint8_t) (packet.len & 0xff));
	mcuSerial.putc((uint8_t) (packet.len >> 8));
	for(int i=0; i < packet.len; i++)
		mcuSerial.putc((uint8_t) packet.data[i]);

	uartMutex.unlock();
}
