#include "ports.h"
#include <stddef.h>
#include <stdint.h>
#include "utils.h"


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
    uint16_t byteno, portpos, mask,j = 1;

    if(max == MAX_PORT) {       /* if the user wants to scan 65330-65335 the loop will break because */
        setport(bmp, max);      /* i is a 16 bit integer , it cannot hold more than 65335 , i will get*/
        j = 0;                  /* to 0 and the loop will be infinit , if max == 65335 we set that port*/
    }                           /* and stop i before it resets to 0 */

        
    for (uint16_t i = min; i < max+j; ++i) {
            byteno = (i/16);
            portpos = (i%16);

            mask = (1<<portpos);
            bmp[byteno] |= mask;
    }
    return 0;
}

int unset_ran(uint16_t *bmp, uint16_t min, uint16_t max) {
    uint16_t byteno, portpos, mask, j = 1;
    if(max == MAX_PORT) {
        unsetport(bmp, max);
        j = 0;
    }
    
    for(uint16_t i = min; i < max+j; ++i) {
        byteno = (i/16);
        portpos = (i%16);

        mask = ~(1<<portpos);
        bmp[byteno] &= mask; 
    }
    return 0;
}
