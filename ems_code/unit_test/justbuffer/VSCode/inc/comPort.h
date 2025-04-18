#ifndef COMPORT_H
#define COMPORT_H

#include <stdint.h>
#include <stdio.h>
#include <windows.h>

int8_t comInit(uint8_t comPortArg, uint32_t comBaudRateArg);
int64_t comTransmit(const char charArrayArg[], uint32_t bytesArg);
int64_t comReceive(char charArrayArg[], uint32_t bytesArg);

#endif