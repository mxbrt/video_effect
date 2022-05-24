#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D movieTexture;

void main()
{
    float pixelationFactor = 50.0;
    vec2 resolution = vec2(1920, 1080);

    vec2 coord = (gl_FragCoord.xy - mod(gl_FragCoord.xy, pixelationFactor)) / resolution;
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
