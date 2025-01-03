#version 450

layout(location = 0) in vec3 vertPosition;
layout(location = 1) in vec3 vertNormal;
layout(location = 2) in vec3 vertTangent;
layout(location = 3) in vec3 vertBitangent;
layout(location = 4) in vec2 vertTexCoord;

layout (location = 5)  in vec4 instanceModelMatColumn1;
layout (location = 6)  in vec4 instanceModelMatColumn2;
layout (location = 7)  in vec4 instanceModelMatColumn3;
layout (location = 8)  in vec4 instanceModelMatColumn4;
layout (location = 9)  in vec3 instanceNormalMatColumn1;
layout (location = 10) in vec3 instanceNormalMatColumn2;
layout (location = 11) in vec3 instanceNormalMatColumn3;

layout (location = 0) out vec3 fragPosWorld;
layout (location = 1) out vec3 fragNormalWorld;
layout (location = 2) out vec3 fragTangentWorld;
layout (location = 3) out vec3 fragBitangentWorld;
layout (location = 4) out vec2 fragTexCoord;
layout (location = 5) out vec4 fragPosLight;

struct PointLight {
	vec4 vertPosition;
	vec4 color;
};

layout(set = 0, binding = 0) uniform GlobalUBO {
	mat4 projection;
	mat4 view;
	mat4 invView;
	vec4 ambientLightColor;
	vec4 mainLightColour;
	mat4 lightProjection;
	mat4 lightView;
	PointLight pointLights[30];
	uint pointLightCount;
} ubo;

void main() {
	mat3 normalMatrix = mat3(
		instanceNormalMatColumn1,
		instanceNormalMatColumn2,
		instanceNormalMatColumn3
		);

	const mat4 modelMatrix = mat4(
		instanceModelMatColumn1, 
		instanceModelMatColumn2, 
		instanceModelMatColumn3, 
		instanceModelMatColumn4
		);

	vec4 vertPositionWorld = modelMatrix * vec4(vertPosition, 1.0);
	gl_Position = ubo.projection * ubo.view * vertPositionWorld;

	fragNormalWorld = normalize(normalMatrix * vertNormal);
	fragTangentWorld = normalize(normalMatrix * vertTangent);
	fragBitangentWorld = normalize(normalMatrix * vertBitangent);
	fragPosWorld = vertPositionWorld.xyz;
	fragTexCoord = vertTexCoord;
	fragPosLight = ubo.lightProjection * ubo.lightView * vertPositionWorld;
}
