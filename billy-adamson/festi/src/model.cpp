#include "model.hpp"

#include "utils.hpp"

// libs
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp>
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <stb_image.h>

// std
#include <cassert>
#include <cstring>
#include <unordered_map>
#include <map>
#include <memory>
#include <iostream>
#include <stdexcept>
#include <algorithm>

template <>
struct std::hash<festi::Vertex> {
	size_t operator()(const festi::Vertex& vertex) const {
		size_t seed = 0;
		festi::hashCombine(
			seed, 
			vertex.position, vertex.normal, vertex.tangent,
			vertex.bitangent, vertex.uv);
		return seed;
  	}
};

namespace festi {

std::vector<uint32_t> MaterialsSSBO::offsets{};
std::unordered_map<std::string, uint32_t> FestiModel::materialNamesMap;

FestiModel::FestiModel(FestiDevice& device) : festiDevice{device} {
	static uint32_t currentID = 0;
	id = currentID++;
}

FestiModel::~FestiModel() {}

std::shared_ptr<FestiModel> FestiModel::createPointLight(FestiDevice& device, FS_ModelMap& gameObjects,
	float radius, glm::vec4 color) {

	auto gameObject = std::make_shared<FestiModel>(device);

	gameObject->transform.scale.x = radius;
	gameObject->pointLight = std::make_unique<PointLightComponent>();
	gameObject->pointLight->color = color;
	addObjectToSceneWithName(gameObject, gameObjects);

	return gameObject;
}

std::shared_ptr<FestiModel> FestiModel::createModelFromFile(
	FestiDevice& device, 
	FestiMaterials& festiMaterials,
	FS_ModelMap& gameObjects,
	const std::string& filepath,
	const std::string& mtlDirPath,
	const std::string& imgDirPath) {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str(), mtlDirPath.c_str())) {
		throw std::runtime_error(warn + err); 
	}

	std::vector<ObjFaceData> faceData{};

	auto gameObject = std::make_shared<FestiModel>(device);

	std::unordered_map<Vertex, uint32_t> uniqueVertices{};
	for (const auto& shape : shapes) {
		for (size_t i = 0; i < shape.mesh.indices.size(); i++) {
		const auto& index = shape.mesh.indices[i];
		if (shape.mesh.num_face_vertices[(size_t)(i / 3)] != 3) {
			throw std::runtime_error("Mesh is not triangulated");}

		Vertex vertex{};
		if (index.vertex_index >= 0) {
			vertex.position = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2],
			};
		}

		if (index.normal_index >= 0) {
			vertex.normal = {
				attrib.normals[3 * index.normal_index + 0],
				attrib.normals[3 * index.normal_index + 1],
				attrib.normals[3 * index.normal_index + 2],
			};
		}

		if (index.texcoord_index >= 0) {
			vertex.uv = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				attrib.texcoords[2 * index.texcoord_index + 1],
			};
		}

		if (uniqueVertices.count(vertex) == 0) {
			uniqueVertices[vertex] = static_cast<uint32_t>(gameObject->vertices.size());
			gameObject->vertices.push_back(vertex);
		}
		gameObject->indices.push_back(uniqueVertices[vertex]);
		}
		FestiModel::setTangentsBitangentsShapeArea(gameObject->vertices, gameObject->indices, gameObject->shapeArea);
		faceData.resize(shape.mesh.material_ids.size());
		for (size_t i = 0; i < materials.size(); i++) {
			uint32_t id;
			auto& mat = materials[i];
			auto it = materialNamesMap.find(mat.name);
			// does this obj contain a material we dont have?
			if (it != materialNamesMap.end()) {
				// if so, find material id
				id = it->second;
			}
			else {
				// if no, create a new material, populate it, and set id to that material
				id = (uint32_t)materialNamesMap.size();

				Material newMaterial;
				newMaterial.diffuseTextureIndex = festiMaterials.appendTextureMap(mat, imgDirPath, FS_ImageMapFlags::DIFFUSE);
				newMaterial.normalTextureIndex = festiMaterials.appendTextureMap(mat, imgDirPath, FS_ImageMapFlags::NORMAL);
				newMaterial.specularTextureIndex = festiMaterials.appendTextureMap(mat, imgDirPath, FS_ImageMapFlags::SPECULAR);
				newMaterial.shininess = mat.shininess;
				newMaterial.diffuseColor = {mat.diffuse[0], mat.diffuse[1], mat.diffuse[2], 1.f};
				newMaterial.specularColor = {mat.specular[0], mat.specular[1], mat.specular[2], 1.f};
				materialNamesMap[mat.name] = id;
				festiMaterials.Mssbo.materials[id] = newMaterial;
			}

			for (size_t j = 0; j < shape.mesh.material_ids.size(); j++) {
				if (shape.mesh.material_ids[j] == (int)i) {
					ObjFaceData objFaceData{};
					objFaceData.materialID = id;
					faceData[j] = objFaceData;
				}
			}
		}
	}

	gameObject->createVertexBuffer(gameObject->vertices);
	gameObject->createIndexBuffer(gameObject->indices);
	gameObject->faceData.resize(faceData.size());
	gameObject->faceData = faceData;
	addObjectToSceneWithName(gameObject, gameObjects);
	return gameObject;
}

