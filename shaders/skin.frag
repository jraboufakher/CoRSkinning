#version 330 core
uniform sampler2D uDiffuse;
in  vec2 vUV;
out vec4 FragColor;

void main(){
    vec2 flippedUV = vec2(vUV.x, 1.0 - vUV.y);
    FragColor = texture(uDiffuse, flippedUV);
    //FragColor = vec4(0.0, 1.0, 0.0, 1.0);
}