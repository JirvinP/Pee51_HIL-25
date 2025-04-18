#include "MCP2210.h"

int8_t mcp2210Init(void) {
	fclose(fopen("MCP2210.txt", "w"));
	if (system("..\\..\\..\\..\\MCP2210\\MCP2210CLI.exe -connectI=0 > NUL\r\n")) {
		return -1;
	}
	return 0;
}

void mcp2210Transfer(uint8_t txArrayArg[], uint8_t rxArrayArg[], uint8_t bytesArg) {
	// build tx command
	char MCP2210_command[200];
	char MCP2210_spiTxfer[50] = {0};
	char MCP2210_cli[] = "..\\..\\..\\..\\MCP2210\\MCP2210CLI.exe";
	char MCP2210_suffix[] = " -bd=" STR(CLOCKSPEED) " -cs=gp0 -idle=1 -actv=0 -f=\"MCP2210.txt\" > NUL\r\n";
	char hexstring[4] = {0};
	char txArrayString[40] = {0};
	for (uint8_t byte = 0; byte < bytesArg; byte++) {
		if (byte == 0) {
			snprintf(hexstring, 4, "%02X,", txArrayArg[byte]);
			strcpy(txArrayString, hexstring);
		} else if (byte == bytesArg - 1) {
			snprintf(hexstring, 4, "%02X", txArrayArg[byte]);
			strcat(txArrayString, hexstring);
		} else {
			snprintf(hexstring, 4, "%02X,", txArrayArg[byte]);
			strcat(txArrayString, hexstring);
		}
	}
	snprintf(MCP2210_spiTxfer, 50, " -spiTxfer=\"%s\"", txArrayString);
	strcpy(MCP2210_command, MCP2210_cli);
	strcat(MCP2210_command, MCP2210_spiTxfer);
	strcat(MCP2210_command, MCP2210_suffix);
	system(MCP2210_command);

	// parse rx
	FILE* pFile;
	pFile = fopen("MCP2210.txt", "r");
	char line[100] = {0};
	uint8_t lineIndex = 0;
	if (pFile != NULL) {
		fgets(line, sizeof(line), pFile);
		fgets(line, sizeof(line), pFile);
		fclose(pFile);
	}
	char* charPtr;
	charPtr = strtok(line, " ");
	for (uint8_t byte = 0; byte < bytesArg; byte++) {
		charPtr = strtok(NULL, ",");
		rxArrayArg[byte] = strtoul(charPtr, NULL, 16);
	}
	fclose(fopen("MCP2210.txt", "w"));
}