#ifndef MCP2210_H
#define MCP2210_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CLOCKSPEED 1000000
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

int8_t mcp2210Init(void);
void mcp2210Transfer(uint8_t txArrayArg[], uint8_t rxArrayArg[], uint8_t bytesArg);

#endif