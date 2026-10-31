#include "ports.h"




void zero(uint16_t *dest, uint16_t size) {
    uint16_t *p;
    uint16_t n;

    for(n = size,p = dest; n; --n,p++)
        *p = 0;


    return;
}
bool getbit(uint16_t *bmp, uint8_t bitno) {
    uint8_t byteno, bitpos, mask;
    bool bit;
    
    byteno = (bitno/8);
    bitpos = (bitno%8);

    mask = (1 << bitpos);
    bit = ((bmp[byteno] & mask)>> bitpos) ? true: false;

    return bit;
}
void setbit(uint16_t *bmp,uint8_t bitno) {
    uint8_t byteno, bitpos, mask;


    byteno = (bitno/8);
    bitpos = (bitno%8);

    mask = (1<<bitpos);

    bmp[byteno] |= mask;
}

bitmap *mkbmp(uint16_t bits) {
    uint16_t bytes;
    bitmap *p;

    bytes = (bits/8) + 1;
        
    p = malloc((size_t)bytes);
    if(p == NULL) {
        return (bitmap *)0;
    }
    zero(p, (uint16_t)bytes);
    
    return p;
}
// we want bit number 5
//            |           
// 00000000 00000000 
// mask =   00100000

int main() {
    bitmap *p;
    uint8_t mask;
    uint8_t byteno, bitpos;


    p = mkbmp(16);

    if(p == 0) {
        fprintf(stderr, "mkbmp failed allocating mem(malloc failure).\n");
        return -1;
    }

    setbit(p, 10);
    
     printf("bit %d is %d\n", 10, getbit(p, 1));

    free(p);
    p = NULL;
    return 0;
}
