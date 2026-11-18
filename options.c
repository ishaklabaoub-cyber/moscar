#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include "options.h"
#include "ports.h"

// defining options and their properties
option_entry opt_prop[] = {                    /* returns values meanings: 1 -> program should exit , -1 -> error detected program must exit, 0 -> success */

    {"-help", 'h', 0, &help_opt, "- Displaying options.\n. Usage: ./moscar [--help | -h]\n"},
    {"-port", 'p', 1, &port_ran, "- Setting the port range needed to scan.\n. Usage: ./moscar <ipv4> [--port | -p] [N | N,M | N-M]\n"},
    {"-timeout", 't', 1, &timeout, "- Setting a timeout for a connection.\n. Usage: ./moscar <ipv4> --timeout <ms>\n"},   
    {"-version", 'v', 0, &version, "- Displaying the scanner's version.\nUsage: ./moscar -v || --version"},
    {"-json" , 'j', 0, &json_f, "- Provides a json output file contains a summary of the scan.\nUsage: ./moscar -j || --json"},
    {NULL, 0, 0, NULL, NULL}
};

// TODO: FIX the open_ports array (it is not taking values)
int json_f(const char *arg, struct config *cfg) {
    (void) arg;
    FILE *fp;


    fp = fopen("json.txt", "w");
    if(fp == NULL) {
        perror("fopen");
        return -1;
    }

    fprintf(fp, "{\n");
    if(cfg->input_addr == NULL){
        fprintf(fp, "   \"ip address\": No ip address is given\n");
    } else {
        fprintf(fp, "   \"ip address\": %s\n",cfg->input_addr);
    }
    fprintf(fp, "   \"scanned ports\": [");

    for(uint16_t i = 1;i != MAX_PORT && i != 0; ++i) {
        if(getport(cfg->bmp, i)) {
            fprintf(fp,"%d,",i);
        }
    }
    fprintf(fp, "]\n");
    fprintf(fp, "   \"open ports\": [");
    for(int i = 0;cfg->open_ports[i] != 0; ++i) {
        fprintf(fp, "%ld,",cfg->open_ports[i]);
    }
    fprintf(fp, "]\n");
    fprintf(fp, "}\n");
    fclose(fp);
    return 0;
}

int version(const char *arg, struct config *cfg) {
    (void) arg;
    (void) cfg;

    printf("moscar v0.2\n");
    return 1;
}
int timeout(const char *arg, struct config *cfg) {
    if(arg == NULL) {
        // default timeout is set if nothing gets passed
        cfg->timeout = 300;
        return 1;
    } else {
        if(!isthere_digits(arg)) {
            fprintf(stderr, "You did not pass a valid timeout number '%s' expected(<ms> e.g.: 300).\n",arg);
            return -1;
        }
    }

    char* str = strdup(arg);
    if(str == NULL) {
        perror("strdup");
        return -1;
    }
    char *endptr = NULL;
    int v;

    errno = 0;
    v = strtol(str, &endptr, 10);
    
    if(endptr[0] == '\0' && v < MAX_TIMEOUT && v > 0){ // valid timeout number
        cfg->timeout = v;
        free(str);
        str = NULL;
        return 0;
    } else {
        if(v >= MAX_TIMEOUT) {
            fprintf(stderr, "This timeout (%d) takes a lot of time to run a scan.\n-Timeout should be between 0 and 1000.\n",v);
        } else if(v < 0) {
            fprintf(stderr, "You passed a negative timeout (%d).\n",v);
        }else {
            fprintf(stderr, "You passed an Invalid timeout number(%d).\n",v);
        }
        free(str);
        str = NULL;
        return -1;
    }
}

