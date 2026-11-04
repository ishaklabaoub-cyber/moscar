#include "ports.h"
#include "utils.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>





bool getport(uint16_t *bmp, uint16_t portno) {
    uint16_t byteno, portpos, mask;
    bool port;
    
    byteno = (portno/16); // byte number
    portpos = (portno%16); // port position in the byte

    mask = (1 << portpos);
    port = ((bmp[byteno] & mask)>> portpos) ? true: false;

    return port;
}
void setport(uint16_t *bmp,uint16_t portno) {
    uint16_t byteno, portpos, mask;


    byteno = (portno/16);
    portpos = (portno%16);

    mask = (1<<portpos); // new byte to OR it to the wanted byte 

    bmp[byteno] |= mask;
}

void unsetport(uint16_t *bmp, uint16_t portno) {
   uint16_t byteno, portpos, mask; 

   byteno = (portno/16);
   portpos = (portno/16);

   mask = ~(1<<portpos);

   bmp[byteno] &= mask;
}
void set_ran(uint16_t *bmp, uint16_t min, uint16_t max) {
    uint16_t byteno, portpos, mask;
    
    for (uint16_t i = min; i < max + 1; ++i) {
            byteno = (i/16);
            portpos = (i%16);

            mask = (1<<portpos);
            bmp[byteno] |= mask;
    }
}

void unset_ran(uint16_t *bmp, uint16_t min, uint16_t max) {
    uint16_t byteno, portpos, mask;

    for(uint16_t i = min; i < max+1; ++i) {
        byteno = (i/16);
        portpos = (i%16);

        mask = ~(1<<portpos);
        bmp[byteno] &= mask; 
    }
}

bitmap *mkbmp(uint16_t ports) {
    uint16_t bytes;
    bitmap *p;

    bytes = (ports/8) + 1;
        
    p = malloc((size_t)bytes);
    if(p == NULL) {
        return (bitmap *)0;
    }
    memset(p, 0, (size_t)bytes);
    
    return p;
}

int main() {
    bitmap *p;

    p = mkbmp(MAX_PORT);

    if(p == 0) {
        fprintf(stderr, "mkbmp failed allocating mem(malloc failure).\n");
        return -1;
    }

    //setport(p, b);
    set_ran(p, 15, 54);

    setport(p, 52);
    unset_ran(p, 20, 50);

    for(uint16_t i = 10; i < 60+1; ++i) {
        printf("port %d --> %d\n", i, getport(p, i));
    }
    free(p);
    p = NULL;
    return 0;
}
