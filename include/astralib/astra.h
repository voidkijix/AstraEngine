#pragma once
#include <string>
#include <glm/glm.hpp>


// add input, and fix render to not loop, and also make proper dynamic obj

namespace astra {
    void init();
    void obj_add(const std::string&filepath,const std::string& texturePath, const glm::vec3& position);
    void render();
    bool should_close(); // not done
    void cleanup(); // not done
}