int  port_ran(const char *arg, struct config *cfg)
{
    if(arg == NULL) {
        // default port range if nothing -p or --port has 0 arguments passed
        fprintf(stderr, "port_ran: arg passed empty.\nSetting port range to [20,80].\n");
        set_ran(cfg->bmp, $16 20, $16 80);
        return 0;
    } else {
        if(!isthere_digits(arg)) {
            fprintf(stderr, "You did not pass a valid port numbers '%s' (expected N or N-M or [N,N-M] as a list, e.g. 80 or 20-80 || 20,50-80).\n",arg);
            return -1; 
        }
    }
   
    
    long p_max, p_min, v;
    char *str = strdup(arg);
    char *tokens[2];
    long ts[2];
    char *token;
    char *endptr = NULL;
    int  i, j;
    if(str == NULL) {
        perror("strdup");
        return -1;
    }

    cfg->port_list = 0;
    for(i = 0;str[i] != '\0'; ++i) {
        if(str[i] == ','){
            cfg->port_list = 1;
            break;
        }
    }

    if(cfg->port_list == 1){
        i = 0; 
        char *list_tokens[MAX_CAP];

        /* tokenize with (,) delimiter first then separate single port number from ranges */
        token = strtok(str, LIS_DELIM);
        while(token != NULL && i < MAX_CAP-1) {
            list_tokens[i++] = token;   
            token = strtok(NULL, LIS_DELIM);    
        }
        list_tokens[i] = NULL;
        
        for(i = 0 ;list_tokens[i] != NULL ;++i){
            j = 0;
            v = strtol(list_tokens[i], &endptr, 10);
            
            if(endptr[0] == '\0') { // Valid single number 
              if(v > MAX_PORT || v <= 0){
                  fprintf(stderr, "You entered an Invalid port range or number(%ld).\n", v);
                  free(str);
                  str = NULL;
                  return -1;
              }
              setport(cfg->bmp,$16 v); 

            } else if(endptr[0] == '-') { // '-' indicates a range
                
                token = strtok(list_tokens[i], TOK_DELIM);
                while(token != NULL) {
                    // assigning port values while tokenizing
                    v = strtol(token, &endptr, 10);
                    if(endptr[0] == '\0' && v > 0) {
                        ts[j++] = v;
                    } else {
                        fprintf(stderr, "Invalid port range.\n");
                        free(str);
                        str = NULL;
                        return -1;
                    }
                   if(j == 2)break; 
                   token = strtok(NULL, TOK_DELIM);
                }
                if(j == 2){
                    if(MIN(ts[0],ts[1]) > MAX_PORT || MAX(ts[0],ts[1]) > MAX_PORT) {
                        fprintf(stderr, "You exceeded the maximum port range(%d).\n", MAX_PORT);
                        free(str);
                        return -1;
                    }
                    set_ran(cfg->bmp, $16 MIN(ts[0],ts[1]), $16 MAX(ts[0],ts[1]));
                    
                }else {
                    fprintf(stderr,"Invalid port range.\n");
                    fprintf(stderr, "You did not pass a valid port numbers '%s' (expected N or N-M or [N,N-M] as a list, e.g. 80 or 20-80 || 20,50-80).\n",arg);
                    free(str);
                    str = NULL;
                    return -1;
                }
            } else {
                fprintf(stderr, "Invalid port range or number.\n");
                free(str);
                str = NULL;
                return -1;
            }
        }
    } else if(cfg->port_list == 0){

        i = 0;
        token = strtok(str, TOK_DELIM);
        while(token != NULL && i < 2) {
            tokens[i++] = token;    
            token = strtok(NULL, TOK_DELIM);
        }
    
        /* checking bounds of tokens */
        if(i > 2) {
            fprintf(stderr, "too many port numbers.\nUsage: -p or --port int-int\n");
            free(str);
            str = NULL;
            return -1;
        }

        /* checking if the tokens are valid to pass them to strtol() */
        for(int j = 0;j < i; ++j) {
            if(strtol(tokens[j], &endptr, 10) == LONG_MAX ||
            strtol(tokens[j], &endptr, 10) == LONG_MIN ||   
            strtol(tokens[j], &endptr, 10) == 0      ) {
                free(str);
                str = NULL;
                return -1;
            }
        }
        if(i == 2){
            p_min = strtol(tokens[1], &endptr, 10);
            p_max = strtol(tokens[0], &endptr, 10);
    
            if(p_min < MIN_PORT) {      // Checking bounds
                p_min = MIN_PORT;
            } else if(p_max > MAX_PORT){
                p_max = MAX_PORT;
            }
        } else if(i == 1){
            if((p_min = p_max = strtol(tokens[0], &endptr, 10)) == 0) {
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
        if(p_min == p_max){
            setport(cfg->bmp, $16 p_min);
        }else {
            set_ran(cfg->bmp, $16 p_min, $16 p_max);
        }
    }
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
int exec_opt(char *option, char *arg, struct config *cfg) 
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
               if(strcmp(option,"-json") == 0){
                   cfg->jo = 1;
                   free(option);
                   option = NULL;
                   return 0;
               }
               free(option);
               option = NULL;
               return (opt_prop[i].takes_args) ? opt_prop[i].handler(arg, cfg) : 
                                                 opt_prop[i].handler("do not need args", cfg);
           }
       }
    } else{
        for(int i = 0;opt_prop[i].handler != NULL; ++i) {
  
           if(option[0] == opt_prop[i].short_name) 
           {
                if(strcmp(option,"-j") == 0){
                    cfg->jo = 1;
                    free(option);
                    option = NULL;
                    return 0;
               }
               free(option);
               option = NULL;
               return (opt_prop[i].takes_args) ? opt_prop[i].handler(arg, cfg) : 
                                                 opt_prop[i].handler("do not need args", cfg);
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
