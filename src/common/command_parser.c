#include <stdio.h>
#include <string.h>

#include "command_parser.h"
#include "utils.h"

static cstring skip_spaces(cstring text) {
  while (text && (*text == ' ' || *text == '\t'))
    text++;
  return text;
}

static size_t token_length(cstring text) {
  size_t len = 0;

  while (text[len] != '\0' && text[len] != ' ' && text[len] != '\t')
    len++;

  return len;
}

static size_t command_path_len(const CommandSpec *spec) {
  size_t len = 0;

  if (!spec)
    return 0;

  while (len < COMMAND_MAX_PATH && spec->path[len])
    len++;

  return len;
}

static bool path_matches(cstring input, const CommandSpec *spec,
                         cstring *args_out) {
  cstring cursor = input;

  if (!spec || !spec->path[0])
    return false;

  for (size_t i = 0; i < COMMAND_MAX_PATH && spec->path[i]; i++) {
    size_t input_len = 0;
    size_t path_len = 0;

    cursor = skip_spaces(cursor);
    if (is_blank_string(cursor))
      return false;

    input_len = token_length(cursor);
    path_len = strlen(spec->path[i]);
    if (input_len != path_len || strncmp(cursor, spec->path[i], path_len) != 0)
      return false;

    cursor += input_len;
  }

  if (args_out)
    *args_out = cursor;

  return true;
}

static const CommandSpec *find_command_spec(cstring input,
                                            CommandContext context,
                                            cstring *args_out) {
  size_t command_count = 0;
  const CommandSpec *specs = command_specs(&command_count);
  const CommandSpec *best = NULL;
  cstring best_args = NULL;

  for (size_t i = 0; i < command_count; i++) {
    const CommandSpec *spec = &specs[i];
    cstring args = NULL;

    if ((spec->contexts & context) == 0)
      continue;

    if (!path_matches(input, spec, &args))
      continue;

    if (!best || command_path_len(spec) > command_path_len(best)) {
      best = spec;
      best_args = args;
    }
  }

  if (args_out)
    *args_out = best_args;

  return best;
}

static void set_error(Command *command, cstring message) {
  if (!command || !message)
    return;

  snprintf(command->error, sizeof(command->error), "%s", message);
}

static void set_usage_error(Command *command, const CommandSpec *spec) {
  if (!spec || !spec->usage) {
    set_error(command, "ERROR invalid command\n");
    return;
  }

  snprintf(command->error, sizeof(command->error), "ERROR usage: %s\n",
           spec->usage);
}

static bool split_first_word(string text, string *word, string *rest) {
  string cursor = NULL;

  if (!text || !word)
    return false;

  cursor = text;
  while (*cursor == ' ' || *cursor == '\t')
    cursor++;

  if (*cursor == '\0')
    return false;

  *word = cursor;
  while (*cursor != '\0' && *cursor != ' ' && *cursor != '\t')
    cursor++;

  if (*cursor != '\0') {
    *cursor = '\0';
    cursor++;
  }

  if (rest)
    *rest = (string)skip_spaces(cursor);

  return true;
}

static bool parse_no_args(string args, Command *command) {
  if (!is_blank_string(skip_spaces(args)))
    return false;

  command->argc = 0;
  return true;
}

static bool build_help_text(CommandContext context, string buffer,
                            size_t buffer_size) {
  size_t command_count = 0;
  const CommandSpec *specs = command_specs(&command_count);
  size_t used = 0;

  if (!buffer || buffer_size == 0)
    return false;

  used = (size_t)snprintf(buffer, buffer_size, "Commands:\n");
  if (used >= buffer_size)
    return false;

  for (size_t i = 0; i < command_count; i++) {
    const CommandSpec *spec = &specs[i];
    int written = 0;

    if (!spec->usage || (spec->contexts & context) == 0)
      continue;

    written = snprintf(buffer + used, buffer_size - used, "  %s\n",
                       spec->usage);
    if (written < 0 || (size_t)written >= buffer_size - used)
      return false;

    used += (size_t)written;
  }

  return true;
}

static bool parse_help(string args, Command *command) {
  if (!parse_no_args(args, command))
    return false;

  if (!build_help_text(command->context, command->text, sizeof(command->text)))
    return false;

  command->argc = 1;
  command->argv[0] = command->text;
  return true;
}

static bool parse_word(string args, Command *command) {
  string word = NULL;
  string rest = NULL;

  if (!split_first_word(args, &word, &rest))
    return false;

  if (!is_blank_string(rest))
    return false;

  command->argc = 1;
  command->argv[0] = word;
  return true;
}

static bool parse_word_rest(string args, Command *command) {
  string word = NULL;
  string rest = NULL;

  if (!split_first_word(args, &word, &rest))
    return false;

  if (is_blank_string(rest))
    return false;

  command->argc = 2;
  command->argv[0] = word;
  command->argv[1] = rest;
  return true;
}

static bool parse_rest(string args, Command *command) {
  string rest = (string)skip_spaces(args);

  if (is_blank_string(rest))
    return false;

  command->argc = 1;
  command->argv[0] = rest;
  return true;
}

static bool parse_args(const CommandSpec *spec, string args, Command *command) {
  switch (spec->args) {
  case COMMAND_ARGS_NONE:
    return parse_no_args(args, command);
  case COMMAND_ARGS_WORD:
    return parse_word(args, command);
  case COMMAND_ARGS_WORD_REST:
    return parse_word_rest(args, command);
  case COMMAND_ARGS_REST:
    return parse_rest(args, command);
  }

  return false;
}

bool parse_command(cstring input, CommandContext context, Command *command) {
  const CommandSpec *spec = NULL;
  cstring args = NULL;
  cstring trimmed = NULL;
  size_t input_len = 0;

  if (!command)
    return false;

  memset(command, 0, sizeof(*command));
  command->action = ACTION_INVALID;
  command->context = context;

  if (!input) {
    set_error(command, "ERROR expected command\n");
    return false;
  }

  input_len = strlen(input);
  if (input_len >= sizeof(command->text)) {
    set_error(command, "ERROR command too long\n");
    return false;
  }

  snprintf(command->text, sizeof(command->text), "%s", input);

  trimmed = skip_spaces(command->text);
  if (is_blank_string(trimmed)) {
    set_error(command, "ERROR empty command\n");
    return false;
  }

  if (trimmed[0] != '/' && strcmp(trimmed, "q") != 0) {
    set_error(command, "ERROR expected command\n");
    return false;
  }

  spec = find_command_spec(trimmed, context, &args);
  if (!spec) {
    set_error(command, "ERROR unknown command\n");
    return false;
  }

  command->spec = spec;
  command->action = spec->action;

  args = (string)skip_spaces(args);
  if (spec->action == ACTION_HELP) {
    if (!parse_help((string)args, command)) {
      command->spec = NULL;
      command->action = ACTION_INVALID;
      set_usage_error(command, spec);
      return false;
    }
    return true;
  }

  if (!parse_args(spec, (string)args, command)) {
    command->spec = NULL;
    command->action = ACTION_INVALID;
    set_usage_error(command, spec);
    return false;
  }

  return true;
}
