#include "utils.h"

#include <string.h>

bool is_blank_string(const char *text) {
  return text == NULL || text[0] == '\0';
}

void trim_newline(char *text) {
  if (text == NULL) {
    return;
  }

  text[strcspn(text, "\r\n")] = '\0';
}
