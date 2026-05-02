#include <string.h>

#include "utils.h"

bool is_blank_string(cstring text) {
  return text == NULL || text[0] == '\0';
}

void trim_newline(string text) {
  if (text == NULL) {
    return;
  }

  text[strcspn(text, "\r\n")] = '\0';
}
