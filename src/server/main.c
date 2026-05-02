#include <stdlib.h>

#include "zmq_server.h"
#include "../common/common.h"

int main(int argc, string argv[]) {
  int port = 5555;

  if (argc > 1) {
    port = atoi(argv[1]);
  }

  run_zmq_server(port);
  return 0;
}
