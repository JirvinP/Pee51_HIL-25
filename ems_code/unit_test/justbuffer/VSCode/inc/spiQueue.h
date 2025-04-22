// -exec set output-radix 16
// https://www.ghsi.de/pages/subpages/Online%20CRC%20Calculation/indexDetails.php
// https://www.h-schmidt.net/FloatConverter/IEEE754.html
// https://www.rapidtables.com/convert/number/hex-to-binary.html
// https://www.simonv.fr/TypesConvert/?float
// https://www.simonv.fr/TypesConvert/?integers
// https://www.sunshine2k.de/coding/javascript/crc/crc_js.html

/**
 * @file spiQueue.h
 * @author Sefa Ozturk (S.H.Ozturk@outlook.com)
 * @brief queue for use with SPI
 * @version 0.6
 * @date 2025-04-22
 */

#ifndef SPIQUEUE_H
#define SPIQUEUE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// clang-format off
/**
 * \defgroup group_config project config
 * @brief precompiler settings for easy migration of code.
 * vscodeproject 0, noprint 1 when running on st.
 * vscodeproject 1 when running in vscode.
 * @{
 */
#define VSCODEPROJECT	1 /**< switch between vscode or st environment */
#define NOPRINT			0 /**< disables all print calls */
 /** @} */

/**
 * \defgroup group_packet packet layout
 * @brief packet layout using index and size
 * @{
 */
#define SQ_ID_INDEX			0  /**< byte index of identifier in packet */
#define SQ_ID_SIZE			1  /**< identifier size in bytes */
#define SQ_PAYLOAD_INDEX	1  /**< byte index of payload in packet */
#define SQ_PAYLOAD_SIZE		8  /**< payload size in bytes */
#define SQ_ACK_INDEX		9  /**< byte index of ack in packet */
#define SQ_ACK_SIZE			2  /**< ack size in bytes */
#define SQ_CRC_INDEX		11 /**< byte index of crc in packet */
#define SQ_CRC_SIZE			2  /**< crc size in bytes */
#define SQ_FRAME_SIZE		11 /**< packet size in bytes minus crc */
#define SQ_PACKET_SIZE		13 /**< overall packet size */
 /** @} */

/**
 * \defgroup group_ids packet ids
 * @brief ids
 * @note these must correspond with the lexicon in spiqueue.c
 * @{
 */
#define ID_TEST_UINT8	0xA0 /**< id of test variable for uint8_t types */
#define ID_TEST_UINT16	0xA1 /**< id of test variable for uint16_t types */
#define ID_TEST_UINT32	0xA2 /**< id of test variable for uint32_t types */
#define ID_TEST_SINT8	0xA3 /**< id of test variable for int8_t types */
#define ID_TEST_SINT16	0xA4 /**< id of test variable for int16_t types */
#define ID_TEST_SINT32	0xA5 /**< id of test variable for int32_t types */
#define ID_TEST_FRAC32	0xA6 /**< id of test variable for float types */
#define ID_TEST_FRAC64	0xA7 /**< id of test variable for double types */
#define ID_TEST_BINARY	0xA8 /**< id of test variable for bool types */
#define ID_FILLER		0x01 /**< id of tx filler package for transfer when no setpoint is available */
 /** @} */
// clang-format on

/** @brief pretty method to define polynomials */
#define X(pos) (1 << pos)

/** @brief easy method to get array size */
#define arraysize(arrayArg) (sizeof(arrayArg) / sizeof(arrayArg[0]))

