#include "comPort.h"

HANDLE comHandleFile;

int8_t comInit(uint8_t comPortArg, uint32_t comBaudRateArg) {
	DCB comConfig;
	BOOL retVal;
	char comPortString[6] = {0};
	snprintf(comPortString, 6, "COM%d", comPortArg);
	comHandleFile = CreateFile(comPortString, (GENERIC_READ | GENERIC_WRITE), 0, NULL, OPEN_EXISTING, 0, NULL);
	if (comHandleFile == INVALID_HANDLE_VALUE) {
		return -1;
	}
	SecureZeroMemory(&comConfig, sizeof(DCB));
	comConfig.DCBlength = sizeof(DCB);
	if (GetCommState(comHandleFile, &comConfig)) {
		return -1;
	}
	comConfig.BaudRate = comBaudRateArg;
	comConfig.ByteSize = 8;
	comConfig.Parity = NOPARITY;
	comConfig.StopBits = ONESTOPBIT;
	if (SetCommState(comHandleFile, &comConfig)) {
		return -1;
	}
	return 0;
}

int64_t comTransmit(const char charArrayArg[], uint32_t bytesArg) {
	uint64_t numberOfBytesWritten;
	if (WriteFile(comHandleFile, charArrayArg, bytesArg, (LPDWORD)&numberOfBytesWritten, NULL)) {
		printf("Error: Could not write serial port\r\n");
		CloseHandle(comHandleFile);
		return -1;
	}
	return numberOfBytesWritten;
}

int64_t comReceive(char charArrayArg[], uint32_t bytesArg) {
	uint64_t numberOfBytesRead;
	if (ReadFile(comHandleFile, &charArrayArg, bytesArg, (LPDWORD)&numberOfBytesRead, NULL)) {
		printf("Error: Could not read serial port\r\n");
		CloseHandle(comHandleFile);
		return -1;
	}
	return numberOfBytesRead;
}
