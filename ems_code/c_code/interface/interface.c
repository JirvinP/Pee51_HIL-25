/**
 * @file interface.c
 * @author Sefa Ozturk (S.H.Ozturk@outlook.com)
 * @brief FIFO buffer for use with SPI
 * @version 0.3
 * @date 2024-11-20
 */

#define X(pos) (1 << pos)

#include "interface.h"

#if HARDWARE_CRC
/**
 * @brief Device descriptor for ST CRC peripheral
 */
extern CRC_HandleTypeDef hcrc;
#endif

/**
 * @brief Global crcData variable
 */
struct structCrcData crcData;

/**
 * @brief Global error code variable
 */
uint8_t errorVal = ec_no_error;

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
	{0xC1, SINT32,	"Power battery 1",		"kW"	},
	{0xC2, SINT32,	"Power battery 2",		"kW"	},
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

/**
 * @brief Allocates memory and initialises a buffer according to the structBuffer layout
 * @param[in] bufferSizeMaxArg Maximum amount of frames the buffer may hold
 * @retval Ptr to the new buffer
 * @note - Equipped with errorCatcher()
 */
struct structBuffer* bufferCreate(uint8_t bufferSizeMaxArg) {
	// Malloc new buffer
	struct structBuffer* structBufferPtr = malloc(sizeof(struct structBuffer));

	// Check if malloc allocation was successful
	if (structBufferPtr == NULL) {
		errorCatcher(ec_buffer_malloc_failed);
		return NULL;
	}

	// Initialize buffer standard fields
	structBufferPtr->bufferSizeCurrent = 0;
	structBufferPtr->bufferSizeMax = bufferSizeMaxArg;
	structBufferPtr->headFramePtr = NULL;
	structBufferPtr->tailFramePtr = NULL;

	// Return buffer
	return structBufferPtr;
}

/**
 * @brief Removes the buffer and all its child elements
 * @param[in] *structBufferPtrArg Ptr to the structBuffer instance
 * @retval 0 on success, -1 on failure
 * @note - Equipped with errorCatcher()
 * @note - Zeroing the structBufferPtr after removing is recommended practice
 */
int8_t bufferRemove(struct structBuffer* structBufferPtrArg) {
	// Check if ptr points to a plausible address
	if (structBufferPtrArg == NULL) {
		errorCatcher(ec_buffer_doesnt_exist);
		return -1;
	}

	// Remove all frames
	while (structBufferPtrArg->headFramePtr != NULL) {
		if (bufferRemoveFrame(structBufferPtrArg)) {
			errorCatcher(ec_buffer_remove_failed);
			return -1;
		}
	}

	// Free buffer
	free(structBufferPtrArg);

	return 0;
}

/**
 * @brief Allocates memory and initialises a frame according to the structFrame layout
 * @param[in] receiveArrayArg [0]: A predefined ID recorded by the coder used to distinguish variables as they turn abstracted while in SPI transfer [1-8]: Raw bytes making up a datatype [9]: CRC value
 * @retval Ptr to the new frame
 * @note - Equipped with errorCatcher()
 */
static struct structFrame* bufferAppendFrame(uint8_t receiveArrayArg[]) {
	// Malloc new frame
	struct structFrame* newFrame = malloc(sizeof(struct structFrame));

	// Check if malloc allocation was successful
	if (newFrame == NULL) {
		errorCatcher(ec_frame_malloc_failed);
		return NULL;
	}

	// Set newFrame fields from array data
	newFrame->identifier = receiveArrayArg[0];
	memcpy(newFrame->unionPayload.uint8, receiveArrayArg + 1, 8);
	newFrame->crcValue = ((uint16_t)receiveArrayArg[9] << 8) | receiveArrayArg[10];

	// Initialize newFrame standard fields
	newFrame->verified = false;
	newFrame->verified = false;
	newFrame->nextFramePtr = NULL;

	// Return frame
	return newFrame;
}

/**
 * @brief Remove a frame from the head of the buffer
 * @param[in] *structBufferPtrArg Ptr to the structBuffer instance
 * @retval 0 on success, -1 on failure
 * @note - Equipped with errorCatcher()
 */
int8_t bufferRemoveFrame(struct structBuffer* structBufferPtrArg) {
	// Check if frame exists
	if (structBufferPtrArg->headFramePtr == NULL) {
		errorCatcher(ec_no_frame_exists);
		return -1;
	}

	// When single frame reset head and tail ptrs to NULL
	if (structBufferPtrArg->headFramePtr == structBufferPtrArg->tailFramePtr) {
		structBufferPtrArg->bufferSizeCurrent--;
		free(structBufferPtrArg->headFramePtr);
		structBufferPtrArg->headFramePtr = NULL;
		structBufferPtrArg->tailFramePtr = NULL;

		// When multiple frames move head ptr
	} else {
		struct structFrame* previousHeadFramePtr = structBufferPtrArg->headFramePtr;
		structBufferPtrArg->headFramePtr = structBufferPtrArg->headFramePtr->nextFramePtr;
		structBufferPtrArg->bufferSizeCurrent--;
		free(previousHeadFramePtr);
	}
	return 0;
}

