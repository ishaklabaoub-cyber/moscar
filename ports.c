#include "ports.h"
#include <stddef.h>
#include <stdint.h>



bool getport(uint16_t *bmp, uint16_t portno) {
    uint16_t byteno, portpos, mask;
    bool port;

    byteno = (portno/16); // byte number
    portpos = (portno%16); // port position in the byte

    mask = (1 << portpos);
    port = ((bmp[byteno] & mask)>> portpos) ? true: false;

    return port;
}
int setport(uint16_t *bmp,uint16_t portno) {
    uint16_t byteno, portpos, mask;
    

    byteno = (portno/16);
    portpos = (portno%16);

    mask = (1<<portpos); // new byte to OR it to the wanted byte 

    bmp[byteno] |= mask;
    return 0;
}

int unsetport(uint16_t *bmp, uint16_t portno) {
    uint16_t byteno, portpos, mask; 
    
    byteno = (portno/16);
    portpos = (portno%16);

    mask = ~(1<<portpos);

    bmp[byteno] &= mask;
    return 0;
}
int set_ran(uint16_t *bmp, uint16_t min, uint16_t max) {
    uint16_t byteno, portpos, mask;
    
    for (uint16_t i = min; i < max+1; ++i) {
            byteno = (i/16);
            portpos = (i%16);

            mask = (1<<portpos);
            bmp[byteno] |= mask;
    }
    return 0;
}

int unset_ran(uint16_t *bmp, uint16_t min, uint16_t max) {
    uint16_t byteno, portpos, mask;
    
    for(uint16_t i = min; i < max+1; ++i) {
        byteno = (i/16);
        portpos = (i%16);

        mask = ~(1<<portpos);
        bmp[byteno] &= mask; 
    }
    return 0;
}
