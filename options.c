#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include "options.h"
#include "utils.h"



int  port_ran(const char *arg, struct config *cfg)
{
    if(arg == NULL) {
        // default port range if nothing -p or --port has 0 arguments passed
        fprintf(stderr, "port_ran: arg passed empty.\nSetting port range to [20,80].\n");
        cfg->port_min = 20;
        cfg->port_max = 80;
        return 0;
    } else {
        if(!isthere_digits(arg)) {
            fprintf(stderr, "You did not pass a valid port numbers '%s' (expected N or N-M, e.g. 80 or 20-80).\n",arg);
            return -1; 
        }
    }
   
    
    long p_max,p_min;
    char *str = strdup(arg);
    char *tokens[2];
    char *token;
    char **endptr = 0;
    int  i = 0;
    if(str == NULL) {
        perror("perror");
        return -1;
    }


    token = strtok(str, TOK_DELIM);
    while(token != NULL && i < 2) {
        tokens[i++] = token;    
        token = strtok(NULL, TOK_DELIM);
    }
    
    /* checking bounds of tokens */
    if(i > 2) {
        fprintf(stderr, "too many port numbers.\nUsage: -p or --port int,int\n");
        free(str);
        str = NULL;
        return -1;
    }

    /* checking if the tokens are valid to pass them to strtol() */
    for(int j = 0;j < i; ++j) {
        if(strtol(tokens[j], endptr, 10) == LONG_MAX ||
           strtol(tokens[j], endptr, 10) == LONG_MIN ||   
           strtol(tokens[j], endptr, 10) == 0      ) {
            free(str);
            str = NULL;
            return -1;
        }
    }
    if(i == 2){
        p_min = strtol(tokens[1], endptr, 10);
        p_max = strtol(tokens[0], endptr, 10);
        
        if(p_min < MIN_PORT) {      // Checking bounds
            p_min = MIN_PORT;
        } else if(p_max > MAX_PORT){
            p_max = MAX_PORT;
        }
    } else if(i == 1){
        if((p_min = p_max = strtol(tokens[0], endptr, 10)) == 0) {
            fprintf(stderr, "port_ran: passed an invalid port range '%s' (expected N or N-M, e.g. 80 or 20-80).\n", arg);
            fprintf(stderr, "Usage: --help or -h for Usage.\n");
            free(str);
            str = NULL;
            return -1;
        }
        
        if(p_min < MIN_PORT) {
            p_min = p_max = MIN_PORT;
        } else if(p_max > MAX_PORT){
            p_min = p_max = MAX_PORT;
        } 
    } else { 
        // an error occurred
        free(str);
        str = NULL;
        return -1;
    }
    if(p_min > p_max) {
            long temp = p_min;
            p_min = p_max;
            p_max = temp;
    }
    cfg->port_min = p_min;
    cfg->port_max = p_max;
    free(str);
    str = NULL;
    return 0;
}

char  *check_opt(char *arg)
{
    char option[MAX_CAP];
    int j = 0;
    if(arg[1] == '-') {
        
        option[j++] = '-';
        for(int i = 2; isalpha(arg[i]) && j < MAX_CAP-1; ++i) {
            option[j++] = arg[i];
        }
        option[j] = '\0';
    
    } else if(isalpha(arg[1])) {
    
        for(int i = 1; isalpha(arg[i]) && j < MAX_CAP-1; ++i) {
            option[j++] = arg[i];
        }
        option[j] = '\0';
    
    } else {
        return NULL;
    }
    return strdup(option);
}
int help_opt(const char *arg, struct config *cfg) 
{
    (void)arg;
    (void)cfg;
    printf("\n\t\t Moscar's options:\n");
    for(int i = 0;opt_prop[i].description != NULL; ++i) {
        printf("%s", opt_prop[i].description);
    }
    return 1;
}
int exec_opt(char *option, char *arg, struct config *cofg) 
{
    if(option == NULL) {
        fprintf(stderr, "exec_opt: passed an empty string\n");
        free(option);
        return -1;
    }
    if(strlen(option) > 1){
       for(int i = 0;opt_prop[i].handler != NULL; ++i) {
           if(strcmp(option,opt_prop[i].long_name) == 0) 
           {
               free(option);
               option = NULL;
               return (opt_prop[i].takes_args) ? opt_prop[i].handler(arg, cofg) : 
                                                 opt_prop[i].handler("do not need args", cofg);
           }
       }
    } else{
        for(int i = 0;opt_prop[i].handler != NULL; ++i) {
  
           if(option[0] == opt_prop[i].short_name) 
           {
               free(option);
               option = NULL;
               return (opt_prop[i].takes_args) ? opt_prop[i].handler(arg, cofg) : 
                                                 opt_prop[i].handler("do not need args", cofg);
           }
       }
    }
       // if there isn't a match free the option
       free(option);
       option = NULL;
       fprintf(stderr, "Invalid option entered\n");
       fprintf(stderr,"for help: --help or -h.\n");
       return -1;
}
// defining options and their properties
option_entry opt_prop[] = {                    /* returns values meanings: 1 -> program should exit , -1 -> error detected program must exit, 0 -> success */

    {"-help", 'h', 0, &help_opt, "- Displaying options.\n. Usage: ./moscar [--help | -h]\n"},
    {"-port", 'p', 1, &port_ran, "- Setting the port range needed to scan.\n. Usage: ./moscar <ipv4> [--port | -p] [N | N,M | N-M]\n"},
    {NULL, 0, 0, NULL, NULL}

};
