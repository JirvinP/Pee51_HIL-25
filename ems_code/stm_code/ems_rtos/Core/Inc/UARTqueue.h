/*
 * queue.h
 *
 *  Created on: Jan 5, 2025
 *      Author: ralfp
 *
 *      sauce: https://www.geeksforgeeks.org/queue-in-c/
 */

#ifndef INC_UARTQUEUE_H_
#define INC_UARTQUEUE_H_

#define MAX_SIZE	(100)
#define MAX_STRING_SIZE (150)

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "stm32h5xx_hal.h"

struct queue{
	ALIGN_32BYTES (uint8_t string[MAX_SIZE][MAX_STRING_SIZE]);
	int front;
	int rear;
};

bool is_empty(struct queue* qu);
bool is_full(struct queue* qu);
void enqueue(struct queue* qu, char* string);
uint8_t* delet(struct queue* qu);
void create_new(struct queue* qu);
void free_queue(struct queue* qu);

#endif /* INC_UARTQUEUE_H_ */
