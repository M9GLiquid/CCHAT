#include <stdio.h>
#include <string.h>

#include "zmq_state.h"
#include "../common/utils.h"

static bool is_valid_client_id(int client_id) {
  return client_id >= 0 && client_id < MAX_CLIENTS;
}

static bool is_valid_name(cstring name) {
  size_t len = 0;

  if (is_blank_string(name))
    return false;

  len = strlen(name);
  if (len >= MAX_USERNAME)
    return false;

  for (size_t i = 0; i < len; i++) {
    if (name[i] == ' ' || name[i] == '\t')
      return false;
  }

  return true;
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
}

void server_destroy(Server *server) {
  if (!server)
    return;

  for (int i = 0; i < MAX_CLIENTS; i++) {
    Client *client = &server->clients[i];

    if (client->identity)
      zframe_destroy(&client->identity);

    memset(client, 0, sizeof(*client));
  }
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

  found = find_client(server, identity);

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
    snprintf(client->name, sizeof(client->name), "user%d", i);
    return i;
  }

  return -1;
}

int server_add_client(Server *server, const zframe_t *identity) {
  int added = -1;
  if (!server || !identity)
    return -1;

  added = add_client(server, identity);

  return added;
}

bool server_set_client_name(Server *server, int client_id, cstring name) {
  Client *client = NULL;

  if (!server || !is_valid_client_id(client_id) || !is_valid_name(name))
    return false;

  client = &server->clients[client_id];
  if (!client->active)
    return false;

  snprintf(client->name, sizeof(client->name), "%s", name);
  return true;
}

cstring server_get_client_name(Server *server, int client_id) {
  Client *client = NULL;

  if (!server || !is_valid_client_id(client_id))
    return "unknown";

  client = &server->clients[client_id];
  if (!client->active || is_blank_string(client->name))
    return "unknown";

  return client->name;
}

static int find_client_id_by_name(Server *server, cstring name) {
  if (!server || is_blank_string(name))
    return -1;

  for (int i = 0; i < MAX_CLIENTS; i++) {
    Client *client = &server->clients[i];

    if (!client->active)
      continue;

    if (strcmp(client->name, name) == 0)
      return i;
  }

  return -1;
}

bool is_client_name_taken(Server *server, cstring name,
                          int *client_id_out) {
  int client_id = find_client_id_by_name(server, name);

  if (client_id_out)
    *client_id_out = client_id;

  return client_id != -1;
}

bool server_format_user_list(Server *server, string buffer, size_t buffer_size) {
  size_t used = 0;
  int count = 0;

  if (!server || !buffer || buffer_size == 0)
    return false;

  used = (size_t)snprintf(buffer, buffer_size, "Connected users:\n");
  if (used >= buffer_size)
    return false;

  for (int i = 0; i < MAX_CLIENTS; i++) {
    int written = 0;
    Client *client = &server->clients[i];

    if (!client->active)
      continue;

    written = snprintf(buffer + used, buffer_size - used, "  %s\n",
                       server_get_client_name(server, i));
    if (written < 0 || (size_t)written >= buffer_size - used)
      return false;

    used += (size_t)written;
    count++;
  }

  if (count == 0) {
    int written = snprintf(buffer + used, buffer_size - used, "  none\n");
    if (written < 0 || (size_t)written >= buffer_size - used)
      return false;
  }

  return true;
}

bool server_get_client_identity_copy(Server *server, int client_id,
                                     zframe_t **identity_out) {
  zframe_t *copy = NULL;

  if (!identity_out)
    return false;
  *identity_out = NULL;

  if (!server || !is_valid_client_id(client_id))
    return false;

  Client *client = &server->clients[client_id];
  if (!client->active || !client->identity) 
    return false;

  copy = zframe_dup(client->identity);

  if (!copy)
    return false;

  *identity_out = copy;
  return true;
}

void server_remove_client(Server *server, int client_id) {
  if (!server || !is_valid_client_id(client_id))
    return;

  Client *client = &server->clients[client_id];
  if (!client->active) {
    return;
  }

  if (client->identity)
    zframe_destroy(&client->identity);

  memset(client, 0, sizeof(*client));
}
