#version 320 es
precision highp float;

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D mouseTexture;
uniform vec2 resolution;
uniform vec3 mouse;

void main()
{
    vec2 mouseUV = mouse.xy / resolution.xy;
    float mouseDist = abs(distance(TexCoords, mouseUV));
    vec4 lastColor = texture(mouseTexture, TexCoords);

    if (mouseDist < 0.15 && mouse.z > 0.0) {
        FragColor = mix(lastColor, vec4(1.0), 0.1);
    } else {
        FragColor = mix(lastColor, vec4(0.0), 0.1);
    }
}
