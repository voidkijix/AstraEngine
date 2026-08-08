#pragma once
#include <string>

namespace astra {
    void init();
    void obj_add(const std::string&filepath);
    void render();
    bool should_close();
    void cleanup();
}