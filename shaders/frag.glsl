precision mediump float;

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D movieTexture;
uniform sampler2D effectTexture;
uniform vec2 resolution;
uniform float amount;
uniform float time;

#ifdef Swirl
// Some useful functions
precision highp float;
vec3 mod289(vec3 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec2 mod289(vec2 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec3 permute(vec3 x) { return mod289(((x*34.0)+1.0)*x); }

//
// Description : GLSL 2D simplex noise function
//      Author : Ian McEwan, Ashima Arts
//  Maintainer : ijm
//     Lastmod : 20110822 (ijm)
//     License :
//  Copyright (C) 2011 Ashima Arts. All rights reserved.
//  Distributed under the MIT License. See LICENSE file.
//  https://github.com/ashima/webgl-noise
//
float snoise(vec2 v) {

    // Precompute values for skewed triangular grid
    const vec4 C = vec4(0.211324865405187,
                        // (3.0-sqrt(3.0))/6.0
                        0.366025403784439,
                        // 0.5*(sqrt(3.0)-1.0)
                        -0.577350269189626,
                        // -1.0 + 2.0 * C.x
                        0.024390243902439);
                        // 1.0 / 41.0

    // First corner (x0)
    vec2 i  = floor(v + dot(v, C.yy));
    vec2 x0 = v - i + dot(i, C.xx);

    // Other two corners (x1, x2)
    vec2 i1 = vec2(0.0);
    i1 = (x0.x > x0.y)? vec2(1.0, 0.0):vec2(0.0, 1.0);
    vec2 x1 = x0.xy + C.xx - i1;
    vec2 x2 = x0.xy + C.zz;

    // Do some permutations to avoid
    // truncation effects in permutation
    i = mod289(i);
    vec3 p = permute(
            permute( i.y + vec3(0.0, i1.y, 1.0))
                + i.x + vec3(0.0, i1.x, 1.0 ));

    vec3 m = max(0.5 - vec3(
                        dot(x0,x0),
                        dot(x1,x1),
                        dot(x2,x2)
                        ), 0.0);

    m = m*m ;
    m = m*m ;

    // Gradients:
    //  41 pts uniformly over a line, mapped onto a diamond
    //  The ring size 17*17 = 289 is close to a multiple
    //      of 41 (41*7 = 287)

    vec3 x = 2.0 * fract(p * C.www) - 1.0;
    vec3 h = abs(x) - 0.5;
    vec3 ox = floor(x + 0.5);
    vec3 a0 = x - ox;

    // Normalise gradients implicitly by scaling m
    // Approximation of: m *= inversesqrt(a0*a0 + h*h);
    m *= 1.79284291400159 - 0.85373472095314 * (a0*a0+h*h);

    // Compute final noise value at P
    vec3 g = vec3(0.0);
    g.x  = a0.x  * x0.x  + h.x  * x0.y;
    g.yz = a0.yz * vec2(x1.x,x2.x) + h.yz * vec2(x1.y,x2.y);
    return 130.0 * dot(m, g);
}

void effect(float intensity)
{
    float offset2 = snoise(vec2(time * 0.01, time * 0.01));
    float offset = snoise(TexCoords * log(amount / 2.0)) * .5 + 0.5;

    offset = mix(offset + offset2, 0.0, intensity);
    FragColor = texture(movieTexture, TexCoords + offset);
}
precision mediump float;
#endif

#ifdef Voronoi
vec2 random2( vec2 p ) {
    return fract(sin(vec2(dot(p,vec2(127.1,311.7)),dot(p,vec2(269.5,183.3))))*43758.5453);
}

void effect(float intensity) {
    vec2 st = TexCoords;
    vec3 color = vec3(.0);

    // Scale
    float scale = 1.0 + (100.0 - amount) + 1000.0 * intensity;
    st *= scale;

    // Tile the space
    vec2 iSt = floor(st);
    vec2 fSt = fract(st);

    float mDist = 10.;  // minimum distance
    vec2 mPoint;        // minimum point
    vec2 mCell;

    for (int j=-1; j<=1; j++ ) {
        for (int i=-1; i<=1; i++ ) {
            vec2 neighbor = vec2(float(i),float(j));
            vec2 point = random2(iSt + neighbor);
            point = 0.5 + 0.5*sin(time * 0.1 + 6.2831*point);
            vec2 diff = neighbor + point - fSt;
            float dist = length(diff);

            if( dist < mDist ) {
                mCell = iSt + neighbor;
                mDist = dist;
                mPoint = point;
            }
        }
    }

    // Assign a color using the closest point position
    vec2 moviePoint = (mCell + mPoint) / scale;
    color += texture(movieTexture, moviePoint).rgb;

    FragColor = vec4(color,1.0);
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
            color += texture(movieTexture, TexCoords + offset);
        }
    }
    FragColor = color / pow(size * 2.0 + 1.0, 2.0);
}
#endif

#ifdef Pixel
void effect(float intensity)
{
    if (intensity >= 1.0) {
        FragColor = texture(movieTexture, TexCoords);
    } else {
        vec2 pixelFactor = resolution / (amount - amount * intensity);
        vec2 coord = round(TexCoords * pixelFactor) / pixelFactor;
        FragColor = texture(movieTexture, coord);
    }
}
#endif

#ifdef Debug
void effect(float intensity)
{
    FragColor = vec4(vec3(intensity), 1.0);
}
#endif


void main() {
    float intensity = clamp(texture(effectTexture, TexCoords).x, 0.0, 1.0);
    if (amount == 0.0) {
        FragColor = texture(movieTexture, TexCoords);
    } else {
        effect(intensity);
    }
}
