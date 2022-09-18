#version 330

in vec3 position;
in vec3 normal;

out vec3 transformedNormal;
out vec3 originalNormal;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main() {
  gl_Position = projection * view * model * vec4(position, 1.0);
  mat3 normalMatrix = transpose(inverse(mat3(view * model)));
  transformedNormal = normalMatrix * normal;
  originalNormal = abs(normal);
}
