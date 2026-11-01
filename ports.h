#ifndef PORTS_H
#define PORTS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include "utils.h"

typedef u_int16_t bitmap;

bool getbit(uint16_t*,uint8_t);
void setbit(uint16_t*,uint8_t);
bitmap *mkbmp(uint16_t);

#endif
