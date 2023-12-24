#include <stdio.h>
#include <stdlib.h>

#include "include/vkapp.h"



int main() {
    VkApp app = {0};
    populateVkApp(600, 800, "shartvk triangle", &app);
    return app_run(&app);
}

