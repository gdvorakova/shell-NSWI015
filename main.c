#include <stdlib.h>
#include "run_modes.h"

int
main(int argc, char **argv) {
    // Try running with -c argument
    c_option_run(argc, argv);
    // If -c was not applicable try other possibilites
    if (argc == 1) {
        interactive_mode_loop();
    } else if (argc == 2) {
        noninteractive_run(argv[1]);
    }
    // TODO print usages
    exit(EXIT_FAILURE);
}
