/**
 * @file spiQueue.c
 * @author Sefa Ozturk (S.H.Ozturk@outlook.com)
 * @brief queue for use with SPI
 * @version 0.6
 * @date 2025-04-22
 */

#include "spiQueue.h"

// ERROR --------------------------------------------------------------------------------------------------------------------

/**
 * @brief global error code variable
 * @note - include this variable using external in main.c
 */
uint8_t errorVal = ec_no_error;

/**
 * @brief method to debug
 * @param[in] errorCodeArg enums from the global error code list in spiqueue.h
 */
void errorCatcher(uint8_t errorCodeArg) {
	errorVal = errorCodeArg;
#if !NOERRORPRINT
	PRINT("\x1B[31merrorVal: 0x%02X\n\x1B[0m", errorCodeArg);
#endif
}

/**
 * @brief resets errorval to ec_no_error
 */
void errorReset(void) {
	errorVal = ec_no_error;
}

// CRC ----------------------------------------------------------------------------------------------------------------------

static void crcCalcTable(struct structCrcData* crcDataArg);
static uint32_t crcReflect(uint32_t bitSequenceArg, uint8_t bitSequenceWidthArg);

/**
 * @brief initialize crcdata using config fields
 * @param[in] crcDataArg struct pointer containing crcdata config and data
 * @retval 0 on success, -1 on failure
 * @note - Equipped with errorCatcher()
 */
int8_t crcInit(struct structCrcData* crcDataArg) {
	if (!((crcDataArg->config.bitLength == 8) || (crcDataArg->config.bitLength == 16) || (crcDataArg->config.bitLength == 32))) {
		errorCatcher(ec_crc_length_bad);
		return -1;
	}
	uint32_t bitLengthMaskTemp = 0xFFFFFFFF >> (32 - crcDataArg->config.bitLength);
	if (crcDataArg->config.polynomial == 0) {
		errorCatcher(ec_crc_polynomial_zero);
		return -1;
	}
	if (crcDataArg->config.polynomial > bitLengthMaskTemp) {
		errorCatcher(ec_crc_polynomial_oversized);
		return -1;
	}
	if (crcDataArg->config.initialValue > bitLengthMaskTemp) {
		errorCatcher(ec_crc_initvalue_oversized);
		return -1;
	}
	if (crcDataArg->config.finalXorValue > bitLengthMaskTemp) {
		errorCatcher(ec_crc_finalxor_oversized);
		return -1;
	}
	crcDataArg->automatic.bitLengthMask = bitLengthMaskTemp;
	crcCalcTable(crcDataArg);
	return 0;
}

/**
 * @brief calculate crc using the slow method
 * @param[in] crcDataArg struct pointer containing crcdata config and data
 * @param[in] arrayArg array pointer to frame
 * @param[in] arraySizeArg size of arrayarg
 * @retval checksum masked depending on crc bitlength
 */
uint32_t crcCalcSlow(struct structCrcData* crcDataArg, uint8_t arrayArg[], uint8_t arraySizeArg) {
	uint32_t checksum = crcDataArg->config.initialValue;
	uint32_t highestBitPos = 1 << (crcDataArg->config.bitLength - 1);
	for (uint8_t byte = 0; byte < arraySizeArg; byte++) {
		if (crcDataArg->config.inputReflected) {
			checksum ^= crcReflect(arrayArg[byte], 8) << (crcDataArg->config.bitLength - 8);
		} else {
			checksum ^= arrayArg[byte] << (crcDataArg->config.bitLength - 8);
		}
		for (uint8_t bit = 0; bit < 8; bit++) {
			if ((checksum & highestBitPos) != 0) {
				checksum = (checksum << 1) ^ crcDataArg->config.polynomial;
			} else {
				checksum <<= 1;
			}
		}
	}
	if (crcDataArg->config.resultReflected) {
		return (crcReflect(checksum, crcDataArg->config.bitLength) ^ crcDataArg->config.finalXorValue) & crcDataArg->automatic.bitLengthMask;
	} else {
		return (checksum ^ crcDataArg->config.finalXorValue) & crcDataArg->automatic.bitLengthMask;
	}
}

/**
 * @brief calculate crc using the fast method aka lookup method
 * @param[in] crcDataArg struct pointer containing crcdata config and data
 * @param[in] arrayArg array pointer to frame
 * @param[in] arraySizeArg size of arrayarg
 * @retval checksum masked depending on crc bitlength
 */
