#ifndef PARSING_H
#define PARSING_H
#include "utils.h"

int         parsing_func(char *,struct config *, char **); // PARSING THE CL ARGS
int         parse_args(int argc, char* argv[], struct config *cofg);   // PARSING CL ARGUMENTS

#endif
