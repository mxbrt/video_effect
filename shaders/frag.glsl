precision mediump float;

#define PI 3.1415926535897932384626433832795

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D mpvTexture;
uniform sampler2D effectTexture;
uniform sampler2D simplexNoiseTexture;
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
vec2 random2( vec2 p ) {
    return fract(sin(vec2(dot(p,vec2(127.1,311.7)),dot(p,vec2(269.5,183.3))))*43758.5453);
}
precision mediump float;

void effect(float intensity) {
    vec2 st = TexCoords;
    vec3 color = vec3(.0);

    // Scale
    //float scale = 1.0 + ((100.0 - amount) + 100.0 * intensity);
    float step_intensity = floor(intensity * 100.0) / 100.0;
    float scale_factor = amount - (amount * step_intensity) + 1.0;
    vec2 scale = resolution / scale_factor;
    st *= scale;

    // Tile the space
    vec2 iSt = floor(st);
    vec2 fSt = fract(st);

    float mDist = 10.;  // minimum distance
    vec2 mPoint;        // minimum point
    vec2 mCell;

    for (float j=-1.0; j<=1.0; j++ ) {
        for (float i=-1.0; i<=1.0; i++ ) {
            vec2 neighbor = vec2(i,j);
            vec2 point = random2(iSt + neighbor);
            point = 0.5 + 0.5*sin(6.2831*point);
            vec2 diff = neighbor + point - fSt;
            float dist_squared = dot(diff, diff);

            if( dist_squared < mDist ) {
                mCell = iSt + neighbor;
                mDist = dist_squared;
                mPoint = point;
            }
        }
    }

    // Assign a color using the closest point position
    vec2 moviePoint = (mCell + mPoint) / scale;
    color += texture(mpvTexture, moviePoint).rgb;

    vec4 lastColor = texture(lastFrame, moviePoint);
    FragColor = vec4(mix(lastColor.rgb, color, 0.025 + intensity),1.0);
}
#endif

#ifdef Glitch
void effect(float intensity) {
    vec4 lastColor = texture(lastFrame, TexCoords);
    vec4 color = texture(mpvTexture, TexCoords);

    FragColor = color;
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

#ifdef Debug
void effect(float intensity)
{
    FragColor = vec4(vec3(intensity), 1.0);
    //FragColor = texture(simplexNoiseTexture, TexCoords);
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
