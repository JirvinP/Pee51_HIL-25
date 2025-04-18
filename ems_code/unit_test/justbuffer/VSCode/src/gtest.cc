#include "gtest/gtest.h"
#include <stdint.h>

extern "C" {
#include "spiQueue.h"
#include "spiQueueEvil.h"
}

extern uint8_t errorVal;

// CPP OVERLOAD -------------------------------------------------------------------------------------------------

int8_t spiQueuePost(struct structSpiQueue* structSpiQueuePtrArg, uint8_t identifierArg, int32_t payloadValueArg) {
	return spiQueuePostInt(structSpiQueuePtrArg, identifierArg, payloadValueArg);
}

int8_t spiQueuePost(struct structSpiQueue* structSpiQueuePtrArg, uint8_t identifierArg, int64_t payloadValueArg) {
	return spiQueuePostInt(structSpiQueuePtrArg, identifierArg, payloadValueArg);
}

int8_t spiQueuePost(struct structSpiQueue* structSpiQueuePtrArg, uint8_t identifierArg, uint32_t payloadValueArg) {
	return spiQueuePostInt(structSpiQueuePtrArg, identifierArg, payloadValueArg);
}

int8_t spiQueuePost(struct structSpiQueue* structSpiQueuePtrArg, uint8_t identifierArg, uint64_t payloadValueArg) {
	return spiQueuePostInt(structSpiQueuePtrArg, identifierArg, payloadValueArg);
}

int8_t spiQueuePost(struct structSpiQueue* structSpiQueuePtrArg, uint8_t identifierArg, double payloadValueArg) {
	return spiQueuePostFrac(structSpiQueuePtrArg, identifierArg, payloadValueArg);
}

// TESTS ---------------------------------------------------------------------------------------------------------

// CRC ----------------------------------------------------------------------------------------------------------------------

class errorTest : public ::testing::Test {
  protected:
	errorTest() {
		errorVal = 0x00;
	}
};

TEST_F(errorTest, errorCatcher) {
	RecordProperty("description_1", "test if an error code gets globally stored");
	ASSERT_EQ(ec_no_error, 0x00);
	ASSERT_EQ(errorVal, ec_no_error);
	errorCatcher(0x01);
	ASSERT_EQ(errorVal, 0x01);
	errorCatcher(ec_sq_already_exist);
	ASSERT_EQ(errorVal, ec_sq_already_exist);
}

TEST_F(errorTest, errorReset) {
	RecordProperty("description_1", "test if an error code can be reset");
	errorCatcher(0x01);
	ASSERT_EQ(errorVal, 0x01);
	errorReset();
	ASSERT_EQ(errorVal, ec_no_error);
}

// CRC ----------------------------------------------------------------------------------------------------------------------

class crcTest : public ::testing::Test {
  protected:
	crcTest() {
		memset(&crcData, 0, sizeof(crcData));
		errorReset();
	}
};

TEST_F(crcTest, crcInit_normal) {
	RecordProperty("description_1", "crc inits without error on CCITT 16 ZERO");
	crcData.config.bitLength = 16;
	crcData.config.polynomial = X(12) + X(5) + X(0);
	crcData.config.initialValue = 0x0000;
	crcData.config.finalXorValue = 0x0000;
	crcData.config.inputReflected = false;
	crcData.config.resultReflected = false;
	ASSERT_EQ(crcInit(&crcData), 0);
	ASSERT_EQ(errorVal, ec_no_error);
}

TEST_F(crcTest, crcInit_bitlength_bad) {
	RecordProperty("description_1", "crc init fails on bad crc length");
	crcData.config.bitLength = 1;
	crcData.config.polynomial = X(12) + X(5) + X(0);
	crcData.config.initialValue = 0x0000;
	crcData.config.finalXorValue = 0x0000;
	crcData.config.inputReflected = false;
	crcData.config.resultReflected = false;
	ASSERT_EQ(crcInit(&crcData), -1);
	ASSERT_EQ(errorVal, ec_crc_length_bad);
}

TEST_F(crcTest, crcInit_polynomial_zero) {
	RecordProperty("description_1", "crc inits fails on non set / zero polynomial");
	crcData.config.bitLength = 16;
	crcData.config.polynomial = 0;
	crcData.config.initialValue = 0x0000;
	crcData.config.finalXorValue = 0x0000;
	crcData.config.inputReflected = false;
	crcData.config.resultReflected = false;
	ASSERT_EQ(crcInit(&crcData), -1);
	ASSERT_EQ(errorVal, ec_crc_polynomial_zero);
}

TEST_F(crcTest, crcInit_polynomial_oversized) {
	RecordProperty("description_1", "crc inits fails on oversized polynomial");
	crcData.config.bitLength = 16;
	crcData.config.polynomial = X(16);
	crcData.config.initialValue = 0x0000;
	crcData.config.finalXorValue = 0x0000;
	crcData.config.inputReflected = false;
	crcData.config.resultReflected = false;
	ASSERT_EQ(crcInit(&crcData), -1);
	ASSERT_EQ(errorVal, ec_crc_polynomial_oversized);
}

TEST_F(crcTest, crcInit_initvalue_oversized) {
	RecordProperty("description_1", "crc inits fails on oversized initial value");
	crcData.config.bitLength = 16;
	crcData.config.polynomial = X(12) + X(5) + X(0);
	crcData.config.initialValue = 0xFFFF + 1;
	crcData.config.finalXorValue = 0x0000;
	crcData.config.inputReflected = false;
	crcData.config.resultReflected = false;
	ASSERT_EQ(crcInit(&crcData), -1);
	ASSERT_EQ(errorVal, ec_crc_initvalue_oversized);
}

TEST_F(crcTest, crcInit_finalxor_oversized) {
	RecordProperty("description_1", "crc inits fails on oversized finalxor value");
	crcData.config.bitLength = 16;
	crcData.config.polynomial = X(12) + X(5) + X(0);
	crcData.config.initialValue = 0x0000;
	crcData.config.finalXorValue = 0xFFFF + 1;
	crcData.config.inputReflected = false;
	crcData.config.resultReflected = false;
	ASSERT_EQ(crcInit(&crcData), -1);
	ASSERT_EQ(errorVal, ec_crc_finalxor_oversized);
}

TEST_F(crcTest, crcCalcSlow_8bit_random1) {
	RecordProperty("description_1", "crc8 calculation with randomized parameters");
	crcData.config.bitLength = 8;
	// crcData.config.polynomial = 0xF3
	crcData.config.polynomial = X(7) + X(6) + X(5) + X(4) + X(1) + X(0);
	crcData.config.initialValue = 0x14;
	crcData.config.finalXorValue = 0x67;
	crcData.config.inputReflected = true;
	crcData.config.resultReflected = true;
	ASSERT_EQ(crcInit(&crcData), 0);
	uint8_t raw[SQ_PACKET_SIZE] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D};
	ASSERT_EQ(crcCalcSlow(&crcData, raw, SQ_FRAME_SIZE), 0xEC);
	ASSERT_EQ(errorVal, ec_no_error);
}

TEST_F(crcTest, crcCalcSlow_8bit_random2) {
	RecordProperty("description_1", "crc8 calculation with randomized parameters");
	crcData.config.bitLength = 8;
	// crcData.config.polynomial = 0xF4
	crcData.config.polynomial = X(7) + X(6) + X(5) + X(4) + X(2);
	crcData.config.initialValue = 0x15;
	crcData.config.finalXorValue = 0x68;
	crcData.config.inputReflected = false;
	crcData.config.resultReflected = false;
	ASSERT_EQ(crcInit(&crcData), 0);
	uint8_t raw[SQ_PACKET_SIZE] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D};
	ASSERT_EQ(crcCalcSlow(&crcData, raw, SQ_FRAME_SIZE), 0xE4);
	ASSERT_EQ(errorVal, ec_no_error);
}

