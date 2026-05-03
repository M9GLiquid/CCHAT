/* CHANGED: Minimal CZMQ DEALER client interface for relay messaging in src2. */
#ifndef CLIENT_H
#define CLIENT_H

#include <czmq.h>
#include <stdbool.h>

#include "../common/common.h"

typedef struct {
  zactor_t *network_actor;
  char server_ip[64];
  int port;
  char identity[64];
  char session_id[MAX_SESSION_ID];
} Client;

int client_connect(Client *client, cstring server_ip, int port);
void client_run(Client *client);
void client_disconnect(Client *client);

#endif
