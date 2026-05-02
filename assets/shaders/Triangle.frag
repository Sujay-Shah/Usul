#version 450

layout(location = 0) out vec4 outColor;
layout(location = 0) in vec3 fragColor;

layout(push_constant) uniform PushConstants {
    vec3 color;
} pc;

void main() {
    outColor = vec4(fragColor * pc.color, 1.0);
}