TEST_F(crcTest, crcCalcSlow_16bit_random1) {
	RecordProperty("description_1", "crc16 calculation with randomized parameters");
	crcData.config.bitLength = 16;
	// crcData.config.polynomial = 0x1234
	crcData.config.polynomial = X(12) + X(9) + X(5) + X(4) + X(2);
	crcData.config.initialValue = 0xABCD;
	crcData.config.finalXorValue = 0x1234;
	crcData.config.inputReflected = true;
	crcData.config.resultReflected = false;
	ASSERT_EQ(crcInit(&crcData), 0);
	uint8_t raw[SQ_PACKET_SIZE] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D};
	ASSERT_EQ(crcCalcSlow(&crcData, raw, SQ_FRAME_SIZE), 0x409C);
	ASSERT_EQ(errorVal, ec_no_error);
}

TEST_F(crcTest, crcCalcSlow_16bit_random2) {
	RecordProperty("description_1", "crc16 calculation with randomized parameters");
	crcData.config.bitLength = 16;
	// crcData.config.polynomial = 0xF345
	crcData.config.polynomial = X(15) + X(14) + X(13) + X(12) + X(9) + X(8) + X(6) + X(2) + X(0);
	crcData.config.initialValue = 0x1468;
	crcData.config.finalXorValue = 0x6745;
	crcData.config.inputReflected = true;
	crcData.config.resultReflected = true;
	ASSERT_EQ(crcInit(&crcData), 0);
	uint8_t raw[SQ_PACKET_SIZE] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D};
	ASSERT_EQ(crcCalcSlow(&crcData, raw, SQ_FRAME_SIZE), 0x9AE6);
	ASSERT_EQ(errorVal, ec_no_error);
}

TEST_F(crcTest, crcCalcSlow_32bit_random1) {
	RecordProperty("description_1", "crc32 calculation with randomized parameters");
	crcData.config.bitLength = 32;
	// crcData.config.polynomial = 0xABCD
	crcData.config.polynomial = X(15) + X(13) + X(11) + X(9) + X(8) + X(7) + X(6) + X(3) + X(2) + X(0);
	crcData.config.initialValue = 0x14AC32EF;
	crcData.config.finalXorValue = 0x67432681;
	crcData.config.inputReflected = false;
	crcData.config.resultReflected = false;
	ASSERT_EQ(crcInit(&crcData), 0);
	uint8_t raw[SQ_PACKET_SIZE] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D};
	ASSERT_EQ(crcCalcSlow(&crcData, raw, SQ_FRAME_SIZE), 0xF7F8DD7C);
	ASSERT_EQ(errorVal, ec_no_error);
}

TEST_F(crcTest, crcCalcSlow_32bit_random2) {
	RecordProperty("description_1", "crc32 calculation with randomized parameters");
	crcData.config.bitLength = 32;
	// crcData.config.polynomial = 0x12345678
	crcData.config.polynomial = X(28) + X(25) + X(21) + X(20) + X(18) + X(14) + X(12) + X(10) + X(9) + X(6) + X(5) + X(4) + X(3);
	crcData.config.initialValue = 0x3456789A;
	crcData.config.finalXorValue = 0xABCDEF12;
	crcData.config.inputReflected = true;
	crcData.config.resultReflected = true;
	ASSERT_EQ(crcInit(&crcData), 0);
	uint8_t raw[SQ_PACKET_SIZE] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D};
	ASSERT_EQ(crcCalcSlow(&crcData, raw, SQ_FRAME_SIZE), 0xA94B68A2);
	ASSERT_EQ(errorVal, ec_no_error);
}

TEST_F(crcTest, crcCalcFast_8bit_mad_loop) {
	RecordProperty("description_1", "crcCalcFast vs crcCalcSlow looped");
	uint8_t raw[SQ_PACKET_SIZE] = {0};
	crcData.config.bitLength = 8;
	for (uint8_t loop = 0; loop < 100; loop++) {
		for (uint8_t arrayIndex = 0; arrayIndex < SQ_FRAME_SIZE; arrayIndex++) {
			raw[arrayIndex] = rand() % (UINT8_MAX + 1);
		}
		crcData.config.polynomial = rand() % UINT8_MAX + 1;
		crcData.config.initialValue = rand() % (UINT8_MAX + 1);
		crcData.config.finalXorValue = rand() % (UINT8_MAX + 1);
		crcData.config.inputReflected = rand() % (true + 1);
		crcData.config.resultReflected = rand() % (true + 1);
		ASSERT_EQ(crcInit(&crcData), 0);
		ASSERT_EQ(crcCalcSlow(&crcData, raw, SQ_FRAME_SIZE), crcCalcFast(&crcData, raw, SQ_FRAME_SIZE));
		ASSERT_EQ(errorVal, ec_no_error);
	}
}

TEST_F(crcTest, crcCalcFast_16bit_mad_loop) {
	RecordProperty("description_1", "crcCalcFast vs crcCalcSlow looped");
	uint8_t raw[SQ_PACKET_SIZE] = {0};
	crcData.config.bitLength = 16;
	for (uint8_t loop = 0; loop < 100; loop++) {
		for (uint8_t arrayIndex = 0; arrayIndex < SQ_FRAME_SIZE; arrayIndex++) {
			raw[arrayIndex] = rand() % (UINT8_MAX + 1);
		}
		crcData.config.polynomial = rand() % UINT16_MAX + 1;
		crcData.config.initialValue = rand() % (UINT16_MAX + 1);
		crcData.config.finalXorValue = rand() % (UINT16_MAX + 1);
		crcData.config.inputReflected = rand() % (true + 1);
		crcData.config.resultReflected = rand() % (true + 1);
		ASSERT_EQ(crcInit(&crcData), 0);
		ASSERT_EQ(crcCalcSlow(&crcData, raw, SQ_FRAME_SIZE), crcCalcFast(&crcData, raw, SQ_FRAME_SIZE));
		ASSERT_EQ(errorVal, ec_no_error);
	}
}

TEST_F(crcTest, crcCalcFast_32bit_mad_loop) {
	RecordProperty("description_1", "crcCalcFast vs crcCalcSlow looped");
	uint8_t raw[SQ_PACKET_SIZE] = {0};
	crcData.config.bitLength = 32;
	for (uint8_t loop = 0; loop < 100; loop++) {
		for (uint8_t arrayIndex = 0; arrayIndex < SQ_FRAME_SIZE; arrayIndex++) {
			raw[arrayIndex] = rand() % (UINT8_MAX + 1);
		}
		crcData.config.polynomial = rand() % UINT32_MAX + 1;
		crcData.config.initialValue = rand() % ((uint64_t)UINT32_MAX + 1);
		crcData.config.finalXorValue = rand() % ((uint64_t)UINT32_MAX + 1);
		crcData.config.inputReflected = rand() % (true + 1);
		crcData.config.resultReflected = rand() % (true + 1);
		ASSERT_EQ(crcInit(&crcData), 0);
		ASSERT_EQ(crcCalcSlow(&crcData, raw, SQ_FRAME_SIZE), crcCalcFast(&crcData, raw, SQ_FRAME_SIZE));
		ASSERT_EQ(errorVal, ec_no_error);
	}
}

// SPIQUEUE -----------------------------------------------------------------------------------------------------------------

class spiQueueTest : public ::testing::Test {
  protected:
	spiQueueTest() {
		crcData.config.bitLength = 16;
		crcData.config.polynomial = X(12) + X(5) + X(0);
		crcData.config.initialValue = 0x0;
		crcData.config.finalXorValue = 0x0;
		crcData.config.inputReflected = false;
		crcData.config.resultReflected = false;
		crcInit(&crcData);
		errorReset();
	}
};

TEST_F(spiQueueTest, spiQueueCreate) {
	RecordProperty("description_1", "Test creation of empty queue");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), 0);
	ASSERT_EQ(structSpiQueueReceive->sizeCurrent, 0);
	ASSERT_EQ(structSpiQueueReceive->sizeMax, 10);
	ASSERT_TRUE(structSpiQueueReceive->tailPacketPtr == NULL);
	ASSERT_TRUE(structSpiQueueReceive->headPacketPtr == NULL);
	free(structSpiQueueReceive);
	ASSERT_EQ(errorVal, ec_no_error);
}

TEST_F(spiQueueTest, spiQueueCreate_malloc_fail) {
	RecordProperty("description_1", "Test creation of empty queue");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreateEvil(&structSpiQueueReceive, 10), -1);
	ASSERT_EQ(errorVal, ec_sq_malloc_failed);
}

