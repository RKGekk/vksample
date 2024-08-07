#version 450

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoords;

layout(location = 0) out vec4 outColor;

void main() {
    //outColor = vec4(fragTexCoords, 0.0f, 1.0f);
    outColor = texture(texSampler, fragTexCoords);
}
