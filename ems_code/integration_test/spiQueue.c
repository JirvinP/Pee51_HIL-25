/**
 * @file spiQueue.c
 * @author Sefa Ozturk (S.H.Ozturk@outlook.com)
 * @brief FIFO buffer for use with SPI
 * @version 0.4
 * @date 2025-01-06
 */

#define X(pos) (1 << pos)

#include "spiQueue.h"

#if HARDWARE_CRC
/**
 * @brief Device descriptor for ST CRC peripheral
 */
extern CRC_HandleTypeDef hcrc;
#endif

/**
 * @brief Global error code variable
 */
uint8_t errorVal = ec_no_error;

// ERROR --------------------------------------------------------------------------------------------------------------------

/**
 * @brief Stores errorCodeArg in the global errorVal variable
 * @param[in] errorCodeArg By preference enums from the global error code list
 */
void errorCatcher(uint8_t errorCodeArg) {
	errorVal = errorCodeArg;
	PRINT("\x1B[31merrorVal: 0x%02X\n\x1B[0m", errorCodeArg);
}

/**
 * @brief Resets errorCodeArg in the global errorVal variable to 0
 */
void errorReset(void) {
	errorVal = ec_no_error;
}

// CRC ----------------------------------------------------------------------------------------------------------------------

/**
 * @brief XXX
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
 * @brief Global crcData variable
 */
struct structCrcData crcData;

/**
 * @brief XXX
 * @param[in] crcDataArg XXX
 * @retval None
 */
void crcInit(struct structCrcData* crcDataArg) {
	crcDataArg->bitLength = 16;
	crcDataArg->polynomial = X(12) + X(5) + X(0);
	crcDataArg->initialValue = 0x0;
	crcDataArg->finalXorValue = 0x0;
	crcDataArg->inputReflected = false;
	crcDataArg->resultReflected = false;
	crcDataArg->bitLengthMask = 0xFFFFFFFF >> (32 - crcDataArg->bitLength);
	crcCalcTable(crcDataArg);
}

/**
 * @brief XXX
 * @param[in] crcDataArg XXX
 * @retval None
 */
uint32_t crcCalcSlow(struct structCrcData* crcDataArg, uint8_t dataArrayArg[], uint8_t dataArrayLengthArg) {
	uint32_t checksum = crcDataArg->initialValue;
	uint32_t highestBitPos = 1 << (crcDataArg->bitLength - 1);
	for (uint8_t byte = 0; byte < dataArrayLengthArg; byte++) {
		if (crcDataArg->inputReflected) {
			checksum ^= crcReflect(dataArrayArg[byte], 8) << (crcDataArg->bitLength - 8);
		} else {
			checksum ^= dataArrayArg[byte] << (crcDataArg->bitLength - 8);
		}
		for (uint8_t bit = 0; bit < 8; bit++) {
			if ((checksum & highestBitPos) != 0) {
				checksum = (checksum << 1) ^ crcDataArg->polynomial;
			} else {
				checksum <<= 1;
			}
		}
	}
	if (crcDataArg->resultReflected) {
		return (crcReflect(checksum, crcDataArg->bitLength) ^ crcDataArg->finalXorValue) & crcDataArg->bitLengthMask;
	} else {
		return (checksum ^ crcDataArg->finalXorValue) & crcDataArg->bitLengthMask;
	}
}

/**
 * @brief XXX
 * @param[in] crcDataArg XXX
 * @retval None
 */
uint32_t crcCalcFast(struct structCrcData* crcDataArg, uint8_t dataArrayArg[], uint8_t dataArrayLengthArg) {
	uint8_t index = 0;
	uint32_t checksum = crcDataArg->initialValue;
	for (uint8_t byte = 0; byte < dataArrayLengthArg; byte++) {
		if (crcDataArg->inputReflected) {
			index = crcReflect(dataArrayArg[byte], 8) ^ (checksum >> (crcDataArg->bitLength - 8));
		} else {
			index = dataArrayArg[byte] ^ (checksum >> (crcDataArg->bitLength - 8));
		}
		checksum = crcDataArg->lookUpTable[index] ^ (checksum << 8);
	}
	if (crcDataArg->resultReflected) {
		return (crcReflect(checksum, crcDataArg->bitLength) ^ crcDataArg->finalXorValue) & crcDataArg->bitLengthMask;
	} else {
		return (checksum ^ crcDataArg->finalXorValue) & crcDataArg->bitLengthMask;
	}
}

