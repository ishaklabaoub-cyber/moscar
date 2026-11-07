#include "utils.h"
#include "parsing.h"
#include "options.h"
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>



int parsing_func(char *argv, struct config *cofg, char **option)
{
    if(argv == NULL) {
        fprintf(stderr, "parsing_func: argv passed as NULL.\n");
        fprintf(stderr, "for help: --help or -h\n");
        return -1;
    }
    if(argv[0] == '-' && (isalpha(argv[1]) || argv[1] == '-')) {
            if((*option = check_opt(argv)) == NULL) {
                fprintf(stderr, "Invalid option.\n");
                // *option is already NULL does not need to be freed
                return -1;
            }
            for(int i = 0;opt_prop[i].long_name != NULL; ++i) {     // check the option if it takes arguments
                
                if(opt_prop[i].short_name == *option[0] ||
                   strcmp(opt_prop[i].long_name,*option) == 0) {

                   cofg->takes_args = opt_prop[i].takes_args; 
                }
            }
    } else{
        fprintf(stderr, "parsing_func: Invalid option entered.\n");
        return -1;
    }
    
    return 0;
}

int parse_args(int argc, char *argv[], struct config *cofg)
{
    int    func_ret,index;
    enum   prog_state state = _RESUME;
    char   *option = 0;
    
    index = -1;
    if (argc >= 2) {
        for(int j = 0;j < argc; ++j) {
            if((func_ret = is_valid_ipv4(argv[j])) != 1) {
                state |= _WAIT;
            } else if(func_ret == 1){
                cofg->input_addr = argv[j];
                state |= _RESUME;
                index = j;
                break;
            }
        }

        for(int i = 1;i < argc; ++i) {
            if(i == index)continue;
        
            if(parsing_func(argv[i], cofg, &option) == -1){
                // parsing failed
                state &= 0;
                state |= _ERR;
            } else {
                state |= _WAIT;
            }

            if((state & _ERR) == _ERR) {
                // error has occurred the program must exit
                return -1;
            } else if((state & _WAIT) == _WAIT) {
                /* program should wait for options:
                 * - State's of the program must be checked for error (_ERR) for options that needs to resume the connection or 
                 *   other functionalities.
                 * - For options that needs to exit the program like (--help or -h)
                 */

                /* if the option takes arguments pass the next CL argument */
                func_ret = (cofg->takes_args) ? exec_opt(option, argv[++i], cofg) : exec_opt(option, argv[i], cofg);   
                
                if(func_ret == 1) {
                    // the program needs to exit
                    state |= _EXIT;
                } else if(func_ret == -1) {
                     // error occurred the program must exit
                    state &=  0;
                    state |= _ERR;
                } else {
                    state |= _RESUME;
                }
            } else {
                // if never freed 
                free(option);
                option = NULL;
            }
            // Deciding what the program should do
            if((state & _ERR) == _ERR) {
                return -1;
            } else if ((state & _EXIT) == _EXIT) {
                return 1;
            } else {
                if (cofg->input_addr == NULL) {
                    fprintf(stderr, "no IP address is given.\n");
                    return -1;
                } else {
                    state &= 0;
                    state |= _RESUME;
                }
               
            }
            if((state & _RESUME) == _RESUME ) {
                  continue;
            }
        }
    } else{
        fprintf(stderr, "too few arguments.");
        printf("Usage: %s <ipaddr> ...\n", argv[0]);
        printf("For options: %s --help\n", argv[0]);
        return -1;
    }
	
	printf("%s is a valid IPv4 address.\n", cofg->input_addr);
    return 0;
}
