/**
 * @file spiQueue.c
 * @author Sefa Ozturk (S.H.Ozturk@outlook.com)
 * @brief queue for use with SPI
 * @version 0.5
 * @date 2025-01-28
 */
#include "spiQueue.h"
#include "spiQueueEvil.h"

/**
 * @brief evil cousin of spiQueueCreate
 * @param[in] structSpiQueuePtrArg double pointer to the spiqueue pointer
 * @param[in] sizeMaxArg maximum amount of packets the spiqueue may hold
 * @retval 0 on success, -1 on failure
 * @note - equipped with errorCatcher()
 */
int8_t spiQueueCreateEvil(struct structSpiQueue** structSpiQueuePtrArg, uint8_t sizeMaxArg) {
	// check if spiqueue already exists
	if (*structSpiQueuePtrArg != NULL) {
		errorCatcher(ec_sq_already_exist);
		return -1;
	}
	// malloc new spiqueue
	struct structSpiQueue* newStructSpiQueue = EVILMALLOC(sizeof(struct structSpiQueue));
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
 * @brief evil cousin of spiQueuePacketAppend
 * @param[in] arrayArg 	[0]: predefined id recorded by the lexicon used to distinguish variables as they turn abstracted while in spi transfer.
 *  					[1-8]: unpacked datatype bytes making up a variable value
 * 						[9-10]: ack value
 * 						[11-12]: crc value
 * @retval pointer to the new packet
 * @note - equipped with errorcatcher()
 */
static struct structPacket* spiQueuePacketAppendEvil(uint8_t arrayArg[]) {
	// malloc new packet
	struct structPacket* newPacket = EVILMALLOC(sizeof(struct structPacket));
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
 * @brief evil cousin of spiQueuePostArray
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

int8_t spiQueuePostArrayEvil(struct structSpiQueue* structSpiQueuePtrArg, uint8_t arrayArg[], uint8_t arraySizeArg, bool crcCheckArg) {
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
		structSpiQueuePtrArg->headPacketPtr = spiQueuePacketAppendEvil(arrayArg);
		if (structSpiQueuePtrArg->headPacketPtr == NULL) {
			// the failure to malloc a packet is already caught in spiqueuepacketappendevil
			return -1;
		}
		structSpiQueuePtrArg->sizeCurrent++;
		structSpiQueuePtrArg->tailPacketPtr = structSpiQueuePtrArg->headPacketPtr;
		// append packet into non empty spiqueue and increment packet count if spiqueuepacketappend succeeds
		// append packet to to the tail and move tail pointer
	} else {
		structSpiQueuePtrArg->tailPacketPtr->nextPacketPtr = spiQueuePacketAppendEvil(arrayArg);
		if (structSpiQueuePtrArg->tailPacketPtr->nextPacketPtr == NULL) {
			// the failure to malloc a packet is already caught in spiqueuepacketappendevil
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