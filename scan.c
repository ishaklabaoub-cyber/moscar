#include "scan.h"
#include <sys/socket.h>
#include <errno.h>
#include <sys/poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <stdio.h>

enum port_state scan_port(int socket_fd,const char *input_addr,long port,struct config *cfg)
{
    if(cfg->timeout <= 0) {
        // timeout function did not get called , setting timeout to default(300)
        cfg->timeout = 300;
    }
    struct sockaddr_in addr;		        /* ip address */
	struct pollfd pfd;			            /* for a non-blocking socket */
	int    expected_error = EINPROGRESS;	/* to identify that the connection is in progress */
	int    err;   
    static int i = 0;
    socklen_t len = sizeof(err);
    
    addr.sin_family = AF_INET;
	addr.sin_port = htons(port);	
	if( inet_pton(AF_INET, input_addr, &addr.sin_addr) != 1){
		fprintf(stderr, "INVALID IP ADDRESS : %s\n",input_addr);
		perror("inet_pton");
		close(socket_fd);
    }
    /***************** must add the poll() function for non-blocking sockets*******************/ 
    if(fcntl(socket_fd, F_SETFL, O_NONBLOCK) == -1) {
        perror("fcntl");
    }
    pfd.fd = socket_fd;	
    pfd.events = POLLOUT;
    pfd.revents = 0;
		
    errno = 0;
    int ret = connect(socket_fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in));		/* trying to connect */
		
    if(ret == -1){
        int current_error = errno;
		if(current_error == expected_error){	// connection in progress

				int return_val_poll = poll(&pfd, 1, cfg->timeout);
				if(pfd.revents & POLLOUT){	// the socket is writable and O_NONBLOCK must be set
					
					if(getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, &err, &len) == -1){
						// getsockopt() failed
                        perror("getsockopt");
						close(socket_fd);
                        return _ERROR;
					} else{
                        close(socket_fd);
						if(err == 0){	// connection succeeded
                            cfg->open_ports[i++] = port;
							return _OPEN;

						} else if(err == ECONNREFUSED){ // connection refused
							// closed port.
                            return _CLOSED;
						} else if(err == ETIMEDOUT){	// connection timed out
                            return _FILTERED;
						} else if(err == ENETUNREACH){  // network unreachable
						    return _UNREACHABLE;	
						} else{
						    return _UNDEFINED;
                        }
					}

				} else if(pfd.revents & POLLERR){	//check if any errors occured in pfd.revents
                    fprintf(stderr, "error occured.\n");
					close(socket_fd);
                    return _ERROR;
				} else if(return_val_poll == 0){	// the poll timeout ran out of time
				    close(socket_fd);
                    return _FILTERED;
                } else if(return_val_poll == -1){
                    perror("poll");
                    close(socket_fd);
                    return _ERROR;
                }
			} else if(current_error == ECONNREFUSED){
                // closed port
                close(socket_fd);
                return _CLOSED;
            } else if(current_error == ENETUNREACH){
                // network unreachable
                close(socket_fd);
			    return _UNREACHABLE;
            }	
		} 

		// connection succeed
		printf("port %ld is open on %s\n", port, input_addr);
        cfg->open_ports[i++] = port;
        printf("\t\tscan_port DEBUG: open_port[%ld]\n",cfg->open_ports[i-1]);
        close(socket_fd);
        return _OPEN;
    
}