uint32_t crcCalcFast(struct structCrcData* crcDataArg, uint8_t arrayArg[], uint8_t arraySizeArg) {
	uint8_t index;
	uint32_t checksum = crcDataArg->config.initialValue;
	for (uint8_t byte = 0; byte < arraySizeArg; byte++) {
		if (crcDataArg->config.inputReflected) {
			index = crcReflect(arrayArg[byte], 8) ^ (checksum >> (crcDataArg->config.bitLength - 8));
		} else {
			index = arrayArg[byte] ^ (checksum >> (crcDataArg->config.bitLength - 8));
		}
		checksum = crcDataArg->automatic.lookUpTable[index] ^ (checksum << 8);
	}
	if (crcDataArg->config.resultReflected) {
		return (crcReflect(checksum, crcDataArg->config.bitLength) ^ crcDataArg->config.finalXorValue) & crcDataArg->automatic.bitLengthMask;
	} else {
		return (checksum ^ crcDataArg->config.finalXorValue) & crcDataArg->automatic.bitLengthMask;
	}
}

/**
 * @brief internal function to reflect input or output
 * @param[in] bitSequenceArg either a frame byte input or the output checksum
 * @param[in] bitSequenceWidthArg bit length of bitsequencearg
 * @retval reflected bitsequencearg
 */
static uint32_t crcReflect(uint32_t bitSequenceArg, uint8_t bitSequenceWidthArg) {
	uint32_t reflection = 0;
	uint8_t i;
	for (i = 0; i < bitSequenceWidthArg; ++i) {
		if (bitSequenceArg & 1) {
			reflection |= 1 << (bitSequenceWidthArg - 1 - i);
		}
		bitSequenceArg >>= 1;
	}
	return reflection;
}

/**
 * @brief populate the lookup table
 * @param[in] crcDataArg struct pointer containing crcData config and data
 */
static void crcCalcTable(struct structCrcData* crcDataArg) {
	uint32_t highestBitPos = 1 << (crcDataArg->config.bitLength - 1);
	for (uint16_t byte = 0; byte < 256; byte++) {
		uint32_t checksum = byte;
		for (uint8_t bit = 0; bit < crcDataArg->config.bitLength; bit++) {
			if ((checksum & highestBitPos) != 0) {
				checksum = (checksum << 1) ^ crcDataArg->config.polynomial;
			} else {
				checksum <<= 1;
			}
		}
		crcDataArg->automatic.lookUpTable[byte] = checksum & crcDataArg->automatic.bitLengthMask;
	}
}

/**
 * @brief pretty print the lookup table
 * @param[in] crcDataArg struct pointer containing crcdata config and data
 * @param[in] hexOutputArg print as hex if true, else as decimal
 * @param[in] tableFormatArg print into a block if true, else long string
 */
void crcCalcTablePrint(struct structCrcData* crcDataArg, bool hexOutputArg, bool tableFormatArg) {
	uint16_t index = 0;
	uint8_t columns = 0;
	PRINT("\x1B[30;47m\n// clang-format off\n");
	PRINT("uint%d_t lookUpTable[256] = { \\\n", crcDataArg->config.bitLength);
	columns = (crcDataArg->config.bitLength == 32 ? 8 : 16);
	while (index < 256) {
		if (hexOutputArg) {
			PRINT("0x%0*luX", crcDataArg->config.bitLength / 4, crcDataArg->automatic.lookUpTable[index]);
		} else {
			PRINT("%lu", crcDataArg->automatic.lookUpTable[index]);
		}
		index++;
		if (tableFormatArg) {
			if (index % columns != 0) {
				PRINT(", ");
			} else {
				if (index != 256) {
					PRINT(", \\\n");
				} else {
					PRINT("  \\");
				}
			}
		} else {
			if (index != 256) {
				PRINT(", ");
			}
		}
	}
	PRINT("\n};\n// clang-format on\x1B[0m\n\n");
}

// SPIQUEUE -----------------------------------------------------------------------------------------------------------------

/** @brief structure definition for the lexicon */
struct structLexicon {
	uint8_t identifier;		  /**< ID code */
	uint8_t dataType;		  /**< C datatype */
	uint8_t varString[32];	  /**< Printable variable name */
	uint8_t varUnitString[8]; /**< Printable variable unit specifier */
};