void FestiModel::setTangentsBitangentsShapeArea(std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, float& area) {
    for (size_t i = 0; i < indices.size(); i += 3) {

        Vertex& v0 = vertices[indices[i + 0]];
        Vertex& v1 = vertices[indices[i + 1]];
        Vertex& v2 = vertices[indices[i + 2]];

        glm::vec3 deltaPos1 = v1.position - v0.position;
        glm::vec3 deltaPos2 = v2.position - v0.position;
        glm::vec2 deltaUV1 = v1.uv - v0.uv;
        glm::vec2 deltaUV2 = v2.uv - v0.uv;
		
        float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
        glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
        glm::vec3 bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;

        v0.tangent += tangent;
        v1.tangent += tangent;
        v2.tangent += tangent;

        v0.bitangent += bitangent;
        v1.bitangent += bitangent;
        v2.bitangent += bitangent;

		area += glm::length(glm::cross(v1.position - v0.position, v2.position - v0.position));
    }

    for (auto& vertex : vertices) {
        vertex.tangent = glm::normalize(vertex.tangent);
        vertex.bitangent = glm::normalize(vertex.bitangent);
    }
}

std::vector<Instance> FestiModel::getTransformsToRndPointsOnSurface(const AsInstanceData& keyframe, Transform& childTransform) {

	auto gen = std::mt19937(keyframe.seed);
	std::vector<Instance> instanceMatrices;
	std::uniform_real_distribution<float> dis(0.0, 1.0);

	const glm::mat4& parentModelMatrix = transform.getModelMatrix();
	const glm::mat3& childNormalMatrix = childTransform.getNormalMatrix();
	const glm::vec3 up = glm::normalize(childNormalMatrix * glm::vec3(0.f, 0.f, 1.f));

	for (size_t layer = 0; layer < keyframe.layers; ++layer) {
		for (size_t i = 0; i < indices.size(); i += 3) {

			glm::vec3 v0 = parentModelMatrix * glm::vec4(vertices[indices[i	   ]].position, 1.0f);
			glm::vec3 v1 = parentModelMatrix * glm::vec4(vertices[indices[i + 1]].position, 1.0f);
			glm::vec3 v2 = parentModelMatrix * glm::vec4(vertices[indices[i + 2]].position, 1.0f);

			const glm::vec3 norm = glm::cross(v1 - v0, v2 - v0);
			const float triangleArea = glm::length(norm) * .5f;
			const glm::vec3 triangleNormal = glm::normalize(norm);
			const glm::vec3 axis = glm::cross(up, triangleNormal);

			// Raise vertices of parent up to correct height of current layer
			const glm::vec4 h = glm::vec4(layer * keyframe.layerSeparation * triangleNormal, 1.f);
			v0 += h;
			v1 += h;
			v2 += h;

			const uint32_t numInstances = (uint32_t)(keyframe.density * triangleArea / glm::dot(transform.scale, transform.scale));
			const float randomFactor = keyframe.randomness * 1000;

			for (uint32_t j = 0; j < numInstances; ++j) {
				// Generate a random point on the triangle
				float u = std::round(dis(gen) * randomFactor) / randomFactor;
				float v = std::round(dis(gen) * randomFactor) / randomFactor;
				if (u + v > 1.0f) {
					u = 1.f - u;
					v = 1.f - v;
				}
				
				// Create initial instance matrix
				Transform instanceTransform = childTransform;
				instanceTransform.scale *= transform.scale;
				if (glm::length(axis) > glm::epsilon<float>()) {
					float dot = glm::dot(up, triangleNormal);
					if (glm::abs(dot) < glm::epsilon<float>()) dot = 0.f;
					const float angle = acos(dot);
					glm::quat quatRot = glm::angleAxis(angle, glm::normalize(axis));
					glm::quat quatTrans = quatRot;
					if (dot < 0) quatRot = glm::angleAxis(glm::two_pi<float>() - angle, glm::normalize(axis));
					instanceTransform.rotation = glm::eulerAngles(quatRot);
					instanceTransform.translation = glm::vec3(glm::toMat4(quatTrans) * glm::vec4(instanceTransform.translation, 1.0f));
				}
				instanceTransform.translation += (1.0f - u - v) * v0 + u * v1 + v * v2;

				// Create random offset matrix if available
				Transform offsets{};
				const auto& maxOff = keyframe.maxOffset;
				const auto& minOff = keyframe.minOffset;
				if (maxOff.scale != Transform{}.scale || minOff.scale != Transform{}.scale) {
					offsets.scale *= minOff.scale + dis(gen) * (maxOff.scale - minOff.scale);
				}
				if (maxOff.rotation != glm::vec3() || minOff.rotation != glm::vec3()) {
					offsets.rotation += minOff.rotation + dis(gen) * (maxOff.rotation - minOff.rotation);
				}
				if (maxOff.translation != glm::vec3() || minOff.translation != glm::vec3()) {
					offsets.translation += minOff.translation + dis(gen) * (maxOff.translation - minOff.translation); // RE-ADD BRACKETS IF ISSUE OCCURS
				}

				// Apply offsets
				glm::mat4 modelMat = instanceTransform.getModelMatrix() * offsets.getModelMatrix();
				glm::mat4 normalMat = instanceTransform.getNormalMatrix() * offsets.getNormalMatrix();
				instanceMatrices.push_back(Instance{modelMat, normalMat});
			}
		}
	}
    return instanceMatrices;
}

