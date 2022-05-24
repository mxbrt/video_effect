#version 320 es
precision highp float;

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D movieTexture;
uniform sampler2D mouseTexture;
uniform vec2 resolution;

void main()
{
    vec4 mouseVal = texture(mouseTexture, TexCoords);
    if (mouseVal.x > 0.9) {
        FragColor = texture(movieTexture, TexCoords);
    } else {
        float blockiness = 28.0;
        vec2 pixelFactor = resolution * (1.0 / (blockiness - blockiness * mouseVal.x));
        vec2 coord = round(TexCoords * pixelFactor) / pixelFactor;
        vec4 col = vec4(0);
        for (int i = -1; i < 2; i++) {
            for (int j = -1; j < 2; j++) {
                vec2 offset = vec2(i,j) / resolution;
                col += texture(movieTexture, coord + offset);
            }
        }
        col /= vec4(9);
        FragColor = col;
    }
//#define DEBUG_MOUSE
#ifdef DEBUG_MOUSE
    FragColor = vec4(vec3(mouseVal.x), 1.0);
#endif
}
