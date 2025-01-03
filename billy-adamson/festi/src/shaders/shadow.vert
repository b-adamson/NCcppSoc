#version 450

layout(location = 0) in vec3 vertPosition;

layout(location = 1)  in vec4 instanceModelMatColumn1;
layout(location = 2)  in vec4 instanceModelMatColumn2;
layout(location = 3)  in vec4 instanceModelMatColumn3;
layout(location = 4)  in vec4 instanceModelMatColumn4;

layout(push_constant) uniform Push {
    mat4 lightSpace;
} push;

void main() {
    const mat4 modelMatrix = mat4(
        instanceModelMatColumn1, 
        instanceModelMatColumn2, 
        instanceModelMatColumn3, 
        instanceModelMatColumn4);

    vec4 lightSpacePosition = push.lightSpace * modelMatrix * vec4(vertPosition, 1.0);
    gl_Position = lightSpacePosition;
}
