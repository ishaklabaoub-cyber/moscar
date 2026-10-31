#ifndef PORTS_H
#define PORTS_H

#include <stdio.h>
#include <stdlib.h>

typedef unsigned short int16;
typedef unsigned char  int8;
typedef int16 bitmap;


void zero(int16*,int16);
bitmap *mkbmp(int16);

#endif