/** @brief supported datatypes */
enum lexiconDataTypes {
	X,		/**< BAD */
	BINARY, /**< boolean */
	UINT8,	/**< unsigned integer 8bit*/
	UINT16, /**< unsigned integer 16bit */
	UINT32, /**< unsigned integer 32bit */
	SINT8,	/**< signed integer 8bit */
	SINT16, /**< signed integer 16bit */
	SINT32, /**< signed integer 32bit */
	FRAC32, /**< float */
	FRAC64, /**< double */
};

// clang-format off
/** @brief lexicon with easily recognizable structure columns */
const struct structLexicon lexicon[] = {
// BAD IDs
	{0x00, X,		"X",					"X"			},
	{0xFF, X,		"X",					"X"			},

// FILLER	
	{0x01, UINT8,	"Filler",				"F"			},

// TEST	
	{0xA0, UINT8,	"Test UINT8",			"T"			},
	{0xA1, UINT16,	"Test UINT16",			"T"			},
	{0xA2, UINT32,	"Test UINT32",			"T"			},
	{0xA3, SINT8,	"Test SINT8",			"T"			},
	{0xA4, SINT16,	"Test SINT16",			"T"			},
	{0xA5, SINT32,	"Test SINT32",			"T"			},
	{0xA6, FRAC32,	"Test FRAC32",			"T"			},
	{0xA7, FRAC64,	"Test FRAC64",			"T"			},
	{0xA8, BINARY,	"Test BINARY",			"T"			},
	{0xA9, UINT32,	"Test latency",			"100us"		},

// OUTBOUND	
	{0xB1, FRAC64,	"Setpoint battery 1",	"kW"		},
	{0xB2, FRAC64,	"Setpoint battery 2",	"kW"		},
	{0xB3, FRAC64,	"Setpoint DG 1",		"kW"		},
	{0xB4, FRAC64,	"Setpoint DG 2",		"kW"		},

// INBOUND	
	{0xC1, FRAC64,	"Power battery 1",		"kW"		},
	{0xC2, FRAC64,	"Power battery 2",		"kW"		},
	{0xC3, FRAC32,	"SOC battery 1",		"%%"		},
	{0xC4, FRAC32,	"SOC battery 2",		"%%"		},
	{0xC5, UINT32,	"Power DG 1",			"kW"		},
	{0xC6, UINT32,	"Power DG 2",			"kW"		},
	{0xC7, FRAC32,	"SFOC 1",				"gr/kWh"	},
	{0xC8, FRAC32,	"SFOC 2",				"gr/kWh"	},
	{0xC9, UINT8,	"OPstate",				"enum"		}
};
// clang-format on

/**
 * @brief allocates memory and initialises a spiqueue according to the structspiqueue layout
 * @param[in] structSpiQueuePtrArg double pointer to the spiqueue pointer
 * @param[in] sizeMaxArg maximum amount of packets the spiqueue may hold
 * @retval 0 on success, -1 on failure
 * @note - equipped with errorCatcher()
 */
int8_t spiQueueCreate(struct structSpiQueue** structSpiQueuePtrArg, uint8_t sizeMaxArg) {
	// check if spiqueue already exists
	if (*structSpiQueuePtrArg != NULL) {
		errorCatcher(ec_sq_already_exist);
		return -1;
	}
	// malloc new spiqueue
	struct structSpiQueue* newStructSpiQueue = malloc(sizeof(struct structSpiQueue));
	// check if malloc was successful
	if (newStructSpiQueue == NULL) {
		errorCatcher(ec_sq_malloc_failed);
		return -1;
	}
	// initialize spiqueue default fields
	newStructSpiQueue->sizeCurrent = 0;
	newStructSpiQueue->sizeMax = sizeMaxArg;
	newStructSpiQueue->headPacketPtr = NULL;
	newStructSpiQueue->tailPacketPtr = NULL;
	// set address of malloced spiqueue to argument pointer
	*structSpiQueuePtrArg = newStructSpiQueue;
	return 0;
}

/**
 * @brief removes the spiqueue and all its child packets
 * @param[in] structSpiQueuePtrArg double pointer to the spiqueue pointer
 * @retval 0 on success, -1 on failure
 * @note - equipped with errorcatcher()
 */
