out vec2 TexCoords; // texcoords are in the normalized [0,1] range for the viewport-filling quad part of the triangle

void main() {
    float x = float(((uint(gl_VertexID) + 2u) / 3u)%2u);
    float y = float(((uint(gl_VertexID) + 1u) / 3u)%2u);
    gl_Position = vec4(-1.0f + x*2.0f, -1.0f+y*2.0f, 0.0f, 1.0f);
    TexCoords = vec2(x, y);
}
