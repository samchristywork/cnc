#version 330

in vec3 transformedNormal;
in vec3 originalNormal;
out vec4 outputColor;

void main() {
  vec3 color = originalNormal;
  float lighting = abs(dot(transformedNormal, vec3(0, 0, -1)));
  outputColor = vec4(color * lighting, 1.0f);
}
