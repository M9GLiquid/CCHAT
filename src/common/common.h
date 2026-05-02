#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>

typedef const char *cstring;
typedef char *string;

#define MAX_CLIENTS 100
#define MAX_CHANNELS 32
#define MAX_CHANNEL_NAME 32
#define MAX_USERNAME 32
#define MAX_MESSAGE 512
#define BUFFER_SIZE 1024
#define PORT_MAX 65535

#define NAME_PREFIX "$NAME "

#endif
