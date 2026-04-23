#include "zmq_client.h"
#include "../common/common.h"
#include "../common/utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_connection_usage(const char *app_name) {
  printf("Usage: %s <server_ip> <port>\n", app_name);
  printf("Example: %s 127.0.0.1 5555\n", app_name);
}

static void *receiver_thread_main(void *arg) {
  Client *client = (Client *)arg;
  if (!client)
    return NULL;

  while (client->running && !zsys_interrupted) {
    char *buffer = zstr_recv(client->socket);

    if (!buffer) {
      client->running = false;
      return NULL;
    }

    printf("%s", buffer);
    if (!strchr(buffer, '\n'))
      printf("\n");
    fflush(stdout);
    zstr_free(&buffer);
  }
  return NULL;
}

int client_connect(Client *client, const char *server_ip, int port) {
  char endpoint[128];

  if (!client || is_blank_string(server_ip)) {
    fprintf(stderr, "Client connection failed: invalid argument.\n");
    return -1;
  }

  if (port <= 0 || port > PORT_MAX) {
    fprintf(stderr, "Client connection failed: invalid port %d.\n", port);
    return -1;
  }

  memset(client, 0, sizeof(*client));

  client->socket = zsock_new(ZMQ_DEALER);
  if (!client->socket) {
    fprintf(stderr, "Failed to create DEALER socket\n");
    return -1;
  }

  zsock_set_linger(client->socket, 0);

  snprintf(endpoint, sizeof(endpoint), "tcp://%s:%d", server_ip, port);
  if (zsock_connect(client->socket, "%s", endpoint) < 0) {
    fprintf(stderr, "Failed to connect to server at %s\n", endpoint);
    zsock_destroy(&client->socket);
    return -1;
  }

  client->running = true;
  return 0;
}

void client_run(Client *client) {
  char input[BUFFER_SIZE];

  if (!client || !client->socket)
    return;

  if (pthread_create(&client->receiver_thread, NULL, receiver_thread_main,
                     client) != 0) {
    perror("Failed to create receiver thread");
    client->running = false;
    return;
  }

  printf("Connected.\n");
  printf("Send format: <target_id>:<message>\n");
  printf("Examples:\n");
  printf("  1:hello from client\n");
  printf("  0:ping back\n");
  printf("  QUIT\n");

  while (client->running && !zsys_interrupted) {
    if (!fgets(input, sizeof(input), stdin))
      break;

    trim_newline(input);
    if (input[0] == '\0')
      continue;

    if (strcmp(input, "QUIT") == 0) {
      client->running = false;
      break;
    }

    if (zstr_send(client->socket, input) != 0) {
      fprintf(stderr, "Failed to send message to server.\n");
      client->running = false;
      break;
    }
  }

  client->running = false;
  zsys_interrupted = 1;
  pthread_join(client->receiver_thread, NULL);
}

void client_disconnect(Client *client) {
  if (!client)
    return;

  client->running = false;

  if (client->socket)
    zsock_destroy(&client->socket);
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
