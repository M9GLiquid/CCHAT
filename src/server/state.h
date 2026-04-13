#ifndef server_H
#define server_H

#include "../common/common.h"
#include <pthread.h>

typedef struct {
    int fd;
    bool active;
    char username[MAX_USERNAME];
} Client;

typedef struct {
    char name[MAX_CHANNEL_NAME];
    int members[MAX_CLIENTS];
    int member_count;
    bool active;
} Channel;

typedef struct {
    Client clients[MAX_CLIENTS];
    Channel channels[MAX_CHANNELS];
    pthread_mutex_t lock;
} Server;

void server_init(Server *server);

int server_add_client(Server *server, int fd);
void server_remove_client(Server *server, int client_id);

int server_find_channel(Server *server, const char *name);
int server_create_channel(Server *server, const char *name);

int server_join_channel(Server *server, int client_id, const char *channel_name);
int server_leave_channel(Server *server, int client_id, const char *channel_name);

bool server_client_in_channel(Server *server, int client_id, int channel_id);

#endif