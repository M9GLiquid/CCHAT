#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdbool.h>
#include <stddef.h>

#include "common.h"

#define COMMAND_MAX_ARGS 8
#define COMMAND_MAX_PATH 4

typedef enum {
  ACTION_INVALID = 0,
  ACTION_HELP,
  ACTION_PRIVATE_MESSAGE,
  ACTION_USER_LIST,
  ACTION_RENAME,
  ACTION_BROADCAST,
  ACTION_QUIT,
  ACTION_JOINED,
  ACTION_LEFT
} Action;

typedef enum {
  COMMAND_CONTEXT_CLIENT = 1 << 0,
  COMMAND_CONTEXT_SERVER = 1 << 1
} CommandContext;

typedef enum {
  COMMAND_ARGS_NONE,
  COMMAND_ARGS_WORD,
  COMMAND_ARGS_WORD_REST,
  COMMAND_ARGS_REST
} CommandArgKind;

typedef struct {
  Action action;
  CommandContext contexts;
  cstring path[COMMAND_MAX_PATH];
  cstring usage;
  CommandArgKind args;
} CommandSpec;

const CommandSpec *command_specs(size_t *count);

#endif