/**
 * @brief XXX
 * @param[in] crcDataArg XXX
 * @retval None
 */
void crcCalcTable(struct structCrcData* crcDataArg) {
	uint32_t highestBitPos = 1 << (crcDataArg->bitLength - 1);
	for (uint16_t byte = 0; byte < 256; byte++) {
		uint32_t checksum = byte;
		for (uint8_t bit = 0; bit < crcDataArg->bitLength; bit++) {
			if ((checksum & highestBitPos) != 0) {
				checksum = (checksum << 1) ^ crcDataArg->polynomial;
			} else {
				checksum <<= 1;
			}
		}
		crcDataArg->lookUpTable[byte] = checksum & crcDataArg->bitLengthMask;
	}
}

/**
 * @brief XXX
 * @param[in] crcDataArg XXX
 * @retval None
 */
void crcCalcTablePrint(struct structCrcData* crcDataArg, bool hexOutputArg, bool tableFormatArg) {
	uint16_t index = 0;
	uint8_t columns = 0;
	PRINT("\x1B[30;47m\n// clang-format off\n");
	PRINT("uint%d_t lookUpTable[256] = { \\\n", crcDataArg->bitLength);
	columns = (crcDataArg->bitLength == 32 ? 8 : 16);
	while (index < 256) {
		if (hexOutputArg) {
			PRINT("0x%0*lX", crcDataArg->bitLength / 4, crcDataArg->lookUpTable[index]);
		} else {
			PRINT("%d", crcDataArg->lookUpTable[index]);
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

/**
 * @brief Structure definition for the lexicon
 */
struct structLexicon {
	uint8_t identifier;		  /**< ID code */
	uint8_t dataType;		  /**< C datatype */
	uint8_t varString[32];	  /**< Printable variable name */
	uint8_t varUnitString[8]; /**< Printable variable unit specifier */
};

/**
 * @brief Supported datatypes
 */
enum lexiconDataTypes {
	X,
	BOOL,
	UINT8,
	UINT16,
	UINT32,
	SINT8,
	SINT16,
	SINT32,
	FRAC32,
	FRAC64
};

// clang-format off
/**
 * @brief Dictionary with easily recognizable structure
 */
const struct structLexicon lexicon[] = {
// BAD IDs
	{0x00, X,		"X",					"X"		},
	{0xFF, X,		"X",					"X"		},

// FILLER
	{0x01, UINT8,	"F",					"F"		},

// TEST
	{0xA0, UINT8,	"Test UINT8",			"T"		},
	{0xA1, UINT16,	"Test UINT16",			"T"		},
	{0xA2, UINT32,	"Test UINT32",			"T"		},
	{0xA3, SINT8,	"Test SINT8",			"T"		},
	{0xA4, SINT16,	"Test SINT16",			"T"		},
	{0xA5, SINT32,	"Test SINT32",			"T"		},
	{0xA6, FRAC32,	"Test FRAC32",			"T"		},
	{0xA7, FRAC64,	"Test FRAC64",			"T"		},
	{0xA8, BOOL,	"Test BOOL",			"T"		},
	{0xA9, UINT32,	"Test latency",			"T"		},

// OUTBOUND
	{0xB1, SINT32,	"Setpoint battery 1",	"kW"	},
	{0xB2, SINT32,	"Setpoint battery 2",	"kW"	},
	{0xB3, UINT32,	"Setpoint DG 1",		"kW"	},
	{0xB4, UINT32,	"Setpoint DG 2",		"kW"	},
	{0xB5, BOOL,	"Overload",				"bool"	},

// INBOUND
	{0xC1, FRAC64,	"Power battery 1",		"kW"	},
	{0xC2, FRAC64,	"Power battery 2",		"kW"	},
	{0xC3, FRAC32,	"SOC battery 1",		"%%"	},
	{0xC4, FRAC32,	"SOC battery 2",		"%%"	},
	{0xC5, UINT32,	"Power DG 1",			"kW"	},
	{0xC6, UINT32,	"Power DG 2",			"kW"	},
	{0xC7, FRAC32,	"SFOC 1",				"%%"	},
	{0xC8, FRAC32,	"SFOC 2",				"%%"	},
	{0xC9, UINT8,	"OPstate",				"enum"	}
};
// clang-format on

/**
 * @brief Allocates memory and initialises a spiQueue according to the structSpiQueue layout
 * @param[in] sizeMaxArg Maximum amount of frames the spiQueue may hold
 * @retval Ptr to the new spiQueue
 * @note - Equipped with errorCatcher()
 */
struct structSpiQueue* spiQueueCreate(uint8_t sizeMaxArg) {
	// Malloc new spiQueue
	struct structSpiQueue* structSpiQueuePtr = malloc(sizeof(struct structSpiQueue));

	// Check if malloc allocation was successful
	if (structSpiQueuePtr == NULL) {
		errorCatcher(ec_spiQueue_malloc_failed);
		return NULL;
	}

	// Initialize spiQueue standard fields
	structSpiQueuePtr->sizeCurrent = 0;
	structSpiQueuePtr->sizeMax = sizeMaxArg;
	structSpiQueuePtr->headFramePtr = NULL;
	structSpiQueuePtr->tailFramePtr = NULL;

	// Return spiQueue
	return structSpiQueuePtr;
}

/**
 * @brief Removes the spiQueue and all its child elements
 * @param[in] *structSpiQueuePtrArg Ptr to the structSpiQueue instance
 * @retval 0 on success, -1 on failure
 * @note - Equipped with errorCatcher()
 * @note - Zeroing the structSpiQueuePtr after removing is recommended practice
 */
int8_t spiQueueRemove(struct structSpiQueue* structSpiQueuePtrArg) {
	// Check if ptr points to a plausible address
	if (structSpiQueuePtrArg == NULL) {
		errorCatcher(ec_spiQueue_doesnt_exist);
		return -1;
	}

	// Remove all frames
	while (structSpiQueuePtrArg->headFramePtr != NULL) {
		if (spiQueueFrameRemove(structSpiQueuePtrArg)) {
			errorCatcher(ec_spiQueue_remove_failed);
			return -1;
		}
	}

	// Free spiQueue
	free(structSpiQueuePtrArg);

	return 0;
}

/**
 * @brief Allocates memory and initialises a frame according to the structFrame layout
 * @param[in] receiveArrayArg [0]: A predefined ID recorded by the coder used to distinguish variables as they turn abstracted while in SPI transfer [1-8]: Raw bytes making up a datatype [9]: CRC value
 * @retval Ptr to the new frame
 * @note - Equipped with errorCatcher()
 */
static struct structFrame* spiQueueFrameAppend(uint8_t receiveArrayArg[]) {
	// Malloc new frame
	struct structFrame* newFrame = malloc(sizeof(struct structFrame));

	// Check if malloc allocation was successful
	if (newFrame == NULL) {
		errorCatcher(ec_frame_malloc_failed);
		return NULL;
	}

	// Set newFrame fields from array data
	newFrame->identifier = receiveArrayArg[0];
	memcpy(newFrame->payload.uint8, receiveArrayArg + 1, 8);
	memcpy(newFrame->ack.returnCrc.uint8, receiveArrayArg + 9, 2);
	memcpy(newFrame->crc.value.uint8, receiveArrayArg + 11, 2);

	// initialize newFrame fields
	newFrame->crc.verified = false;
	newFrame->crc.good = false;
	newFrame->ack.retrieved = false;
	newFrame->nextFramePtr = NULL;

	// Return frame
	return newFrame;
}

/**
 * @brief Remove a frame from the head of the spiQueue
 * @param[in] *structSpiQueuePtrArg Ptr to the structSpiQueue instance
 * @retval 0 on success, -1 on failure
 * @note - Equipped with errorCatcher()
 */
int8_t spiQueueFrameRemove(struct structSpiQueue* structSpiQueuePtrArg) {
	// Check if frame exists
	if (structSpiQueuePtrArg->headFramePtr == NULL) {
		errorCatcher(ec_no_frame_exists);
		return -1;
	}

	// When single frame reset head and tail ptrs to NULL
	if (structSpiQueuePtrArg->headFramePtr == structSpiQueuePtrArg->tailFramePtr) {
		structSpiQueuePtrArg->sizeCurrent--;
		free(structSpiQueuePtrArg->headFramePtr);
		structSpiQueuePtrArg->headFramePtr = NULL;
		structSpiQueuePtrArg->tailFramePtr = NULL;

		// When multiple frames move head ptr
	} else {
		struct structFrame* previousHeadFramePtr = structSpiQueuePtrArg->headFramePtr;
		structSpiQueuePtrArg->headFramePtr = structSpiQueuePtrArg->headFramePtr->nextFramePtr;
		structSpiQueuePtrArg->sizeCurrent--;
		free(previousHeadFramePtr);
	}
	return 0;
}

/**
 * @brief Appends a frame to the spiQueue tail using an array
 * @param[in] *structSpiQueuePtrArg Ptr to the structSpiQueue instance
 * @param[in] arrayArg [0]: A predefined ID recorded by the coder used to distinguish variables as they turn abstracted while in SPI transfer [1-8]: Raw bytes making up a datatype [9]: CRC value
 * @retval 0 on success, -1 on failure
 * @note - Equipped with errorCatcher()
 */
int8_t spiQueuePostArray(struct structSpiQueue* structSpiQueuePtrArg, uint8_t arrayArg[], bool crcCheckArg) {
	// Check if frame exists
	if (structSpiQueuePtrArg == NULL) {
		errorCatcher(ec_spiQueue_doesnt_exist);
		return -1;
	}

	// Check if spiQueue is full
	if (structSpiQueuePtrArg->sizeCurrent >= structSpiQueuePtrArg->sizeMax) {
		errorCatcher(ec_spiQueue_full);
		return -1;
	}

	// Appending first frame to spiQueue and increment frame count if spiQueueFrameAppend succeeds
	// Head and tail point to same frame
	if (structSpiQueuePtrArg->headFramePtr == NULL) {
		structSpiQueuePtrArg->headFramePtr = spiQueueFrameAppend(arrayArg);
		if (structSpiQueuePtrArg->headFramePtr == NULL) {
			return -1;
		}
		structSpiQueuePtrArg->sizeCurrent++;
		structSpiQueuePtrArg->tailFramePtr = structSpiQueuePtrArg->headFramePtr;

		// Append frame into non empty spiQueue and increment frame count if spiQueueFrameAppend succeeds
		// Append frame to to the tail and move tail ptr
	} else {
		structSpiQueuePtrArg->tailFramePtr->nextFramePtr = spiQueueFrameAppend(arrayArg);
		if (structSpiQueuePtrArg->tailFramePtr->nextFramePtr == NULL) {
			return -1;
		}
		structSpiQueuePtrArg->sizeCurrent++;
		structSpiQueuePtrArg->tailFramePtr = structSpiQueuePtrArg->tailFramePtr->nextFramePtr;
	}

	// Check CRC and set crcVerified if crcCheckArg
	if (crcCheckArg) {
		structSpiQueuePtrArg->tailFramePtr->crc.verified = true;
		if (structSpiQueuePtrArg->tailFramePtr->crc.value.uint16 == GETCRC(arrayArg)) {
			structSpiQueuePtrArg->tailFramePtr->crc.good = true;
		}
	}

	return 0;
}

/**
 * @brief Finds the c datatype value as in the lexicon for the specified ID
 * @param[in] identifierArg A predefined ID recorded by the coder used to distinguish variables
 * @retval datatype
 */
static int16_t spiQueueFindType(uint8_t identifierArg) {
	// Check if ID is legal
	if (identifierArg == 0x00 || identifierArg == 0xFF) {
		errorCatcher(ec_bad_id);
		return -1;
	}

	// Find ID array index
	// Index scope for loop and return
	uint8_t index;
	for (index = 0; index < UINT8_MAX; index++) {
		if (lexicon[index].identifier == identifierArg) {
			break;
		}
	}

	// Return the datatype for found index of ID
	return lexicon[index].dataType;
}

/**
 * @brief Create frame using ID and decimal value parameters
 * @param[in] *structSpiQueuePtrArg Ptr to the structSpiQueue instance
 * @param[in] identifierArg A predefined ID recorded by the coder used to distinguish variables
 * @param[in] payloadValueArg Integer value
 * @retval 0 on success, -1 on failure
 * @note - Equipped with errorCatcher()
 */
int8_t spiQueuePostInt(struct structSpiQueue* structSpiQueuePtrArg, uint8_t identifierArg, int64_t payloadValueArg) {
	// Create temporary storage units
	uint8_t arrayTemp[SPIQUEUE_SIZE] = {0};
	union unionPayload payloadTemp = {0};

	// Set ID
	arrayTemp[0] = identifierArg;

	// Switch case on datatype
	switch (spiQueueFindType(identifierArg)) {

	case BOOL:
		if (payloadValueArg != 0 && payloadValueArg != 1) {
			errorCatcher(ec_payload_out_of_range);
			return -1;
		}
		arrayTemp[1] = payloadValueArg;
		break;

	// For integer cases check if payloadValueArg is within legal range
	// Copy payloadValueArg directly to arrayTemp, no interpretation required
	case UINT8:
		if (payloadValueArg < 0 || payloadValueArg > UINT8_MAX) {
			errorCatcher(ec_payload_out_of_range);
			return -1;
		}
		arrayTemp[1] = payloadValueArg;
		break;

	// interpret and copy to arrayTemp
	case UINT16:
		if (payloadValueArg < 0 || payloadValueArg > UINT16_MAX) {
			errorCatcher(ec_payload_out_of_range);
			return -1;
		}
		payloadTemp.uint16 = payloadValueArg;
		memcpy(arrayTemp + 1, payloadTemp.uint8, 2);
		break;
	case UINT32:
		if (payloadValueArg < 0 || payloadValueArg > UINT32_MAX) {
			errorCatcher(ec_payload_out_of_range_1);
			return -1;
		}
		payloadTemp.uint32 = payloadValueArg;
		memcpy(arrayTemp + 1, payloadTemp.uint8, 4);
		break;
	case SINT8:
		if (payloadValueArg < INT8_MIN || payloadValueArg > INT8_MAX) {
			errorCatcher(ec_payload_out_of_range);
			return -1;
		}
		payloadTemp.sint8 = payloadValueArg;
		memcpy(arrayTemp + 1, payloadTemp.uint8, 1);
		break;
	case SINT16:
		if (payloadValueArg < INT16_MIN || payloadValueArg > INT16_MAX) {
			errorCatcher(ec_payload_out_of_range);
			return -1;
		}
		payloadTemp.sint16 = payloadValueArg;
		memcpy(arrayTemp + 1, payloadTemp.uint8, 2);
		break;
	case SINT32:
		if (payloadValueArg < INT32_MIN || payloadValueArg > INT32_MAX) {
			errorCatcher(ec_payload_out_of_range_2);
			return -1;
		}
		payloadTemp.sint32 = payloadValueArg;
		memcpy(arrayTemp + 1, payloadTemp.uint8, 4);
		break;

	// Floating cases have no range checks
	// Natural numbers like 1 2 3 might also pass as floats
	case FRAC32:
		payloadTemp.frac32 = payloadValueArg;
		memcpy(arrayTemp + 1, payloadTemp.uint8, 4);
		break;
	case FRAC64:
		payloadTemp.frac64 = payloadValueArg;
		memcpy(arrayTemp + 1, payloadTemp.uint8, 8);
		break;

	// When no datatype is found, which would be the most likely result of ID being invalid
	default:
		errorCatcher(ec_payload_no_datatype);
		return -1;
	}

	// Fill CRC fields
	union unionCrc crc;
	crc.uint16 = GETCRC(arrayTemp);
	memcpy(arrayTemp + 11, crc.uint8, 2);

	// Create frame from arrayTemp
	spiQueuePostArray(structSpiQueuePtrArg, arrayTemp, false);
	return 0;
}

/**
 * @brief Create frame using ID and decimal value parameters
 * @param[in] *structSpiQueuePtrArg Ptr to the structSpiQueue instance
 * @param[in] identifierArg A predefined ID recorded by the coder used to distinguish variables
 * @param[in] payloadValueArg Decimal value
 * @retval 0 on success, -1 on failure
 * @note - Equipped with errorCatcher()
 */
int8_t spiQueuePostDec(struct structSpiQueue* structSpiQueuePtrArg, uint8_t identifierArg, double payloadValueArg) {
	// Create temporary storage units
	uint8_t arrayTemp[SPIQUEUE_SIZE] = {0};
	union unionPayload payloadTemp = {0};

	// Set ID
	arrayTemp[0] = identifierArg;

	// Switch case on datatype
	switch (spiQueueFindType(identifierArg)) {

	// interpret and copy to arrayTemp
	// Floating cases have no range checks
	case FRAC32:
		payloadTemp.frac32 = payloadValueArg;
		memcpy(arrayTemp + 1, payloadTemp.uint8, 4);
		break;
	case FRAC64:
		payloadTemp.frac64 = payloadValueArg;
		memcpy(arrayTemp + 1, payloadTemp.uint8, 8);
		break;

	// When no datatype is found, which would be the most likely result of ID being invalid
	default:
		errorCatcher(ec_payload_no_datatype);
		return -1;
	}

	// Fill CRC fields
	union unionCrc crc;
	crc.uint16 = GETCRC(arrayTemp);
	memcpy(arrayTemp + 11, crc.uint8, 2);

	// Create frame from arrayTemp
	spiQueuePostArray(structSpiQueuePtrArg, arrayTemp, false);
	return 0;
}

/**
 * @brief Puts the ID, Payloads and CRC values from head frame to appointed array
 * @param[in] *structSpiQueuePtrArg Ptr to the structSpiQueue instance
 * @param[out] arrayArg[] Ptr to array to retrieve values to
 * @retval 0 on success, -1 on failure
 * @note - Equipped with errorCatcher()
 * @note - arrayArg[] should at least be uint8_t arrayArg[10]
 */
int8_t spiQueueGetArray(struct structSpiQueue* structSpiQueuePtrArg, uint8_t arrayArg[]) {
	// Check if frame exists
	if (structSpiQueuePtrArg->headFramePtr == NULL) {
		// errorCatcher(ec_no_frame_exists_get);
		// return -1;
		spiQueuePostInt(structSpiQueuePtrArg, 0x01, 0x0);
	}

	struct structFrame* movingFramePtr = structSpiQueuePtrArg->headFramePtr;
	// check if frame 1 has been retrieved before
	// check if frame 2 frame exists
	// move to frame 2 if frame 1 has been retrieved
	if (movingFramePtr->ack.retrieved == true) {
		if (movingFramePtr->nextFramePtr != NULL) {
			movingFramePtr = movingFramePtr->nextFramePtr;
		}
	}
	movingFramePtr->ack.retrieved = true;

	// put cbuf into array
	uint8_t arrayTemp[SPIQUEUE_SIZE] = {0};
	// get id
	arrayTemp[0] = movingFramePtr->identifier;
	// get payload
	memcpy(arrayTemp + 1, movingFramePtr->payload.uint8, 8);
	// get returnCrc
	memcpy(arrayTemp + 9, movingFramePtr->ack.returnCrc.uint8, 2);
	// get crc
	memcpy(arrayTemp + 11, movingFramePtr->crc.value.uint8, 2);
	// transfer temp to arg
	memcpy(arrayArg, arrayTemp, SPIQUEUE_SIZE);
	return 0;
}

/**
 * @brief Removes frames if acknowledged
 */
void spiQueueProcessAck(struct structSpiQueue* spiQueueTransmitPtrArg, struct structSpiQueue* spiQueueReceivePtrArg, bool ignoreAck) {
	if (ignoreAck) {
		if (spiQueueTransmitPtrArg->sizeCurrent > 0) {
			spiQueueFrameRemove(spiQueueTransmitPtrArg);
		}
	}
}

bool spiQueueNoDuplicate(uint8_t arrayArg[]) {
	static uint8_t memoryArray[SPIQUEUE_SIZE] = {0};
	if (memcmp(memoryArray, arrayArg, SPIQUEUE_SIZE) == 0) {
		// they are the same therefore duplicate=true -> NO!_duplicate=false
		return false;
	}
	memcpy(memoryArray, arrayArg, SPIQUEUE_SIZE);
	return true;
}
