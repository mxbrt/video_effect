precision mediump float;

out vec4 FragColor;

in vec2 TexCoords;

#define N_FINGERS 3
#define TARGET_DELTA 16.6666

uniform sampler2D effectTexture;
uniform vec2 resolution;
uniform vec3[N_FINGERS] fingers;
uniform float fingerRadius;
uniform float effectFadeIn;
uniform int reset;
uniform float delta;

void main()
{
    vec2 aspectRatio = vec2(resolution.x / resolution.y, 1.0);
    vec2 texUV = TexCoords * aspectRatio;

    float fingerDist = 10000.0;
    for (int i = 0; i < N_FINGERS; i++) {
        vec3 finger = fingers[i];
        vec3 fingerUV = vec3((finger.xy / resolution.xy) * aspectRatio, finger.z);
        vec3 diff = vec3(texUV, 0.0) - fingerUV;
        fingerDist = min(fingerDist, abs(dot(diff, diff)));
    }

    if (reset > 0) {
        FragColor.r = 0.0;
    } else {
        float lastIntensity = texture(effectTexture, TexCoords).r;
        float dist = sqrt(fingerDist);
        if (dist < fingerRadius) {
#ifdef Voronoi
            highp float effectFade = effectFadeIn;
#else
            highp float effectFade = effectFadeIn * (fingerRadius - dist);
#endif
            effectFade *= (delta / TARGET_DELTA);
            FragColor.r = clamp(lastIntensity + effectFade, 0.0, 3.0);
        } else {
            FragColor.r = lastIntensity;
        }
    }

}
