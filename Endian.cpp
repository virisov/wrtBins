#include <stdint.h>
#include "Endian.h"

// return 0 for big endian, 1 for little endian.

bool little_endian()
{
	volatile uint32_t i = 0x01234567;
	return (*((uint8_t*)(&i))) == 0x67;
};