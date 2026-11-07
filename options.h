#ifndef OPTIONS_H
#define OPTIONS_H

#include "utils.h"

typedef int (*opthandler)(const char *arg, struct config *cfg);

typedef struct {
    const char  *long_name;     // "help", "ports"
    const char  short_name;     // 'h', 'p'
    int         takes_args;     // 0, 1
    opthandler  handler;        // option function
    const char  *description;   // used to generate the help text
} option_entry;

extern option_entry opt_prop[];

int         exec_opt(char*, char*,struct config *);        // EXECUTING OPTIONS
char        *check_opt(char *);                            // RETURN THE OPTION CHARACTER
int         help_opt(const char *arg, struct config *cfg); // DISPLAYING OPTIONS
int         port_ran(const char *arg, struct config *cfg); // SETTING PORT RANGE
int         timeout(const  char *arg, struct config *cfg); // SETTING A TIMEOUT FOR A CONNECTION
                                                           
#endif
