/**
 * @file spiQueue.h evil cousin
 * @author Sefa Ozturk (S.H.Ozturk@outlook.com)
 * @brief queue for use with SPI
 * @version 0.5
 * @date 2025-01-28
 */

#ifndef SPIQUEUEEVIL_H
#define SPIQUEUEEVIL_H

/** @brief fake malloc to force zero return */
#define EVILMALLOC(X) (NULL)

int8_t spiQueueCreateEvil(struct structSpiQueue** structSpiQueuePtrArg, uint8_t sizeMaxArg);
int8_t spiQueuePostArrayEvil(struct structSpiQueue* structSpiQueuePtrArg, uint8_t arrayArg[], uint8_t arraySizeArg, bool crcCheckArg);

#endif