int8_t spiQueueRemove(struct structSpiQueue** structSpiQueuePtrArg) {
	// check if ptr is not zero
	if (*structSpiQueuePtrArg == NULL) {
		errorCatcher(ec_sq_doesnt_exist);
		return -1;
	}
	// remove all packets
	while ((*structSpiQueuePtrArg)->headPacketPtr != NULL) {
		if (spiQueuePacketRemove(*structSpiQueuePtrArg)) {
			errorCatcher(ec_sq_remove_failed);
			return -1;
		}
	}
	// free spiqueue
	free(*structSpiQueuePtrArg);
	// zero the address
	*structSpiQueuePtrArg = NULL;
	return 0;
}

/**
 * @brief allocates memory and initialises a packet according to the structpacket layout
 * @param[in] arrayArg 	[0]: predefined id recorded by the lexicon used to distinguish variables as they turn abstracted while in spi transfer.
 *  					[1-8]: unpacked datatype bytes making up a variable value
 * 						[9-10]: ack value
 * 						[11-12]: crc value
 * @retval pointer to the new packet
 * @note - equipped with errorcatcher()
 */
static struct structPacket* spiQueuePacketAppend(uint8_t arrayArg[]) {
	// malloc new packet
	struct structPacket* newPacket = malloc(sizeof(struct structPacket));
	// check if malloc was successful
	if (newPacket == NULL) {
		errorCatcher(ec_sq_packet_malloc_failed);
		return NULL;
	}
	// set newpacket fields from array data
	newPacket->identifier = arrayArg[SQ_ID_INDEX];
	memcpy(newPacket->payload.uint8, arrayArg + SQ_PAYLOAD_INDEX, SQ_PAYLOAD_SIZE);
	memcpy(newPacket->ack.returnCrc.uint8, arrayArg + SQ_ACK_INDEX, SQ_ACK_SIZE);
	memcpy(newPacket->crc.value.uint8, arrayArg + SQ_CRC_INDEX, SQ_CRC_SIZE);
	// initialize newpacket fields
	newPacket->crc.verified = false;
	newPacket->crc.good = false;
	newPacket->ack.retrieved = false;
	newPacket->nextPacketPtr = NULL;
	// return packet
	return newPacket;
}

/**
 * @brief remove a packet from the head of the spiqueue
 * @param[in] structSpiQueuePtrArg pointer to the structspiqueue instance
 * @retval 0 on success, -1 on failure
 * @note - equipped with errorcatcher()
 */
int8_t spiQueuePacketRemove(struct structSpiQueue* structSpiQueuePtrArg) {
	// check if a single packet is present
	if (structSpiQueuePtrArg->headPacketPtr == NULL) {
		errorCatcher(ec_sq_no_packet_exists);
		return -1;
	}
	// when single frame reset head and tail ptrs to null
	if (structSpiQueuePtrArg->headPacketPtr == structSpiQueuePtrArg->tailPacketPtr) {
		structSpiQueuePtrArg->sizeCurrent--;
		free(structSpiQueuePtrArg->headPacketPtr);
		structSpiQueuePtrArg->headPacketPtr = NULL;
		structSpiQueuePtrArg->tailPacketPtr = NULL;
		// when multiple frames move head ptr
	} else {
		struct structPacket* previousheadPacketPtr = structSpiQueuePtrArg->headPacketPtr;
		structSpiQueuePtrArg->headPacketPtr = structSpiQueuePtrArg->headPacketPtr->nextPacketPtr;
		structSpiQueuePtrArg->sizeCurrent--;
		free(previousheadPacketPtr);
	}
	return 0;
}

/**
 * @brief appends a packet to the spiqueue tail by means of an array
 * @param[in] structSpiQueuePtrArg pointer to the structspiqueue instance
 * @param[in] arrayArg 	[0]: predefined id recorded by the lexicon used to distinguish variables as they turn abstracted while in spi transfer.
 *  					[1-8]: unpacked datatype bytes making up a variable value
 * 						[9-10]: ack value
 * 						[11-12]: crc value
 * @param[in] arraySizeArg size of arrayarg
 * @param[in] crcCheckArg check crc value if set
 * @retval 0 on success, -1 on failure
 * @note - equipped with errorcatcher()
 */
