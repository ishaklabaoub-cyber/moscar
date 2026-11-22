#ifndef  UTILS_H
#define UTILS_H

#include <assert.h>
#include <errno.h>
#include "ports.h"

#define TOK_DELIM " -"
#define LIS_DELIM " ,"
#define MAX_CAP      10000
#define MIN_PORT     1
#define MAX_PORT     65535 
#define MAX_TIMEOUT  1000 
#define MIN(X,Y) (((X) < (Y)) ? X : Y)
#define MAX(X,Y) (((X) > (Y)) ? X : Y)

struct config {
    bitmap bmp[MAX_PORT];       /* bitmap for setting-unsetting ports */
    uint16_t portm;           
    int  timeout;               /* for the option to set a timeout for a connection */
    int  port_list;             /* when this variable is set to 1 that means the ports are passed as a list */
    int  takes_args;            /* when this variable is set to 1 that means the option takes arguments*/
    int  jo;                    /* when this variable is set to 1 that means that the user includes the json option */
    char *input_addr;           /* the IP address given by the user */
    long  open_ports[MAX_PORT]; /* array for storing open ports after the scan */
};

enum prog_state {
    _ERR    = 01,               // error occured must terminate the program
    _EXIT   = 02,               // should exit the program due to an option like --help or -h(it should display exit)           
    _WAIT   = 04,               // program should wait
    _RESUME = 16                // program should continue executing 
};

int  is_valid_ipv4(const char*);                    // CHECK FOR A VALID IPv4 ADDRESS
int  isthere_digits(const char*);                   // CHECK IF THERE IS DIGITS IN A STRING

#endif
