#version 320 es
precision highp float;

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D mouseTexture;
uniform vec2 resolution;
uniform vec3 mouse;
uniform float mouseRadius;
uniform float mouseFadeIn;
uniform float mouseFadeOut;

void main()
{
    vec2 aspectRatio = vec2(resolution.x / resolution.y, 1.0);
    vec2 mouseUV = (mouse.xy / resolution.xy) * aspectRatio;
    vec2 texUV = TexCoords * aspectRatio;
    float mouseDist = abs(distance(texUV, mouseUV));
    vec4 lastColor = texture(mouseTexture, TexCoords);

    if (mouseDist < mouseRadius && mouse.z > 0.0) {
        FragColor = mix(lastColor, vec4(1.1), mouseFadeIn * 0.2);
    } else {
        FragColor = mix(lastColor, vec4(-0.1), mouseFadeOut * 0.2);
    }
}
