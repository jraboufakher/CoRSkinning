#version 330 core
in vec2  vUV;
in vec3  vNormal;

out vec4 oColor;

void main(){
  float L = max(dot(normalize(vNormal),vec3(0,0,1)),0.1);
  oColor = vec4(vec3(1,0.8,0.6)*L,1);
}
