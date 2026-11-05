#ifndef PORTS_H
#define PORTS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>

typedef uint16_t bitmap;
#define $16 (uint16_t)

bool getport(uint16_t*,uint16_t);
int setport(uint16_t*,uint16_t);
int unsetport(uint16_t*, uint16_t);
int set_ran(uint16_t*, uint16_t, uint16_t);
int unset_ran(uint16_t*,uint16_t, uint16_t);

#endif