TEST_F(spiQueueTest, spiQueueCreate_twice) {
	RecordProperty("description_1", "Test double creation of empty queue");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), 0);
	ASSERT_EQ(errorVal, ec_no_error);
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), -1);
	ASSERT_EQ(errorVal, ec_sq_already_exist);
	free(structSpiQueueReceive);
}

TEST_F(spiQueueTest, spiQueueRemove) {
	RecordProperty("description_1", "Test removal of empty queue");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), 0);
	ASSERT_EQ(spiQueueRemove(&structSpiQueueReceive), 0);
	ASSERT_EQ(errorVal, ec_no_error);
}

TEST_F(spiQueueTest, spiQueueRemove_twice) {
	RecordProperty("description_1", "Test double removal of empty queue");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), 0);
	ASSERT_EQ(spiQueueRemove(&structSpiQueueReceive), 0);
	ASSERT_EQ(errorVal, ec_no_error);
	ASSERT_EQ(spiQueueRemove(&structSpiQueueReceive), -1);
	ASSERT_EQ(errorVal, ec_sq_doesnt_exist);
}

TEST_F(spiQueueTest, spiQueuePostArray_fill) {
	RecordProperty("description_1", "Test appending raw data to queue");
	RecordProperty("description_2", "Test queue size counter");
	RecordProperty("description_2", "Test identifier fill");
	RecordProperty("description_3", "Test payload fill");
	RecordProperty("description_4", "Test ack fill");
	RecordProperty("description_5", "Test crc fill");
	RecordProperty("description_6", "Test removal of single frame");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), 0);
	ASSERT_EQ(structSpiQueueReceive->sizeCurrent, 0);
	ASSERT_EQ(structSpiQueueReceive->sizeMax, 10);
	ASSERT_TRUE(structSpiQueueReceive->tailPacketPtr == NULL);
	ASSERT_TRUE(structSpiQueueReceive->headPacketPtr == NULL);
	uint8_t raw[SQ_PACKET_SIZE] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D};
	ASSERT_EQ(spiQueuePostArray(structSpiQueueReceive, raw, arraysize(raw), false), 0);
	ASSERT_EQ(structSpiQueueReceive->sizeCurrent, 1);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr, structSpiQueueReceive->tailPacketPtr);
	ASSERT_TRUE(structSpiQueueReceive->headPacketPtr->nextPacketPtr == NULL);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->identifier, 0x01);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[0], 0x02);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[1], 0x03);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[2], 0x04);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[3], 0x05);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[4], 0x06);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[5], 0x07);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[6], 0x08);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[7], 0x09);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint32, 0x05040302);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->ack.retrieved, false);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->ack.returnCrc.uint8[0], 0x0A);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->ack.returnCrc.uint8[1], 0x0B);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->ack.returnCrc.uint16, 0x0B0A);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->crc.verified, false);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->crc.good, false);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->crc.value.uint8[0], 0x0C);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->crc.value.uint8[1], 0x0D);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->crc.value.uint16, 0x0D0C);
	ASSERT_EQ(spiQueuePacketRemove(structSpiQueueReceive), 0);
	ASSERT_EQ(structSpiQueueReceive->sizeCurrent, 0);
	ASSERT_TRUE(structSpiQueueReceive->tailPacketPtr == NULL);
	ASSERT_TRUE(structSpiQueueReceive->headPacketPtr == NULL);
	ASSERT_EQ(spiQueueRemove(&structSpiQueueReceive), 0);
	ASSERT_EQ(errorVal, ec_no_error);
}

TEST_F(spiQueueTest, spiQueuePostArray_malloc_fail) {
	RecordProperty("description_1", "Force a malloc failure on spiQueuePostArray");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), 0);
	uint8_t raw[SQ_PACKET_SIZE] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D};
	ASSERT_EQ(spiQueuePostArrayEvil(structSpiQueueReceive, raw, arraysize(raw), false), -1);
	ASSERT_EQ(spiQueueRemove(&structSpiQueueReceive), 0);
	ASSERT_EQ(errorVal, ec_sq_packet_malloc_failed);
}

TEST_F(spiQueueTest, spiQueuePostArray_crc) {
	RecordProperty("description_1", "Test crc verification using CRC16_CCITT_ZERO");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), 0);
	uint8_t raw[SQ_PACKET_SIZE] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x8A, 0xF2};
	ASSERT_EQ(spiQueuePostArray(structSpiQueueReceive, raw, arraysize(raw), true), 0);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->crc.verified, true);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->crc.good, true);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->crc.value.uint16, 0xF28A);
	ASSERT_EQ(spiQueuePacketRemove(structSpiQueueReceive), 0);
	ASSERT_EQ(spiQueueRemove(&structSpiQueueReceive), 0);
	ASSERT_EQ(errorVal, ec_no_error);
}

TEST_F(spiQueueTest, spiQueuePostArray_crc_fail) {
	RecordProperty("description_1", "Test crc verification failure using CRC16_CCITT_ZERO");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), 0);
	uint8_t raw[SQ_PACKET_SIZE] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x8A, 0x00};
	ASSERT_EQ(spiQueuePostArray(structSpiQueueReceive, raw, arraysize(raw), true), 0);

	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->crc.verified, true);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->crc.good, false);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->crc.value.uint16, 0x008A);
	ASSERT_EQ(spiQueuePacketRemove(structSpiQueueReceive), 0);
	ASSERT_EQ(spiQueueRemove(&structSpiQueueReceive), 0);
	ASSERT_EQ(errorVal, ec_no_error);
}

