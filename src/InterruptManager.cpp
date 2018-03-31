/*
 * InterruptManager.cpp
 *
 *  Created on: 30.03.2018
 *      Author: Maxi
 */
#include "InterruptManager.h"
#include <map>

/* stores the assocation from pin name to interrupt object */
std::map<int16_t, InterruptIn*> interruptPinMap;

/* Pointer to queue */
Queue<int32_t, INTERRUPT_QUEUE_LEN>* interruptQueue;

MemoryPool<InterruptInfo, INTERRUPT_MAX_PINS> infoMemPool;

void InterruptFunctionRising(InterruptInfo* pInfo);
void InterruptFunctionFalling(InterruptInfo* pInfo);

/* register a queue in which the pin interrupts will be stored */
void InterruptManager_RegisterQueue(Queue<int32_t, INTERRUPT_QUEUE_LEN>* queue) {
	interruptQueue = queue;
}

/* Register an interrupt for that pin */
bool InterruptManager_RegisterInterrupt(int16_t pin, InterruptMode mode) {

	if(interruptPinMap.count(pin) != 0) {
		debugSerial.printf("Interrupt Pin %d already registered\n", (int) pin);
		return false;
	}

	//TODO: How to check that pin number is actually valid pin?
	//Create new InterruptIn pin
	InterruptIn* interruptPin = new InterruptIn((PinName) pin);
	interruptPinMap[pin] = interruptPin;

	InterruptInfo* intInfo;
	intInfo = infoMemPool.alloc();
	if(!intInfo) {
		debugSerial.printf("Failed to allocate interrupt info object!\n");
		interruptPinMap.erase(interruptPinMap.find(pin));
		delete interruptPin;
		return false;
	}
	intInfo->mode = mode;
	intInfo->pin = pin;

	//Hook wanted interrupt
	if(mode == Falling || mode == Change) {
		interruptPin->fall(callback(InterruptFunctionFalling, intInfo));
	}
	if(mode == Rising || mode == Change){
		interruptPin->rise(callback(InterruptFunctionRising, intInfo));
	}

	debugSerial.printf("Registered pin %d for mode %d\n", pin, (int) mode);

	return true;
}

inline void InterruptManager_EmitEvent(int16_t pin, bool rising) {
	if(interruptQueue) {
		//We will encode the event as follows: pin is the rightmost 16 bits,
		//rising/falling will be the leftmost bit (0x80)
		interruptQueue->put((int32_t*)(pin | ((int)rising) << 16));
	}
}

//An interrupt has occurred. We know through the pInfo object what pin and mode it was.
void InterruptFunctionRising(InterruptInfo* pInfo) {
	InterruptManager_EmitEvent(pInfo->pin, true);
}

void InterruptFunctionFalling(InterruptInfo* pInfo) {
	InterruptManager_EmitEvent(pInfo->pin, false);
}

void InterruptManager_Reset() {
	for(std::map<int16_t, InterruptIn*>::iterator it = interruptPinMap.begin(); it != interruptPinMap.end(); it++) {
		int16_t pin = it->first;
		InterruptIn* intPin = it->second;
		intPin->fall(NULL);
		intPin->rise(NULL);
		delete intPin;
		debugSerial.printf("Unregistering pin %d\n", (int)pin);
	}
	interruptPinMap.clear();
}
