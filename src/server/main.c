#include "server.h"

#include <stdlib.h>

int main(int argc, char *argv[]) {
  int port = 5555;
 
  if (argc > 1) {
    port = atoi(argv[1]);
  }

  run_server(port);
  return 0;
}