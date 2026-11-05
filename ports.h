#ifndef PORTS_H
#define PORTS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>

typedef uint16_t bitmap;

bool getport(uint16_t*,uint16_t);
void setport(uint16_t*,uint16_t);
void unsetport(uint16_t*, uint16_t);
void set_ran(uint16_t*, uint16_t, uint16_t);
void unset_ran(uint16_t*,uint16_t, uint16_t);
bitmap *mkbmp(uint16_t);

#endif
