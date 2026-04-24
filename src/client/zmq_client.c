#include "zmq_client.h"
#include "../common/utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define lock(client) pthread_mutex_lock(&(client)->lock)
#define unlock(client) pthread_mutex_unlock(&(client)->lock)

static void print_connection_usage(const char *app_name) {
  printf("Usage: %s <server_ip> <port>\n", app_name);
  printf("Example: %s 127.0.0.1 5555\n", app_name);
}

static void client_stop(Client *client) {
  if (!client)
    return;

  lock(client);
  client->running = false;
  unlock(client);
}

static bool is_client_running(Client *client) {
  bool running = false;

  if (!client)
    return false;
  lock(client);
  running = client->running;
  unlock(client);
  return running;
}

static bool client_queue_message(Client *client, const char *message) {
  size_t next_tail = 0;
  bool queued = false;

  if (!client || !message)
    return false;

  lock(client);

  if (client->outbox_count < CLIENT_OUTBOX_CAPACITY) {
    next_tail = (client->outbox_tail + 1) % CLIENT_OUTBOX_CAPACITY;
    char *slot = client->outbox[client->outbox_tail];
    snprintf(slot, BUFFER_SIZE, "%s", message);
    client->outbox_tail = next_tail;
    client->outbox_count++;
    queued = true;
  }

  unlock(client);
  return queued;
}

static bool client_peek_message(Client *client, char *message,
                                size_t message_size) {
  bool available = false;

  if (!client || !message || message_size == 0)
    return false;

  lock(client);

  if (client->outbox_count > 0) {
    snprintf(message, message_size, "%s", client->outbox[client->outbox_head]);
    available = true;
  }

  unlock(client);
  return available;
}

static void client_drop_message(Client *client) {
  if (!client)
    return;

  lock(client);

  if (client->outbox_count > 0) {
    client->outbox_head = (client->outbox_head + 1) % CLIENT_OUTBOX_CAPACITY;
    client->outbox_count--;
  }

  unlock(client);
}

static void client_generate_identity(Client *client) {
  if (!client)
    return;

  snprintf(client->identity, sizeof(client->identity), "cchat-%ld-%lld",
           (long)getpid(), (long long)zclock_usecs());
}

static bool client_open_socket(Client *client) {
  char endpoint[128];

  if (!client || is_blank_string(client->server_ip))
    return false;

  client->socket = zsock_new(ZMQ_DEALER);
  if (client->socket == NULL) {
    fprintf(stderr, "Failed to create DEALER socket\n");
    return false;
  }

  zsock_set_identity(client->socket, client->identity);
  zsock_set_probe_router(client->socket, 1);     // send msg on new connection.
  zsock_set_reconnect_ivl(client->socket, 1000); // 1 sec
  zsock_set_reconnect_ivl_max(client->socket, 5000); // 5 sec
  zsock_set_sndtimeo(client->socket, 100); // timeout for send operation
  zsock_set_linger(client->socket, 0);     // close fast

  snprintf(endpoint, sizeof(endpoint), "tcp://%s:%d", client->server_ip,
           client->port);
  if (zsock_connect(client->socket, "%s", endpoint) < 0) {
    fprintf(stderr, "Failed to connect to server at %s\n", endpoint);
    zsock_destroy(&client->socket);
    return false;
  }

  return true;
}

static void *network_thread_main(void *arg) {
  Client *client = (Client *)arg;
  zpoller_t *poller = NULL;
  char outbound[BUFFER_SIZE];

  if (!client || !client->socket)
    return NULL;

  poller = zpoller_new(client->socket, NULL);
  if (!poller) {
    fprintf(stderr, "Failed to create client poller\n");
    return NULL;
  }

  while (is_client_running(client)) {
    while (client_peek_message(client, outbound, sizeof(outbound))) {
      if (zstr_send(client->socket, outbound) == 0) {
        client_drop_message(client);
      } else {
        fprintf(stderr, "Send delayed; waiting for reconnect.\n");
        zclock_sleep(250);
        break;
      }
    }

    if (!is_client_running(client))
      break;

    if (zpoller_wait(poller, 100) == client->socket) {
      char *incoming = zstr_recv(client->socket);

      if (!incoming) {
        fprintf(stderr, "Disconnected from the server; reconnecting. \n");
        zclock_sleep(250);
        continue;
      }

      printf("%s", incoming);
      if (!strchr(incoming, '\n'))
        printf("\n");
      fflush(stdout);
      zstr_free(&incoming);
      continue;
    }

    if (zpoller_terminated(poller))
      break;
  }

  zpoller_destroy(&poller);

  if (client->socket)
    zsock_destroy(&client->socket);

  return NULL;
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

  if (pthread_mutex_init(&client->lock, NULL)) {
    perror("pthread_mutex_init");
    return -1;
  }

  snprintf(client->server_ip, sizeof(client->server_ip), "%s", server_ip);
  client->port = port;
  client->running = true;
  client_generate_identity(client);

  if (!client_open_socket(client)) {
    pthread_mutex_destroy(&client->lock);
    return -1;
  }
  return 0;
}

void client_run(Client *client) {
  char input[BUFFER_SIZE];

  if (!client || !client->socket)
    return;

  if (pthread_create(&client->network_thread, NULL, network_thread_main,
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

  while (is_client_running(client) && !zsys_interrupted) {
    if (!fgets(input, sizeof(input), stdin)) {
      client_stop(client);
      break;
    }

    trim_newline(input);
    if (input[0] == '\0')
      continue;

    if (strcmp(input, "QUIT") == 0) {
      client->running = false;
      break;
    }

    if (!client_queue_message(client, input))
      fprintf(stderr, "OPutbound queue full, message dropped. \n");
  }

  client_stop(client);
  pthread_join(client->network_thread, NULL);
}

void client_disconnect(Client *client) {
  if (!client)
    return;

  client_stop(client);

  if (client->socket)
    zsock_destroy(&client->socket);

  pthread_mutex_destroy(&client->lock);
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
