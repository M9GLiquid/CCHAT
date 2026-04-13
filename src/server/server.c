#include "server.h"
#include "state.h"
#include "../common/common.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

typedef struct {
  Server *server;
  int client_id;
} ClientThreadArgs;

static bool is_blank_string(const char *text) {
  return text == NULL || text[0] == '\0';
}

static void trim_newline(char *text) {
  if (text == NULL) {
    return;
  }

  text[strcspn(text, "\r\n")] = '\0';
}

static bool has_invalid_name_input(Server *server, const char *text) {
  return server == NULL || is_blank_string(text);
}

void send_text(int fd, const char *text) {
  if (fd < 0 || text ==NULL) {
    return;
  }

  send(fd, text, strlen(text), 0);
}

static void send_prefixed_line(int fd, const char *prefix, const char *message) {
  char buffer[BUFFER_SIZE];

  if (fd < 0 ||prefix == NULL ||message == NULL) {
    return;
  }

  snprintf(buffer, sizeof(buffer), "%s %s\n", prefix, message);
  send_text(fd, buffer);
}

static void send_info(int fd, const char *message) {
  send_prefixed_line(fd, "INFO", message);
}

static void send_error(int fd, const char *message) {
  send_prefixed_line(fd, "Error", message);
}

static int get_client_fd(Server *server, int client_id) {
  int fd = -1;

  if (server == NULL || client_id < 0 || client_id >= MAX_CLIENTS) {
    return -1;
  }

  pthread_mutex_lock(&server->lock);

  Client *client = &server->clients[client_id];
  if (client->active) {
    fd = client->fd;
  }

  pthread_mutex_unlock(&server->lock);
  return fd;
}

static void get_client_username(Server *server, int client_id, char *out, size_t out_size) {
  if (out == NULL || out_size == 0) {
    return;
  }

  out[0] = '\0';

  if (server == NULL || client_id < 0 ||client_id >= MAX_CLIENTS) {
    return;
  }

  pthread_mutex_lock(&server->lock);

  Client *client = &server->clients[client_id];
  if (client->active) {
    strncpy(out, client->username, out_size -1);
    out[out_size - 1] = '\0';
  }

  pthread_mutex_unlock(&server->lock);
}

static bool set_client_nick(Server *server, int client_id, const char *nick) {
  if (has_invalid_name_input(server, nick)) {
    return false;
  }

  if (client_id < 0 || client_id >= MAX_CLIENTS) {
    return false;
  }

  pthread_mutex_lock(&server->lock);

  Client *client = &server->clients[client_id];
  if (!client->active) {
    pthread_mutex_unlock(&server->lock);
    return false;
  }

  strncpy(client->username, nick, MAX_USERNAME -1);
  client->username[MAX_USERNAME - 1] = '\0';

  pthread_mutex_unlock(&server->lock);
  return true;
}

static void broadcast_to_channel(Server *server, const char *channel_name, const char *sender, const char *message) {
  int target_fds[MAX_CLIENTS];
  int target_count = 0;
  char buffer[BUFFER_SIZE];

  if (has_invalid_name_input(server, channel_name) || is_blank_string(sender) || message == NULL) {
    return;
  }

  snprintf(buffer, sizeof(buffer), "MSG %s %s %s\n", channel_name, sender, message);

  pthread_mutex_lock(&server->lock);

  int channel_id = -1;
  for (int i = 0; i < MAX_CHANNELS; i++){
    Channel *channel = &server->channels[i];

    if (!channel->active) {
      continue;
    }

    if (strcmp(channel->name, channel_name) != 0) {
      continue;
    }

    channel_id = i;
    break;
  }

  if (channel_id < 0) {
    pthread_mutex_unlock(&server->lock);
    return;
  }

  Channel *channel = &server->channels[channel_id];

  for (int i = 0; i < channel->member_count; i++) {
    int member_id = channel->members[i];
    if (member_id < 0 ||member_id >= MAX_CLIENTS) {
       continue;
    }

    Client *client = &server->clients[member_id];
    if (!client->active) {
      continue;
    }

    target_fds[target_count] = client->fd;
    target_count++;
  }

  pthread_mutex_unlock(&server->lock);

  for (int i = 0; i < target_count; i++) {
    send_text(target_fds[i], buffer);
  }
}

static void handle_nick(Server *server, int client_id, char *args) {
  int fd = get_client_fd(server, client_id);

  if (fd < 0) {
    return;
  }

  if (is_blank_string(args)) {
    send_error(fd, "Usage: NICK <name>");
    return;
  }

  if (!set_client_nick(server, client_id, args)) {
    send_error(fd, "Could not update nickname");
    return;
  }

  send_info(fd, "Nickname updated");
}

static void handle_join(Server *server, int client_id, char *args) {
  int fd = get_client_fd(server, client_id);

  if (fd < 0) {
    return;
  }

  if (is_blank_string(args)) {
    send_error(fd, "Usage: JOIN <channel>");
    return;
  }

  if (server_join_channel(server, client_id, args) < 0) {
    send_error(fd, "Could not joint channel");
    return;
  }

  send_info(fd, "Joined channel");
}