int8_t spiQueuePostArray(struct structSpiQueue* structSpiQueuePtrArg, uint8_t arrayArg[], uint8_t arraySizeArg, bool crcCheckArg) {
	// check if spiqueue exists
	if (structSpiQueuePtrArg == NULL) {
		errorCatcher(ec_sq_doesnt_exist_post);
		return -1;
	}
	// check if spiqueue is full
	if (structSpiQueuePtrArg->sizeCurrent >= structSpiQueuePtrArg->sizeMax) {
		errorCatcher(ec_sq_full);
		return -1;
	}
	// check if array length is correct
	if (arraySizeArg != SQ_PACKET_SIZE) {
		errorCatcher(ec_sq_incorrect_array_length);
		return -1;
	}
	// appending first packet to spiqueue and increment packet count if spiqueuepacketappend succeeds
	// head and tail point to same packet
	if (structSpiQueuePtrArg->headPacketPtr == NULL) {
		structSpiQueuePtrArg->headPacketPtr = spiQueuePacketAppend(arrayArg);
		if (structSpiQueuePtrArg->headPacketPtr == NULL) {
			// the failure to malloc a packet is already caught in spiqueuepacketappend
			return -1;
		}
		structSpiQueuePtrArg->sizeCurrent++;
		structSpiQueuePtrArg->tailPacketPtr = structSpiQueuePtrArg->headPacketPtr;
		// append packet into non empty spiqueue and increment packet count if spiqueuepacketappend succeeds
		// append packet to to the tail and move tail pointer
	} else {
		structSpiQueuePtrArg->tailPacketPtr->nextPacketPtr = spiQueuePacketAppend(arrayArg);
		if (structSpiQueuePtrArg->tailPacketPtr->nextPacketPtr == NULL) {
			// the failure to malloc a packet is already caught in spiqueuepacketappend
			return -1;
		}
		structSpiQueuePtrArg->sizeCurrent++;
		structSpiQueuePtrArg->tailPacketPtr = structSpiQueuePtrArg->tailPacketPtr->nextPacketPtr;
	}
	// check crc and set crcverified if crccheckarg
	if (crcCheckArg) {
		structSpiQueuePtrArg->tailPacketPtr->crc.verified = true;
		if (structSpiQueuePtrArg->tailPacketPtr->crc.value.uint16 == GETCRC(arrayArg)) {
			structSpiQueuePtrArg->tailPacketPtr->crc.good = true;
		}
	}
	return 0;
}

/**
 * @brief find the c datatype value in the lexicon for the specified id
 * @param[in] identifierArg a predefined id recorded by the lexicon used to distinguish variables
 * @retval datatype enum
 */
static int16_t spiQueueFindType(uint8_t identifierArg) {
	// check if id is legal
	if (identifierArg == 0x00 || identifierArg == 0xFF) {
		errorCatcher(ec_sq_bad_id);
		return -1;
	}
	// find id array index
	// index scope for loop and return
	uint8_t index;
	uint8_t lexiconHighestIndex = (sizeof(lexicon) / sizeof(lexicon[0])) - 1;
	for (index = 0; index < lexiconHighestIndex; index++) {
		if (lexicon[index].identifier == identifierArg) {
			break;
		}
	}
	// return the datatype for found index of id
	return lexicon[index].dataType;
}

/**
 * @brief create packet using id and integer value parameters
 * @param[in] structSpiQueuePtrArg pointer to the structspiqueue instance
 * @param[in] identifierArg a predefined id recorded by the coder used to distinguish variables
 * @param[in] payloadValueArg integer value
 * @retval 0 on success, -1 on failure
 * @note - equipped with errorcatcher()
 */
