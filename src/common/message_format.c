#include <stdio.h>

#include "message_format.h"

static cstring ACTION_TEMPLATES[] = {
    [ACTION_JOINED] = "%s joined.\n",    [ACTION_LEFT] = "%s left.\n",
    [ACTION_RENAME] = "%s is now %s.\n", [ACTION_PRIVATE_MESSAGE] = "%s: %s\n",
    [ACTION_BROADCAST] = "%s: %s\n",
};

static cstring format_template(Action action) {
  size_t format_count = sizeof(ACTION_TEMPLATES) / sizeof(ACTION_TEMPLATES[0]);

  if ((int)action < 0 || (size_t)action >= format_count)
    return NULL;

  return ACTION_TEMPLATES[action];
}

FormattedMessage format_message(Action action, cstring sender,
                                cstring message) {
  FormattedMessage result = {0};
  cstring template = format_template(action);
  int written = 0;

  if (!template || !sender)
    return result;

  written = snprintf(result.text, sizeof(result.text), template, sender,
                     message ? message : "");
  result.ok = written >= 0 && (size_t)written < sizeof(result.text);
  return result;
}
