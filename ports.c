#include "ports.h"
#include "utils.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>





bool getbit(uint16_t *bmp, uint16_t bitno) {
    uint16_t byteno, bitpos, mask;
    bool bit;
    
    byteno = (bitno/16); // byte number
    bitpos = (bitno%16); // bit position in the byte

    mask = (1 << bitpos);
    bit = ((bmp[byteno] & mask)>> bitpos) ? true: false;

    return bit;
}
void setbit(uint16_t *bmp,uint16_t bitno) {
    uint16_t byteno, bitpos, mask;


    byteno = (bitno/16);
    bitpos = (bitno%16);

    mask = (1<<bitpos); // new byte to OR it to the wanted byte 

    bmp[byteno] |= mask;
}
void set_ran(uint16_t *bmp, uint16_t min, uint16_t max) {
    uint16_t byteno, bitpos, mask;
    
    for (uint16_t i = min; i < max + 1; ++i) {
            byteno = (i/16);
            bitpos = (i%16);

            mask = (1<<bitpos);
            bmp[byteno] |= mask;
    }
}

bitmap *mkbmp(uint16_t bits) {
    uint16_t bytes;
    bitmap *p;

    bytes = (bits/8) + 1;
        
    p = malloc((size_t)bytes);
    if(p == NULL) {
        return (bitmap *)0;
    }
    memset(p, 0, (size_t)bytes);
    
    return p;
}

int main() {
    bitmap *p;
    uint16_t n,m;

    scanf("%hu %hu", &n, &m);
    p = mkbmp(MAX_PORT);

    if(p == 0) {
        fprintf(stderr, "mkbmp failed allocating mem(malloc failure).\n");
        return -1;
    }

    //setbit(p, b);
    set_ran(p, n, m);

    for(uint16_t i = n-10; i < m+10; ++i) {
        printf("bit %d --> %d\n", i, getbit(p, i));
    }
    free(p);
    p = NULL;
    return 0;
}