int8_t spiQueuePostInt(struct structSpiQueue* structSpiQueuePtrArg, uint8_t identifierArg, int64_t payloadValueArg) {
	// create temporary variables
	uint8_t arrayTemp[SQ_PACKET_SIZE] = {0};
	union unionPayload payloadTemp = {0};
	// set id
	arrayTemp[SQ_ID_INDEX] = identifierArg;
	// switch case on datatype
	switch (spiQueueFindType(identifierArg)) {
	case BINARY:
		if (payloadValueArg != 0 && payloadValueArg != 1) {
			errorCatcher(ec_sq_payload_out_of_range_binary);
			return -1;
		}
		payloadTemp.binary = payloadValueArg;
		memcpy(arrayTemp + SQ_PAYLOAD_INDEX, payloadTemp.uint8, 1);
		break;
	// for integer cases check if payloadvaluearg is within legal range
	case UINT8:
		if (payloadValueArg < 0 || payloadValueArg > UINT8_MAX) {
			errorCatcher(ec_sq_payload_out_of_range_uint8);
			return -1;
		}
		payloadTemp.uint8[0] = payloadValueArg;
		memcpy(arrayTemp + SQ_PAYLOAD_INDEX, payloadTemp.uint8, 1);
		break;
	case UINT16:
		if (payloadValueArg < 0 || payloadValueArg > UINT16_MAX) {
			errorCatcher(ec_sq_payload_out_of_range_uint16);
			return -1;
		}
		payloadTemp.uint16 = payloadValueArg;
		memcpy(arrayTemp + SQ_PAYLOAD_INDEX, payloadTemp.uint8, 2);
		break;
	case UINT32:
		if (payloadValueArg < 0 || payloadValueArg > UINT32_MAX) {
			errorCatcher(ec_sq_payload_out_of_range_uint32);
			return -1;
		}
		payloadTemp.uint32 = payloadValueArg;
		memcpy(arrayTemp + SQ_PAYLOAD_INDEX, payloadTemp.uint8, 4);
		break;
	case SINT8:
		if (payloadValueArg < INT8_MIN || payloadValueArg > INT8_MAX) {
			errorCatcher(ec_sq_payload_out_of_range_sint8);
			return -1;
		}
		payloadTemp.sint8 = payloadValueArg;
		memcpy(arrayTemp + SQ_PAYLOAD_INDEX, payloadTemp.uint8, 1);
		break;
	case SINT16:
		if (payloadValueArg < INT16_MIN || payloadValueArg > INT16_MAX) {
			errorCatcher(ec_sq_payload_out_of_range_sint16);
			return -1;
		}
		payloadTemp.sint16 = payloadValueArg;
		memcpy(arrayTemp + SQ_PAYLOAD_INDEX, payloadTemp.uint8, 2);
		break;
	case SINT32:
		if (payloadValueArg < INT32_MIN || payloadValueArg > INT32_MAX) {
			errorCatcher(ec_sq_payload_out_of_range_sint32);
			return -1;
		}
		payloadTemp.sint32 = payloadValueArg;
		memcpy(arrayTemp + SQ_PAYLOAD_INDEX, payloadTemp.uint8, 4);
		break;
	// floating cases have no range checks
	// natural numbers might also pass as floats
	case FRAC32:
		payloadTemp.frac32 = payloadValueArg;
		memcpy(arrayTemp + SQ_PAYLOAD_INDEX, payloadTemp.uint8, 4);
		break;
	case FRAC64:
		payloadTemp.frac64 = payloadValueArg;
		memcpy(arrayTemp + SQ_PAYLOAD_INDEX, payloadTemp.uint8, 8);
		break;
	// when no datatype is found, which would be the most likely result of id being invalid
	default:
		errorCatcher(ec_sq_payload_no_datatype);
		return -1;
	}
	// fill crc fields
	union unionCrc crc;
	crc.uint16 = GETCRC(arrayTemp);
	memcpy(arrayTemp + SQ_CRC_INDEX, crc.uint8, SQ_CRC_SIZE);
	// create packet from arraytemp
	spiQueuePostArray(structSpiQueuePtrArg, arrayTemp, arraysize(arrayTemp), false);
	return 0;
}

/**
 * @brief create packet using id and fractional value parameters
 * @param[in] structSpiQueuePtrArg pointer to the structspiqueue instance
 * @param[in] identifierArg a predefined id recorded by the coder used to distinguish variables
 * @param[in] payloadValueArg fractional value
 * @retval 0 on success, -1 on failure
 * @note - equipped with errorcatcher()
 */
