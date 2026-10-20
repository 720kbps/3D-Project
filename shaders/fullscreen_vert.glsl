#version 410

// One triangle that covers the whole screen, generated from the vertex index, so no vertex buffer is needed.
// Vertex 0, 1, 2 -> (-1,-1), (3,-1), (-1,3). The part outside [-1, 1] is clipped away, and unlike two
// triangles forming a quad there is no diagonal seam where pixels get shaded twice.
out vec2 texCoord;

void main()
{
    vec2 position = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2) * 2.0 - 1.0;
    texCoord = position * 0.5 + 0.5;
    gl_Position = vec4(position, 0.0, 1.0);
}
