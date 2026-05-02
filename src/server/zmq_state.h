#ifndef STATE_H
#define STATE_H

#include <czmq.h>

#include "../common/common.h"

typedef struct {
  zframe_t *identity;
  char name[MAX_USERNAME];
  bool active;
} Client;

typedef struct {
  Client clients[MAX_CLIENTS];
} Server;

void server_init(Server *server);
void server_destroy(Server *server);

int server_get_client(Server *server, const zframe_t *identity);
int server_add_client(Server *server, const zframe_t *identity);
bool server_set_client_name(Server *server, int client_id, cstring name);
cstring server_get_client_name(Server *server, int client_id);
bool is_client_name_taken(Server *server, cstring name, int *client_id_out);
bool server_format_user_list(Server *server, string buffer, size_t buffer_size);
bool server_get_client_identity_copy(Server *server, int client_id,
                                     zframe_t **identity_out);
void server_remove_client(Server *server, int client_id);

#endif
