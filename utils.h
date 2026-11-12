#ifndef  UTILS_H
#define UTILS_H

#include <assert.h>
#include "ports.h"

#define TOK_DELIM " -"
#define LIS_DELIM " ,"
#define MAX_CAP      6400
#define MIN_PORT     1
#define MAX_PORT     65535 
#define MAX_TIMEOUT  1000 
#define MIN(X,Y) (((X) < (Y)) ? X : Y)
#define MAX(X,Y) (((X) > (Y)) ? X : Y)

struct config {
    bitmap bmp[MAX_PORT];
    int  timeout;
    int  port_list;
    int  takes_args;
    char *input_addr;
    long port_max;
    long port_min;
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
