/* CHANGED: Minimal CZMQ DEALER client interface for relay messaging in src2. */
#ifndef CLIENT_H
#define CLIENT_H

#include "../common/common.h"

#include <czmq.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>

#define CLIENT_OUTBOX_CAPACITY 64

typedef struct {
  zsock_t *socket;
  pthread_t network_thread;
  pthread_mutex_t lock;
  bool running;
  char server_ip[64];
  int port;
  char identity[64];
  char outbox[CLIENT_OUTBOX_CAPACITY][BUFFER_SIZE];
  size_t outbox_head;
  size_t outbox_tail;
  size_t outbox_count;
} Client;

int client_connect(Client *client, const char *server_ip, int port);
void client_run(Client *client);
void client_disconnect(Client *client);

#endif
