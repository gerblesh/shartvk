#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include "include/vkapp.h"



int main() {
    VkApp app = {
        .width = 800,
        .height = 600,
        .title = "Hello Triangle SDL",
        .window = NULL,
        .renderer = NULL,
        .instance = VK_NULL_HANDLE,
    };

    return app_run(&app);
}

