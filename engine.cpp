#include <iostream>
#include <string_view>
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GLFW/glfw3.h>
#include <shader/shader.h>
#include <vector>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>
#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobjloader/tiny_obj_loader.h>
#include <unordered_map>
#include <astralib/astra.h>


static GLFWwindow* window = nullptr;
static unsigned int instanceVBO = 0;
static Shader* shaderProgram = nullptr;

std::vector<glm::vec3> instance_position;
std::vector<glm::mat4> instance_model;
std::vector<unsigned int> vao_list;
std::vector<unsigned int> index_counts;
static unsigned int texture1 = 0;

// frame timing
static float lastFrame = 0.0f;
static float deltaTime = 0.0f;

// camera and transformations
static glm::vec3 cameraPos = glm::vec3(0.0f, 10.0f, 25.0f);
static glm::mat4 view = glm::mat4(1.0f);
static glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1920.0f / 1080.0f, 0.1f, 100.0f);

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

std::vector<Vertex> vertices;
std::vector<unsigned int> indices;

bool load_obj(const std::string& path) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> material;
    
    vertices.clear();
    indices.clear();


    bool triangulate = true;
    std::string warn, err;
    if (!tinyobj::LoadObj(&attrib, &shapes, &material, &warn, &err, path.c_str(), nullptr, triangulate)) {
        std::cout << "OBJ Loading Error: " << warn << err << std::endl;
        return false;
    }

    std::unordered_map<std::string, unsigned int> unique_vertices;

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};

            // extract position
            vertex.Position = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            // extract normal if its there
            if (index.normal_index >= 0) {
                vertex.Normal = {
                    attrib.normals[3 * index.normal_index + 0],
                    attrib.normals[3 * index.normal_index + 1],
                    attrib.normals[3 * index.normal_index + 2]
                };
            }
            else {
                vertex.Normal = glm::vec3(0.0f,1.0f,0.0f);
            }
            // extracts the UV cords if its there
            if (index.texcoord_index >= 0) {
                vertex.TexCoords = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    attrib.texcoords[2 * index.texcoord_index + 1]
                };
            }
            else {
                vertex.TexCoords = glm::vec2(0.0f,0.0f);
            }
            // hash key, for duplication
            std::string key = std::to_string(index.vertex_index) + "_" +
                std::to_string(index.normal_index >= 0 ? index.normal_index : -1) + "_" +
                std::to_string(index.texcoord_index >= 0 ? index.texcoord_index : -1);
            

            if (unique_vertices.count(key) == 0) {
                unique_vertices[key] = static_cast<unsigned int>(vertices.size());
                vertices.push_back(vertex);
            }
            indices.push_back(unique_vertices[key]);
        }
    }
    return true;
}


namespace astra {

