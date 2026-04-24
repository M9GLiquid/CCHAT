#ifndef STATE_H
#define STATE_H

#include "../common/common.h"
#include <czmq.h>

typedef struct {
  zframe_t *identity;
  bool active;
} Client;

typedef struct {
  Client clients[MAX_CLIENTS];
} Server;

void server_init(Server *server);
void server_destroy(Server *server);

int server_get_client(Server *server, const zframe_t *identity);
int server_add_client(Server *server, const zframe_t *identity);
bool server_get_client_identity_copy(Server *server, int client_id,
                                     zframe_t **identity_out);
void server_remove_client(Server *server, int client_id);

#endif
