#ifndef SOCKET_PLATFORM_H
#define SOCKET_PLATFORM_H

#include <sys/socket.h>

/* Use MSG_NOSIGNAL on Linux to avoid SIGPIPE during sends. */
static inline int socket_send_flags(void) {
  #ifdef MSG_NOSIGNAL
    return MSG_NOSIGNAL;
  #else
    return 0;
  #endif
}

#endif