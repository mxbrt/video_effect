precision mediump float;

out vec4 FragColor;

in vec2 TexCoords;

#define N_FINGERS 3

uniform sampler2D effectTexture;
uniform vec2 resolution;
uniform vec3[N_FINGERS] fingers;
uniform float fingerRadius;
uniform float effectFadeIn;
uniform float effectFadeOut;

void main()
{
    vec2 aspectRatio = vec2(resolution.x / resolution.y, 1.0);
    vec2 texUV = TexCoords * aspectRatio;

    float fingerDist = 10000.0;
    for (int i = 0; i < N_FINGERS; i++) {
        vec3 finger = fingers[i];
        vec2 fingerUV = (finger.xy / resolution.xy) * aspectRatio;
        fingerDist = min(fingerDist, abs(distance(
                        vec3(texUV, 0.0),
                        vec3(fingerUV, finger[2]))));
    }

    float lastIntensity = texture(effectTexture, TexCoords).r;
    float effectFade = fingerDist < fingerRadius ?
        effectFadeIn * 0.2 :
        -effectFadeOut * 0.1;
    FragColor.r = clamp(lastIntensity + effectFade, 0.0, 3.0);
}
