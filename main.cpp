#include <astralib/astra.h>
#include <iostream>

//example, init is to initalize the window, obj_add is to add your desired obj, and render starts the render loop

int main() {
    astra::init();
    astra::obj_add("model.obj");
    astra::render();
    return 0;
}