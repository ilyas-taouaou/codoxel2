#version 460
#extension GL_EXT_buffer_reference : require

layout(location = 0) in vec2 inPosition;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    gl_Position = vec4(inPosition, 0.0, 1.0);
    fragTexCoord = inPosition * 0.5 + 0.5;
}