    void init() {
        // buncha glfw stuff, aswell as telling openGL to cull faces for performance aswell as depth
        std::cout << "Engine Started" << std::endl;
        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_SAMPLES, 4);
        glfwWindowHint(GLFW_SAMPLES,4);
        window = glfwCreateWindow(1920, 1080, "AstraEngine", NULL, NULL);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwMakeContextCurrent(window);
        gladLoadGL(glfwGetProcAddress);
        glClearColor(0.2f,0.3f,0.3f,1.0f);
        glEnable(GL_CULL_FACE); 
        glEnable(GL_DEPTH_TEST); 
        glEnable(GL_MULTISAMPLE); 
        shaderProgram = new Shader("include/shader/shader.vert", "include/shader/shader.frag"); 
    }

    void obj_add(const std::string&filepath) {
        load_obj(filepath);

        glm::mat4 model_matrix = glm::scale(glm::mat4(1.0f), glm::vec3(0.1f));
        instance_model.push_back(model_matrix);

        // AAAAAAAAAAAAAAAAAAAAAAA
        unsigned int VAO;
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);
        // iam so confused but ill try
        // empty tracking ID in VBO for a chunk of ghraphics memory
        unsigned int VBO;
        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        // copy vectors into the VBO box and put it in vram
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex),
        vertices.data(), GL_STATIC_DRAW);
        // indicees, same steps
        unsigned int EBO;
        glGenBuffers(1, &EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
        indices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glEnableVertexAttribArray(0);
        
        // position attribute
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0,3, GL_FLOAT,GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Position));

        // normal attribute

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1,3, GL_FLOAT,GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));

        // UV texture attribute
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2,2, GL_FLOAT,GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

        // setting up instance VBO
        glGenBuffers(1, &instanceVBO);
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        glBufferData(GL_ARRAY_BUFFER, instance_model.size() * sizeof(glm::mat4), instance_model.data(), GL_STATIC_DRAW);
        // a loop from location 1 to 4 setting up glVertexAttribPointer and calling glVertexAtrribDivisor
        for (unsigned int i = 0; i < 4; i++) {
            glEnableVertexAttribArray(3 + i);
            glVertexAttribPointer(3 + i, 4 ,GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(i * sizeof(glm::vec4)));
            glVertexAttribDivisor(3 + i, 1);
        }
        vao_list.push_back(VAO);
        index_counts.push_back(indices.size());

            stbi_set_flip_vertically_on_load(true);
        glGenTextures(1, &texture1);
    glBindTexture(GL_TEXTURE_2D, texture1);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    int width, height, nrChannels;
    unsigned char *data = stbi_load("texture.jpg", &width, &height, &nrChannels, 0); //should become modular eventually
    if (data) {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0 , format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cout << "Texture Error" << std::endl;
    }
    stbi_image_free(data);
    }

    void render() {
            //glPolygonMode(GL_FRONT_AND_BACK,GL_LINE); -- debug

    double mouseX = 0.0, mouseY = 0.0;
    std::cout << "Engine Render" << std::endl;
    // render loop
        while(!glfwWindowShouldClose(window)) { 

            // get mouse position relative to window center
            glfwGetCursorPos(window, &mouseX, &mouseY);

            // convert mouse coordinates to rotation angles in radians
            float rotY = static_cast<float>((mouseX - 960.0) * 0.005); // Horizontal rotation
            float rotX = static_cast<float>((mouseY - 540.0) * 0.005); // Vertical rotation

            // apply rotation to model matrix
            glm::mat4 model_matrix = glm::scale(glm::mat4(1.0f), glm::vec3(0.1f));
            model_matrix = glm::rotate(model_matrix, rotY, glm::vec3(0.0f, 1.0f, 0.0f));
            model_matrix = glm::rotate(model_matrix, rotX, glm::vec3(1.0f, 0.0f, 0.0f));

            // update instance VBO buffer with the new rotated matrix
            glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(model_matrix));


            // standard deltatime
            float currentFrame = (float)glfwGetTime();
            float deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            view = glm::lookAt(cameraPos,glm::vec3(0.0f, 0.0f, 0.0f),glm::vec3(0.0f, 1.0f, 0.0f));
            // sending the matrices to shader program's input slots, please save me
            glUseProgram(shaderProgram->ID);
            glUniform3f(glGetUniformLocation(shaderProgram->ID, "lightPos"), 0.0f, 5.0f, 5.0f);
            glUniform3f(glGetUniformLocation(shaderProgram->ID, "viewPos"), cameraPos.x, cameraPos.y, cameraPos.z);
            glUniform3f(glGetUniformLocation(shaderProgram->ID, "lightColor"), 1.2f, 1.0f, 2.0f);
            glUniform3f(glGetUniformLocation(shaderProgram->ID, "objectColor"), 1.0f, 0.5f, 0.31f);

            glUniformMatrix4fv(glGetUniformLocation(shaderProgram->ID, "view"), 1, GL_FALSE, glm::value_ptr(view)); 
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram->ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture1);
            glUniform1i(glGetUniformLocation(shaderProgram->ID, "texture_diffuse1"), 0);

            //glBindVertexArray(VAO);

            for(size_t i = 0; i < vao_list.size(); i++) {
                glBindVertexArray(vao_list[i]);
                glDrawElementsInstanced(GL_TRIANGLES, static_cast<GLsizei>(index_counts[i]), GL_UNSIGNED_INT, 0, static_cast<GLsizei>(instance_model.size()));
            }

            //glDrawElementsInstanced(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0, static_cast<GLsizei>(instance_model.size()));

            glfwSwapBuffers(window);
            glfwPollEvents();
        }
        std::cout << "Engine Stop Render" << std::endl;
        glfwTerminate();
        std::cout << "Engine Terminate" << std::endl;
    }
}