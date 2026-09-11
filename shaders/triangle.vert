#version 450

struct vertex {
    vec4 position;
    vec4 color;
};

layout(std430, binding = 0) readonly buffer VertexBuffer {
    vertex vertices[];
};

layout(std430, binding = 1) readonly buffer IndexBuffer {
    uint indices[];
};

layout(location = 0) out vec4 outColor;

void main()
{
    uint index = indices[gl_VertexIndex];

    vertex v = vertices[index];

    gl_Position = v.position;

    outColor = v.color;
}