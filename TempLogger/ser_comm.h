#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ser_comm
#define _ser_comm

#include "types.h"

//Timeout for serial port message (ms)
#define TOUT 5 

extern u8 serlen;
void ReadSerial(u8 *buffptr, void (*fun()));

#endif

#ifdef __cplusplus
}
#endif
