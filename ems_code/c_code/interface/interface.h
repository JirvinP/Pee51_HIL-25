// https://www.simonv.fr/TypesConvert/?integers
// https://www.simonv.fr/TypesConvert/?float
// -exec set output-radix 16
// https://www.sunshine2k.de/coding/javascript/crc/crc_js.html

/**
 * @file interface.h
 * @author Sefa Ozturk (S.H.Ozturk@outlook.com)
 * @brief FIFO buffer for use with SPI
 * @version 0.3
 * @date 2024-11-20F
 */

#ifndef INTERFACE_H
#define INTERFACE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define VSCODE 1
#define NOPRINT 1
#define RTTPRINT 0

#if VSCODE
#define GETCRC(ARRAY) crcCalcFast(&crcData, ARRAY, 9)
#else
#include "crc.h"
#include "stm32h5xx_hal.h"
#define GETCRC(ARRAY) HAL_CRC_Calculate(&hcrc, (uint32_t*)ARRAY, 9)
#define bufferPost(structBufferPtrArg, identifierArg, payloadValueArg) _Generic((payloadValueArg), \
	bool: bufferPostBool,                                                                          \
	int: bufferPostInt,                                                                            \
	uint8_t: bufferPostInt,                                                                        \
	uint16_t: bufferPostInt,                                                                       \
	uint32_t: bufferPostInt,                                                                       \
	uint64_t: bufferPostInt,                                                                       \
	int8_t: bufferPostInt,                                                                         \
	int16_t: bufferPostInt,                                                                        \
	int32_t: bufferPostInt,                                                                        \
	int64_t: bufferPostInt,                                                                        \
	float: bufferPostDec,                                                                          \
	double: bufferPostDec)(structBufferPtrArg, identifierArg, payloadValueArg)
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
	ec_buffer_malloc_failed = 0x01,
	ec_buffer_doesnt_exist = 0x02,
	ec_buffer_full = 0x03,
	ec_frame_malloc_failed = 0x04,
	ec_no_frame_exists = 0x05,
	ec_no_frame_exists_requested = 0x0B,
	ec_frame_full = 0x06,
	ec_payload_out_of_range = 0x09,
	ec_payload_no_datatype = 0x0A,
	ec_bad_crc = 0x0B,
	ec_bad_id = 0x0C,
	ec_buffer_remove_failed = 0x0D
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
 * @brief Method for converting datatypes
 */
union unionPayload {
	bool binary;
	uint8_t uint8[8];
	uint16_t uint16;
	uint32_t uint32;
	int8_t int8;
	int16_t int16;
	int32_t int32;
	float frac32;
	double frac64;
};

/**
 * @brief Linked list frame structure each representative of a single SPI transmission.
 */
struct structFrame {
	uint8_t identifier;				  /**< A predefined ID recorded by the codex used to distinguish variables as they turn abstracted while in SPI transfer  */
	union unionPayload unionPayload;  /**< Union of all datatypes holding Payload value */
	uint16_t crcValue;				  /**< Checksum on ID and PAYLOAD */
	bool verified;					  /**< if inbound: crc, if outbound: ack */
	struct structFrame* nextFramePtr; /**< Ptr to the following frame */
};

/**
 * @brief Structure for making a 2D linked list FIFO buffer for use with SPI.
 */
struct structBuffer {
	uint8_t bufferSizeCurrent;		  /**< Number of frames inside the buffer */
	uint8_t bufferSizeMax;			  /**< Number of maximum allowed frames inside the buffer */
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

struct structBuffer* bufferCreate(uint8_t bufferSizeMaxArg);
int8_t bufferRemove(struct structBuffer* structBufferPtrArg);
int8_t bufferRemoveFrame(struct structBuffer* structBufferPtrArg);
int8_t bufferPostArray(struct structBuffer* structBufferPtrArg, uint8_t receiveArrayArg[], bool crcCheckArg);
int8_t bufferPostInt(struct structBuffer* structBufferPtrArg, uint8_t identifierArg, int64_t payloadValueArg);
int8_t bufferPostDec(struct structBuffer* structBufferPtrArg, uint8_t identifierArg, double payloadValueArg);
int8_t bufferPostBool(struct structBuffer* structBufferPtrArg, uint8_t identifierArg, bool payloadValueArg);
int8_t bufferGetArray(struct structBuffer* structBufferPtrArg, uint8_t arrayArg[]);
int8_t bufferGetArraySeek(struct structBuffer* structBufferPtrArg, uint8_t identifierArg, uint8_t arrayArg[]);

#endif
