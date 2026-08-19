#define STB_IMAGE_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION
#include <stb_image/stb_image.h>
#include <tinyobjloader/tiny_obj_loader.h>
#include <iostream>
#include <string_view>
#include <cstdlib>
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GLFW/glfw3.h>
#include <shader/shader.h>
#include <vector>
#include <unordered_map>
#include <astralib/astra.h>



struct mesh {
    std::vector<glm::vec3> instance_position;
    std::vector<glm::mat4> instance_model;
    unsigned int VAO;
    unsigned int VBO;   
    unsigned int EBO;
    unsigned int instance_VBO = 0;
    unsigned int vao_list;
    unsigned int index_count;
    unsigned int texture_ID;
    std::vector<glm::mat4> instance_models;
};

static GLFWwindow* window = nullptr;
static Shader* shaderProgram = nullptr;
//static unsigned int texture1 = 0;

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
bool load_obj(const std::string& obj_path, std::vector<Vertex>& out_vertices, std::vector<unsigned int>& out_indices) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> material;
    
    out_vertices.clear();
    out_indices.clear();


    bool triangulate = true;
    std::string warn, err;
    if (!tinyobj::LoadObj(&attrib, &shapes, &material, &warn, &err, obj_path.c_str(), nullptr, triangulate)) {
        std::cout << "OBJ error : " << std::endl;
        std::cout << "failed to load OBJ at " << obj_path << "\nterminating engine" << std::endl;
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
                unique_vertices[key] = static_cast<unsigned int>(out_vertices.size());
                out_vertices.push_back(vertex);
            }
            out_indices.push_back(unique_vertices[key]);
        }
    }
    return true;
}


namespace astra {
    static std::vector<mesh> loaded_meshes;

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
    void obj_add(const std::string&filepath, const std::string& texturepath, const glm::vec3& position) {
        
        mesh local_mesh;

        std::vector<Vertex> local_vertices;   // local vertex container
        std::vector<unsigned int> local_indices; // local indices container

        
        // load_obj function + validiation check and termintation
        if (!load_obj(filepath, local_vertices, local_indices)) {
            std::cout << "OBJ error : " << std::endl;
            std::cout << "failed to load OBJ file at " << filepath << "\n terminating engine" << std::endl;
            std::exit(EXIT_FAILURE);
        }
        local_mesh.index_count = static_cast<unsigned int>(local_indices.size());

        //glm::mat4 model_matrix = glm::scale(glm::mat4(1.0f), glm::vec3(0.1f));

        glm::mat4 model_matrix = glm::translate(glm::mat4(1.0f), position);
        model_matrix = glm::scale(model_matrix, glm::vec3(0.1f));

        local_mesh.instance_models.push_back(model_matrix);

        // AAAAAAAAAAAAAAAAAAAAAAA
        glGenVertexArrays(1, &local_mesh.VAO);
        glBindVertexArray(local_mesh.VAO);

        // iam so confused but ill try
        // empty tracking ID in VBO for a chunk of ghraphics memory
        glGenBuffers(1, &local_mesh.VBO);
        glBindBuffer(GL_ARRAY_BUFFER, local_mesh.VBO);

        // copy vectors into the VBO box and put it in vram
        glBufferData(GL_ARRAY_BUFFER, local_vertices.size() * sizeof(Vertex),
        local_vertices.data(), GL_STATIC_DRAW);

        // indicees, same steps
        glGenBuffers(1, &local_mesh.EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, local_mesh.EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, local_indices.size() * sizeof(unsigned int),
        local_indices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, local_mesh.VBO);
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
        glGenBuffers(1, &local_mesh.instance_VBO);
        glBindBuffer(GL_ARRAY_BUFFER, local_mesh.instance_VBO);
        glBufferData(GL_ARRAY_BUFFER, local_mesh.instance_models.size() * sizeof(glm::mat4), local_mesh.instance_models.data(), GL_STATIC_DRAW);
        // a loop from location 1 to 4 setting up glVertexAttribPointer and calling glVertexAtrribDivisor
        for (unsigned int i = 0; i < 4; i++) {
            glEnableVertexAttribArray(3 + i);
            glVertexAttribPointer(3 + i, 4 ,GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(i * sizeof(glm::vec4)));
            glVertexAttribDivisor(3 + i, 1);
        }

    stbi_set_flip_vertically_on_load(true);
    glGenTextures(1, &local_mesh.texture_ID);
    glBindTexture(GL_TEXTURE_2D, local_mesh.texture_ID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    int width, height, nrChannels;
    unsigned char *data = stbi_load(texturepath.c_str(), &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0 , format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cout << "texture error : " << std::endl;
        std::cout << "failed to load texture at " << texturepath << "\nterminating engine" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    stbi_image_free(data);
    glBindVertexArray(0);
    loaded_meshes.push_back(local_mesh);
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

            for(auto& mesh : loaded_meshes) {
            // update transformation for the first instance of each mesh
            if (!mesh.instance_models.empty()) {
                    glm::mat4 model_matrix = mesh.instance_models[0]; 
        
                    // Apply rotation on top of the existing position
                    model_matrix = glm::rotate(model_matrix, rotY, glm::vec3(0.0f, 1.0f, 0.0f));
                    model_matrix = glm::rotate(model_matrix, rotX, glm::vec3(1.0f, 0.0f, 0.0f));

                    // Update the instance VBO buffer with the newly transformed matrix
                    glBindBuffer(GL_ARRAY_BUFFER, mesh.instance_VBO);
                    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(model_matrix));
                }

                // bind mesh-specific texture
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, mesh.texture_ID);
                glUniform1i(glGetUniformLocation(shaderProgram->ID, "texture_diffuse1"), 0);

                // bind mesh-specific VAO and finnaly draw
                glBindVertexArray(mesh.VAO);
                glDrawElementsInstanced(GL_TRIANGLES, static_cast<GLsizei>(mesh.index_count), GL_UNSIGNED_INT, 0, static_cast<GLsizei>(mesh.instance_models.size()));
            }

            glfwSwapBuffers(window);
            glfwPollEvents();
        }
        std::cout << "Engine Stop Render" << std::endl;
        glfwTerminate();
        std::cout << "Engine Terminate" << std::endl;
    }
}