void FestiModel::createVertexBuffer(const std::vector<Vertex> &vertices) {
	vertexCount = static_cast<uint32_t>(vertices.size());
	hasVertexBuffer = vertexCount > 0;
	if (!hasVertexBuffer) {return;}
	assert(vertexCount >= 3 && "Vertex count must be at least 3");
	vertexBuffer = FestiBuffer::writeToLocalGPU(
		(void*)vertices.data(), 
		festiDevice, 
		sizeof(vertices[0]), 
		vertexCount, 
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

void FestiModel::createIndexBuffer(const std::vector<uint32_t> &indices) {
	indexCount = static_cast<uint32_t>(indices.size());
	hasIndexBuffer = indexCount > 0;
	if (!hasIndexBuffer) {return;}
	indexBuffer = FestiBuffer::writeToLocalGPU(
		(void*)indices.data(), 
		festiDevice, 
		sizeof(indices[0]), 
		indexCount, 
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
}

void FestiModel::createInstanceBuffer(uint32_t size) {
	assert(size > 0 && "Cannot create instance buffer with size < 1");
	if (!hasVertexBuffer) {return;}
	instanceBuffer = std::make_unique<FestiBuffer>(
		festiDevice,
		sizeof(Instance),
		size,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

void FestiModel::writeToInstanceBuffer(const std::vector<Instance>& instances) {
	instanceCount = (uint32_t)instances.size();
	instanceBuffer->writeToBuffer((void*)instances.data(), instances.size() * sizeof(instances[0]));
}

void FestiModel::setInstanceBufferSizesOnGameObjects(FS_ModelMap& gameObjects) {
	for (auto& kv : gameObjects) {
		uint32_t instanceBufferSize = 1;
		auto& obj = kv.second;
		for (auto& kv : obj->keyframes.asInstanceData) {
			auto& KFasInstanceData = kv.second;
			if (!KFasInstanceData.parentObject) {continue;}
			const auto& scale = KFasInstanceData.parentObject->transform.scale;
			uint32_t KFInstances = 
				(uint32_t)(KFasInstanceData.density * KFasInstanceData.parentObject->shapeArea / glm::dot(scale, scale));
			if (instanceBufferSize < KFInstances) {instanceBufferSize = KFInstances;}
		}
		obj->createInstanceBuffer(instanceBufferSize);
	}
}

void FestiModel::draw(VkCommandBuffer commandBuffer) {
	if (instanceBuffer == nullptr) { return; }
	if (hasIndexBuffer) {
		vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, 0, 0, 0);
	} else {
		vkCmdDraw(commandBuffer, vertexCount, instanceCount, 0, 0);
	}
}

void FestiModel::bind(VkCommandBuffer commandBuffer) {
	if (instanceBuffer == nullptr) { return; }
	VkBuffer buffers[] = {vertexBuffer->getBuffer(), instanceBuffer->getBuffer()};
	VkDeviceSize offsets[2] = {0};
	if (instanceBuffer->getBufferSize() == 0) { return; }
	vkCmdBindVertexBuffers(commandBuffer, 0, 2, buffers, offsets);

	if (hasIndexBuffer) {
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
	}
}

glm::mat4 FestiModel::Transform::getModelMatrix() {
	const float c3 = glm::cos(rotation.z);
	const float s3 = glm::sin(rotation.z);
	const float c2 = glm::cos(rotation.x);
	const float s2 = glm::sin(rotation.x);
	const float c1 = glm::cos(rotation.y);
	const float s1 = glm::sin(rotation.y);
	return glm::mat4{
		{
			scale.x * (c1 * c3 + s1 * s2 * s3),
			scale.x * (c2 * s3),
			scale.x * (c1 * s2 * s3 - c3 * s1),
			0.0f,
		},
		{
			scale.y * (c3 * s1 * s2 - c1 * s3),
			scale.y * (c2 * c3),
			scale.y * (c1 * c3 * s2 + s1 * s3),
			0.0f,
		},
		{
			scale.z * (c2 * s1),
			scale.z * (-s2),
			scale.z * (c1 * c2),
			0.0f,
		},
		{
			translation.x, translation.y, translation.z, 1.0f
		}};
}

glm::mat4 FestiModel::Transform::getNormalMatrix() {
	const glm::vec3 invScale = 1.0f / scale;
	const float c3 = glm::cos(rotation.z);
	const float s3 = glm::sin(rotation.z);
	const float c2 = glm::cos(rotation.x);
	const float s2 = glm::sin(rotation.x);
	const float c1 = glm::cos(rotation.y);
	const float s1 = glm::sin(rotation.y);
	return glm::mat4{
		{
			invScale.x * (c1 * c3 + s1 * s2 * s3),
			invScale.x * (c2 * s3),
			invScale.x * (c1 * s2 * s3 - c3 * s1),
			0.f
		},
		{
			invScale.y * (c3 * s1 * s2 - c1 * s3),
			invScale.y * (c2 * c3),
			invScale.y * (c1 * c3 * s2 + s1 * s3),
			0.f
		},
		{
			invScale.z * (c2 * s1),
			invScale.z * (-s2),
			invScale.z * (c1 * c2),
			0.f
		},
		{0.f, 0.f, 0.f, 1.f}
	};	
}

glm::vec3 FestiModel::WorldProperties::getDirectionVector() {
	return glm::normalize(glm::vec3(
		glm::cos(mainLightDirection.x) * glm::sin(mainLightDirection.y),
		-glm::sin(mainLightDirection.x),
		glm::cos(mainLightDirection.y) * glm::cos(mainLightDirection.x)));
}

void FestiModel::insertKeyframe(uint32_t frame, KeyFrameFlags flags, std::vector<uint32_t> faceIDs) {
    if (flags & FS_KEYFRAME_POS_ROT_SCALE) {
        keyframes.transforms[frame] = transform;
        keyframes.inMotion.insert(frame);
    }

    if (flags & FS_KEYFRAME_FACE_MATERIALS) {
        int numberOfFaces = getNumberOfFaces();
        if (!hasVertexBuffer) throw std::runtime_error("Cannot keyframe material data on object with no materials");
        if (std::any_of(faceIDs.begin(), faceIDs.end(), [numberOfFaces](int x) { return x >= numberOfFaces; })) {
            throw std::runtime_error("Cannot keyframe on a faceID that does not exist");
        }
        for (auto& id : faceIDs) {
            keyframes.objFaceData[id][frame] = faceData[id];
            keyframes.modifiedFaces.insert(id);
        }
    }

    if (flags & FS_KEYFRAME_POINT_LIGHT) {
        if (!pointLight) throw std::runtime_error("Cannot keyframe point-light data for non point-light");
        keyframes.pointLightData[frame] = *pointLight;
    }

    if (flags & FS_KEYFRAME_AS_INSTANCE) {
		if (!hasVertexBuffer) throw std::runtime_error("Cannot keyframe models that don't have vertices");
        keyframes.asInstanceData[frame] = asInstanceData;
    }

    if (flags & FS_KEYFRAME_WORLD) {
        if (!world) throw std::runtime_error("Cannot keyframe world data on a local object");
        keyframes.worldProperties[frame] = *world;
    }

	if (flags & FS_KEYFRAME_VISIBILITY) {
		// if (world) throw std::runtime_error("Cannot change visibility of the scene");
		keyframes.visibility[frame] = visibility;
	}

	// if (flags & FS_KEYFRAME_BUILDING) {
	// 	if (!hasVertexBuffer) throw std::runtime_error("Cannot create a building on an object with no vertices");
	// 	keyframes.buildingData[frame] = buildingData;
	// }
}

void FestiModel::addObjectToSceneWithName(FS_Model& object, FS_ModelMap& gameObjects) {
	object->insertKeyframe(0, FS_KEYFRAME_POS_ROT_SCALE | FS_KEYFRAME_VISIBILITY);
	if (object->pointLight) {
		object->insertKeyframe(0, FS_KEYFRAME_POINT_LIGHT);
	} else if (object->world) {
		object->insertKeyframe(0, FS_KEYFRAME_WORLD);
	} else {
		object->insertKeyframe(0, FS_KEYFRAME_FACE_MATERIALS | FS_KEYFRAME_AS_INSTANCE);
	}
	gameObjects.emplace(object->getId(), object);
}

std::vector<VkVertexInputAttributeDescription> Vertex::getAttributeDescriptions() {
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	attributeDescriptions.push_back({0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)});
	attributeDescriptions.push_back({1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)});
	attributeDescriptions.push_back({2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, tangent)});
	attributeDescriptions.push_back({3, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, bitangent)});
	attributeDescriptions.push_back({4, 0, VK_FORMAT_R32G32_SFLOAT   , offsetof(Vertex, uv)});

	attributeDescriptions.push_back({5,  1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Instance, modelMatColumn1)});
	attributeDescriptions.push_back({6,  1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Instance, modelMatColumn2)});
	attributeDescriptions.push_back({7,  1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Instance, modelMatColumn3)});
	attributeDescriptions.push_back({8,  1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Instance, modelMatColumn4)});
	attributeDescriptions.push_back({9,  1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Instance, normalMatColumn1)});
	attributeDescriptions.push_back({10, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Instance, normalMatColumn2)});
	attributeDescriptions.push_back({11, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Instance, normalMatColumn3)});
	
	return attributeDescriptions;
}

std::vector<VkVertexInputBindingDescription> Vertex::getBindingDescriptions() {
	std::vector<VkVertexInputBindingDescription> bindingDescriptions;
	bindingDescriptions.push_back({0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX});
	bindingDescriptions.push_back({1, sizeof(Instance), VK_VERTEX_INPUT_RATE_INSTANCE});
	return bindingDescriptions;
}

}  // namespace festi
