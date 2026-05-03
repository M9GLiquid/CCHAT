#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../common/command_parser.h"
#include "../common/common.h"
#include "../common/utils.h"
#include "zmq_client.h"

#define CLIENT_TRACK_NAME_PREFIX "$LOCAL_NAME "
#define CLIENT_SESSION_REFRESH_INTERVAL_MS 2000

static void print_connection_usage(cstring app_name) {
  printf("Usage: %s <server_ip> <port>\n", app_name);
  printf("Example: %s 127.0.0.1 5555\n", app_name);
}

static bool send_reconnect_payload(zsock_t *socket, cstring session_id,
                                   cstring name) {
  char payload[BUFFER_SIZE];

  if (!socket || is_blank_string(name) || is_blank_string(session_id))
    return true;

  snprintf(payload, sizeof(payload), "%s1 %s %s", RECONNECT_PREFIX, session_id,
           name);
  return zstr_send(socket, payload) == 0;
}

static void client_generate_session(Client *client) {
  long pid = 0;
  long long now = 0;

  if (!client)
    return;

  pid = (long)getpid();
  now = (long)(long)zclock_usecs();
  snprintf(client->identity, sizeof(client->identity), "cchat-%ld-%lld", pid,
           now);
  snprintf(client->session_id, sizeof(client->session_id),
           "cchat-session-%ld-%lld", pid, now);
}

static zsock_t *client_open_socket(Client *client) {
  char endpoint[128];
  zsock_t *socket = NULL;

  if (!client || is_blank_string(client->server_ip))
    return NULL;

  socket = zsock_new(ZMQ_DEALER);
  if (!socket) {
    fprintf(stderr, "Failed to create DEALER socket\n");
    return NULL;
  }

  zsock_set_identity(socket, client->identity);
  zsock_set_probe_router(socket, 1);         // send msg on new connection.
  zsock_set_reconnect_ivl(socket, 1000);     // 1 sec
  zsock_set_reconnect_ivl_max(socket, 5000); // 5 sec
  zsock_set_sndtimeo(socket, 100);           // timeout for send operation
  zsock_set_linger(socket, 0);               // close fast

  snprintf(endpoint, sizeof(endpoint), "tcp://%s:%d", client->server_ip,
           client->port);
  if (zsock_connect(socket, "%s", endpoint) < 0) {
    fprintf(stderr, "Failed to connect to server at %s\n", endpoint);
    zsock_destroy(&socket);
    return NULL;
  }

  return socket;
}

static void client_network_actor(zsock_t *pipe, void *arg) {
  Client *client = (Client *)arg;
  zsock_t *socket = NULL;
  zpoller_t *poller = NULL;
  char current_name[MAX_USERNAME] = {0};

  if (!client) {
    zsock_signal(pipe, 1);
    return;
  }

  socket = client_open_socket(client);
  if (!socket) {
    zsock_signal(pipe, 1);
    return;
  }

  poller = zpoller_new(pipe, socket, NULL);
  if (!poller) {
    fprintf(stderr, "Failed to create client poller\n");
    zsock_destroy(&socket);
    zsock_signal(pipe, 1);
    return;
  }

  zsock_signal(pipe, 0);

  while (!zsys_interrupted) {
    void *rdy_socket = zpoller_wait(poller, CLIENT_SESSION_REFRESH_INTERVAL_MS);

    if (rdy_socket == pipe) { // Internal comm, talking to background worker.
      string outbound = zstr_recv(pipe);
      if (!outbound)
        break;

      if (strcmp(outbound, "$TERM") == 0) {
        zstr_free(&outbound);
        break;
      }

      if (strncmp(outbound, CLIENT_TRACK_NAME_PREFIX,
                  strlen(CLIENT_TRACK_NAME_PREFIX)) == 0) {
        snprintf(current_name, sizeof(current_name), "%s",
                 outbound + strlen(CLIENT_TRACK_NAME_PREFIX));
        if (!send_reconnect_payload(socket, client->session_id, current_name))
          fprintf(stderr, "Reconnect payload send delayed.\n");
        zstr_free(&outbound);
        continue;
      }

      if (zstr_send(socket, outbound) != 0)
        fprintf(stderr, "Send delayed; waiting for reconnect.\n");

      zstr_free(&outbound);
    } else if (rdy_socket == socket) { // Server sent us something
      string incoming = zstr_recv(socket);
      if (!incoming) {
        fprintf(stderr, "Disconnected from the server; reconnecting... \n");
        zclock_sleep(250);
        continue;
      }

      if (strcmp(incoming, RECONNECT_REQUEST) == 0) {
        if (!send_reconnect_payload(socket, client->session_id, current_name))
          fprintf(stderr, "Reconnect payload send delayed.\n");
        zstr_free(&incoming);
        continue;
      }

      if (strcmp(incoming, RECONNECT_OK) != 0) {
        printf("%s", incoming);
        if (!strchr(incoming, '\n'))
          printf("\n");
        fflush(stdout);
      }
      zstr_free(&incoming);
      continue;
    }

    if (!rdy_socket && !zpoller_terminated(poller) &&
        !is_blank_string(current_name)) {
      if (!send_reconnect_payload(socket, client->session_id, current_name))
        fprintf(stderr, "Reconnect payload send delayed.\n");
      continue;
    }

    if (zpoller_terminated(poller))
      break;
  }

  zpoller_destroy(&poller);
  zsock_destroy(&socket);
}

