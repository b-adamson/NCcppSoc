#version 450

// IN
layout (location = 0) in vec3 fragPosWorld;
layout (location = 1) in vec3 fragNormalWorld;
layout (location = 2) in vec3 fragTangentWorld;
layout (location = 3) in vec3 fragBitangentWorld;
layout (location = 4) in vec2 fragTexCoord;
layout (location = 5) in vec4 fragPosLight;

// OUT
layout (location = 0) out vec4 outColour;

const uint FS_UNSPECIFIED = 4294967295;

struct PointLight {
    vec4 position;
    vec4 colour;
};

struct Material {
    vec4 diffuseColor;
    vec4 specularColor;
    float shininess;
    uint diffuseTextureIndex;
    uint specularTextureIndex;
    uint normalTextureIndex;
};

struct ObjFaceData {
	uint faceID;
	float saturation;
	float contrast;
	vec2 uvOffset;
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

layout(std430, set = 1, binding = 0) readonly buffer MaterialsSSBO {
	ObjFaceData objFaceData[65536];
	Material materials[200];
} Mssbo;
layout(set = 1, binding = 1) uniform sampler2D textures[500];

layout(set = 2, binding = 0) uniform sampler2DShadow shadowMap;

layout(push_constant) uniform Push {
	uint objectID;
	uint offset; // to index into face material ids
} push;

void main() {
	const ObjFaceData faceData = Mssbo.objFaceData[gl_PrimitiveID + push.offset];
	const uint materialIndex = faceData.faceID;
	const uint diffuseIndex = Mssbo.materials[materialIndex].diffuseTextureIndex;
	const uint normalIndex = Mssbo.materials[materialIndex].normalTextureIndex;
	const uint specularIndex = Mssbo.materials[materialIndex].specularTextureIndex;

	const vec2 offsetTexCoord = fragTexCoord + faceData.uvOffset;

	const vec3 lightDir = -vec3(ubo.lightView[0][2],
								ubo.lightView[1][2],
								ubo.lightView[2][2]);

	// Check for normal map
	vec3 surfaceNormal;
	if (normalIndex != FS_UNSPECIFIED) {
		mat3 TBN = mat3(fragTangentWorld, fragBitangentWorld, fragNormalWorld);
		vec3 normalMap = texture(textures[normalIndex], offsetTexCoord).rgb;
		normalMap = normalMap * 2.0 - 1.0;
		surfaceNormal = normalize(TBN * normalMap);
	} else {
		surfaceNormal = fragNormalWorld;}

	// Check for specular map
	vec3 specularMap;
	if (specularIndex != FS_UNSPECIFIED) {
		specularMap = texture(textures[specularIndex], offsetTexCoord).rgb;
	} else {
		specularMap = Mssbo.materials[materialIndex].specularColor.rgb;
	}

	// Check for diffuse map
	vec4 diffuseMap;
	if (diffuseIndex != FS_UNSPECIFIED) {
		diffuseMap = texture(textures[diffuseIndex], offsetTexCoord);
	} else {
		diffuseMap = Mssbo.materials[materialIndex].diffuseColor;
	}

	// Apply shadows
	const float constBias = .01f;
	vec3 shadowMapcoords = (fragPosLight.xyz / fragPosLight.w);
	shadowMapcoords.xy = shadowMapcoords.xy * 0.5f + 0.5f;
	float bias = constBias + 0.05 * max(0.0, dot(surfaceNormal, lightDir));
	shadowMapcoords.z -= bias;
	float outShadow = texture(shadowMap, shadowMapcoords);
	if (shadowMapcoords.x < 0.0 || shadowMapcoords.x > 1.0 ||
		shadowMapcoords.y < 0.0 || shadowMapcoords.y > 1.0 ||
		shadowMapcoords.z < 0.0 || shadowMapcoords.z > 1.0) {
		outShadow = 1.0;
	}

	// Apply ambient and directional light
	vec3 diffuseLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
	diffuseLight += outShadow * ubo.mainLightColour.xyz * ubo.mainLightColour.w * 
		max(dot(surfaceNormal, lightDir), 0);
	vec3 specularLight = vec3(0.0);
	vec3 cameraPosWorld = ubo.invView[3].xyz;

	// Apply light from point lights
	vec3 viewDirection = normalize(cameraPosWorld - fragPosWorld);
	for (uint i = 0; i < ubo.pointLightCount; ++i) {
		PointLight light = ubo.pointLights[i];
		vec3 directionToLight = light.position.xyz - fragPosWorld;
		float attenuation = 1.0 / dot(directionToLight, directionToLight);
		directionToLight = normalize(directionToLight);
		vec3 intensity = light.colour.xyz * light.colour.w * attenuation;
		diffuseLight += intensity * max(dot(surfaceNormal, directionToLight), 0);

		// specular lighting
		vec3 halfAngle = normalize(directionToLight + viewDirection);
		float blinnTerm = dot(surfaceNormal, halfAngle);
		blinnTerm = clamp(blinnTerm, 0, 1);
		blinnTerm = pow(blinnTerm, Mssbo.materials[materialIndex].shininess);
		specularLight += intensity * blinnTerm * specularMap;
	}

	vec4 colour = vec4(vec4((diffuseLight + specularLight), 1.0) * diffuseMap);

	// apply contrast
	colour = clamp((colour - 0.5) * faceData.contrast + 0.5, 0.0, 1.0);

	// apply saturation
    vec3 grayscale = vec3(dot(colour.rgb, vec3(0.299, 0.587, 0.114)));
	colour = vec4(vec3(mix(grayscale, colour.rgb, faceData.saturation)), 1.0);

	outColour = colour;
}
