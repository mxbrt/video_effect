#version 300 es
precision highp float;

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D movieTexture;
uniform sampler2D effectTexture;
uniform vec2 resolution;
uniform float pixelization;
uniform int inputDebug;

void main()
{
    vec4 intensity = texture(effectTexture, TexCoords);
    if (intensity.x > 0.9) {
        FragColor = texture(movieTexture, TexCoords);
    } else {
        vec2 pixelFactor = resolution / (pixelization - pixelization * intensity.x);
        vec2 coord = round(TexCoords * pixelFactor) / pixelFactor;
        FragColor = texture(movieTexture, coord);
    }
    if (inputDebug == 1) {
        FragColor = vec4(intensity.xy, 0.0, 1.0);
    }
}
