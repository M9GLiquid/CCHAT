#include "state.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>

static bool is_valid_client_id(int client_id) {
  return client_id >= 0 && client_id < MAX_CLIENTS;
}

static bool is_valid_channel_id(int channel_id) {
  return channel_id >= 0 && channel_id < MAX_CHANNELS;
}

static bool has_invalid_name_input(Server *server, const char *name) {
  return server == NULL || name == NULL || name[0] == '\0';
}

static void remove_client_from_channel(Channel *channel, int client_id) {
  for (int i = 0; i < channel->member_count; i++) {
    if (channel->members[i] != client_id){
      continue;
    }
    
    for (int j = i; j < channel->member_count - 1; j++) {
      channel->members[j] = channel->members[j + 1];
    }

    channel->member_count--;
    i--;
  }
}

void server_init(Server *server) {
  if (server == NULL){
    return;
  }

  memset(server, 0, sizeof(*server));
  pthread_mutex_init(&server->lock, NULL);
}

int server_add_client(Server *server, int fd) {
  int added_client_id = -1;

  if (server == NULL ||fd < 0) {
    return -1;
  }

  pthread_mutex_lock(&server->lock);

  for (int i = 0; i < MAX_CLIENTS; i++) {
    Client *client = &server->clients[i];

    if (client->active) {
      continue;
    }

    client->active = true;
    client->fd = fd;
    snprintf(client->username, MAX_USERNAME, "guest%d", i);

    added_client_id = i;
    break;
  }
  
  pthread_mutex_unlock(&server->lock);
  return added_client_id;
}

void server_remove_client(Server *server, int client_id) {
  if (server == NULL ||!is_valid_client_id(client_id)) {
    return;
  }

  pthread_mutex_lock(&server->lock);

  Client *client = &server->clients[client_id];
  if (!client->active) {
    pthread_mutex_unlock(&server->lock);
    return;
  }

  for (int i = 0; i < MAX_CHANNELS; i++) {
    Channel *channel = &server->channels[i];

    if (!channel->active) {
      continue;
    }

    remove_client_from_channel(channel, client_id);
  }

  int fd = client->fd;
  memset(client, 0, sizeof(*client));

  pthread_mutex_unlock(&server->lock);

  close(fd);
}

int server_find_channel(Server *server, const char *name) {
  if (has_invalid_name_input(server, name)) {
    return -1;
  }

  for (int i = 0; i < MAX_CHANNELS; i++) {
    Channel *channel = &server->channels[i];

    if (!channel->active) {
      continue;
    }

    if (strcmp(channel->name, name) == 0) {
      return i;
    }
  }

  return -1;
}

int server_create_channel(Server *server, const char *name) {
  if (has_invalid_name_input(server, name)) {
    return -1;
  }

  for (int i = 0; i < MAX_CHANNELS; i++) {
    Channel *channel = &server->channels[i];

    if (channel->active) {
      continue;
    }

    channel->active = true;
    channel->member_count = 0;
    strncpy(channel->name, name, MAX_CHANNEL_NAME -1);
    channel->name[MAX_CHANNEL_NAME - 1] = '\0';

    return i;
  }
}

bool server_client_in_channel(Server * server, int client_id, int channel_id) {
  if (server == NULL) {
    return false;
  }

  if (!is_valid_client_id(client_id) || !is_valid_channel_id(channel_id)) {
    return false;
  }

  Channel *channel = &server->channels[channel_id];
  if (!channel->active) {
    return false;
  }

  for (int i = 0; i < channel->member_count; i++) {
    if (channel->members[i] == client_id) {
      return true;
    }
  }

  return false;
}

int server_join_channel(Server *server, int client_id, const char *channel_name){
  int channel_id = -1;

  if (server == NULL || !is_valid_client_id(client_id)) {
    return -1;
  }

  if (channel_name == NULL || channel_name[0] == '\0') {
    return -1;
  }

  pthread_mutex_lock(&server->lock);

  Client *client = &server->clients[client_id];
  if (!client->active) {
    pthread_mutex_unlock(&server->lock);
    return -1;
  }

  channel_id = server_find_channel(server, channel_name);
  if (channel_id == -1) {
    channel_id = server_create_channel(server, channel_name);
  }

  if (!is_valid_channel_id(channel_id)) {
    pthread_mutex_unlock(&server->lock);
    return -1;
  }

  Channel *channel = &server->channels[channel_id];
  
  for (int i = 0; i < channel->member_count; i++) {
    if (channel->members[i] != client_id) {
      continue;
    }

    pthread_mutex_unlock(&server->lock);
    return channel_id;
  }

  channel->members[channel->member_count] = client_id;
  channel->member_count++;

  pthread_mutex_unlock(&server->lock);
  return channel_id;
}

int server_leave_channel(Server *server, int client_id, const char* channel_name) {
  if (server == NULL || !is_valid_client_id(client_id)) {
    return -1;
  }

  if (channel_name == NULL || channel_name[0] == '\0') {
    return -1;
  }

  pthread_mutex_lock(&server->lock);

  Client *client = &server->clients[client_id];
  if (!client->active) {
    pthread_mutex_unlock(&server->lock);
    return -1;
  }

  int channel_id = server_find_channel(server, channel_name);
  if (!is_valid_channel_id(channel_id)) {
    pthread_mutex_unlock(&server->lock);
    return -1;
  }

  Channel *channel = &server->channels[channel_id];

  for (int i = 0; i < channel->member_count; i++) {
    if (channel->members[i] != client_id) {
      continue;
    }

    for (int j = i; j < channel->member_count - 1; j++) {
      channel->members[j] = channel->members[j + 1];
    }

    channel->member_count--;
    pthread_mutex_unlock(&server->lock);
    return channel_id;
  }

  pthread_mutex_unlock(&server->lock);
  return -1;
}