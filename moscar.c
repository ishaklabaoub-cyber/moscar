#include <netinet/in.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <sys/socket.h>
#include "scan.h"
#include "parsing.h"
#include "utils.h"

int main(int argc, char *argv[])
{
    int    start_f = 0;
    int    open_count, closed_count, filtered_count, unreachable_count, error_count;    
    long   ports_scanned, port;
    int    prev_err = 0;
    enum   port_state p_state;
    struct config cofg = {{0},{0}, 0, 0, NULL, 0L, 0L};
	int    socket_fd;				        /* socket file descriptor */
    struct timespec start_time, end_time;    /* for calculating proccess time */

    timespec_get(&start_time, TIME_UTC);

    if(parse_args(argc, argv, &cofg) != 0) {
        // exit the program
        return 1;
    }

    // initializing counters
    open_count        = 0;
    closed_count      = 0;
    filtered_count    = 0;
    unreachable_count = 0;
    error_count       = 0;
    ports_scanned     = 0L;

    
    if(cofg.ran_port[0] == 0 && cofg.ran_port[1] == 0 && cofg.s_port[0] == 0 && cofg.s_port[1] == 0) {
        // if an IP address passed and nothing after it
        fprintf(stderr, "No port range is set.\nSetting port range to [20-80].\n");
        cofg.ran_port[0] = 20;
        cofg.ran_port[1] = 80;
        cofg.list_port = 0;
    }

    for(int i = 0,j = 0; ; ++i){
	    if(cofg.list_port == 1) {
            if(cofg.s_port[i] != 0 && i < MAX_PORT) {
                port = cofg.s_port[i];
            }else {
                if(j > MAX_PORT){
                    fprintf(stderr, "main: index exceeds the array's range.\n");
                    return -1;
                }
                if(start_f == 0) {
                    port = cofg.ran_port[j];
                    start_f = 1;
                } else if(start_f == 1) {
                    if(port + 1 == cofg.ran_port[j + 1] + 1){
                        j++;
                        start_f = 0;
                    }
                    if(cofg.ran_port[j] == 0){
                        break;
                    }
                    if(start_f == 0){
                        continue;
                    }
                    port++;
                } else {
                    fprintf(stderr,"main: index exceeds the array's range.\n");
                    return -1;
                }
            }
        } else {
            if(i > MAX_PORT){
                fprintf(stderr, "main: index exceeds the array's range.\n");
                return -1;
            }
            if(start_f == 0){    
                port = cofg.ran_port[j];
                start_f = 1;
            } else{
                if(port + 1 == cofg.ran_port[j + 1] + 1)break;
                port++;
            }
        }
        
        printf("\t\tmain DEBUG: port[%ld] j[%d]\n", port, j);
		socket_fd = socket(AF_INET, SOCK_STREAM, 0);		/* creating an endpoint */
		if(socket_fd == -1){
			// socket() failed
            if(prev_err != errno)perror("socket");
		    error_count++;
			prev_err = errno;
            continue;
        }

        p_state = scan_port(socket_fd, cofg.input_addr, port);
        switch(p_state) {
            case _OPEN:
                printf("port %ld is open on %s\n", port, cofg.input_addr);
                open_count++;
                break;
            case _CLOSED:
                closed_count++;
                break;
            case _FILTERED:
                filtered_count++;
                break;
            case _UNREACHABLE:
                printf("Network is unreachable on port %ld\n", port);
                unreachable_count++;
                break;
            case _UNDEFINED:
			    printf("The error is beyond these errors( ENETUNREACH, ETIMEDOUT, ECONNREFUSED)\n");		
                error_count++;
                break;
            case _ERROR:
                error_count++;
                break;
            default:
                fprintf(stderr, "INVALID PORT STATE.\n");
                break;
        }
        ports_scanned++;
	}


    timespec_get(&end_time, TIME_UTC);
    
    double time_taken = (end_time.tv_sec - start_time.tv_sec) + (double)(end_time.tv_nsec - start_time.tv_nsec) / 1000000000.0;
    

    // SUMMARY
    printf("\nSUMMARY: ports scanned[%ld] { open ports[%d], closed ports[%d], filtered ports[%d], unreachable ports[%d], errors[%d]} --> time taken: [%.2lf s]\n", 
                       ports_scanned, open_count, closed_count, filtered_count, unreachable_count, error_count, time_taken);
	return 0;
}
