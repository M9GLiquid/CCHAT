/* CZMQ Relay */

#include <czmq.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zmq_server.h"
#include "../common/command_parser.h"
#include "../common/common.h"
#include "../common/message_format.h"
#include "../common/utils.h"
#include "zmq_state.h"

typedef struct {
  int port;
  bool started;
} ServerActorArgs;

static bool extract_payload(zmsg_t *message, string buffer, size_t buffer_size) {
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
                       cstring reply_msg) {
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
                              cstring message) {
  zframe_t *identity = NULL;
  bool ok = false;

  if (!server || !router || !message)
    return false;

  if (!server_get_client_identity_copy(server, client_id, &identity))
    return false;

  ok = send_reply(router, identity, message);
  zframe_destroy(&identity);
  return ok;
}

static void broadcast_to_clients(Server *server, zsock_t *router,
                                 cstring message) {
  if (!server || !router || !message)
    return;

  for (int i = 0; i < MAX_CLIENTS; i++)
    (void)send_to_client_id(server, router, i, message);
}

static bool name_is_available(Server *server, int client_id, cstring name) {
  int existing_id = -1;

  if (!is_client_name_taken(server, name, &existing_id))
    return true;

  return existing_id == client_id;
}

static bool register_client_name(Server *server, zsock_t *router,
                                 const zframe_t *identity, int client_id,
                                 cstring buffer) {
  FormattedMessage notice;
  cstring name = NULL;

  if (!server || !router || !identity || !buffer)
    return false;

  if (strncmp(buffer, NAME_PREFIX, strlen(NAME_PREFIX)) != 0)
    return false;

  name = buffer + strlen(NAME_PREFIX);
  if (!name_is_available(server, client_id, name)) {
    (void)send_reply(router, identity, "ERROR name already in use\n");
    return true;
  }

  if (!server_set_client_name(server, client_id, name)) {
    (void)send_reply(router, identity, "ERROR invalid name\n");
    return true;
  }

  notice = format_message(ACTION_JOINED,
                          server_get_client_name(server, client_id), NULL);
  if (notice.ok)
    broadcast_to_clients(server, router, notice.text);

  return true;
}

static void handle_private_message(Server *server, zsock_t *router,
                                   int sender_id, const Command *command) {
  FormattedMessage inbound;
  FormattedMessage outbound;
  int target_id = -1;

  if (command->argc < 2 ||
      !is_client_name_taken(server, command->argv[0], &target_id)) {
    (void)send_to_client_id(server, router, sender_id,
                            "ERROR user not found\n");
    return;
  }

  inbound = format_message(ACTION_PRIVATE_MESSAGE,
                           server_get_client_name(server, sender_id),
                           command->argv[1]);
  if (inbound.ok)
    (void)send_to_client_id(server, router, target_id, inbound.text);

  if (target_id == sender_id)
    return;

  outbound = format_message(ACTION_PRIVATE_MESSAGE,
                            server_get_client_name(server, sender_id),
                            command->argv[1]);
  if (outbound.ok)
    (void)send_to_client_id(server, router, sender_id, outbound.text);
}

static void handle_name_change(Server *server, zsock_t *router, int client_id,
                               const Command *command) {
  FormattedMessage notice;
  char old_name[MAX_USERNAME];

  if (command->argc < 1 ||
      !name_is_available(server, client_id, command->argv[0])) {
    (void)send_to_client_id(server, router, client_id,
                            "ERROR name already in use\n");
    return;
  }

  snprintf(old_name, sizeof(old_name), "%s",
           server_get_client_name(server, client_id));
  if (!server_set_client_name(server, client_id, command->argv[0])) {
    (void)send_to_client_id(server, router, client_id, "ERROR invalid name\n");
    return;
  }

  notice = format_message(ACTION_RENAME, old_name,
                          server_get_client_name(server, client_id));
  if (notice.ok)
    broadcast_to_clients(server, router, notice.text);
}

static void handle_client_leave(Server *server, zsock_t *router,
                                const zframe_t *identity, int client_id) {
  char name[MAX_USERNAME];
  FormattedMessage notice;

  snprintf(name, sizeof(name), "%s", server_get_client_name(server, client_id));
  (void)send_reply(router, identity, "Goodbye.\n");
  server_remove_client(server, client_id);

  notice = format_message(ACTION_LEFT, name, NULL);
  if (notice.ok)
    broadcast_to_clients(server, router, notice.text);
}

static void handle_client_command(Server *server, zsock_t *router,
                                  const zframe_t *identity, int client_id,
                                  cstring command_text) {
  char users[BUFFER_SIZE];
  Command command;

  if (!parse_command(command_text, COMMAND_CONTEXT_CLIENT, &command)) {
    (void)send_reply(router, identity, command.error);
    return;
  }

  switch (command.action) {
  case ACTION_HELP:
    (void)send_reply(router, identity, command.text);
    break;
  case ACTION_PRIVATE_MESSAGE:
    handle_private_message(server, router, client_id, &command);
    break;
  case ACTION_USER_LIST:
    if (server_format_user_list(server, users, sizeof(users)))
      (void)send_reply(router, identity, users);
    else
      (void)send_reply(router, identity, "ERROR could not list users\n");
    break;
  case ACTION_RENAME:
    handle_name_change(server, router, client_id, &command);
    break;
  case ACTION_QUIT:
    handle_client_leave(server, router, identity, client_id);
    break;
  case ACTION_BROADCAST:
  case ACTION_JOINED:
  case ACTION_LEFT:
  case ACTION_INVALID:
    (void)send_reply(router, identity, "ERROR invalid command\n");
    break;
  }
}

