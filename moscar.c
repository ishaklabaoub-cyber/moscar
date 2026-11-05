#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <sys/socket.h>
#include "ports.h"
#include "scan.h"
#include "parsing.h"
#include "utils.h"

int main(int argc, char *argv[])
{
    int    open_count, closed_count, filtered_count, unreachable_count, error_count;    
    long   ports_scanned, port;
    int    prev_err = 0;
    enum   port_state p_state;
    struct config cofg = {{0}, 0, 0, NULL, 0L, 0L};
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

    // Checking if there any ports are set if not setting the default range [20-80]
    uint16_t byte, bitpos, j;
    for(j = 0;j < MAX_PORT; ++j) {
        byte   = cofg.bmp[j/16];
        bitpos = (j%16);
        if( (byte>>bitpos) )break;
    }
    if(j == MAX_PORT){
        fprintf(stderr, "No ports are set.\nSetting the default port range[20-80].");
        set_ran(cofg.bmp, $16 20, $16 80);
    }

    for(port = 1;port < MAX_PORT +1 ; ++port){
        
        if( !(getport(cofg.bmp, $16 port)) )continue;       // if port not set --> continue 
        
		socket_fd = socket(AF_INET, SOCK_STREAM, 0);		/* creating an endpoint */
		if(socket_fd == -1){
			// socket() failed
            if(prev_err != errno)perror("socket");
		    error_count++, ports_scanned++;
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