#if VSCODEPROJECT
/** @brief using software lookup table to calculate crc */
#define GETCRC(ARRAY) crcCalcFast(&crcData, ARRAY, SQ_FRAME_SIZE)
#else
#include "crc.h"
#include "stm32h5xx_hal.h"
extern CRC_HandleTypeDef hcrc;
/** @brief using hardware peripheral to calculate crc */
#define GETCRC(ARRAY) HAL_CRC_Calculate(&hcrc, (uint32_t*)ARRAY, SQ_FRAME_SIZE)
/** @brief overload macro which will transform into spiqueuepostint or spiqueuepostfrac depending on payloadvaluearg */
#define spiQueuePost(structSpiQueuePtrArg, identifierArg, payloadValueArg) _Generic((payloadValueArg), \
	uint8_t: spiQueuePostInt,                                                                          \
	uint16_t: spiQueuePostInt,                                                                         \
	uint32_t: spiQueuePostInt,                                                                         \
	uint64_t: spiQueuePostInt,                                                                         \
	int8_t: spiQueuePostInt,                                                                           \
	int16_t: spiQueuePostInt,                                                                          \
	int32_t: spiQueuePostInt,                                                                          \
	int64_t: spiQueuePostInt,                                                                          \
	float: spiQueuePostFrac,                                                                           \
	double: spiQueuePostFrac)(structSpiQueuePtrArg, identifierArg, payloadValueArg)
#endif

#if NOPRINT
/** @brief zeros calls to print() macro */
#define PRINT(...)
#else
#include <stdio.h>
/** @brief passes print() macro calls to printf() */
#define PRINT(...) printf(__VA_ARGS__)
#endif

/** @brief global error code list, usefull for debugging in realtime */
enum errorCode {
	ec_no_error,
	ec_crc_finalxor_oversized,
	ec_crc_initvalue_oversized,
	ec_crc_length_bad,
	ec_crc_polynomial_oversized,
	ec_crc_polynomial_zero,
	ec_sq_already_exist,
	ec_sq_bad_id,
	ec_sq_doesnt_exist_post,
	ec_sq_doesnt_exist,
	ec_sq_packet_malloc_failed,
	ec_sq_full,
	ec_sq_incorrect_array_length,
	ec_sq_malloc_failed,
	// ec_sq_no_packet_exists_get,
	ec_sq_no_packet_exists,
	ec_sq_not_implemented,
	ec_sq_payload_no_datatype_2,
	ec_sq_payload_no_datatype,
	ec_sq_payload_out_of_range_binary,
	ec_sq_payload_out_of_range_sint16,
	ec_sq_payload_out_of_range_sint32,
	ec_sq_payload_out_of_range_sint8,
	ec_sq_payload_out_of_range_uint16,
	ec_sq_payload_out_of_range_uint32,
	ec_sq_payload_out_of_range_uint8,
	ec_sq_remove_failed
};

/** @brief crcdata sub struct containing crc data which to to be manually set crcinit() */
struct structCrcConfig {
	uint8_t bitLength;		/**< crc size, 8 or 16 or 32 bits */
	uint32_t polynomial;	/**< crc polynomial */
	uint32_t initialValue;	/**< initial checksum value */
	uint32_t finalXorValue; /**< xor final checksum with this value */
	bool inputReflected;	/**< reverse bit order of input */
	bool resultReflected;	/**< reverse bit order of output */
};

/** @brief crcdata sub struct containing crc data which will be automatically set by crcinit() */
struct structCrcDataAutomatic {
	uint32_t bitLengthMask;	   /**< mask calculated using bitlength */
	uint32_t lookUpTable[256]; /**< lookuptable for use with crccalcfast() */
};

/**
 * @brief crcdata top struct containing sub structs structcrcconfig and structcrcdataautomatic.
 * @note  crcdata element is declared spiqueue.h!
 */
struct structCrcData {
	struct structCrcConfig config;			 /**< holds crcdata config paramaters */
	struct structCrcDataAutomatic automatic; /**< holds crcdata data inititialized by crcinit() */
};

#if VSCODEPROJECT
	struct structCrcData crcData = {0};
#endif

/**
 * @brief crc value sub struct for converting crcvalue between two uint8[2]s and uint16
 * @note  this approach was taken to minimize endianness mistakes
 */
union unionCrc {
	uint8_t uint8[2]; /**< value split up in uint8 array */
	uint16_t uint16;  /**< value in uint16 */
};