int client_connect(Client *client, cstring server_ip, int port) {
  if (!client || is_blank_string(server_ip)) {
    fprintf(stderr, "Client connection failed: invalid argument.\n");
    return -1;
  }

  if (port <= 0 || port > PORT_MAX) {
    fprintf(stderr, "Client connection failed: invalid port %d.\n", port);
    return -1;
  }

  memset(client, 0, sizeof(*client));

  snprintf(client->server_ip, sizeof(client->server_ip), "%s", server_ip);
  client->port = port;
  client_generate_session(client);

  client->network_actor = zactor_new(client_network_actor, client);
  if (!client->network_actor)
    return -1;

  return 0;
}

void client_run(Client *client) {
  char name[MAX_USERNAME];
  char input[BUFFER_SIZE];
  char track_name_message[BUFFER_SIZE];
  Command command;

  if (!client || !client->network_actor)
    return;

  printf("Client Version: %s\n", CLIENT_VERSION);
  printf("Connected.\n");
  do {
    printf("Enter name: ");
    fflush(stdout);
    if (!fgets(name, sizeof(name), stdin))
      return;
    trim_newline(name);
  } while (is_blank_string(name));

  snprintf(track_name_message, sizeof(track_name_message), "%s%s",
           CLIENT_TRACK_NAME_PREFIX, name);
  if (zstr_send(client->network_actor, track_name_message) != 0) {
    fprintf(stderr, "Failed to queue name for network actor. \n");
    return;
  }

  if (parse_command("/help", COMMAND_CONTEXT_CLIENT, &command))
    printf("%s", command.text);

  while (!zsys_interrupted) {
    printf("> ");
    fflush(stdout);

    if (!fgets(input, sizeof(input), stdin))
      break;

    trim_newline(input);
    if (input[0] == '\0')
      continue;

    if (!parse_command(input, COMMAND_CONTEXT_CLIENT, &command)) {
      printf("%s", command.error);
      continue;
    }

    if (command.action == ACTION_HELP) {
      printf("%s", command.text);
      continue;
    }

    if (command.action == ACTION_QUIT) {
      if (zstr_send(client->network_actor, "/quit") != 0)
        fprintf(stderr, "Failed to queue quit message for network actor.\n");
      break;
    }

    if (zstr_send(client->network_actor, input) != 0) {
      fprintf(stderr, "Failed to queue message for network actor.\n");
      break;
    }

    if (command.action == ACTION_RENAME) {
      snprintf(track_name_message, sizeof(track_name_message), "%s%s",
               CLIENT_TRACK_NAME_PREFIX, command.argv[0]);
      if (zstr_send(client->network_actor, track_name_message) != 0)
        fprintf(stderr, "Failed to track client name for reconnect.\n");
    }
  }
}

void client_disconnect(Client *client) {
  if (!client)
    return;

  if (client->network_actor)
    zactor_destroy(&client->network_actor);
  memset(client, 0, sizeof(*client));
}

int main(int argc, string argv[]) {
  Client client;
  cstring server_ip = "127.0.0.1";
  int port = 5555;

  if (argc == 3) {
    server_ip = argv[1];
    port = atoi(argv[2]);
  } else if (argc != 1) {
    print_connection_usage(argv[0]);
    return EXIT_FAILURE;
  }

  if (client_connect(&client, server_ip, port) != 0)
    return EXIT_FAILURE;

  client_run(&client);
  client_disconnect(&client);
  return EXIT_SUCCESS;
}