static void handle_leave(Server *server, int client_id, char *args) {
  int fd = get_client_fd(server, client_id);

  if (fd < 0) {
    return;
  }

  if (is_blank_string(args)) {
    send_error(fd, "Usage: LEAVE <channel>");
    return;
  }

  if (server_leave_channel(server, client_id, args) < 0) {
    send_error(fd, "Could not leave channel");
    return;
  }

  send_info(fd, "Left channel");
}

static void handle_msg(Server *server, int client_id, char *args) {
  int fd = get_client_fd(server, client_id);
  char sender[MAX_USERNAME];
  char *channel_name = NULL;
  char *message = NULL;
  int channel_id = -1;

  if (fd < 0) {
    return;
  }

  if (is_blank_string(args)) {
    send_error(fd, "Usage: MSG <channel> <message>");
    return;
  }

  channel_name = strtok(args, " ");
  message = strtok(NULL, "");

  if (is_blank_string(channel_name) ||message == NULL) {
    send_error(fd, "Usage: MSG <channel <message>");
    return;
  }

  channel_id = server_find_channel(server, channel_name);
  if (channel_id < 0) {
    send_error(fd, "Channel does not exist");
    return;
  }

  if (!server_client_in_channel(server, client_id, channel_id)) {
    send_error(fd, "You are not in that channel");
    return;
  }

  get_client_username(server, client_id, sender, sizeof(sender));
  if (is_blank_string(sender)) {
    send_error(fd, "Could not read username");
    return;
  }

  broadcast_to_channel(server, channel_name, sender, message);
}

bool handle_command(Server *server, int client_id, char *line) {
  int fd = get_client_fd(server, client_id);
  char *command = NULL;
  char *args = NULL;

  if (fd < 0 ||line == NULL) {
    return false;
  }

  trim_newline(line);

  if (line[0] == '\0') {
    return true;
  }

  command = strtok(line, " ");
  args = strtok(NULL, "");

 if (command == NULL) {
  send_error(fd, "Invalid command");
  return true;
 } 

  if (strcmp(command, "NICK") == 0) {
    handle_nick(server, client_id, args);
    return true;
  }

  if (strcmp(command, "JOIN") == 0) {
    handle_join(server, client_id, args);
    return true;
  }

  if (strcmp(command, "LEAVE") == 0) {
    handle_leave(server, client_id, args);
    return true;
  }

  if (strcmp(command, "MSG") == 0) {
    handle_msg(server, client_id, args);
    return true;
  }

  if (strcmp(command, "QUIT") == 0) {
    send_info(fd, "Goodbye");
    return false;
  }

  send_error(fd, "Unknown Command");
  return true;
}

static void *client_thread(void *arg) {
  ClientThreadArgs *thread_args = (ClientThreadArgs *) arg;
  Server *server = NULL;
  int client_id = -1;
  int fd = -1;
  char buffer[BUFFER_SIZE];

  if (thread_args == NULL) {
    return NULL;
  }

  server = thread_args->server;
  client_id = thread_args->client_id;
  free(thread_args);

  fd = get_client_fd(server, client_id);

  if (fd < 0) {
    return NULL;
  }

  send_info(fd, "Welcome to CChat!");
  send_info(fd, "Commands: JOIN, LEAVE, MSG, NICK, QUIT");

  while (1) {
    ssize_t bytes_received = recv(fd, buffer, sizeof(buffer) -1, 0);

    if (bytes_received <= 0) {
      server_remove_client(server, client_id);
      return NULL;
    }

    buffer[bytes_received] = '\0';

    if (handle_command(server, client_id, buffer)) {
      continue;
    }

    server_remove_client(server, client_id);
    return NULL;
  }
}

void run_server(int port) {
  int server_fd = -1;
  int option = 1;
  struct sockaddr_in address;
  Server server;

  if (port <= 0 || port > PORT_MAX) {
    fprintf(stderr, "Invalid port: %d\n", port);
    exit(EXIT_FAILURE);
  }

  server_init(&server);

  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0) {
    perror("Failed to enable SO_REUSEADDR on server socket");
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons((uint16_t)port);

  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0 ) {
    perror("bind");
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  if (listen(server_fd, 10) < 0) {
    perror("listen");
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  printf("Server listening on port %d\n", port);

  while (1) {
    int client_fd = accept(server_fd, NULL, NULL);
    int client_id = -1;
    pthread_t thread_id;
    ClientThreadArgs *thread_args = NULL;

    if (client_fd < 0) {
      perror("accept");
      continue;
    }

    client_id = server_add_client(&server, client_fd);
    if (client_id < 0) {
      send_text(client_fd, "INFO: Server full");
      close(client_fd);
      continue;
    }

    thread_args = malloc(sizeof(*thread_args));
    if (thread_args == NULL) {
      perror("malloc");
      server_remove_client(&server, client_id);
      continue;
    }

    thread_args->server = &server;
    thread_args->client_id = client_id;

    if (pthread_create(&thread_id, NULL, client_thread, thread_args) != 0) {
      perror("pthread_create");
      free(thread_args);
      server_remove_client(&server, client_id);
      continue;
    }

    pthread_detach(thread_id);
  }
}
