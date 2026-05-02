#include "commands.h"

static const CommandSpec COMMAND_SPECS[] = {
    {ACTION_HELP, COMMAND_CONTEXT_CLIENT | COMMAND_CONTEXT_SERVER, {"/help"},
     "/help", COMMAND_ARGS_NONE},
    {ACTION_PRIVATE_MESSAGE, COMMAND_CONTEXT_CLIENT, {"/msg"},
     "/msg <user> <message>", COMMAND_ARGS_WORD_REST},
    {ACTION_USER_LIST, COMMAND_CONTEXT_CLIENT, {"/user", "list"}, "/user list",
     COMMAND_ARGS_NONE},
    {ACTION_RENAME, COMMAND_CONTEXT_CLIENT, {"/rename"},
     "/rename <new_name>", COMMAND_ARGS_WORD},
    {ACTION_BROADCAST, COMMAND_CONTEXT_SERVER, {"/broadcast"},
     "/broadcast <message>", COMMAND_ARGS_REST},
    {ACTION_QUIT, COMMAND_CONTEXT_CLIENT | COMMAND_CONTEXT_SERVER, {"/quit"},
     "/quit or q", COMMAND_ARGS_NONE},
    {ACTION_QUIT, COMMAND_CONTEXT_CLIENT | COMMAND_CONTEXT_SERVER, {"q"}, NULL,
     COMMAND_ARGS_NONE},
};

const CommandSpec *command_specs(size_t *count) {
  if (count)
    *count = sizeof(COMMAND_SPECS) / sizeof(COMMAND_SPECS[0]);

  return COMMAND_SPECS;
}