/**
 * @brief Appends a frame to the buffer tail using an array
 * @param[in] *structBufferPtrArg Ptr to the structBuffer instance
 * @param[in] receiveArrayArg [0]: A predefined ID recorded by the coder used to distinguish variables as they turn abstracted while in SPI transfer [1-8]: Raw bytes making up a datatype [9]: CRC value
 * @retval 0 on success, -1 on failure
 * @note - Equipped with errorCatcher()
 */
int8_t bufferPostArray(struct structBuffer* structBufferPtrArg, uint8_t receiveArrayArg[], bool crcCheckArg) {
	// Check if frame exists
	if (structBufferPtrArg == NULL) {
		errorCatcher(ec_buffer_doesnt_exist);
		return -1;
	}

	// Check if buffer is full
	if (structBufferPtrArg->bufferSizeCurrent >= structBufferPtrArg->bufferSizeMax) {
		errorCatcher(ec_buffer_full);
		return -1;
	}

	// Appending first frame to buffer and increment frame count if bufferAppendFrame succeeds
	// Head and tail point to same frame
	if (structBufferPtrArg->headFramePtr == NULL) {
		structBufferPtrArg->headFramePtr = bufferAppendFrame(receiveArrayArg);
		if (structBufferPtrArg->headFramePtr == NULL) {
			return -1;
		}
		structBufferPtrArg->bufferSizeCurrent++;
		structBufferPtrArg->tailFramePtr = structBufferPtrArg->headFramePtr;

		// Append frame into non empty buffer and increment frame count if bufferAppendFrame succeeds
		// Append frame to to the tail and move tail ptr
	} else {
		structBufferPtrArg->tailFramePtr->nextFramePtr = bufferAppendFrame(receiveArrayArg);
		if (structBufferPtrArg->tailFramePtr->nextFramePtr == NULL) {
			return -1;
		}
		structBufferPtrArg->bufferSizeCurrent++;
		structBufferPtrArg->tailFramePtr = structBufferPtrArg->tailFramePtr->nextFramePtr;
	}

	// Check CRC and set verified if crcCheckArg
	if (crcCheckArg) {
		uint16_t crcTemp = GETCRC(receiveArrayArg);
		if (structBufferPtrArg->tailFramePtr->crcValue == crcTemp) {
			structBufferPtrArg->tailFramePtr->verified = true;
		}
	}

	return 0;
}

/**
 * @brief Finds the c datatype value as in the lexicon for the specified ID
 * @param[in] identifierArg A predefined ID recorded by the coder used to distinguish variables
 * @retval datatype
 */
