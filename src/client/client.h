#ifndef CLIENT_H
#define CLIENT_H

#include <stdbool.h>
#include <stddef.h>

typedef struct {
  int socket_fd;
  bool running;
} Client;

int client_connect(Client *client, const char *server_ip, int port);
void client_run(Client *client);
void client_disconnect(Client *client);

#endif
