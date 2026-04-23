/* CHANGED: Minimal CZMQ DEALER client interface for relay messaging in src2. */
#ifndef CLIENT_H
#define CLIENT_H

#include <czmq.h>
#include <pthread.h>
#include <stdbool.h>

typedef struct {
  zsock_t *socket;
  bool running;
  pthread_t receiver_thread;
} Client;

int client_connect(Client *client, const char *server_ip, int port);
void client_run(Client *client);
void client_disconnect(Client *client);

#endif