static void handle_server_command(Server *server, zsock_t *router,
                                  cstring command_text) {
  FormattedMessage broadcast;
  Command command;

  if (!parse_command(command_text, COMMAND_CONTEXT_SERVER, &command)) {
    fprintf(stderr, "%s", command.error);
    return;
  }

  if (command.action == ACTION_BROADCAST) {
    broadcast = format_message(command.action, "SERVER", command.argv[0]);
    if (broadcast.ok)
      broadcast_to_clients(server, router, broadcast.text);
  }
}

static void server_actor(zsock_t *pipe, void *arg) {
  ServerActorArgs *args = (ServerActorArgs *)arg;
  Server server;
  zsock_t *router = NULL;
  zpoller_t *poller = NULL;
  int port = 0;

  if (!args) {
    zsock_signal(pipe, 1);
    return;
  }

  port = args->port;
  args->started = false;

  server_init(&server);

  router = zsock_new(ZMQ_ROUTER);
  if (!router) {
    fprintf(stderr, "Failed to create ROUTER socket\n");
    server_destroy(&server);
    zsock_signal(pipe, 1);
    return;
  }

  zsock_set_linger(router, 0);

  if (zsock_bind(router, "tcp://*:%d", port) < 0) {
    fprintf(stderr, "Failed to bind ROUTER socket on port %d\n", port);
    zsock_destroy(&router);
    server_destroy(&server);
    zsock_signal(pipe, 1);
    return;
  }

  poller = zpoller_new(pipe, router, NULL);
  if (!poller) {
    fprintf(stderr, "Failed to create server poller\n");
    zsock_destroy(&router);
    server_destroy(&server);
    zsock_signal(pipe, 1);
    return;
  }

  args->started = true;
  printf("Server listening on port %d\n", port);
  zsock_signal(pipe, 0);

  while (!zsys_interrupted) {
    void *which = zpoller_wait(poller, -1);
    zmsg_t *incoming = NULL;
    zframe_t *identity = NULL;
    char buffer[BUFFER_SIZE];
    int client_id = -1;

    if (which == pipe) {
      string pipe_command = zstr_recv(pipe);
      if (!pipe_command)
        break;

      if (strcmp(pipe_command, "$TERM") == 0) {
        zstr_free(&pipe_command);
        break;
      }

      handle_server_command(&server, router, pipe_command);
      zstr_free(&pipe_command);
      continue;
    }

    if (which != router) {
      if (zpoller_terminated(poller))
        break;
      continue;
    }

    incoming = zmsg_recv(router);
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
      (void)send_reply(router, identity, "ERROR server full\n");
      zframe_destroy(&identity);
      zmsg_destroy(&incoming);
      continue;
    }

    if (register_client_name(&server, router, identity, client_id, buffer)) {
      zframe_destroy(&identity);
      zmsg_destroy(&incoming);
      continue;
    }

    if (buffer[0] != '\0')
      handle_client_command(&server, router, identity, client_id, buffer);

    zframe_destroy(&identity);
    zmsg_destroy(&incoming);
  }

  zpoller_destroy(&poller);
  zsock_destroy(&router);
  server_destroy(&server);
}

void run_zmq_server(int port) {
  ServerActorArgs args = {.port = port, .started = false};
  zactor_t *actor = NULL;
  char input[BUFFER_SIZE];
  Command command;

  if (port <= 0 || port > PORT_MAX) {
    fprintf(stderr, "Invalid port: %d\n", port);
    exit(EXIT_FAILURE);
  }

  actor = zactor_new(server_actor, &args);
  if (!actor || !args.started) {
    if (actor)
      zactor_destroy(&actor);
    exit(EXIT_FAILURE);
  }

  if (parse_command("/help", COMMAND_CONTEXT_SERVER, &command))
    printf("%s", command.text);

  while (!zsys_interrupted) {
    printf("server> ");
    fflush(stdout);

    if (!fgets(input, sizeof(input), stdin))
      break;

    trim_newline(input);
    if (is_blank_string(input))
      continue;

    if (!parse_command(input, COMMAND_CONTEXT_SERVER, &command)) {
      if (strcmp(input, "q") == 0)
        break;

      printf("%s", command.error);
      continue;
    }

    if (command.action == ACTION_HELP) {
      printf("%s", command.text);
      continue;
    }

    if (command.action == ACTION_QUIT)
      break;

    if (zstr_send(actor, input) != 0) {
      fprintf(stderr, "Failed to queue server command.\n");
      break;
    }
  }

  zactor_destroy(&actor);
}