TEST_F(spiQueueTest, spiQueuePostArray_two_frames) {
	RecordProperty("description_1", "Test appending raw data to queue twice");
	RecordProperty("description_2", "Test removing head frame");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), 0);
	ASSERT_EQ(structSpiQueueReceive->sizeCurrent, 0);
	ASSERT_EQ(structSpiQueueReceive->sizeMax, 10);
	ASSERT_TRUE(structSpiQueueReceive->tailPacketPtr == NULL);
	ASSERT_TRUE(structSpiQueueReceive->headPacketPtr == NULL);
	uint8_t raw[SQ_PACKET_SIZE] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D};
	ASSERT_EQ(spiQueuePostArray(structSpiQueueReceive, raw, arraysize(raw), false), 0);
	uint8_t raw2[SQ_PACKET_SIZE] = {0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D};
	ASSERT_EQ(spiQueuePostArray(structSpiQueueReceive, raw2, arraysize(raw2), false), 0);
	ASSERT_EQ(structSpiQueueReceive->sizeCurrent, 2);
	ASSERT_NE(structSpiQueueReceive->headPacketPtr, structSpiQueueReceive->tailPacketPtr);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->nextPacketPtr, structSpiQueueReceive->tailPacketPtr);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->identifier, 0x01);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[0], 0x02);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[1], 0x03);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[2], 0x04);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[3], 0x05);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[4], 0x06);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[5], 0x07);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[6], 0x08);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[7], 0x09);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint32, 0x05040302);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->ack.retrieved, false);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->ack.returnCrc.uint8[0], 0x0A);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->ack.returnCrc.uint8[1], 0x0B);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->ack.returnCrc.uint16, 0x0B0A);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->crc.verified, false);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->crc.good, false);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->crc.value.uint8[0], 0x0C);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->crc.value.uint8[1], 0x0D);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->crc.value.uint16, 0x0D0C);
	ASSERT_EQ(structSpiQueueReceive->tailPacketPtr->identifier, 0x11);
	ASSERT_EQ(structSpiQueueReceive->tailPacketPtr->payload.uint8[0], 0x12);
	ASSERT_EQ(structSpiQueueReceive->tailPacketPtr->payload.uint8[1], 0x13);
	ASSERT_EQ(structSpiQueueReceive->tailPacketPtr->payload.uint8[2], 0x14);
	ASSERT_EQ(structSpiQueueReceive->tailPacketPtr->payload.uint8[3], 0x15);
	ASSERT_EQ(structSpiQueueReceive->tailPacketPtr->payload.uint8[4], 0x16);
	ASSERT_EQ(structSpiQueueReceive->tailPacketPtr->payload.uint8[5], 0x17);
	ASSERT_EQ(structSpiQueueReceive->tailPacketPtr->payload.uint8[6], 0x18);
	ASSERT_EQ(structSpiQueueReceive->tailPacketPtr->payload.uint8[7], 0x19);
	ASSERT_EQ(structSpiQueueReceive->tailPacketPtr->payload.uint32, 0x15141312);
	ASSERT_EQ(structSpiQueueReceive->tailPacketPtr->ack.retrieved, false);
	ASSERT_EQ(structSpiQueueReceive->tailPacketPtr->ack.returnCrc.uint8[0], 0x1A);
	ASSERT_EQ(structSpiQueueReceive->tailPacketPtr->ack.returnCrc.uint8[1], 0x1B);
	ASSERT_EQ(structSpiQueueReceive->tailPacketPtr->ack.returnCrc.uint16, 0x1B1A);
	ASSERT_EQ(structSpiQueueReceive->tailPacketPtr->crc.verified, false);
	ASSERT_EQ(structSpiQueueReceive->tailPacketPtr->crc.good, false);
	ASSERT_EQ(structSpiQueueReceive->tailPacketPtr->crc.value.uint8[0], 0x1C);
	ASSERT_EQ(structSpiQueueReceive->tailPacketPtr->crc.value.uint8[1], 0x1D);
	ASSERT_EQ(structSpiQueueReceive->tailPacketPtr->crc.value.uint16, 0x1D1C);
	ASSERT_EQ(spiQueuePacketRemove(structSpiQueueReceive), 0);
	ASSERT_EQ(structSpiQueueReceive->sizeCurrent, 1);
	ASSERT_TRUE(structSpiQueueReceive->headPacketPtr == structSpiQueueReceive->tailPacketPtr);
	ASSERT_TRUE(structSpiQueueReceive->headPacketPtr->nextPacketPtr == NULL);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->identifier, 0x11);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[0], 0x12);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[1], 0x13);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[2], 0x14);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[3], 0x15);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[4], 0x16);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[5], 0x17);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[6], 0x18);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[7], 0x19);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint32, 0x15141312);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->ack.retrieved, false);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->ack.returnCrc.uint8[0], 0x1A);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->ack.returnCrc.uint8[1], 0x1B);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->ack.returnCrc.uint16, 0x1B1A);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->crc.verified, false);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->crc.good, false);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->crc.value.uint8[0], 0x1C);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->crc.value.uint8[1], 0x1D);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->crc.value.uint16, 0x1D1C);
	ASSERT_EQ(spiQueuePacketRemove(structSpiQueueReceive), 0);
	ASSERT_EQ(structSpiQueueReceive->sizeCurrent, 0);
	ASSERT_TRUE(structSpiQueueReceive->headPacketPtr == NULL);
	ASSERT_TRUE(structSpiQueueReceive->headPacketPtr == structSpiQueueReceive->tailPacketPtr);
	ASSERT_EQ(spiQueueRemove(&structSpiQueueReceive), 0);
	ASSERT_EQ(errorVal, ec_no_error);
}

TEST_F(spiQueueTest, spiQueuePostArray_three_frames) {
	RecordProperty("description_1", "Test appending raw data to queue three times");
	RecordProperty("description_2", "Test removing head frame twice");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), 0);
	ASSERT_EQ(structSpiQueueReceive->sizeCurrent, 0);
	ASSERT_EQ(structSpiQueueReceive->sizeMax, 10);
	ASSERT_TRUE(structSpiQueueReceive->tailPacketPtr == NULL);
	ASSERT_TRUE(structSpiQueueReceive->headPacketPtr == NULL);
	uint8_t raw[SQ_PACKET_SIZE] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D};
	ASSERT_EQ(spiQueuePostArray(structSpiQueueReceive, raw, arraysize(raw), false), 0);
	uint8_t raw2[SQ_PACKET_SIZE] = {0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D};
	ASSERT_EQ(spiQueuePostArray(structSpiQueueReceive, raw2, arraysize(raw2), false), 0);
	uint8_t raw3[SQ_PACKET_SIZE] = {0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D};
	ASSERT_EQ(spiQueuePostArray(structSpiQueueReceive, raw3, arraysize(raw3), false), 0);
	ASSERT_EQ(structSpiQueueReceive->sizeCurrent, 3);
	ASSERT_NE(structSpiQueueReceive->headPacketPtr, structSpiQueueReceive->tailPacketPtr);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->nextPacketPtr->nextPacketPtr, structSpiQueueReceive->tailPacketPtr);
	ASSERT_TRUE(structSpiQueueReceive->headPacketPtr->nextPacketPtr->nextPacketPtr->nextPacketPtr == NULL);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->identifier, 0x01);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[0], 0x02);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[1], 0x03);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[2], 0x04);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[3], 0x05);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[4], 0x06);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[5], 0x07);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[6], 0x08);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[7], 0x09);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint32, 0x05040302);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->ack.retrieved, false);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->ack.returnCrc.uint8[0], 0x0A);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->ack.returnCrc.uint8[1], 0x0B);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->ack.returnCrc.uint16, 0x0B0A);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->crc.verified, false);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->crc.good, false);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->crc.value.uint8[0], 0x0C);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->crc.value.uint8[1], 0x0D);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->crc.value.uint16, 0x0D0C);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->nextPacketPtr->identifier, 0x11);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->nextPacketPtr->payload.uint8[0], 0x12);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->nextPacketPtr->payload.uint8[1], 0x13);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->nextPacketPtr->payload.uint8[2], 0x14);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->nextPacketPtr->payload.uint8[3], 0x15);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->nextPacketPtr->payload.uint8[4], 0x16);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->nextPacketPtr->payload.uint8[5], 0x17);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->nextPacketPtr->payload.uint8[6], 0x18);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->nextPacketPtr->payload.uint8[7], 0x19);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->nextPacketPtr->payload.uint32, 0x15141312);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->nextPacketPtr->ack.retrieved, false);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->nextPacketPtr->ack.returnCrc.uint8[0], 0x1A);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->nextPacketPtr->ack.returnCrc.uint8[1], 0x1B);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->nextPacketPtr->ack.returnCrc.uint16, 0x1B1A);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->nextPacketPtr->crc.verified, false);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->nextPacketPtr->crc.good, false);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->nextPacketPtr->crc.value.uint8[0], 0x1C);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->nextPacketPtr->crc.value.uint8[1], 0x1D);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->nextPacketPtr->crc.value.uint16, 0x1D1C);
	ASSERT_EQ(structSpiQueueReceive->tailPacketPtr->identifier, 0x21);
	ASSERT_EQ(structSpiQueueReceive->tailPacketPtr->payload.uint8[0], 0x22);
	ASSERT_EQ(structSpiQueueReceive->tailPacketPtr->payload.uint8[1], 0x23);
	ASSERT_EQ(structSpiQueueReceive->tailPacketPtr->payload.uint8[2], 0x24);
	ASSERT_EQ(structSpiQueueReceive->tailPacketPtr->payload.uint8[3], 0x25);
	ASSERT_EQ(structSpiQueueReceive->tailPacketPtr->payload.uint8[4], 0x26);
	ASSERT_EQ(structSpiQueueReceive->tailPacketPtr->payload.uint8[5], 0x27);
	ASSERT_EQ(structSpiQueueReceive->tailPacketPtr->payload.uint8[6], 0x28);
	ASSERT_EQ(structSpiQueueReceive->tailPacketPtr->payload.uint8[7], 0x29);
	ASSERT_EQ(structSpiQueueReceive->tailPacketPtr->payload.uint32, 0x25242322);
	ASSERT_EQ(structSpiQueueReceive->tailPacketPtr->ack.retrieved, false);
	ASSERT_EQ(structSpiQueueReceive->tailPacketPtr->ack.returnCrc.uint8[0], 0x2A);
	ASSERT_EQ(structSpiQueueReceive->tailPacketPtr->ack.returnCrc.uint8[1], 0x2B);
	ASSERT_EQ(structSpiQueueReceive->tailPacketPtr->ack.returnCrc.uint16, 0x2B2A);
	ASSERT_EQ(structSpiQueueReceive->tailPacketPtr->crc.verified, false);
	ASSERT_EQ(structSpiQueueReceive->tailPacketPtr->crc.good, false);
	ASSERT_EQ(structSpiQueueReceive->tailPacketPtr->crc.value.uint8[0], 0x2C);
	ASSERT_EQ(structSpiQueueReceive->tailPacketPtr->crc.value.uint8[1], 0x2D);
	ASSERT_EQ(structSpiQueueReceive->tailPacketPtr->crc.value.uint16, 0x2D2C);
	ASSERT_EQ(spiQueuePacketRemove(structSpiQueueReceive), 0);
	ASSERT_EQ(structSpiQueueReceive->sizeCurrent, 2);
	ASSERT_NE(structSpiQueueReceive->headPacketPtr, structSpiQueueReceive->tailPacketPtr);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->nextPacketPtr, structSpiQueueReceive->tailPacketPtr);
	ASSERT_TRUE(structSpiQueueReceive->headPacketPtr->nextPacketPtr->nextPacketPtr == NULL);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->identifier, 0x11);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[0], 0x12);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[1], 0x13);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[2], 0x14);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[3], 0x15);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[4], 0x16);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[5], 0x17);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[6], 0x18);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[7], 0x19);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint32, 0x15141312);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->ack.retrieved, false);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->ack.returnCrc.uint8[0], 0x1A);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->ack.returnCrc.uint8[1], 0x1B);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->ack.returnCrc.uint16, 0x1B1A);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->crc.verified, false);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->crc.good, false);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->crc.value.uint8[0], 0x1C);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->crc.value.uint8[1], 0x1D);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->crc.value.uint16, 0x1D1C);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->nextPacketPtr->identifier, 0x21);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->nextPacketPtr->payload.uint8[0], 0x22);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->nextPacketPtr->payload.uint8[1], 0x23);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->nextPacketPtr->payload.uint8[2], 0x24);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->nextPacketPtr->payload.uint8[3], 0x25);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->nextPacketPtr->payload.uint8[4], 0x26);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->nextPacketPtr->payload.uint8[5], 0x27);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->nextPacketPtr->payload.uint8[6], 0x28);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->nextPacketPtr->payload.uint8[7], 0x29);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->nextPacketPtr->payload.uint32, 0x25242322);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->nextPacketPtr->ack.retrieved, false);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->nextPacketPtr->ack.returnCrc.uint8[0], 0x2A);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->nextPacketPtr->ack.returnCrc.uint8[1], 0x2B);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->nextPacketPtr->ack.returnCrc.uint16, 0x2B2A);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->nextPacketPtr->crc.verified, false);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->nextPacketPtr->crc.good, false);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->nextPacketPtr->crc.value.uint8[0], 0x2C);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->nextPacketPtr->crc.value.uint8[1], 0x2D);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->nextPacketPtr->crc.value.uint16, 0x2D2C);
	ASSERT_EQ(spiQueuePacketRemove(structSpiQueueReceive), 0);
	ASSERT_EQ(structSpiQueueReceive->sizeCurrent, 1);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr, structSpiQueueReceive->tailPacketPtr);
	ASSERT_TRUE(structSpiQueueReceive->headPacketPtr->nextPacketPtr == NULL);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->identifier, 0x21);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[0], 0x22);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[1], 0x23);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[2], 0x24);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[3], 0x25);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[4], 0x26);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[5], 0x27);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[6], 0x28);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[7], 0x29);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint32, 0x25242322);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->ack.retrieved, false);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->ack.returnCrc.uint8[0], 0x2A);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->ack.returnCrc.uint8[1], 0x2B);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->ack.returnCrc.uint16, 0x2B2A);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->crc.verified, false);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->crc.good, false);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->crc.value.uint8[0], 0x2C);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->crc.value.uint8[1], 0x2D);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->crc.value.uint16, 0x2D2C);
	ASSERT_EQ(spiQueuePacketRemove(structSpiQueueReceive), 0);
	ASSERT_EQ(structSpiQueueReceive->sizeCurrent, 0);
	ASSERT_EQ(spiQueueRemove(&structSpiQueueReceive), 0);
	ASSERT_EQ(errorVal, ec_no_error);
}

