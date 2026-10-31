#include "ports.h"





void zero(int16 *dest, int16 size) {
    int16 *p;
    int16 n;

    for(n = size,p = dest; n; --n,p++)
        *p = 0;


    return;
}


bitmap *mkbmp(int16 bits) {
    int16 bytes;
    bitmap *p;

    bytes = (bits/8) + 1;
        
    p = malloc((size_t)bytes);
    if(p == NULL) {
        return (bitmap *)0;
    }
    zero(p, (int16)bytes);
    
    return p;
}
