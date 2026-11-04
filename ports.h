#ifndef PORTS_H
#define PORTS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include "utils.h"

typedef uint16_t bitmap;

bool getbit(uint16_t*,uint16_t);
void setbit(uint16_t*,uint16_t);
void unsetbit(uint16_t*, uint16_t);
void set_ran(uint16_t*, uint16_t, uint16_t);
void unset_ran(uint16_t*,uint16_t, uint16_t);
bitmap *mkbmp(uint16_t);

#endif