TEST_F(spiQueueTest, spiQueueRemove_single_frame) {
	RecordProperty("description_1", "Test removing queue with one frame");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), 0);
	uint8_t raw[SQ_PACKET_SIZE] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D};
	ASSERT_EQ(spiQueuePostArray(structSpiQueueReceive, raw, arraysize(raw), false), 0);
	ASSERT_EQ(spiQueueRemove(&structSpiQueueReceive), 0);
	ASSERT_EQ(errorVal, ec_no_error);
}

TEST_F(spiQueueTest, spiQueueRemove_double_frame) {
	RecordProperty("description_1", "Test removing queue with two frames");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), 0);
	uint8_t raw[SQ_PACKET_SIZE] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D};
	ASSERT_EQ(spiQueuePostArray(structSpiQueueReceive, raw, arraysize(raw), false), 0);
	uint8_t raw2[SQ_PACKET_SIZE] = {0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D};
	ASSERT_EQ(spiQueuePostArray(structSpiQueueReceive, raw2, arraysize(raw2), false), 0);
	ASSERT_EQ(spiQueueRemove(&structSpiQueueReceive), 0);
	ASSERT_EQ(errorVal, ec_no_error);
}

TEST_F(spiQueueTest, spiQueueRemove_three_frame) {
	RecordProperty("description_1", "Test removing queue with three frames");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), 0);
	uint8_t raw[SQ_PACKET_SIZE] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D};
	ASSERT_EQ(spiQueuePostArray(structSpiQueueReceive, raw, arraysize(raw), false), 0);
	uint8_t raw2[SQ_PACKET_SIZE] = {0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D};
	ASSERT_EQ(spiQueuePostArray(structSpiQueueReceive, raw2, arraysize(raw2), false), 0);
	uint8_t raw3[SQ_PACKET_SIZE] = {0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D};
	ASSERT_EQ(spiQueuePostArray(structSpiQueueReceive, raw3, arraysize(raw3), false), 0);
	ASSERT_EQ(spiQueueRemove(&structSpiQueueReceive), 0);
	ASSERT_EQ(errorVal, ec_no_error);
}

TEST_F(spiQueueTest, spiQueueGetArray) {
	RecordProperty("description_1", "Test returning frame as raw array");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), 0);
	uint8_t raw[SQ_PACKET_SIZE] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D};
	ASSERT_EQ(spiQueuePostArray(structSpiQueueReceive, raw, arraysize(raw), false), 0);
	uint8_t rawGet[SQ_PACKET_SIZE] = {0};
	ASSERT_EQ(spiQueueGetArray(structSpiQueueReceive, rawGet, arraysize(rawGet)), 0);
	ASSERT_EQ(memcmp(raw, rawGet, SQ_PACKET_SIZE), 0);
	ASSERT_EQ(spiQueueRemove(&structSpiQueueReceive), 0);
	ASSERT_EQ(errorVal, ec_no_error);
}

TEST_F(spiQueueTest, spiQueuePost_uint8_normal) {
	RecordProperty("description_1", "Test appending frame using ID and value [UINT8]");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), 0);
	ASSERT_EQ(spiQueuePost(structSpiQueueReceive, ID_TEST_UINT8, 0x12), 0);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->identifier, ID_TEST_UINT8);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[0], 0x12);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[1], 0x00);
	uint8_t rawGet[SQ_PACKET_SIZE] = {0};
	ASSERT_EQ(spiQueueGetArray(structSpiQueueReceive, rawGet, arraysize(rawGet)), 0);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->crc.value.uint16, crcCalcFast(&crcData, rawGet, 11));
	ASSERT_EQ(spiQueueRemove(&structSpiQueueReceive), 0);
	ASSERT_EQ(errorVal, ec_no_error);
}

TEST_F(spiQueueTest, spiQueuePost_uint8_min) {
	RecordProperty("description_1", "Test appending frame using ID and value [UINT8 MIN]");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), 0);
	ASSERT_EQ(spiQueuePost(structSpiQueueReceive, ID_TEST_UINT8, 0x00), 0);
	ASSERT_EQ(spiQueueRemove(&structSpiQueueReceive), 0);
	ASSERT_EQ(errorVal, ec_no_error);
}

