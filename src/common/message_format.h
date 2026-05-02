#ifndef MESSAGE_FORMAT_H
#define MESSAGE_FORMAT_H

#include <stdbool.h>

#include "common.h"
#include "commands.h"

typedef struct {
  bool ok;
  char text[BUFFER_SIZE];
} FormattedMessage;

FormattedMessage format_message(Action action, cstring sender,
                                 cstring message);

#endif
