#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <ctype.h>
#include "utils.h"

int is_valid_ipv4(const char *src)
{
	struct sockaddr_in dst;

	return inet_pton(AF_INET, src, &dst);
}
int  isthere_digits(const char *arg) 
{
    size_t len = strlen(arg);
    for(int i = 0; i < (int)len; ++i) {
        if(isdigit(arg[i])) {
            return 1;
        }
    }
    return 0;
}