TEST_F(spiQueueTest, spiQueuePost_uint8_under) {
	RecordProperty("description_1", "Test appending frame using ID and value [UINT8 UNDER]");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), 0);
	ASSERT_EQ(spiQueuePost(structSpiQueueReceive, ID_TEST_UINT8, 0x00 - 1), -1);
	ASSERT_EQ(spiQueueRemove(&structSpiQueueReceive), 0);
	ASSERT_EQ(errorVal, ec_sq_payload_out_of_range_uint8);
}

TEST_F(spiQueueTest, spiQueuePost_uint8_max) {
	RecordProperty("description_1", "Test appending frame using ID and value [UINT8 MAX]");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), 0);
	ASSERT_EQ(spiQueuePost(structSpiQueueReceive, ID_TEST_UINT8, UINT8_MAX), 0);
	ASSERT_EQ(spiQueueRemove(&structSpiQueueReceive), 0);
	ASSERT_EQ(errorVal, ec_no_error);
}

TEST_F(spiQueueTest, spiQueuePost_uint8_over) {
	RecordProperty("description_1", "Test appending frame using ID and value [UINT8 OVER]");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), 0);
	ASSERT_EQ(spiQueuePost(structSpiQueueReceive, ID_TEST_UINT8, UINT8_MAX + 1), -1);
	ASSERT_EQ(spiQueueRemove(&structSpiQueueReceive), 0);
	ASSERT_EQ(errorVal, ec_sq_payload_out_of_range_uint8);
}

TEST_F(spiQueueTest, spiQueuePost_uint16_normal) {
	RecordProperty("description_1", "Test appending frame using ID and value [UINT16]");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), 0);
	ASSERT_EQ(spiQueuePost(structSpiQueueReceive, ID_TEST_UINT16, 0x1234), 0);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->identifier, ID_TEST_UINT16);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint16, 0x1234);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[0], 0x34);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[1], 0x12);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[2], 0x00);
	uint8_t rawGet[SQ_PACKET_SIZE] = {0};
	ASSERT_EQ(spiQueueGetArray(structSpiQueueReceive, rawGet, arraysize(rawGet)), 0);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->crc.value.uint16, crcCalcFast(&crcData, rawGet, 11));
	ASSERT_EQ(spiQueueRemove(&structSpiQueueReceive), 0);
	ASSERT_EQ(errorVal, ec_no_error);
}

TEST_F(spiQueueTest, spiQueuePost_uint16_min) {
	RecordProperty("description_1", "Test appending frame using ID and value [UINT16 MIN]");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), 0);
	ASSERT_EQ(spiQueuePost(structSpiQueueReceive, ID_TEST_UINT16, 0x0000), 0);
	ASSERT_EQ(spiQueueRemove(&structSpiQueueReceive), 0);
	ASSERT_EQ(errorVal, ec_no_error);
}

TEST_F(spiQueueTest, spiQueuePost_uint16_under) {
	RecordProperty("description_1", "Test appending frame using ID and value [UINT16 UNDER]");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), 0);
	ASSERT_EQ(spiQueuePost(structSpiQueueReceive, ID_TEST_UINT16, 0x0000 - 1), -1);
	ASSERT_EQ(spiQueueRemove(&structSpiQueueReceive), 0);
	ASSERT_EQ(errorVal, ec_sq_payload_out_of_range_uint16);
}

TEST_F(spiQueueTest, spiQueuePost_uint16_max) {
	RecordProperty("description_1", "Test appending frame using ID and value [UINT16 MAX]");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), 0);
	ASSERT_EQ(spiQueuePost(structSpiQueueReceive, ID_TEST_UINT16, UINT16_MAX), 0);
	ASSERT_EQ(spiQueueRemove(&structSpiQueueReceive), 0);
	ASSERT_EQ(errorVal, ec_no_error);
}

TEST_F(spiQueueTest, spiQueuePost_uint16_over) {
	RecordProperty("description_1", "Test appending frame using ID and value [UINT16 OVER]");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), 0);
	ASSERT_EQ(spiQueuePost(structSpiQueueReceive, ID_TEST_UINT16, UINT16_MAX + 1), -1);
	ASSERT_EQ(spiQueueRemove(&structSpiQueueReceive), 0);
	ASSERT_EQ(errorVal, ec_sq_payload_out_of_range_uint16);
}

TEST_F(spiQueueTest, spiQueuePost_uint32_normal) {
	RecordProperty("description_1", "Test appending frame using ID and value [UINT32]");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), 0);
	ASSERT_EQ(spiQueuePost(structSpiQueueReceive, ID_TEST_UINT32, 0x12345678), 0);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->identifier, ID_TEST_UINT32);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint32, 0x12345678);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[0], 0x78);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[1], 0x56);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[2], 0x34);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[3], 0x12);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[4], 0x00);
	uint8_t rawGet[SQ_PACKET_SIZE] = {0};
	ASSERT_EQ(spiQueueGetArray(structSpiQueueReceive, rawGet, arraysize(rawGet)), 0);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->crc.value.uint16, crcCalcFast(&crcData, rawGet, 11));
	ASSERT_EQ(spiQueueRemove(&structSpiQueueReceive), 0);
	ASSERT_EQ(errorVal, ec_no_error);
}

TEST_F(spiQueueTest, spiQueuePost_uint32_min) {
	RecordProperty("description_1", "Test appending frame using ID and value [UINT32 MIN]");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), 0);
	ASSERT_EQ(spiQueuePost(structSpiQueueReceive, ID_TEST_UINT32, 0x00000000), 0);
	ASSERT_EQ(spiQueueRemove(&structSpiQueueReceive), 0);
	ASSERT_EQ(errorVal, ec_no_error);
}

TEST_F(spiQueueTest, spiQueuePost_uint32_under) {
	RecordProperty("description_1", "Test appending frame using ID and value [UINT32 UNDER]");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), 0);
	ASSERT_EQ(spiQueuePost(structSpiQueueReceive, ID_TEST_UINT32, 0x00000000 - 1), -1);
	ASSERT_EQ(spiQueueRemove(&structSpiQueueReceive), 0);
	ASSERT_EQ(errorVal, ec_sq_payload_out_of_range_uint32);
}

TEST_F(spiQueueTest, spiQueuePost_uint32_max) {
	RecordProperty("description_1", "Test appending frame using ID and value [UINT32 MAX]");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), 0);
	ASSERT_EQ(spiQueuePost(structSpiQueueReceive, ID_TEST_UINT32, UINT32_MAX), 0);
	ASSERT_EQ(spiQueueRemove(&structSpiQueueReceive), 0);
	ASSERT_EQ(errorVal, ec_no_error);
}

TEST_F(spiQueueTest, spiQueuePost_uint32_over) {
	RecordProperty("description_1", "Test appending frame using ID and value [UINT32 OVER]");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), 0);
	ASSERT_EQ(spiQueuePost(structSpiQueueReceive, ID_TEST_UINT32, (uint64_t)(UINT32_MAX) + 1), -1);
	ASSERT_EQ(spiQueueRemove(&structSpiQueueReceive), 0);
	ASSERT_EQ(errorVal, ec_sq_payload_out_of_range_uint32);
}

TEST_F(spiQueueTest, spiQueuePost_sint8_normal) {
	RecordProperty("description_1", "Test appending frame using ID and value [SINT8]");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), 0);
	ASSERT_EQ(spiQueuePost(structSpiQueueReceive, ID_TEST_SINT8, -0x02), 0);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->identifier, ID_TEST_SINT8);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.sint8, -0x02);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[0], 0xFE);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[1], 0x00);
	uint8_t rawGet[SQ_PACKET_SIZE] = {0};
	ASSERT_EQ(spiQueueGetArray(structSpiQueueReceive, rawGet, arraysize(rawGet)), 0);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->crc.value.uint16, crcCalcFast(&crcData, rawGet, 11));
	ASSERT_EQ(spiQueueRemove(&structSpiQueueReceive), 0);
	ASSERT_EQ(errorVal, ec_no_error);
}

