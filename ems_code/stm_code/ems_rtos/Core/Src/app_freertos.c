/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : app_freertos.c
 * Description        : FreeRTOS applicative file
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "app_freertos.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdbool.h>
#include <stdio.h>

#include "crc.h"
#include "gpdma.h"
#include "gpio.h"
#include "icache.h"
#include "memorymap.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"

#include "UARTqueue.h"
#include "ems.h"
#include "linked_list.h"
#include "spiQueue.h"
#include "ui.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
enum {
	SPI_TRANSFER_IDLE,
	SPI_TRANSFER_BUSY,
	SPI_TRANSFER_DONE,
	SPI_TRANSFER_ERROR
};

enum {
	UART_TRANSMIT_IDLE,
	UART_TRANSMIT_BUSY,
	UART_TRANSMIT_DONE,
	UART_TRANSMIT_ERROR
};

enum {
	UART_RECEIVE_IDLE,
	UART_RECEIVE_BUSY,
	UART_RECEIVE_DONE,
	UART_RECEIVE_ERROR
};
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define PRNT_UART(a) HAL_UART_Transmit_DMA(&huart3, (uint8_t*)&a, strlen((char*)a));
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
extern DMA_QListTypeDef UART_Tx_Queue;
extern DMA_QListTypeDef UART_Rx_Queue;
extern DMA_QListTypeDef SPI_queue_tx;
extern DMA_QListTypeDef SPI_queue_rx;

struct system* sys = NULL;

struct queue uart_queue;
ALIGN_32BYTES(uint8_t aTxBuffer0[MAX_STRING_SIZE]);
ALIGN_32BYTES(uint8_t aRxBuffer0[MAX_STRING_SIZE]);
volatile uint8_t uartTransferStatus = UART_TRANSMIT_IDLE;
volatile uint8_t uartReceiveStatus = UART_RECEIVE_IDLE;

extern uint8_t errorVal;
struct structSpiQueue* spiQueueTransmit = NULL;
struct structSpiQueue* spiQueueReceive = NULL;
ALIGN_32BYTES(uint8_t spiQueueTransmitArray[13]) = {0};
ALIGN_32BYTES(uint8_t spiQueueReceiveArray[13]) = {0};
volatile uint8_t spiTransferState = SPI_TRANSFER_IDLE;

volatile bool speedGoatReady = false;

extern volatile uint32_t latency;
uint32_t latencyStored = 0;
uint32_t counterid = 0;
uint8_t latencyAnimator = 0;

double test = 0.0;

