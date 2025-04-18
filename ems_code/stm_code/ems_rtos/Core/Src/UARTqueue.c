/*
 * queue.c
 *
 *  Created on: Jan 5, 2025
 *      Author: ralfp
 */

#include <UARTqueue.h>

bool is_empty(struct queue* qu) {
	return (qu->front == -1);
}

bool is_full(struct queue* qu) {
	return ((qu->rear + 1) % MAX_SIZE == qu->front);
}

void enqueue(struct queue* qu, char* string) {
	if (!is_full(qu)) {

		if (qu->front == -1) {
			qu->front = 0;
		}

		int id = (qu->rear + 1) % MAX_SIZE;
		memset(qu->string[id], '\0', MAX_STRING_SIZE);
		strncpy((char*)qu->string[id], string, MAX_STRING_SIZE);
		qu->rear = id;
	}
}

uint8_t* delet(struct queue* qu) {
	if (!is_empty(qu)) {
		uint8_t* temp = (uint8_t*)malloc(MAX_STRING_SIZE * sizeof(uint8_t));
		if(temp == NULL){
			return NULL;
		}
		strncpy((char*)temp, (char*)qu->string[qu->front], MAX_STRING_SIZE);

		if (qu->front == qu->rear) {
			qu->front = qu->rear = -1;
		} else {
			qu->front = (qu->front + 1) % MAX_SIZE;
		}
		return temp;
	}

	return NULL;
}

void create_new(struct queue* qu) {
	qu->front = -1;
	qu->rear = -1;
}

void free_queue(struct queue* qu) {
	free(qu);
}
