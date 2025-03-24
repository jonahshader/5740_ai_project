#include "core_render.h"
#include <string>
#include <cstring>

int main(int argc, char *argv[]) {
  std::string map_file = "jnb_map_simplest.tmx"; // default map file

  // parse command line arguments
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--map") == 0) {
      // make sure we have a value after the --map flag
      if (i + 1 < argc) {
        map_file = argv[i + 1];
        i++; // skip the next argument since we've used it
      }
    }
  }

  jnb::run_game(map_file.c_str(), 0);
  return 0;
}
