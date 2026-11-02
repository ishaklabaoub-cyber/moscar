#include "ports.h"
#include "utils.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>





bool getbit(uint16_t *bmp, uint16_t bitno) {
    uint16_t byteno, bitpos, mask;
    bool bit;
    
    byteno = (bitno/16);
    bitpos = (bitno%16);

    mask = (1 << bitpos);
    bit = ((bmp[byteno] & mask)>> bitpos) ? true: false;

    return bit;
}
void setbit(uint16_t *bmp,uint16_t bitno) {
    uint16_t byteno, bitpos, mask;


    byteno = (bitno/16);
    bitpos = (bitno%16);

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
    memset(p, 0, (size_t)bytes);
    
    return p;
}

int main() {
    bitmap *p;
    uint16_t b;

    scanf("%hu", &b);
    p = mkbmp(MAX_PORT);

    if(p == 0) {
        fprintf(stderr, "mkbmp failed allocating mem(malloc failure).\n");
        return -1;
    }

    setbit(p, b);
    
    printf("bit %hu is %d\n", b, getbit(p, b));
    printf("bit %hu is %d\n", b+1, getbit(p, b+1));

    free(p);
    p = NULL;
    return 0;
}
