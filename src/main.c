#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include "include/vkapp.h"



int main() {
    VkApp helloTriangle;
    helloTriangle.width = 600;
    helloTriangle.height = 800;
    helloTriangle.title = "shartVK App";
    app_run(&helloTriangle);
    return 0;
}

