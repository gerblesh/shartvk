#include <stdio.h>
#include <stdlib.h>
#include "include/vkapp.h"



int main() {
    VKApp helloTriangle;
    helloTriangle.width = 600;
    helloTriangle.height = 800;
    app_run(&helloTriangle);
    return 0;
}