TEST_F(spiQueueTest, spiQueuePost_sint8_min) {
	RecordProperty("description_1", "Test appending frame using ID and value [SINT8 MIN]");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), 0);
	ASSERT_EQ(spiQueuePost(structSpiQueueReceive, ID_TEST_SINT8, INT8_MIN), 0);
	ASSERT_EQ(spiQueueRemove(&structSpiQueueReceive), 0);
	ASSERT_EQ(errorVal, ec_no_error);
}

TEST_F(spiQueueTest, spiQueuePost_sint8_under) {
	RecordProperty("description_1", "Test appending frame using ID and value [SINT8 UNDER]");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), 0);
	ASSERT_EQ(spiQueuePost(structSpiQueueReceive, ID_TEST_SINT8, INT8_MIN - 1), -1);
	ASSERT_EQ(spiQueueRemove(&structSpiQueueReceive), 0);
	ASSERT_EQ(errorVal, ec_sq_payload_out_of_range_sint8);
}

TEST_F(spiQueueTest, spiQueuePost_sint8_max) {
	RecordProperty("description_1", "Test appending frame using ID and value [SINT8 MAX]");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), 0);
	ASSERT_EQ(spiQueuePost(structSpiQueueReceive, ID_TEST_SINT8, INT8_MAX), 0);
	ASSERT_EQ(spiQueueRemove(&structSpiQueueReceive), 0);
	ASSERT_EQ(errorVal, ec_no_error);
}

TEST_F(spiQueueTest, spiQueuePost_sint8_over) {
	RecordProperty("description_1", "Test appending frame using ID and value [SINT8 OVER]");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), 0);
	ASSERT_EQ(spiQueuePost(structSpiQueueReceive, ID_TEST_SINT8, INT8_MAX + 1), -1);
	ASSERT_EQ(spiQueueRemove(&structSpiQueueReceive), 0);
	ASSERT_EQ(errorVal, ec_sq_payload_out_of_range_sint8);
}

TEST_F(spiQueueTest, spiQueuePost_sint16_normal) {
	RecordProperty("description_1", "Test appending frame using ID and value [SINT16]");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), 0);
	ASSERT_EQ(spiQueuePost(structSpiQueueReceive, ID_TEST_SINT16, -0x02), 0);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->identifier, ID_TEST_SINT16);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.sint16, -0x02);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[0], 0xFE);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[1], 0xFF);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[2], 0x00);
	uint8_t rawGet[SQ_PACKET_SIZE] = {0};
	ASSERT_EQ(spiQueueGetArray(structSpiQueueReceive, rawGet, arraysize(rawGet)), 0);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->crc.value.uint16, crcCalcFast(&crcData, rawGet, 11));
	ASSERT_EQ(spiQueueRemove(&structSpiQueueReceive), 0);
	ASSERT_EQ(errorVal, ec_no_error);
}

TEST_F(spiQueueTest, spiQueuePost_sint16_min) {
	RecordProperty("description_1", "Test appending frame using ID and value [SINT16 MIN]");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), 0);
	ASSERT_EQ(spiQueuePost(structSpiQueueReceive, ID_TEST_SINT16, INT16_MIN), 0);
	ASSERT_EQ(spiQueueRemove(&structSpiQueueReceive), 0);
	ASSERT_EQ(errorVal, ec_no_error);
}

TEST_F(spiQueueTest, spiQueuePost_sint16_under) {
	RecordProperty("description_1", "Test appending frame using ID and value [SINT16 UNDER]");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), 0);
	ASSERT_EQ(spiQueuePost(structSpiQueueReceive, ID_TEST_SINT16, INT16_MIN - 1), -1);
	ASSERT_EQ(spiQueueRemove(&structSpiQueueReceive), 0);
	ASSERT_EQ(errorVal, ec_sq_payload_out_of_range_sint16);
}

TEST_F(spiQueueTest, spiQueuePost_sint16_max) {
	RecordProperty("description_1", "Test appending frame using ID and value [SINT16 MAX]");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), 0);
	ASSERT_EQ(spiQueuePost(structSpiQueueReceive, ID_TEST_SINT16, INT16_MAX), 0);
	ASSERT_EQ(spiQueueRemove(&structSpiQueueReceive), 0);
	ASSERT_EQ(errorVal, ec_no_error);
}

TEST_F(spiQueueTest, spiQueuePost_sint16_over) {
	RecordProperty("description_1", "Test appending frame using ID and value [SINT16 OVER]");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), 0);
	ASSERT_EQ(spiQueuePost(structSpiQueueReceive, ID_TEST_SINT16, INT16_MAX + 1), -1);
	ASSERT_EQ(spiQueueRemove(&structSpiQueueReceive), 0);
	ASSERT_EQ(errorVal, ec_sq_payload_out_of_range_sint16);
}

TEST_F(spiQueueTest, spiQueuePost_sint32_normal) {
	RecordProperty("description_1", "Test appending frame using ID and value [SINT32]");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), 0);
	ASSERT_EQ(spiQueuePost(structSpiQueueReceive, ID_TEST_SINT32, -0x02), 0);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->identifier, ID_TEST_SINT32);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.sint32, -0x02);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[0], 0xFE);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[1], 0xFF);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[2], 0xFF);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[3], 0xFF);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[4], 0x00);
	uint8_t rawGet[SQ_PACKET_SIZE] = {0};
	ASSERT_EQ(spiQueueGetArray(structSpiQueueReceive, rawGet, arraysize(rawGet)), 0);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->crc.value.uint16, crcCalcFast(&crcData, rawGet, 11));
	ASSERT_EQ(spiQueueRemove(&structSpiQueueReceive), 0);
	ASSERT_EQ(errorVal, ec_no_error);
}

TEST_F(spiQueueTest, spiQueuePost_sint32_min) {
	RecordProperty("description_1", "Test appending frame using ID and value [SINT32 MIN]");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), 0);
	ASSERT_EQ(spiQueuePost(structSpiQueueReceive, ID_TEST_SINT32, INT32_MIN), 0);
	ASSERT_EQ(spiQueueRemove(&structSpiQueueReceive), 0);
	ASSERT_EQ(errorVal, ec_no_error);
}

TEST_F(spiQueueTest, spiQueuePost_sint32_under) {
	RecordProperty("description_1", "Test appending frame using ID and value [SINT32 UNDER]");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), 0);
	ASSERT_EQ(spiQueuePost(structSpiQueueReceive, ID_TEST_SINT32, (int64_t)(INT32_MIN)-1), -1);
	ASSERT_EQ(spiQueueRemove(&structSpiQueueReceive), 0);
	ASSERT_EQ(errorVal, ec_sq_payload_out_of_range_sint32);
}

TEST_F(spiQueueTest, spiQueuePost_sint32_max) {
	RecordProperty("description_1", "Test appending frame using ID and value [SINT32 MAX]");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), 0);
	ASSERT_EQ(spiQueuePost(structSpiQueueReceive, ID_TEST_SINT32, INT32_MAX), 0);
	ASSERT_EQ(spiQueueRemove(&structSpiQueueReceive), 0);
	ASSERT_EQ(errorVal, ec_no_error);
}

TEST_F(spiQueueTest, spiQueuePost_sint32_over) {
	RecordProperty("description_1", "Test appending frame using ID and value [SINT32 OVER]");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), 0);
	ASSERT_EQ(spiQueuePost(structSpiQueueReceive, ID_TEST_SINT32, (int64_t)(INT32_MAX) + 1), -1);
	ASSERT_EQ(spiQueueRemove(&structSpiQueueReceive), 0);
	ASSERT_EQ(errorVal, ec_sq_payload_out_of_range_sint32);
}

TEST_F(spiQueueTest, spiQueuePost_frac32_integer) {
	RecordProperty("description_1", "Test appending frame using ID and value [FRAC32]");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), 0);
	ASSERT_EQ(spiQueuePost(structSpiQueueReceive, ID_TEST_FRAC32, 0x0A), 0);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->identifier, ID_TEST_FRAC32);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.frac32, 0x0A);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[0], 0x00);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[1], 0x00);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[2], 0x20);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[3], 0x41);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[4], 0x00);
	uint8_t rawGet[SQ_PACKET_SIZE] = {0};
	ASSERT_EQ(spiQueueGetArray(structSpiQueueReceive, rawGet, arraysize(rawGet)), 0);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->crc.value.uint16, crcCalcFast(&crcData, rawGet, 11));
	ASSERT_EQ(spiQueueRemove(&structSpiQueueReceive), 0);
	ASSERT_EQ(errorVal, ec_no_error);
}

