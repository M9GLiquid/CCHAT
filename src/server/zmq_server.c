/* CZMQ Relay */

#include "zmq_server.h"
#include "../common/common.h"
#include "zmq_state.h"

#include <czmq.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool extract_payload(zmsg_t *message, char *buffer, size_t buffer_size) {
  zframe_t *payload = NULL;
  size_t payload_size = 0;

  if (!message || !buffer || buffer_size == 0)
    return false;

  payload = zmsg_last(message);
  if (!payload)
    return false;

  payload_size = zframe_size(payload);
  if (payload_size >= buffer_size)
    payload_size = buffer_size - 1;

  memcpy(buffer, zframe_data(payload), payload_size);
  buffer[payload_size] = '\0';
  return true;
}

static bool send_reply(zsock_t *router, const zframe_t *identity,
                       const char *reply_msg) {
  zmsg_t *reply = NULL;

  if (!router || !identity || !reply_msg)
    return false;

  reply = zmsg_new();
  if (!reply)
    return false;

  if (zmsg_add(reply, zframe_dup((zframe_t *)identity)) != 0) {
    zmsg_destroy(&reply);
    return false;
  }

  if (zmsg_addstr(reply, reply_msg) != 0) {
    zmsg_destroy(&reply);
    return false;
  }

  if (zmsg_send(&reply, router) != 0) {
    zmsg_destroy(&reply);
    return false;
  }

  return true;
}

static bool send_to_client_id(Server *server, zsock_t *router, int client_id,
                              const char *msg) {
  zframe_t *identity = NULL;
  bool ok = false;

  if (!server || !router || !msg)
    return false;

  if (!server_get_client_identity_copy(server, client_id, &identity))
    return false;

  ok = send_reply(router, identity, msg);
  zframe_destroy(&identity);
  return ok;
}

static bool parse_route_payload(const char *buffer, int *target_client_id,
                                const char **message_out) {
  const char *seperator = NULL;
  char id_text[32];
  char *end_ptr = NULL;
  long parsed_id = -1;
  size_t id_len = 0;

  if (!buffer || !target_client_id || !message_out)
    return false;

  seperator = strchr(buffer, ':'); // <target_id> ':' <message>
  if (!seperator)
    return false;

  id_len = (size_t)(seperator - buffer); // Extract the <target_id>
  if (id_len == 0 || id_len >= sizeof(id_text))
    return false;

  memcpy(id_text, buffer, id_len);
  id_text[id_len] = '\0';

  parsed_id = strtol(id_text, &end_ptr, 10);
  if (end_ptr == id_text || *end_ptr != '\0' || parsed_id < 0 ||
      parsed_id > INT_MAX)
    return false;

  *target_client_id = (int)parsed_id;
  *message_out = seperator + 1;
  return true;
}

void run_zmq_server(int port) {
  Server server;
  zsock_t *router = NULL;

  if (port <= 0 || port > PORT_MAX) {
    fprintf(stderr, "Invalid port: %d\n", port);
    exit(EXIT_FAILURE);
  }

  server_init(&server);

  router = zsock_new(ZMQ_ROUTER);
  if (router == NULL) {
    fprintf(stderr, "Failed to create ROUTER socket\n");
    server_destroy(&server);
    exit(EXIT_FAILURE);
  }

  zsock_set_linger(router, 0);

  if (zsock_bind(router, "tcp://*:%d", port) < 0) {
    fprintf(stderr, "Failed to bind ROUTER socket on port %d\n", port);
    zsock_destroy(&router);
    server_destroy(&server);
    exit(EXIT_FAILURE);
  }

  printf("Server listening on port %d\n", port);

  while (!zsys_interrupted) {
    zmsg_t *incoming = zmsg_recv(router);
    zframe_t *identity = NULL;
    char buffer[BUFFER_SIZE];
    char routed_text[BUFFER_SIZE];
    const char *message_text = NULL;
    int client_id = -1;
    int target_client_id = -1;

    if (!incoming)
      break;

    identity = zmsg_pop(incoming);
    if (!identity) {
      zmsg_destroy(&incoming);
      continue;
    }

    if (!extract_payload(incoming, buffer, sizeof(buffer))) {
      zframe_destroy(&identity);
      zmsg_destroy(&incoming);
      continue;
    }

    client_id = server_get_client(&server, identity);
    if (client_id < 0)
      client_id = server_add_client(&server, identity);

    if (client_id < 0) {
      (void)send_reply(router, identity, "Error server full!\n");
      zframe_destroy(&identity);
      zmsg_destroy(&incoming);
      continue;
    }

    if (!parse_route_payload(buffer, &target_client_id, &message_text)) {
      (void)send_reply(router, identity,
                       "ERROR format is <target_id>:<message>\n");
      zframe_destroy(&identity);
      zmsg_destroy(&incoming);
      continue;
    }

    if (!message_text || message_text[0] == '\0') {
      (void)send_reply(router, identity, "ERROR empty message\n");
      zframe_destroy(&identity);
      zmsg_destroy(&incoming);
      continue;
    }

    snprintf(routed_text, sizeof(routed_text), "FROM %d: %s\n", client_id,
             message_text);

    if (!send_to_client_id(&server, router, target_client_id, routed_text)) {
      (void)send_reply(router, identity, "ERROR target unavailable\n");
      zframe_destroy(&identity);
      zmsg_destroy(&incoming);
      continue;
    }

    (void)send_reply(router, identity, "ROUTED\n");

    zframe_destroy(&identity);
    zmsg_destroy(&incoming);
  }

  zsock_destroy(&router);
  server_destroy(&server);
}