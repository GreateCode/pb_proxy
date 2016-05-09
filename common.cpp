#include "common.h"

char *parse_to_hex(char *strhex, char *buff, int size){
	for(int i = 0; i < size; i++){
		sprintf(strhex + i * 5, "0x%02x ", (unsigned char)buff[i]);
	}
	return strhex;
}

