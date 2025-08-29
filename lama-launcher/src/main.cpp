#include <iostream>
#include <string>

#include "lama_server.h"

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Usage: lamallm <mode>\n";
    std::cerr << "Modes: cli, embedded\n";
    return 1;
  }

  std::string mode = argv[1];
  if (mode == "server") {
    run_lama_server();

  } else {
    std::cerr << "Unknown mode: " << mode << "\n";
    return 1;
  }

  return 0;
}
