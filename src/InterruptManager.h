/*
 * InterruptManager.h
 *
 *  Created on: 30.03.2018
 *      Author: Maxi
 */

#ifndef SRC_INTERRUPTMANAGER_H_
#define SRC_INTERRUPTMANAGER_H_

#include <rtos.h>
#include <stdint.h>
#include "project_settings.h"

enum InterruptMode {
	Falling = 0x01,
	Rising = 0x02,
	Change = 0x03
};

struct InterruptInfo {
	int16_t pin;
	InterruptMode mode;
};

#define INTERRUPT_EVENT_RISING		0x01
#define INTERRUPT_EVENT_FALLING		0x02

#define INTERRUPT_MODE_MIN_VALID	0x01
#define INTERRUPT_MODE_MAX_VALID	0x03

/* pin numbers are int16_t, but the queue supports int32_t natively. */

/* register a queue in which the pin interrupts will be stored */
void InterruptManager_RegisterQueue(Queue<int32_t, INTERRUPT_QUEUE_LEN>* queue);

/* Register an interrupt for that pin */
bool InterruptManager_RegisterInterrupt(int16_t pin, InterruptMode mode);

/* Reset all pin mappings */
void InterruptManager_Reset();

#endif /* SRC_INTERRUPTMANAGER_H_ */
