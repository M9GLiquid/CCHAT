#include <errno.h>
#include "client.h"
#include "../common/common.h"
#include "../platform/socket_platform.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

static bool is_blank_string(const char *text) {
  return text == NULL ||text[0] == '\0';
}

static void trim_newline(char *text) {
  if (text == NULL) {
    return;
  }

  text[strcspn(text, "\r\n")] = '\0';
}

static bool send_all(int socket_fd, const char *buffer, size_t length) {
  size_t total_sent = 0;

  if (socket_fd < 0 || buffer == NULL) {
    return false;
  }

  while (total_sent < length) {
    ssize_t sent_now = send(socket_fd, buffer + total_sent, length - total_sent, socket_send_flags());

    if (sent_now <= 0 && errno == EINTR) {
      continue;
    }

    if (sent_now <= 0) {
      return false;
    }

    total_sent += (size_t)sent_now;
  }

  return true;
}

static bool send_line(int socket_fd, const char *line) {
  char buffer[BUFFER_SIZE];
  int written = 0;

  if (socket_fd < 0 || line == NULL) {
    return false;
  }

  written = snprintf(buffer, sizeof(buffer), "%s\n", line);
  if (written < 0 || (size_t)written >= sizeof(buffer)) {
    return false;
  }

  return send_all(socket_fd, buffer, (size_t)written);
}

static void print_connection_usage(const char *program_name) {
  printf("Usage: %s <server_ip> <port>\n", program_name);
  printf("Example: %s 127.0.0.1 5555\n", program_name);
}

static void *receiver_thread(void *arg) {
  Client *client = (Client *)arg;
  char buffer[BUFFER_SIZE];

  if (client == NULL) {
    return NULL;
  }

  while (client->running) {
    ssize_t bytes_received = recv(client->socket_fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes_received <= 0) {
      printf("Disconnected from the server. \n");
      client->running = false;
      return NULL;
    }

    buffer[bytes_received] = '\0';
    printf("%s", buffer);
    fflush(stdout);
  }

  return NULL;
}

int client_connect(Client *client, const char *server_ip, int port) {
  struct sockaddr_in server_address;
  int socket_fd = -1;

  if (client == NULL || is_blank_string(server_ip)) {
    fprintf(stderr, "Client connection failed: invalid argument. \n");
    return -1;
  }

  if (port <= 0 ||port > PORT_MAX) {
    fprintf(stderr, "Client connection failed: invalid port %d. \n", port);
    return -1;
  }

  socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd < 0) {
    perror("Failed to create client socket");
    return -1;
  }

  memset(&server_address, 0, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons((uint16_t)port);

  if (inet_pton(AF_INET, server_ip, &server_address.sin_addr) <= 0) {
    fprintf(stderr, "failed to parse server IP adress: %s\n", server_ip);
    close(socket_fd);
    return -1;
  }

  if (connect(socket_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
    perror("Failed to connect to server");
    close(socket_fd);
    return -1;
  }

  client->socket_fd = socket_fd;
  client->running = true;

  return 0;
}

void client_run(Client *client) {
  pthread_t receiver;
  char input[BUFFER_SIZE];

  if (client == NULL) {
    return;
  }

  if (client->socket_fd < 0) {
    fprintf(stderr, "Client run failed: no active sockets. \n");
    return;
  }

  if (pthread_create(&receiver, NULL, receiver_thread, client) != 0) {
    perror("Failed to create receiver thread");
    client->running = false;
    return;
  }

  printf("Connected.\n");
  printf("  NICK Thomas\n");
  printf("  JOIN general\n");
  printf("  MSG general Hello everyone\n");
  printf("  LEAVE general\n");
  printf("  QUIT\n");

  while (client->running) {
    if (fgets(input, sizeof(input), stdin) == NULL) {
      client->running = false;
      break;
    }

    trim_newline(input);

    if (input[0] == '\0'){
      return;
    }

    if (!send_line(client->socket_fd, input)) {
      fprintf(stderr, "Failed to send message to server. \n");
      client->running = false;
      break;
    }

    if (strcmp(input, "QUIT") == 0) {
      client->running = false;
      break;
    }
  }

  shutdown(client->socket_fd, SHUT_RDWR);
  pthread_join(receiver, NULL);
}

void client_disconnect(Client *client) {
  if (client == NULL) {
    return;
  }

  if (client->socket_fd >= 0) {
    close(client->socket_fd);
    client->socket_fd = -1;
  }

  client->running = false;
}

int main(int argc, char *argv[]) {
  Client client = {
    .socket_fd = -1,
    .running = false
  };

  const char *server_ip = "127.0.0.1";
  int port = 5555;

  if (argc == 3) {
    server_ip = argv[1];
    port = atoi(argv[2]);
  }else if( argc != 1) {
    print_connection_usage(argv[0]);
    return EXIT_FAILURE;
  }

  if (client_connect(&client, server_ip, port) != 0) {
    return EXIT_FAILURE;
  }

  client_run(&client);
  client_disconnect(&client);

  return EXIT_SUCCESS;
}
