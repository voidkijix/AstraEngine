#include <astralib/astra.h>
#include <iostream>

//example, init is to initalize the window, obj_add is to add your desired obj, and render starts the render loop

int main() {
    astra::init();
    astra::obj_add("temp/model.obj","temp/texture.jpg",glm::vec3(0.0f,0.0f,0.0f));
    astra::obj_add("temp/model.obj","temp/texture.jpg",glm::vec3(0.0f,0.0f,7.5f));
    astra::render();
    return 0;
}