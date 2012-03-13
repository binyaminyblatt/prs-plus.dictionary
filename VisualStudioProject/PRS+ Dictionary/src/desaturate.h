#ifndef _MSC_VER
#include <stdint.h>
#else
#include "stdint.h"
#endif 

// Converts UTF-16 character to lower case, removing accents
uint16_t desaturate(uint16_t ch);