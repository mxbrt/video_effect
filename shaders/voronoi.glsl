precision highp float;

out vec4 FragColor;
in vec2 TexCoords;

vec2 random2( vec2 p ) {
    return fract(sin(vec2(dot(p,vec2(127.1,311.7)),dot(p,vec2(269.5,183.3))))*43758.5453);
}

void main() {
    vec2 resolution = vec2(7680, 4320);
    vec2 mpvResolution = vec2(1920, 1080);

    vec2 tileCoord = floor((gl_FragCoord.xy / resolution) * 4.0);
    float tileIdx = tileCoord.x * 4.0 + tileCoord.y;
    vec2 tileSt = fract((gl_FragCoord.xy / resolution) * 4.0);
    float amount = pow(1.36, tileIdx);
    vec3 color = vec3(.0);

    // Scale
    vec2 scale = mpvResolution / amount;
    vec2 st = tileSt * scale;

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
    FragColor.rg = moviePoint;
}