/** @brief crc top struct containing value and flags */
struct structCrc {
	union unionCrc value; /**< value union */
	bool verified;		  /**< flag if value has been checked */
	bool good;			  /**< flag if value has been checked and is correct */
};

/**
 * @brief placeholder
 * @note  part of frame but not utilized, could potentialy send crc back for ack
 */
struct structAck {
	union unionCrc returnCrc; /**< crc -> ack_next */
	bool retrieved;			  /**< placeholder */
};

/**
 * @brief method for converting datatypes
 * @note - these correspond with the lexicondatatypes in spiqueue.c
 */
union unionPayload {
	bool binary;	  /**< boolean */
	uint8_t uint8[8]; /**< unsigned integer 8bit, use uint8[0] if you want the value*/
	uint16_t uint16;  /**< unsigned integer 16bit */
	uint32_t uint32;  /**< unsigned integer 32bit */
	int8_t sint8;	  /**< signed integer 8bit */
	int16_t sint16;	  /**< signed integer 16bit */
	int32_t sint32;	  /**< signed integer 32bit */
	float frac32;	  /**< float */
	double frac64;	  /**< double */
};

/** @brief packet structure */
struct structPacket {
	uint8_t identifier;					/**< a predefined id recorded by the codex used to distinguish variables as they turn abstracted while in spi transfer  */
	union unionPayload payload;			/**< union of all datatypes holding payload value */
	struct structCrc crc;				/**< crc value, check flag and good flag*/
	struct structAck ack;				/**< ack value, retrieved flag */
	struct structPacket* nextPacketPtr; /**< pointer to the following packet */
};

/** @brief structure for making a queue for use with spi. keeps track of last packet address */
struct structSpiQueue {
	uint8_t sizeCurrent;				/**< number of current packets inside the spiqueue */
	uint8_t sizeMax;					/**< number of maximum allowed packets inside the spiqueue */
	struct structPacket* tailPacketPtr; /**< pointer to the last appended packet */
	struct structPacket* headPacketPtr; /**< pointer to the first appended packet */
};

void errorCatcher(uint8_t errorCodeArg);
void errorReset(void);

int8_t crcInit(struct structCrcData* crcDataArg);
uint32_t crcCalcSlow(struct structCrcData* crcDataArg, uint8_t arrayArg[], uint8_t arraySizeArg);
uint32_t crcCalcFast(struct structCrcData* crcDataArg, uint8_t arrayArg[], uint8_t arraySizeArg);
void crcCalcTablePrint(struct structCrcData* crcDataArg, bool hexOutputArg, bool tableFormatArg);

int8_t spiQueueCreate(struct structSpiQueue** structSpiQueuePtrArg, uint8_t sizeMaxArg);
int8_t spiQueueRemove(struct structSpiQueue** structSpiQueuePtrArg);
int8_t spiQueuePacketRemove(struct structSpiQueue* structSpiQueuePtrArg);
int8_t spiQueuePostArray(struct structSpiQueue* structSpiQueuePtrArg, uint8_t arrayArg[], uint8_t arraySizeArg, bool crcCheckArg);
int8_t spiQueuePostInt(struct structSpiQueue* structSpiQueuePtrArg, uint8_t identifierArg, int64_t payloadValueArg);
int8_t spiQueuePostFrac(struct structSpiQueue* structSpiQueuePtrArg, uint8_t identifierArg, double payloadValueArg);
int8_t spiQueueGetArray(struct structSpiQueue* structSpiQueuePtrArg, uint8_t arrayArg[], uint8_t arraySizeArg);
int8_t spiQueueProcessAck(struct structSpiQueue* spiQueueTransmitPtrArg, struct structSpiQueue* spiQueueReceivePtrArg, bool ignoreAck);
int8_t spiQueueNoDuplicate(bool* duplicateArg, uint8_t arrayArg[], uint8_t arraySizeArg);

#endif