TEST_F(spiQueueTest, spiQueuePost_frac32_fractional) {
	RecordProperty("description_1", "Test appending frame using ID and value [FRAC32]");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), 0);
	ASSERT_EQ(spiQueuePost(structSpiQueueReceive, ID_TEST_FRAC32, 0.123), 0);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->identifier, ID_TEST_FRAC32);
	ASSERT_EQ((uint8_t)(structSpiQueueReceive->headPacketPtr->payload.frac32 * 1000), (uint8_t)((float)0.123 * 1000));
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[0], 0x6D);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[1], 0xE7);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[2], 0xFB);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[3], 0x3D);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[4], 0x00);
	uint8_t rawGet[SQ_PACKET_SIZE] = {0};
	ASSERT_EQ(spiQueueGetArray(structSpiQueueReceive, rawGet, arraysize(rawGet)), 0);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->crc.value.uint16, crcCalcFast(&crcData, rawGet, 11));
	ASSERT_EQ(spiQueueRemove(&structSpiQueueReceive), 0);
	ASSERT_EQ(errorVal, ec_no_error);
}

TEST_F(spiQueueTest, spiQueuePost_frac64_integer) {
	RecordProperty("description_1", "Test appending frame using ID and value [FRAC64]");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), 0);
	ASSERT_EQ(spiQueuePost(structSpiQueueReceive, ID_TEST_FRAC64, 0x0A), 0);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->identifier, ID_TEST_FRAC64);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.frac64, 0x0A);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[0], 0x00);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[1], 0x00);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[2], 0x00);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[3], 0x00);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[4], 0x00);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[5], 0x00);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[6], 0x24);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[7], 0x40);
	uint8_t rawGet[SQ_PACKET_SIZE] = {0};
	ASSERT_EQ(spiQueueGetArray(structSpiQueueReceive, rawGet, arraysize(rawGet)), 0);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->crc.value.uint16, crcCalcFast(&crcData, rawGet, 11));
	ASSERT_EQ(spiQueueRemove(&structSpiQueueReceive), 0);
	ASSERT_EQ(errorVal, ec_no_error);
}

TEST_F(spiQueueTest, spiQueuePost_frac64_fractional) {
	RecordProperty("description_1", "Test appending frame using ID and value [FRAC64]");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), 0);
	ASSERT_EQ(spiQueuePost(structSpiQueueReceive, ID_TEST_FRAC64, 0.123), 0);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->identifier, ID_TEST_FRAC64);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.frac64, 0.123);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[0], 0xB0);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[1], 0x72);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[2], 0x68);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[3], 0x91);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[4], 0xED);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[5], 0x7C);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[6], 0xBF);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[7], 0x3F);
	uint8_t rawGet[SQ_PACKET_SIZE] = {0};
	ASSERT_EQ(spiQueueGetArray(structSpiQueueReceive, rawGet, arraysize(rawGet)), 0);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->crc.value.uint16, crcCalcFast(&crcData, rawGet, 11));
	ASSERT_EQ(spiQueueRemove(&structSpiQueueReceive), 0);
	ASSERT_EQ(errorVal, ec_no_error);
}

TEST_F(spiQueueTest, spiQueuePost_bool_false) {
	RecordProperty("description_1", "Test appending frame using ID and value [BOOL]");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), 0);
	ASSERT_EQ(spiQueuePost(structSpiQueueReceive, ID_TEST_BINARY, false), 0);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->identifier, ID_TEST_BINARY);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.binary, false);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[0], 0x00);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[1], 0x00);
	uint8_t rawGet[SQ_PACKET_SIZE] = {0};
	ASSERT_EQ(spiQueueGetArray(structSpiQueueReceive, rawGet, arraysize(rawGet)), 0);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->crc.value.uint16, crcCalcFast(&crcData, rawGet, 11));
	ASSERT_EQ(spiQueueRemove(&structSpiQueueReceive), 0);
	ASSERT_EQ(errorVal, ec_no_error);
}

TEST_F(spiQueueTest, spiQueuePost_bool_true) {
	RecordProperty("description_1", "Test appending frame using ID and value [BOOL]");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), 0);
	ASSERT_EQ(spiQueuePost(structSpiQueueReceive, ID_TEST_BINARY, true), 0);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->identifier, ID_TEST_BINARY);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.binary, true);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[0], 0x01);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->payload.uint8[1], 0x00);
	uint8_t rawGet[SQ_PACKET_SIZE] = {0};
	ASSERT_EQ(spiQueueGetArray(structSpiQueueReceive, rawGet, arraysize(rawGet)), 0);
	ASSERT_EQ(structSpiQueueReceive->headPacketPtr->crc.value.uint16, crcCalcFast(&crcData, rawGet, 11));
	ASSERT_EQ(spiQueueRemove(&structSpiQueueReceive), 0);
	ASSERT_EQ(errorVal, ec_no_error);
}

TEST_F(spiQueueTest, spiQueueProcessAck_placeholder) {
	RecordProperty("description_1", "placeholder");
	struct structSpiQueue* structSpiQueueReceive = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueReceive, 10), 0);
	struct structSpiQueue* structSpiQueueTransmit = NULL;
	ASSERT_EQ(spiQueueCreate(&structSpiQueueTransmit, 10), 0);
	ASSERT_EQ(spiQueueProcessAck(structSpiQueueTransmit, structSpiQueueReceive, false), -1);
	ASSERT_EQ(spiQueueRemove(&structSpiQueueReceive), 0);
	ASSERT_EQ(spiQueueRemove(&structSpiQueueTransmit), 0);
	ASSERT_EQ(errorVal, ec_sq_not_implemented);
}

TEST_F(spiQueueTest, spiQueueNoDuplicate) {
	RecordProperty("description_1", "test spiQueueNoDuplicate by varying two inputs");
	bool noDuplicate = false;
	uint8_t raw[SQ_PACKET_SIZE] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D};
	uint8_t raw2[SQ_PACKET_SIZE] = {0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D};
	ASSERT_EQ(spiQueueNoDuplicate(&noDuplicate, raw, SQ_PACKET_SIZE), 0);
	ASSERT_EQ(noDuplicate, true);
	ASSERT_EQ(spiQueueNoDuplicate(&noDuplicate, raw, SQ_PACKET_SIZE), 0);
	ASSERT_EQ(noDuplicate, false);
	ASSERT_EQ(spiQueueNoDuplicate(&noDuplicate, raw, SQ_PACKET_SIZE), 0);
	ASSERT_EQ(noDuplicate, false);
	ASSERT_EQ(spiQueueNoDuplicate(&noDuplicate, raw2, SQ_PACKET_SIZE), 0);
	ASSERT_EQ(noDuplicate, true);
	ASSERT_EQ(spiQueueNoDuplicate(&noDuplicate, raw2, SQ_PACKET_SIZE), 0);
	ASSERT_EQ(noDuplicate, false);
	ASSERT_EQ(spiQueueNoDuplicate(&noDuplicate, raw, SQ_PACKET_SIZE), 0);
	ASSERT_EQ(noDuplicate, true);
	ASSERT_EQ(spiQueueNoDuplicate(&noDuplicate, raw2, SQ_PACKET_SIZE), 0);
	ASSERT_EQ(noDuplicate, true);
	ASSERT_EQ(errorVal, ec_no_error);
}

TEST_F(spiQueueTest, spiQueueNoDuplicate_bad_length) {
	RecordProperty("description_1", "test spiQueueNoDuplicate for incorrect input length");
	bool noDuplicate = false;
	uint8_t raw[SQ_PACKET_SIZE] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D};
	ASSERT_EQ(spiQueueNoDuplicate(&noDuplicate, raw, SQ_PACKET_SIZE + 1), -1);
	ASSERT_EQ(errorVal, ec_sq_incorrect_array_length);
}

// MAIN ---------------------------------------------------------------------------------------------------------------------

/** Main function calling gtest */
int main(int argc, char* argv[]) {
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}