#include <stdlib.h>

#include "CoreConfig.h"

using namespace stargate;

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    CoreConfig config;
    config.readConfig("stargate.yaml");

    return EXIT_SUCCESS;
}
