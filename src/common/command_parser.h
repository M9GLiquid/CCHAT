#ifndef COMMAND_PARSER_H
#define COMMAND_PARSER_H

#include <stddef.h>

#include "common.h"
#include "commands.h"

typedef struct {
  const CommandSpec *spec;
  Action action;
  CommandContext context;
  int argc;
  char text[BUFFER_SIZE];
  cstring argv[COMMAND_MAX_ARGS];
  char error[128];
} Command;

bool parse_command(cstring input, CommandContext context, Command *command);

#endif
