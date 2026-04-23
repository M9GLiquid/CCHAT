#include "zmq_state.h"

#include <string.h>

#define lock(server) pthread_mutex_lock(&(server)->lock)
#define unlock(server) pthread_mutex_unlock(&(server)->lock)

static bool is_valid_client_id(int client_id) {
  return client_id >= 0 && client_id < MAX_CLIENTS;
}

static bool frame_equals(const zframe_t *frame_a, const zframe_t *frame_b) {
  size_t frame_a_size = 0;
  size_t frame_b_size = 0;

  if (frame_a == NULL || frame_b == NULL)
    return false;

  frame_a_size = zframe_size((zframe_t *)frame_a);
  frame_b_size = zframe_size((zframe_t *)frame_b);
  if (frame_a_size != frame_b_size)
    return false;

  const byte *frame_a_data = zframe_data((zframe_t *)frame_a);
  const byte *frame_b_data = zframe_data((zframe_t *)frame_b);
  return memcmp(frame_a_data, frame_b_data, frame_a_size) == 0;
}

void server_init(Server *server) {
  if (!server)
    return;

  memset(server, 0, sizeof(*server));
  pthread_mutex_init(&server->lock, NULL);
}

void server_destroy(Server *server) {
  if (!server)
    return;

  lock(server);

  for (int i = 0; i < MAX_CLIENTS; i++) {
    Client *client = &server->clients[i];

    if (client->identity)
      zframe_destroy(&client->identity);

    memset(client, 0, sizeof(*client));
  }

  pthread_mutex_unlock(&server->lock);
  pthread_mutex_destroy(&server->lock);
}

static int find_client(Server *server, const zframe_t *identity) {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    Client *client = &server->clients[i];

    if (!client->active || !client->identity)
      continue;

    if (frame_equals(client->identity, identity))
      return i;
  }
  return -1;
}

int server_get_client(Server *server, const zframe_t *identity) {
  int found = -1;

  if (!server || !identity)
    return -1;

  lock(server);
  found = find_client(server, identity);
  unlock(server);

  return found;
}

static int add_client(Server *server, const zframe_t *identity) {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    zframe_t *id_copy = NULL;
    Client *client = &server->clients[i];

    if (client->active)
      continue;

    id_copy = zframe_dup((zframe_t *)identity);
    if (!id_copy)
      return -1;

    client->active = true;
    client->identity = id_copy;
    return i;
  }

  return -1;
}

int server_add_client(Server *server, const zframe_t *identity) {
  int added = -1;
  if (!server || !identity)
    return -1;

  lock(server);
  added = add_client(server, identity);
  unlock(server);

  return added;
}

bool server_get_client_identity_copy(Server *server, int client_id,
                                     zframe_t **identity_out) {
  zframe_t *copy = NULL;

  if (!identity_out)
    return false;
  *identity_out = NULL;

  if (!server || !is_valid_client_id(client_id))
    return false;

  lock(server);

  Client *client = &server->clients[client_id];
  if (!client->active || !client->identity) {
    unlock(server);
    return false;
  }

  copy = zframe_dup(client->identity);
  unlock(server);

  if (!copy)
    return false;

  *identity_out = copy;
  return true;
}

void server_remove_client(Server *server, int client_id) {
  if (!server || !is_valid_client_id(client_id))
    return;

  lock(server);

  Client *client = &server->clients[client_id];
  if (!client->active) {
    unlock(server);
    return;
  }

  if (client->identity)
    zframe_destroy(&client->identity);

  memset(client, 0, sizeof(*client));

  unlock(server);
}