int8_t spiQueuePostFrac(struct structSpiQueue* structSpiQueuePtrArg, uint8_t identifierArg, double payloadValueArg) {
	// create temporary variables
	uint8_t arrayTemp[SQ_PACKET_SIZE] = {0};
	union unionPayload payloadTemp = {0};
	// set id
	arrayTemp[SQ_ID_INDEX] = identifierArg;
	// switch case on datatype
	switch (spiQueueFindType(identifierArg)) {
	case FRAC32:
		payloadTemp.frac32 = payloadValueArg;
		memcpy(arrayTemp + SQ_PAYLOAD_INDEX, payloadTemp.uint8, 4);
		break;
	case FRAC64:
		payloadTemp.frac64 = payloadValueArg;
		memcpy(arrayTemp + SQ_PAYLOAD_INDEX, payloadTemp.uint8, 8);
		break;
	// when no datatype is found, which would be the most likely result of id being invalid
	default:
		errorCatcher(ec_sq_payload_no_datatype);
		return -1;
	}
	// fill crc fields
	union unionCrc crc;
	crc.uint16 = GETCRC(arrayTemp);
	memcpy(arrayTemp + SQ_CRC_INDEX, crc.uint8, SQ_CRC_SIZE);
	// create packet from arraytemp
	spiQueuePostArray(structSpiQueuePtrArg, arrayTemp, arraysize(arrayTemp), false);
	return 0;
}

/**
 * @brief puts the id, payloads and crc values from head frame to appointed array
 * @param[in] structSpiQueuePtrArg pointer to the structspiqueue instance
 * @param[out] arrayArg[] pointer to array to retrieve values to
 * @param[in] arraySizeArg size of arrayarg
 * @retval 0 on success, -1 on failure
 * @note - equipped with errorcatcher()
 */
int8_t spiQueueGetArray(struct structSpiQueue* structSpiQueuePtrArg, uint8_t arrayArg[], uint8_t arraySizeArg) {
	// Check if packet exists
	if (structSpiQueuePtrArg->headPacketPtr == NULL) {
		// errorCatcher(ec_sq_no_packet_exists_get);
		// return -1;
		spiQueuePostInt(structSpiQueuePtrArg, ID_FILLER, 0x00);
	}
	// check if array length is correct
	if (arraySizeArg != SQ_PACKET_SIZE) {
		errorCatcher(ec_sq_incorrect_array_length);
		return -1;
	}
	// get id
	memcpy(arrayArg + SQ_ID_INDEX, &(structSpiQueuePtrArg->headPacketPtr->identifier), SQ_ID_SIZE);
	// get payload
	memcpy(arrayArg + SQ_PAYLOAD_INDEX, structSpiQueuePtrArg->headPacketPtr->payload.uint8, SQ_PAYLOAD_SIZE);
	// get ack
	memcpy(arrayArg + SQ_ACK_INDEX, structSpiQueuePtrArg->headPacketPtr->ack.returnCrc.uint8, SQ_ACK_SIZE);
	// get crc
	memcpy(arrayArg + SQ_CRC_INDEX, structSpiQueuePtrArg->headPacketPtr->crc.value.uint8, SQ_CRC_SIZE);
	return 0;
}

/**
 * @brief placeholder
 */
int8_t spiQueueProcessAck(struct structSpiQueue* spiQueueTransmitPtrArg, struct structSpiQueue* spiQueueReceivePtrArg, bool ignoreAck) {
	if (ignoreAck) {
		if (spiQueueTransmitPtrArg->sizeCurrent > 0) {
			spiQueuePacketRemove(spiQueueTransmitPtrArg);
		}
	} else {
		errorCatcher(ec_sq_not_implemented);
		return -1;
	}
	return 0;
}

/**
 * @brief checks if the input array has changed
 * @param[out] duplicateArg true if changed, false is the same
 * @param[in] arrayArg[] pointer to input array
 * @param[in] arraySizeArg size of arrayarg
 * @retval 0 on success, -1 on failure
 * @note - equipped with errorcatcher()
 */
int8_t spiQueueNoDuplicate(bool* duplicateArg, uint8_t arrayArg[], uint8_t arraySizeArg) {
	// check if array length is correct
	if (arraySizeArg != SQ_PACKET_SIZE) {
		errorCatcher(ec_sq_incorrect_array_length);
		return -1;
	}
	// persistent array to hold previous value
	static uint8_t memoryArray[SQ_PACKET_SIZE] = {0};
	if (memcmp(memoryArray, arrayArg, SQ_PACKET_SIZE) == 0) {
		// they are the same therefore duplicate=true -> NO!_duplicate=false
		*duplicateArg = false;
	} else {
		*duplicateArg = true;
		memcpy(memoryArray, arrayArg, SQ_PACKET_SIZE);
	}
	return 0;
}