/* USER CODE END Variables */
/* Definitions for SPItask */
osThreadId_t SPItaskHandle;
const osThreadAttr_t SPItask_attributes = {
  .name = "SPItask",
  .priority = (osPriority_t) osPriorityHigh,
  .stack_size = 1024 * 4
};
/* Definitions for UItask */
osThreadId_t UItaskHandle;
const osThreadAttr_t UItask_attributes = {
  .name = "UItask",
  .priority = (osPriority_t) osPriorityLow,
  .stack_size = 1024 * 4
};
/* Definitions for EMStask */
osThreadId_t EMStaskHandle;
const osThreadAttr_t EMStask_attributes = {
  .name = "EMStask",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 1024 * 4
};
/* Definitions for spi_mutex */
osMutexId_t spi_mutexHandle;
const osMutexAttr_t spi_mutex_attributes = {
  .name = "spi_mutex"
};
/* Definitions for ems_mutex */
osMutexId_t ems_mutexHandle;
const osMutexAttr_t ems_mutex_attributes = {
  .name = "ems_mutex"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void startup_dial();
void add_to_queue(char* str);
void prnt_queue();
void print_full_queue();
/* USER CODE END FunctionPrototypes */

/* USER CODE BEGIN 5 */
void vApplicationMallocFailedHook(void) {
	while (1)
		;
	/* vApplicationMallocFailedHook() will only be called if
	configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h. It is a hook
	function that will get called if a call to pvPortMalloc() fails.
	pvPortMalloc() is called internally by the kernel whenever a task, queue,
	timer or semaphore is created. It is also called by various parts of the
	demo application. If heap_1.c or heap_2.c are used, then the size of the
	heap available to pvPortMalloc() is defined by configTOTAL_HEAP_SIZE in
	FreeRTOSConfig.h, and the xPortGetFreeHeapSize() API function can be used
	to query the size of free heap space that remains (although it does not
	provide information on how the remaining heap might be fragmented). */
}
/* USER CODE END 5 */

/* USER CODE BEGIN 4 */
void vApplicationStackOverflowHook(xTaskHandle xTask, char* pcTaskName) {
	while (1)
		;
	/* Run time stack overflow checking is performed if
	configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2. This hook function is
	called if a stack overflow is detected. */
}
/* USER CODE END 4 */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
	create_new(&uart_queue);
	clear_screen(&uart_queue);
	logprint(LOG_OK, "System peripherals initialized\r\n", &uart_queue);
	logprint(LOG_NOTE, "Initializing FreeRTOS\r\n", &uart_queue);

	/*uart dma init*/
	MX_UART_Tx_Queue_Config();
	MX_UART_Rx_Queue_Config();
	HAL_DMAEx_List_LinkQ(&handle_GPDMA2_Channel0, &UART_Tx_Queue);
	__HAL_LINKDMA(&huart3, hdmatx, handle_GPDMA2_Channel0);
	HAL_DMAEx_List_LinkQ(&handle_GPDMA2_Channel1, &UART_Rx_Queue);
	__HAL_LINKDMA(&huart3, hdmarx, handle_GPDMA2_Channel1);

	/*spi dma init*/
	MX_SPI_queue_tx_Config();
	HAL_DMAEx_List_LinkQ(&handle_GPDMA1_Channel7, &SPI_queue_tx);
	__HAL_LINKDMA(&hspi1, hdmatx, handle_GPDMA1_Channel7);
	MX_SPI_queue_rx_Config();
	HAL_DMAEx_List_LinkQ(&handle_GPDMA1_Channel6, &SPI_queue_rx);
	__HAL_LINKDMA(&hspi1, hdmarx, handle_GPDMA1_Channel6);
	logprint(LOG_OK, "DMA initialized\r\n", &uart_queue);

	/*spi queue init*/
	struct structSpiQueue* spiQueueTransmit = NULL;
	spiQueueCreate(&spiQueueTransmit, 100);

	struct structSpiQueue* spiQueueReceive = NULL;
	spiQueueCreate(&spiQueueReceive, 100);

	if (spiQueueTransmit == NULL || spiQueueReceive == NULL) {
		logprint(LOG_FAIL, "SPI buffers could not be initialized\r\n", &uart_queue);
		prnt_queue();
		while (1)
			;
	} else {
		logprint(LOG_OK, "SPI buffers initialized\r\n", &uart_queue);
		prnt_queue();
	}

	/*init sys struct*/
	logprint(LOG_OK, "DMA initialized\r\n", &uart_queue);
	sys = initialize_sys(&uart_queue);
	startup_dial();
	print_choice_menu(&uart_queue);
	print_full_queue();

	while (1) {
		if (uartReceiveStatus == UART_RECEIVE_IDLE) {
			uartReceiveStatus = UART_RECEIVE_BUSY;
			HAL_UART_Receive_DMA(&huart3, aRxBuffer0, 1);
		} else if (uartReceiveStatus == UART_RECEIVE_DONE) {
			if (parse_user_optimization_strategy(sys, (char*)aRxBuffer0) < 0) {
				enqueue(&uart_queue, "Invalid option try again\r\n");
				prnt_queue();
				enqueue(&uart_queue, "> ");
				prnt_queue();
				uartReceiveStatus = UART_RECEIVE_IDLE;
			} else {
				char temp[150] = {'\0'};
				snprintf(temp, 150, "Selected option %c\r\n", aRxBuffer0[0]);
				enqueue(&uart_queue, temp);
				prnt_queue();
				break;
			}
		}
	}

	logprint(LOG_NOTE, "Waiting on speedgoat...\r\n", &uart_queue);
	print_full_queue();
	while (!speedGoatReady)
		;
	logprint(LOG_OK, "Speedgoat running\r\n", &uart_queue);
	print_full_queue();

  /* USER CODE END Init */
  /* creation of spi_mutex */
  spi_mutexHandle = osMutexNew(&spi_mutex_attributes);

  /* creation of ems_mutex */
  ems_mutexHandle = osMutexNew(&ems_mutex_attributes);

  /* USER CODE BEGIN RTOS_MUTEX */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* USER CODE END RTOS_QUEUES */
  /* creation of SPItask */
  SPItaskHandle = osThreadNew(SPItxrxtask, NULL, &SPItask_attributes);

  /* creation of UItask */
  UItaskHandle = osThreadNew(UIprinttask, NULL, &UItask_attributes);

  /* creation of EMStask */
  EMStaskHandle = osThreadNew(EMScalctask, NULL, &EMStask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
	/* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
	/* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}
/* USER CODE BEGIN Header_SPItxrxtask */
/**
 * @brief Function implementing the SPItask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_SPItxrxtask */
void SPItxrxtask(void *argument)
{
  /* USER CODE BEGIN SPItask */
	for (;;) {
		HAL_GPIO_WritePin(THREAD_2_GPIO_Port, THREAD_2_Pin, GPIO_PIN_SET);
		spiTransferState = SPI_TRANSFER_BUSY;

		spiQueueGetArray(spiQueueTransmit, spiQueueTransmitArray, SQ_PACKET_SIZE);
		if (HAL_SPI_TransmitReceive_DMA(&hspi1, (uint8_t*)spiQueueTransmitArray, (uint8_t*)spiQueueReceiveArray, SQ_PACKET_SIZE) != HAL_OK) {
			Error_Handler();
		}
		while (spiTransferState != SPI_TRANSFER_DONE)
			;

		if (spiQueueReceiveArray[0] != 0x00 && spiQueueReceiveArray[0] != 0xFF) {
			// check if the rx array is the same as last time
			// if changed put rx array into queue
			bool noDuplicate = false;
			spiQueueNoDuplicate(&noDuplicate, spiQueueReceiveArray, SQ_PACKET_SIZE);
			if (noDuplicate) {
				spiQueuePostArray(spiQueueReceive, spiQueueReceiveArray, SQ_PACKET_SIZE, true);
			}
		}

		// perform ack, but gutted :(
		spiQueueProcessAck(spiQueueTransmit, spiQueueReceive, true);

		if (spiQueueReceive->sizeCurrent > 0) {
			if (spiQueueReceive->headPacketPtr->crc.good == false) {
				spiQueuePacketRemove(spiQueueReceive);
			} else {
				if (spiQueueReceive->headPacketPtr->identifier == 0xA9) {
					latencyStored = latency;
					latencyAnimator = latencyAnimator < 3 ? latencyAnimator + 1 : 0;
				} else {
					parse_simulation_data(sys, spiQueueReceive->headPacketPtr);
				}
				spiQueuePacketRemove(spiQueueReceive);
			}
		}
		spiTransferState = SPI_TRANSFER_IDLE;
		HAL_GPIO_WritePin(THREAD_2_GPIO_Port, THREAD_2_Pin, GPIO_PIN_RESET);
		osDelay(1);
	}
  /* USER CODE END SPItask */
}

/* USER CODE BEGIN Header_UIprinttask */
/**
 * @brief Function implementing the UItask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_UIprinttask */
void UIprinttask(void *argument)
{
  /* USER CODE BEGIN UItask */
	/* Infinite loop */
	for (;;) {
		HAL_GPIO_WritePin(THREAD_3_GPIO_Port, THREAD_3_Pin, GPIO_PIN_SET);
		print_stats(sys, &uart_queue);
		print_full_queue();
		HAL_GPIO_WritePin(THREAD_3_GPIO_Port, THREAD_3_Pin, GPIO_PIN_RESET);
		osDelay(pdMS_TO_TICKS(1000));
	}
  /* USER CODE END UItask */
}

/* USER CODE BEGIN Header_EMScalctask */
/**
 * @brief Function implementing the EMStask thread.
 * @param argument: Not used			send_setpoints(sys, spiQueueTransmit);
 *
 * @retval None
 */
/* USER CODE END Header_EMScalctask */
void EMScalctask(void *argument)
{
  /* USER CODE BEGIN EMStask */
	/* Infinite loop */
	for (;;) {
		HAL_GPIO_WritePin(THREAD_1_GPIO_Port, THREAD_1_Pin, GPIO_PIN_SET);
		counterid = counterid < UINT32_MAX ? counterid + 1 : 0;
		latency = 0;
		spiQueuePost(spiQueueTransmit, TEST_LATENCY_ID, counterid);
		if (sys->goat_preference->mode != INIT || sys->goat_preference->mode == 0) {
			execute_subroutine(sys);
			send_setpoints(sys, spiQueueTransmit);
			// CHECK IF BAD :(
		}
		HAL_GPIO_WritePin(THREAD_1_GPIO_Port, THREAD_1_Pin, GPIO_PIN_RESET);
		osDelay(10);
	}

  /* USER CODE END EMStask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

void add_to_queue(char* str) {
	enqueue(&uart_queue, str);
}

void prnt_queue() {
	if (uartTransferStatus == UART_TRANSMIT_IDLE) {
		uint8_t* temp = delet(&uart_queue);
		if (temp == NULL) {
			return;
		}
		memset(aTxBuffer0, '\0', MAX_STRING_SIZE);
		if (strlen((char*)temp) == 0) {
			free(temp);
			return;
		}
		strncpy((char*)aTxBuffer0, (char*)temp, MAX_STRING_SIZE);
		uartTransferStatus = UART_TRANSMIT_BUSY;
		PRNT_UART(aTxBuffer0);
		free(temp);
	}
}

void print_full_queue() {
	while (!is_empty(&uart_queue)) {
		prnt_queue();
	}
}

void startup_dial() {
	add_to_queue("\r\n");
	add_to_queue("=======================================\r\n");
	add_to_queue("\033[32m");
	add_to_queue("  ______ __  __  _____    ____   _____  \r\n");
	add_to_queue(" |  ____|  \\/  |/ ____|  / __ \\ / ____|\r\n");
	add_to_queue(" | |__  | \\  / | (___   | |  | | (___  \r\n");
	add_to_queue(" |  __| | |\\/| |\\___ \\  | |  | |\\___ \\\r\n");
	add_to_queue(" | |____| |  | |____) | | |__| |____) |\r\n");
	add_to_queue(" |______|_|  |_|_____/   \\____/|_____/ \r\n");
	add_to_queue("\033[0m");
	add_to_queue("\r\n");
	add_to_queue("=======================================\r\n");
	add_to_queue("     (on an actual OS this time)\r\n");
	add_to_queue("\r\n");
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart) {
	uartTransferStatus = UART_TRANSMIT_IDLE;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart) {
	uartReceiveStatus = UART_RECEIVE_DONE;
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart) {
	uartTransferStatus = UART_TRANSMIT_ERROR;
	uartReceiveStatus = UART_RECEIVE_ERROR;
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef* hspi) {
	spiTransferState = SPI_TRANSFER_DONE;
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef* hspi) {
	spiTransferState = SPI_TRANSFER_ERROR;
}

void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin) {
	switch (GPIO_Pin) {
	case SG_RDY_Pin:
		speedGoatReady = true;
		break;
	case USER_BUTTON_Pin:
		speedGoatReady = true;
		break;
	default:
		__NOP();
		break;
	}
}

/* USER CODE END Application */

