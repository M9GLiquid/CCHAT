#include "zmq_client.h"
#include "../common/utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void print_connection_usage(const char *app_name) {
  printf("Usage: %s <server_ip> <port>\n", app_name);
  printf("Example: %s 127.0.0.1 5555\n", app_name);
}

static void client_generate_identity(Client *client) {
  if (!client)
    return;

  snprintf(client->identity, sizeof(client->identity), "cchat-%ld-%lld",
           (long)getpid(), (long long)zclock_usecs());
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
    void *which = zpoller_wait(poller, -1);

    if (which == pipe) {
      char *outbound = zstr_recv(pipe);
      if (!outbound)
        break;

      if (strcmp(outbound, "$TERM") == 0) {
        zstr_free(&outbound);
        break;
      }

      if (zstr_send(socket, outbound) != 0)
        fprintf(stderr, "Send delayed; waiting for reconnect.\n");

      zstr_free(&outbound);
    } else if (which == socket) {
      char *incoming = zstr_recv(socket);
      if (!incoming) {
        fprintf(stderr, "Disconnected from the server; reconnecting... \n");
        zclock_sleep(250);
        continue;
      }

      printf("%s", incoming);
      if (strchr(incoming, '\n'))
        printf("\n");
      fflush(stdout);
      zstr_free(&incoming);
      continue;
    }
    if (zpoller_terminated(poller))
      break;
  }

  zpoller_destroy(&poller);
  zsock_destroy(&socket);
}

int client_connect(Client *client, const char *server_ip, int port) {
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
  client_generate_identity(client);

  client->network_actor = zactor_new(client_network_actor, client);
  if (!client->network_actor)
    return -1;

  return 0;
}

void client_run(Client *client) {
  char input[BUFFER_SIZE];

  if (!client || !client->network_actor)
    return;

  printf("Connected.\n");
  printf("Send format: <target_id>:<message>\n");
  printf("Examples:\n");
  printf("  1:hello from client\n");
  printf("  0:ping back\n");
  printf("  QUIT\n");

  while (!zsys_interrupted) {
    if (!fgets(input, sizeof(input), stdin))
      break;

    trim_newline(input);
    if (input[0] == '\0')
      continue;

    if (strcmp(input, "QUIT") == 0)
      break;

    if (zstr_send(client->network_actor, input) != 0) {
      fprintf(stderr, "Failed to queue message for network actor.\n");
      break;
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

int main(int argc, char *argv[]) {
  Client client;
  const char *server_ip = "127.0.0.1";
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