// https://www.simonv.fr/TypesConvert/?integers
// https://www.simonv.fr/TypesConvert/?float
// -exec set output-radix 16
// https://www.sunshine2k.de/coding/javascript/crc/crc_js.html

/**
 * @file spiQueue.h
 * @author Sefa Ozturk (S.H.Ozturk@outlook.com)
 * @brief FIFO spiQueue for use with SPI
 * @version 0.4
 * @date 2025-01-06
 */

#ifndef spiQueue_H
#define spiQueue_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define SPIQUEUE_SIZE 13

#define VSCODE 1
#define NOPRINT 1
#define RTTPRINT 0

#if VSCODE
#define GETCRC(ARRAY) crcCalcFast(&crcData, ARRAY, 11)
#else
#include "crc.h"
#include "stm32h5xx_hal.h"
#define GETCRC(ARRAY) HAL_CRC_Calculate(&hcrc, (uint32_t*)ARRAY, 11)
#define spiQueuePost(structSpiQueuePtrArg, identifierArg, payloadValueArg) _Generic((payloadValueArg), \
	int: spiQueuePostInt,                                                                              \
	uint8_t: spiQueuePostInt,                                                                          \
	uint16_t: spiQueuePostInt,                                                                         \
	uint32_t: spiQueuePostInt,                                                                         \
	uint64_t: spiQueuePostInt,                                                                         \
	int8_t: spiQueuePostInt,                                                                           \
	int16_t: spiQueuePostInt,                                                                          \
	int32_t: spiQueuePostInt,                                                                          \
	int64_t: spiQueuePostInt,                                                                          \
	float: spiQueuePostDec,                                                                            \
	double: spiQueuePostDec)(structSpiQueuePtrArg, identifierArg, payloadValueArg)
#endif

#if NOPRINT
#define PRINT(...)
#else
#if RTTPRINT
#include "SEGGER_RTT.h"
#define PRINT(...) SEGGER_RTT_printf(0, __VA_ARGS__)
#else
#include <stdio.h>
#define PRINT(...) printf(__VA_ARGS__)
#endif
#endif

/**
 * @brief Global error code list
 */
enum errorCode {
	ec_no_error = 0x00,
	ec_spiQueue_malloc_failed = 0x01,
	ec_spiQueue_doesnt_exist = 0x02,
	ec_spiQueue_full = 0x03,
	ec_frame_malloc_failed = 0x04,
	ec_no_frame_exists = 0x05,
	ec_no_frame_exists_get = 0x0F,
	ec_no_frame_exists_requested = 0x0B,
	ec_frame_full = 0x06,
	ec_payload_out_of_range = 0x10,
	ec_payload_out_of_range_1 = 0x11,
	ec_payload_out_of_range_2 = 0x12,
	ec_payload_out_of_range_3 = 0x13,
	ec_payload_no_datatype = 0x0A,
	ec_bad_crc = 0x0B,
	ec_bad_id = 0x0C,
	ec_spiQueue_remove_failed = 0x0D,
	ec_incorrect_length = 0x0E
};

/**
 * @brief XXX
 */
struct structCrcData {
	uint8_t bitLength;
	uint32_t polynomial;
	uint32_t initialValue;
	uint32_t finalXorValue;
	bool inputReflected;
	bool resultReflected;
	uint32_t bitLengthMask;
	uint32_t lookUpTable[256];
};

/**
 * @brief Method for converting crcvalue
 */
union unionCrc {
	uint8_t uint8[2];
	uint16_t uint16;
};

/**
 * @brief XXX
 */
struct structCrc {
	union unionCrc value; /**< XXX */
	bool verified;		  /**< XXX */
	bool good;			  /**< XXX */
};

/**
 * @brief XXX
 */
struct structAck {
	union unionCrc returnCrc; /**< XXX */
	bool retrieved;			  /**< XXX */
};

/**
 * @brief Method for converting datatypes
 */
union unionPayload {
	bool binary;
	uint8_t uint8[8];
	uint16_t uint16;
	uint32_t uint32;
	int8_t sint8;
	int16_t sint16;
	int32_t sint32;
	float frac32;
	double frac64;
};

/**
 * @brief Linked list frame structure each representative of a single SPI transmission.
 */
struct structFrame {
	uint8_t identifier;				  /**< A predefined ID recorded by the codex used to distinguish variables as they turn abstracted while in SPI transfer  */
	union unionPayload payload;		  /**< Union of all datatypes holding Payload value */
	struct structCrc crc;			  /**< CRC value, check flag and good flag*/
	struct structAck ack;			  /**< CRC value, check flag and good flag*/
	struct structFrame* nextFramePtr; /**< Ptr to the following frame */
};

/**
 * @brief Structure for making a 2D linked list FIFO spiQueue for use with SPI.
 */
struct structSpiQueue {
	uint8_t sizeCurrent;			  /**< Number of frames inside the spiQueue */
	uint8_t sizeMax;				  /**< Number of maximum allowed frames inside the spiQueue */
	struct structFrame* tailFramePtr; /**< Ptr to the last appended frame */
	struct structFrame* headFramePtr; /**< Ptr to the first appended frame */
};

void errorCatcher(uint8_t errorCodeArg);
void errorReset(void);

void crcInit(struct structCrcData* crcDataArg);
uint32_t crcCalcSlow(struct structCrcData* crcDataArg, uint8_t dataArrayArg[], uint8_t dataArrayLengthArg);
uint32_t crcCalcFast(struct structCrcData* crcDataArg, uint8_t dataArrayArg[], uint8_t dataArrayLengthArg);
void crcCalcTable(struct structCrcData* crcDataArg);
void crcCalcTablePrint(struct structCrcData* crcDataArg, bool hexOutputArg, bool tableFormatArg);

struct structSpiQueue* spiQueueCreate(uint8_t sizeMaxArg);
int8_t spiQueueRemove(struct structSpiQueue* structSpiQueuePtrArg);
int8_t spiQueueFrameRemove(struct structSpiQueue* structSpiQueuePtrArg);
int8_t spiQueuePostArray(struct structSpiQueue* structSpiQueuePtrArg, uint8_t arrayArg[], bool crcCheckArg);
int8_t spiQueuePostInt(struct structSpiQueue* structSpiQueuePtrArg, uint8_t identifierArg, int64_t payloadValueArg);
int8_t spiQueuePostDec(struct structSpiQueue* structSpiQueuePtrArg, uint8_t identifierArg, double payloadValueArg);
int8_t spiQueueGetArray(struct structSpiQueue* structSpiQueuePtrArg, uint8_t arrayArg[]);
void spiQueueProcessAck(struct structSpiQueue* spiQueueTransmitPtrArg, struct structSpiQueue* spiQueueReceivePtrArg, bool ignoreAck);
bool spiQueueNoDuplicate(uint8_t arrayArg[]);

#endif
