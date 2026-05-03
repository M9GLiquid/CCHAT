#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>

#include "version.h"

typedef const char *cstring;
typedef char *string;

#define MAX_CLIENTS 100
#define MAX_CHANNELS 32
#define MAX_CHANNEL_NAME 32
#define MAX_SESSION_ID 64
#define MAX_USERNAME 32
#define MAX_MESSAGE 512
#define BUFFER_SIZE 1024
#define PORT_MAX 65535

#define RECONNECT_PREFIX "$RECONNECT "
#define RECONNECT_REQUEST "$RECONNECT?"
#define RECONNECT_OK "RECONNECT_OK\n"

#endif
