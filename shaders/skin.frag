#version 330 core
uniform sampler2D uDiffuse;
in  vec2 vUV;
out vec4 FragColor;

void main(){
    FragColor = texture(uDiffuse, vUV);
}
