#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;      // Location 2 for UVs
layout (location = 3) in mat4 instanceMatrix;  // Occupies locations 3, 4, 5, 6

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform mat4 view;
uniform mat4 projection;

void main()
{
    vec4 worldPosition = instanceMatrix * vec4(aPos, 1.0);
    FragPos = vec3(worldPosition);
    
    Normal = mat3(transpose(inverse(instanceMatrix))) * aNormal;
    TexCoords = aTexCoords;
    
    gl_Position = projection * view * worldPosition;
}