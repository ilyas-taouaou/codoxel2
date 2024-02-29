#version 460

layout(location = 0) out vec4 outColor;
layout(binding = 0) uniform sampler2D texSampler;
layout(location = 1) in vec2 fragTexCoord;

void main() {
    outColor = texture(texSampler, fragTexCoord);
}