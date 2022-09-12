precision mediump float;

#define PI 3.1415926535897932384626433832795

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D mpvTexture;
uniform sampler2D effectTexture;
uniform sampler2D simplexNoiseTexture;
uniform sampler2D voronoiNoise;
uniform sampler2D lastFrame;
uniform vec2 resolution;
uniform float amount;
uniform float time;

#ifdef Swirl
void effect(float intensity)
{
    float x = log((1.0 - amount / 100.0));
    float offset = texture(simplexNoiseTexture, TexCoords * x).r * 0.5;
    offset = mix(offset, 0.0, intensity);
    FragColor = texture(mpvTexture, TexCoords + offset);
}
#endif

#ifdef Voronoi
precision highp float;
void effect(float intensity) {
    intensity += (100.0 - amount) / 100.0;
    float tileIdx = floor((1.0 - intensity) * 15.0);
    float mipmapLevel = tileIdx / 6.0;
    vec2 tileCoord = vec2(floor(tileIdx / 4.0), mod(tileIdx, 4.0));
    if (intensity < 0.99) {
        vec2 moviePoint = texelFetch(voronoiNoise, ivec2((TexCoords + tileCoord) * resolution), 0).rg;
        vec4 color = textureLod(mpvTexture, moviePoint, mipmapLevel);
        vec4 lastColor = texture(lastFrame, moviePoint);
        FragColor = mix(lastColor, color, 0.025 + intensity);
    } else {
        FragColor = texture(mpvTexture, TexCoords);
    }
}
#endif

#ifdef Blur
// FIXME: optimize with horizontal & vertical blur render passes
void effect(float intensity)
{
    float invAspect = resolution.y / resolution.x;
    vec4 color = vec4(0.0);
    float size = 1.0;
    for (float i = -size; i <= size; ++i) {
        for (float j = -size; j <= size; ++j) {
            vec2 offset = vec2(i,j) * vec2(amount) / resolution;
            offset = mix(offset, vec2(0.0), vec2(intensity));
            color += texture(mpvTexture, TexCoords + offset);
        }
    }
    FragColor = color / pow(size * 2.0 + 1.0, 2.0);
}
#endif

#ifdef Pixel
void effect(float intensity)
{
    if (intensity >= 1.0) {
        FragColor = texture(mpvTexture, TexCoords);
    } else {
        vec2 pixelFactor = resolution / (amount - amount * intensity);
        vec2 coord = round(TexCoords * pixelFactor) / pixelFactor;
        FragColor = texture(mpvTexture, coord);
    }
}
#endif

#ifdef Glitch
vec2 random2( vec2 p ) {
    return fract(sin(vec2(dot(p,vec2(127.1,311.7)),dot(p,vec2(269.5,183.3))))*43758.5453);
}

void effect(float intensity)
{
    //vec2 offset = floor(random2(TexCoords) * 10.0) / 100.0;
    //vec4 color = texture(mpvTexture, TexCoords + offset);
    //FragColor = color;
    float mipmapLevel = amount / 10.0;
    vec4 pixel = textureLod(mpvTexture, TexCoords, mipmapLevel);
    FragColor = pixel;
}
#endif

#ifdef Debug
void effect(float intensity)
{
    FragColor = vec4(vec3(intensity), 1.0);
}
#endif

#ifdef Brushed
void effect(float intensity)
{
    vec4 color = vec4(0.0);
    float size = 3.0;
    float step_size = (resolution.x / size) / resolution.x;
    vec2 pixel_size = 1.0 / resolution;
    float t = time * 0.0001;
    vec2 noise = vec2(texture(simplexNoiseTexture, TexCoords * 10.0 + t).r,
                      texture(simplexNoiseTexture, TexCoords * 30.0 + t).r);
    for (float i = 0.0; i < size; ++i) {
        float y = TexCoords.x - TexCoords.y;
        vec2 coords = vec2(step_size * i, y) + noise * 0.001;
        color += texture(mpvTexture, coords);
    }
    vec4 orig_color = texture(mpvTexture, TexCoords);
    vec4 blurred_color = color / size;
    FragColor = mix(blurred_color, orig_color, intensity);
}
#endif

void main() {
    float intensity = clamp(texture(effectTexture, TexCoords).x, 0.0, 1.0);
    if (amount == 0.0) {
        FragColor = texture(mpvTexture, TexCoords);
    } else {
        effect(intensity);
    }
}