static int16_t bufferFindType(uint8_t identifierArg) {
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
 * @param[in] *structBufferPtrArg Ptr to the structBuffer instance
 * @param[in] identifierArg A predefined ID recorded by the coder used to distinguish variables
 * @param[in] payloadValueArg Integer value
 * @retval 0 on success, -1 on failure
 * @note - Equipped with errorCatcher()
 */
int8_t bufferPostInt(struct structBuffer* structBufferPtrArg, uint8_t identifierArg, int64_t payloadValueArg) {
	// Create temporary storage units
	uint8_t arrayTemp[11] = {0};
	union unionPayload unionPayloadTemp = {0};

	// Set ID
	arrayTemp[0] = identifierArg;

	// Switch case on datatype
	switch (bufferFindType(identifierArg)) {

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
		unionPayloadTemp.uint16 = payloadValueArg;
		memcpy(arrayTemp + 1, unionPayloadTemp.uint8, 2);
		break;
	case UINT32:
		if (payloadValueArg < 0 || payloadValueArg > UINT32_MAX) {
			errorCatcher(ec_payload_out_of_range);
			return -1;
		}
		unionPayloadTemp.uint32 = payloadValueArg;
		memcpy(arrayTemp + 1, unionPayloadTemp.uint8, 4);
		break;
	case SINT8:
		if (payloadValueArg < INT8_MIN || payloadValueArg > INT8_MAX) {
			errorCatcher(ec_payload_out_of_range);
			return -1;
		}
		unionPayloadTemp.int8 = payloadValueArg;
		memcpy(arrayTemp + 1, unionPayloadTemp.uint8, 1);
		break;
	case SINT16:
		if (payloadValueArg < INT16_MIN || payloadValueArg > INT16_MAX) {
			errorCatcher(ec_payload_out_of_range);
			return -1;
		}
		unionPayloadTemp.int16 = payloadValueArg;
		memcpy(arrayTemp + 1, unionPayloadTemp.uint8, 2);
		break;
	case SINT32:
		if (payloadValueArg < INT32_MIN || payloadValueArg > INT32_MAX) {
			errorCatcher(ec_payload_out_of_range);
			return -1;
		}
		unionPayloadTemp.int32 = payloadValueArg;
		memcpy(arrayTemp + 1, unionPayloadTemp.uint8, 4);
		break;

	// Floating cases have no range checks
	// Natural numbers like 1 2 3 might also pass as floats
	case FRAC32:
		unionPayloadTemp.frac32 = payloadValueArg;
		memcpy(arrayTemp + 1, unionPayloadTemp.uint8, 4);
		break;
	case FRAC64:
		unionPayloadTemp.frac64 = payloadValueArg;
		memcpy(arrayTemp + 1, unionPayloadTemp.uint8, 8);
		break;

	// When no datatype is found, which would be the most likely result of ID being invalid
	default:
		errorCatcher(ec_payload_no_datatype);
		return -1;
	}

	// Fill CRC fields
	uint16_t crcTemp = GETCRC(arrayTemp);
	arrayTemp[9] = crcTemp >> 8;
	arrayTemp[10] = crcTemp;

	// Create frame from arrayTemp
	bufferPostArray(structBufferPtrArg, arrayTemp, false);
	return 0;
}

/**
 * @brief Create frame using ID and decimal value parameters
 * @param[in] *structBufferPtrArg Ptr to the structBuffer instance
 * @param[in] identifierArg A predefined ID recorded by the coder used to distinguish variables
 * @param[in] payloadValueArg Decimal value
 * @retval 0 on success, -1 on failure
 * @note - Equipped with errorCatcher()
 */
int8_t bufferPostDec(struct structBuffer* structBufferPtrArg, uint8_t identifierArg, double payloadValueArg) {
	// Create temporary storage units
	uint8_t arrayTemp[11] = {0};
	union unionPayload unionPayloadTemp = {0};

	// Set ID
	arrayTemp[0] = identifierArg;

	// Switch case on datatype
	switch (bufferFindType(identifierArg)) {

	// interpret and copy to arrayTemp
	// Floating cases have no range checks
	case FRAC32:
		unionPayloadTemp.frac32 = payloadValueArg;
		memcpy(arrayTemp + 1, unionPayloadTemp.uint8, 4);
		break;
	case FRAC64:
		unionPayloadTemp.frac64 = payloadValueArg;
		memcpy(arrayTemp + 1, unionPayloadTemp.uint8, 8);
		break;

	// When no datatype is found, which would be the most likely result of ID being invalid
	default:
		errorCatcher(ec_payload_no_datatype);
		return -1;
	}

	// Fill CRC fields
	uint16_t crcTemp = GETCRC(arrayTemp);
	arrayTemp[9] = crcTemp >> 8;
	arrayTemp[10] = crcTemp;

	// Create frame from arrayTemp
	bufferPostArray(structBufferPtrArg, arrayTemp, false);
	return 0;
}

/**
 * @brief Create frame using ID and decimal value parameters
 * @param[in] *structBufferPtrArg Ptr to the structBuffer instance
 * @param[in] identifierArg A predefined ID recorded by the coder used to distinguish variables
 * @param[in] payloadValueArg Integer value
 * @retval 0 on success, -1 on failure
 * @note - Equipped with errorCatcher()
 */
int8_t bufferPostBool(struct structBuffer* structBufferPtrArg, uint8_t identifierArg, bool payloadValueArg) {
	// Create temporary storage unit
	uint8_t arrayTemp[11] = {0};

	// Set ID
	arrayTemp[0] = identifierArg;

	// Incorrect datatype for ID
	if (bufferFindType(identifierArg) != BOOL) {
		errorCatcher(ec_payload_no_datatype);
		return -1;
	}

	// Copy payloadValueArg directly to arrayTemp, no interpretation required
	arrayTemp[1] = payloadValueArg;

	// Fill CRC fields
	uint16_t crcTemp = GETCRC(arrayTemp);
	arrayTemp[9] = crcTemp >> 8;
	arrayTemp[10] = crcTemp;

	// Create frame from arrayTemp
	bufferPostArray(structBufferPtrArg, arrayTemp, false);
	return 0;
}

/**
 * @brief Puts the ID, Payloads and CRC values from head frame to appointed array
 * @param[in] *structBufferPtrArg Ptr to the structBuffer instance
 * @param[out] arrayArg[] Ptr to array to retrieve values to
 * @retval 0 on success, -1 on failure
 * @note - Equipped with errorCatcher()
 * @note - arrayArg[] should at least be uint8_t arrayArg[10]
 */
int8_t bufferGetArray(struct structBuffer* structBufferPtrArg, uint8_t arrayArg[]) {
	// Check if frame exists
	if (structBufferPtrArg->headFramePtr == NULL) {
		errorCatcher(ec_no_frame_exists);
		return -1;
	}

	// Temporary store payload
	uint8_t arrayTemp[9] = {0};
	arrayTemp[0] = structBufferPtrArg->headFramePtr->identifier;
	memcpy(arrayTemp + 1, structBufferPtrArg->headFramePtr->unionPayload.uint8, 8);

	// Check CRC
	// Do not return on bad CRC
	if (structBufferPtrArg->headFramePtr->crcValue == GETCRC(arrayTemp)) {
		structBufferPtrArg->headFramePtr->verified = true;
	} else {
		errorCatcher(ec_bad_crc);
	}

	// Transfer to output argument
	memcpy(arrayArg, arrayTemp, 9);
	arrayArg[9] = structBufferPtrArg->headFramePtr->crcValue >> 8;
	arrayArg[10] = structBufferPtrArg->headFramePtr->crcValue;

	// Free frame
	bufferRemoveFrame(structBufferPtrArg);
	return 0;
}

/**
 * @brief Puts Payloads and CRC values from the first frame corresponding to given identifierArg to appointed array
 * @param[in] identifierArg ID of targeted
 * @param[in] *structBufferPtrArg Ptr to the structBuffer instance
 * @param[out] arrayArg[] Ptr to array to retrieve values to
 * @retval 0 on success, -1 on failure
 * @note - Equipped with errorCatcher()
 * @note - arrayArg[] should at least be uint8_t arrayArg[10]
 */
int8_t bufferGetArraySeek(struct structBuffer* structBufferPtrArg, uint8_t identifierArg, uint8_t arrayArg[]) {
	// Check if frame exists
	if (structBufferPtrArg->headFramePtr == NULL) {
		errorCatcher(ec_no_frame_exists);
		return -1;
	}

	// Create moving ptr and move to target frame
	struct structFrame* movingFramePtr = structBufferPtrArg->headFramePtr;
	struct structFrame* previousMovingFramePtr = NULL;
	while (movingFramePtr->identifier != identifierArg) {
		if (movingFramePtr->nextFramePtr == NULL) {
			errorCatcher(ec_no_frame_exists_requested);
			return -1;
		} else {
			previousMovingFramePtr = movingFramePtr;
			movingFramePtr = movingFramePtr->nextFramePtr;
		}
	}

	// Temporary store payload
	uint8_t arrayTemp[9] = {0};
	arrayTemp[0] = movingFramePtr->identifier;
	memcpy(arrayTemp + 1, movingFramePtr->unionPayload.uint8, 8);

	// Check CRC
	// Do not return on bad CRC
	if (movingFramePtr->crcValue == GETCRC(arrayTemp)) {
		movingFramePtr->verified = true;
	} else {
		errorCatcher(ec_bad_crc);
	}

	// Transfer to output argument
	memcpy(arrayArg, arrayTemp, 9);
	arrayArg[9] = structBufferPtrArg->headFramePtr->crcValue >> 8;
	arrayArg[10] = structBufferPtrArg->headFramePtr->crcValue;

	// Free frame
	// When single frame in buffer, reset ptrs to NULL
	if (structBufferPtrArg->tailFramePtr == structBufferPtrArg->headFramePtr) {
		structBufferPtrArg->bufferSizeCurrent--;
		free(structBufferPtrArg->headFramePtr);
		structBufferPtrArg->headFramePtr = NULL;
		structBufferPtrArg->tailFramePtr = NULL;

		// When multiple frames in buffer and ptr on head, move head ptr
	} else if (movingFramePtr == structBufferPtrArg->headFramePtr) {
		struct structFrame* previousHeadFramePtr = structBufferPtrArg->headFramePtr;
		structBufferPtrArg->headFramePtr = structBufferPtrArg->headFramePtr->nextFramePtr;
		structBufferPtrArg->bufferSizeCurrent--;
		free(previousHeadFramePtr);

		// When multiple frames in buffer and ptr not on head, bridge ptrs
	} else {
		previousMovingFramePtr->nextFramePtr = movingFramePtr->nextFramePtr;
		structBufferPtrArg->bufferSizeCurrent--;
		free(movingFramePtr);
	}

	return 0;
}
