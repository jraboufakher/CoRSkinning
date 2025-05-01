#version 330 core

in  vec3 vNormal;
out vec4 FragColor;

void main(){
    vec3 light = normalize(vec3(1,1,1));
    float diff = max(dot(normalize(vNormal), light), 0.0);
    vec3 base  = vec3(0.8,0.8,0.8);
    FragColor  = vec4(base * diff + vec3(0.2), 1.0);
}
