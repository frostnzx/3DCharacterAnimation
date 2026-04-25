#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D texture_diffuse1;
uniform bool useSolidColor;
uniform vec3 solidColor;

void main()
{    
    if (useSolidColor) {
        FragColor = vec4(solidColor, 1.0);
    } else {
        FragColor = texture(texture_diffuse1, TexCoords);
    }
}