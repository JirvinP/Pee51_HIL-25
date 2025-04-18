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
#include <stdbool.h>
// #include "stm32h5xx_hal.h"

struct queue{
	// ALIGN_32BYTES (uint8_t string[MAX_SIZE][MAX_STRING_SIZE]);
    uint8_t string[MAX_SIZE][MAX_STRING_SIZE];
	int front;
	int rear;
};

bool is_empty(struct queue* qu);
bool is_full(struct queue* qu);
void enqueue(struct queue* qu, char* string);
uint8_t* delet(struct queue* qu);
struct queue* construct_queue(void);
void destruct_queue(struct queue* qu);

#endif /* INC_UARTQUEUE_H_ */
