#include "app.hpp"

#include "swap_chain.hpp"
#include "buffer.hpp"
#include "systems/point_light_system.hpp"
#include "systems/main_system.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <cassert>
#include <chrono>
#include <stdexcept>
#include <filesystem>
#include <iostream>
#include <array>
#include <random>
#include <thread>

namespace festi {

void FestiApp::run() {

	// Instantiate world object
	auto worldObj = std::make_shared<FestiModel>(festiDevice);
	worldObj->world = std::make_unique<FestiModel::WorldProperties>();
	FestiModel::addObjectToSceneWithName(worldObj, gameObjects);

	// Create scene objects and setup
	setScene(worldObj);

	// Set maximum instance buffer size on game objects based on
	FestiModel::setInstanceBufferSizesOnGameObjects(gameObjects);

	// Create global pool
	auto globalPool = FestiDescriptorPool::Builder(festiDevice)
		.setMaxSets(5)
		.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, FS_MAX_FRAMES_IN_FLIGHT * 2)
		.addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1)
		.addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, FS_MAXIMUM_IMAGE_DESCRIPTORS + FS_MAX_FRAMES_IN_FLIGHT)
		.build();

	// Config global descriptor set layout
	auto perFrameSetLayout = FestiDescriptorSetLayout::Builder(festiDevice)
		.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS) // Gubo
		.build();
  
	// Config materials descriptor set layout
	auto materialsSetLayout = FestiDescriptorSetLayout::Builder(festiDevice)
		.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT) // Mssbo
		.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, FS_MAXIMUM_IMAGE_DESCRIPTORS) // ImageViews
		.build();
	
	// Config shadow descriptor set layout
	auto shadowMapSetLayout = FestiDescriptorSetLayout::Builder(festiDevice)
		.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.build();

	// Get handles to descriptor sets
	std::vector<VkDescriptorSet> perFrameDescriptorSets(FS_MAX_FRAMES_IN_FLIGHT);
	std::vector<VkDescriptorSet> shadowMapDescriptorSets(FS_MAX_FRAMES_IN_FLIGHT);
	VkDescriptorSet materialDescriptorSet;

	// Create Mssbo for modification at runtime
	auto& Mssbo = festiMaterials.getMssbo();
	Mssbo.appendMaterialFaceIDs(gameObjects);

	// Create buffer on mapped GPU memory intended for Mssbo and write Mssbo to it
	auto MssboBuffer = std::make_unique<FestiBuffer>(
		festiDevice,
		sizeof(MaterialsSSBO),
		1,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	MssboBuffer->writeToBuffer(&Mssbo);

	// Build materials descriptor set with pointers to GPU side Mssbo and imageview array
	auto MssboBufferDescriptorInfo = MssboBuffer->descriptorInfo();
	auto imageViewsDescriptorInfo = festiMaterials.getImageViewsDescriptorInfo();
	FestiDescriptorWriter(materialsSetLayout, *globalPool)
		.writeBuffer(0, &MssboBufferDescriptorInfo)
		.writeImageViews(1, imageViewsDescriptorInfo)
		.build(materialDescriptorSet);

	// Initalise vectors of global buffers on the CPU
	std::vector<std::unique_ptr<FestiBuffer>> GuboBuffers(FS_MAX_FRAMES_IN_FLIGHT);;

	for (uint32_t i = 0; i < FS_MAX_FRAMES_IN_FLIGHT; i++) {
		// Create buffer on mapped GPU memory intended for Gubo
		GuboBuffers[i] = std::make_unique<FestiBuffer>(
			festiDevice,
			sizeof(GlobalUBO),
			1,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		// Build global descriptor set with pointer to Gubo
		auto globalBufferDescriptorInfo = GuboBuffers[i]->descriptorInfo();
		FestiDescriptorWriter(perFrameSetLayout, *globalPool)
			.writeBuffer(0, &globalBufferDescriptorInfo)
			.build(perFrameDescriptorSets[i]);

		// Setup shadow map resources and build associated descriptor set
		auto shadowMapDescriptorInfo = festiRenderer.getShadowImageViewDescriptorInfo(i);
		FestiDescriptorWriter(shadowMapSetLayout, *globalPool)
			.writeImageViews(0, shadowMapDescriptorInfo)
			.build(shadowMapDescriptorSets[i]);
  	}

	// Create rendering systems
	MainSystem mainRenderSystem{
		festiDevice,
		festiRenderer,
		perFrameSetLayout.getDescriptorSetLayout(),
		materialsSetLayout.getDescriptorSetLayout(),
		shadowMapSetLayout.getDescriptorSetLayout()};
	PointLightSystem pointLightSystem{
		festiDevice,
		festiRenderer.getSwapChainRenderPass(),
		perFrameSetLayout.getDescriptorSetLayout()};

	// Create objects for camera and shadow pass
	FestiCamera camera{festiWindow};
	camera.transform.translation = {5.f, 1.f, 0.f};
	camera.transform.rotation.y = -glm::pi<float>() / 2;
	camera.setPerspectiveProjection(glm::radians(40.f), festiRenderer.getAspectRatio(), .1f, 1000.f);
	FestiCamera mainLight{festiWindow};
	mainLight.setOrthographicProjection(-10., 10., -10., 10., .1f, 20.f);

	auto lastFrameTime = std::chrono::high_resolution_clock::now();
	std::chrono::duration<float> frameDuration{1.0f / MAX_FPS};

	// ENGINE MAIN LOOP
	while (!festiWindow.shouldClose()) {

		// Keep max fps constant
		auto currentTime = std::chrono::high_resolution_clock::now();
    	std::chrono::duration<float> dt = currentTime - lastFrameTime;
		if (dt < frameDuration) {
			std::this_thread::sleep_for(frameDuration - dt);
			currentTime = std::chrono::high_resolution_clock::now();
			// std::cout << 1.f / dt.count() << '\n'; // print FPS
		}
		dt = currentTime - lastFrameTime;
		lastFrameTime = currentTime;
		
		// Poll events
		glfwPollEvents();
		checkUserInput();
		
		// Check for key presses and adjust viewerObject in world space
		camera.updateCameraFromKeyPresses(dt.count());

		if (auto commandBuffer = festiRenderer.beginFrame()) {
			uint32_t frameBufferIndex = festiRenderer.getFrameBufferIdx();

			// Set scene to current keyframe
			setSceneToCurrentKeyFrame(Mssbo.offsets, MssboBuffer);
			
			// Set light direction
			glm::vec3 lightDir = glm::vec3(worldObj->world->mainLightDirection, 0.f);
			mainLight.transform.translation = worldObj->world->getDirectionVector() * -10.f;
			mainLight.transform.rotation = lightDir;

			// Helper struct to pass information to render systems for per frame information
			FrameInfo frameInfo {
				frameBufferIndex,
				dt.count(),
				commandBuffer,
				camera,
				mainLight,
				perFrameDescriptorSets[frameBufferIndex],
				materialDescriptorSet,
				shadowMapDescriptorSets[frameBufferIndex],
				gameObjects};

			// Update Gubo based on frame details
			GlobalUBO Gubo{};
			Gubo.directionalColour = worldObj->world->mainLightColour;
			Gubo.ambientLightColor = worldObj->world->ambientColour;
			Gubo.lightProjection = mainLight.getProjection();
			Gubo.lightView = mainLight.getView();
			Gubo.projection = camera.getProjection();
			Gubo.view = camera.getView();
			Gubo.inverseView = camera.getInverseView();
			pointLightSystem.writePointLightsToUBO(frameInfo, Gubo);

			// Write Gubo to mapped GPU side buffer
			GuboBuffers[frameBufferIndex]->writeToBuffer(&Gubo);

			// Perform shadow pass
			festiRenderer.beginShadowPass(commandBuffer);
			mainRenderSystem.createShadowMap(frameInfo);
			festiRenderer.endShadowPass(commandBuffer);

			// Perform main render pass
			festiRenderer.beginSwapChainRenderPass(commandBuffer);
			mainRenderSystem.renderGameObjects(frameInfo);
			pointLightSystem.renderPointLights(frameInfo);
			festiRenderer.endSwapChainRenderPass(commandBuffer);

			festiRenderer.endFrame();
			engineFrameIdx += 1;
		}
	} // ENGINE MAIN LOOP END
	vkDeviceWaitIdle(festiDevice.device());
}

void FestiApp::setScene(std::shared_ptr<FestiModel> scene) {

	// auto floor = FestiModel::createModelFromFile(
	// 	festiDevice, festiMaterials, gameObjects, "models/floor.obj", "models", "materials");
	// floor->transform.scale = {7.f, 3.f, 7.f};

	// auto wall = FestiModel::createModelFromFile(
	// 	festiDevice, festiMaterials, gameObjects, "models/wall.obj", "models", "materials");
	// wall->transform.scale = {.1f, .1f, .2f};

	// auto door = FestiModel::createModelFromFile(
	// 	festiDevice, festiMaterials, gameObjects, "models/door.obj", "models", "materials");
	// door->transform.translation = {0.f, .95f, 0.f};
	// door->transform.scale = {.05f, 0.12f, 0.12f};

	// auto arch = FestiModel::createModelFromFile(
	// 	festiDevice, festiMaterials, gameObjects, "models/arch.obj", "models", "materials");
	// arch->transform.translation = {0.f, 2.f, 0.f};

	// auto mask = FestiModel::createModelFromFile(
	// 	festiDevice, festiMaterials, gameObjects, "models/mask.obj", "models", "materials");
	// mask->transform.scale = {.1f, .1f, .2f};
	// mask->transform.rotation.y = glm::pi<float>();
	// mask->visibility = false;
	// mask->insertKeyframe(0, FS_KEYFRAME_VISIBILITY);

	// auto lamp = FestiModel::createModelFromFile(
	// 	festiDevice, festiMaterials, gameObjects, "models/lamp.obj", "models", "materials");
	// lamp->transform.scale = {.1f, .1f, .1f};

	// auto box = FestiModel::createModelFromFile(
	// 	festiDevice, festiMaterials, gameObjects, "models/box.obj", "models", "materials");
	// box->transform.translation = {0.f, -3.f, 1.5f};
	// box->transform.scale = {3.f, 3.f, 3.f};

	// auto leaf = FestiModel::createModelFromFile(
	// 	festiDevice, festiMaterials, gameObjects, "models/leaf.obj", "models", "materials");
	// leaf->transform.translation.z = -0.4f;
	// leaf->insertKeyframe(0, FS_KEYFRAME_POS_ROT_SCALE);

	// auto streetLight = FestiModel::createModelFromFile(
	// 	festiDevice, festiMaterials, gameObjects, "models/streetLight.obj", "models", "materials");
	// streetLight->transform.rotation = {glm::pi<float>(), 0.f, 0.f};
	// streetLight->transform.scale = {0.3f, 0.2f, 0.3f};
	
	// std::vector<glm::vec3> translations {
	// 	{-2.9f, 2.6f, -0.8f},
	// 	{-1.6f, 2.6f, -0.8f},
	// 	{1.6f, 2.6f, -0.8f},
	// 	{2.9f, 2.6f, -0.8f},
	// };

	// std::array<FS_Model, 9> flames;
	// for (size_t i = 0; i < translations.size(); ++i) {
	// 	flames[i] = FestiModel::createPointLight(festiDevice, gameObjects, .5f, {1.f, 1.f, 1.f, 1.f});
	// 	flames[i]->transform.translation = translations[i];
	// };

	// for (size_t i = 4; i < 9; ++i) {
	// 	flames[i] = FestiModel::createPointLight(festiDevice, gameObjects, .5f, {1.f, 1.f, 1.f, 1.f});
	// };

	// std::random_device rd;
	// std::mt19937 gen(rd());
	// std::uniform_real_distribution<float> dis(0., 1.);

	// scene->world->mainLightColour = {255.f / 255.f, 255.f / 255.f, 255.f / 255.f, 1.f};
	// scene->world->ambientColour = {1.f, 1.f, 1.f, .01f};
	// scene->world->mainLightDirection = {.4f, 0.f};
	// scene->insertKeyframe(0, FS_KEYFRAME_WORLD);

	// for (uint32_t f = 0; f < (uint32_t)SCENE_LENGTH; f++) {
	// 	auto frameRND = dis(gen);		
	// 	floor->transform.rotation.y += glm::pi<float>() / 150;
	// 	floor->insertKeyframe(f, FS_KEYFRAME_POS_ROT_SCALE);

	// 	arch->transform.rotation.y += glm::pi<float>() / 150;
	// 	arch->transform.scale = {frameRND * 0.01 + 0.145f, .15f - (SCENE_LENGTH / 3000.f) + (f / 3000.f), frameRND + 0.5f};
	// 	arch->transform.scale.y += (frameRND - 0.5) * .1f;
	// 	arch->insertKeyframe(f, FS_KEYFRAME_POS_ROT_SCALE);

	// 	door->transform.rotation.y = glm::pi<float>() * f / 150 + glm::pi<float>() / 2;
	// 	door->transform.scale.y = (frameRND - .5) * .01 + .13;
	// 	door->insertKeyframe(f, FS_KEYFRAME_FACE_MATERIALS | FS_KEYFRAME_POS_ROT_SCALE, {0, 2, 6, 8});

	// 	wall->transform.rotation.y += glm::pi<float>() / 150;
	// 	wall->transform.scale.y = (frameRND - .5) * .01 + .1;
	// 	wall->insertKeyframe(f, FS_KEYFRAME_POS_ROT_SCALE);

	// 	mask->transform.rotation.y += glm::pi<float>() / 150;
	// 	mask->transform.scale.y = (frameRND - .5) * .01 + .1;
	// 	mask->insertKeyframe(f, FS_KEYFRAME_POS_ROT_SCALE);

	// 	box->transform.translation.y += .007f;
	// 	box->asInstanceData.parentObject = mask;
	// 	box->asInstanceData.seed = f;
	// 	box->asInstanceData.density = dis(gen) / 10.f;
	// 	box->asInstanceData.minOffset.scale = {.4f, .8f, .3f};
	// 	box->asInstanceData.maxOffset.scale = {1.7f, 2.3f, 2.5f};
	// 	box->insertKeyframe(f, FS_KEYFRAME_AS_INSTANCE | FS_KEYFRAME_POS_ROT_SCALE);

	// 	leaf->asInstanceData.parentObject = mask;
	// 	leaf->asInstanceData.density = 2.f;
	// 	leaf->asInstanceData.seed = (uint32_t)(dis(gen) * 20);
	// 	leaf->asInstanceData.minOffset.translation.x = -9.f;
	// 	leaf->asInstanceData.maxOffset.translation.x = 9.f;
	// 	leaf->insertKeyframe(f, FS_KEYFRAME_AS_INSTANCE);

	// 	lamp->transform.rotation.y = glm::pi<float>() * f / 150 + glm::pi<float>();
	// 	lamp->transform.scale.y = (frameRND - .5) * .01f + .1f;
	// 	lamp->transform.translation.y -= .005f;
	// 	lamp->insertKeyframe(f, FS_KEYFRAME_POS_ROT_SCALE);

	// 	streetLight->transform.translation = glm::vec3(glm::rotate(glm::mat4(1.f), f * glm::pi<float>() / 150, 
	// 		glm::vec3(0.f, 1.f, 0.f)) * glm::vec4(frameRND, frameRND - .5f, 1.5f + frameRND, 1.f));
	// 	streetLight->transform.rotation.y += glm::pi<float>() / 150;
	// 	streetLight->insertKeyframe(f, FS_KEYFRAME_POS_ROT_SCALE);

	// 	for (uint32_t i = 0; i < 4; ++i) {
	// 		flames[i]->transform.translation = 
	// 			glm::vec3((glm::rotate(glm::mat4(1.f), glm::pi<float>() / 150, {0.f, 1.f, 0.f}))
	// 			* glm::vec4(flames[i]->transform.translation, 1.0f));
	// 		flames[i]->transform.translation.y -= 0.005;
	// 		flames[i]->transform.translation.z += (dis(gen) - .5f) * .04f;
	// 		if ((uint32_t)(dis(gen) * 4) == 0) {flames[i]->visibility = !flames[i]->visibility;}
	// 		flames[i]->insertKeyframe(f, FS_KEYFRAME_POS_ROT_SCALE | FS_KEYFRAME_VISIBILITY);
	// 	}

	// 	std::vector<float> xcoords = {0.f, -2.5f, 2.5f, -5.f, 5.f};
	// 	for (uint32_t i = 4; i < sizeof(flames) / sizeof(flames[0]); ++i) {
	// 		flames[i]->transform.translation = glm::vec3(glm::rotate(glm::mat4(1.f), f * glm::pi<float>() / 150, 
	// 			glm::vec3(0.f, 1.f, 0.f)) * glm::vec4(frameRND + xcoords[i - 4], frameRND + 2.8f, 1.3f + frameRND, 1.f));
	// 		flames[i]->insertKeyframe(f, FS_KEYFRAME_POS_ROT_SCALE);
	// 	}

	// 	for (uint32_t j = 0; j < floor->getNumberOfFaces(); ++j) {
	// 		floor->faceData[j].uvOffset = glm::vec2(dis(gen), dis(gen));
	// 		floor->faceData[j].contrast = 5.f;
	// 	}
	// 	floor->insertKeyframe(f, FS_KEYFRAME_FACE_MATERIALS, floor->ALL_FACES());

	// 	for (uint32_t j = 0; j < box->getNumberOfFaces(); ++j) {
	// 		box->faceData[j].saturation = .1f;
	// 		box->faceData[j].contrast = 1.6f;
	// 	}
	// 	box->insertKeyframe(f, FS_KEYFRAME_FACE_MATERIALS, box->ALL_FACES());

	// 	for (uint32_t j = 0; j < wall->getNumberOfFaces(); ++j) {
	// 		wall->faceData[j].contrast = (dis(gen) - .2) * 2 + 4.f;
	// 		wall->faceData[j].uvOffset = glm::vec2(dis(gen), dis(gen));
	// 		wall->faceData[j].saturation = 0.f;
	// 	}
	// 	wall->insertKeyframe(f, FS_KEYFRAME_FACE_MATERIALS, wall->ALL_FACES());

	// 	for (uint32_t i = 0; i < leaf->getNumberOfFaces(); ++i) {
	// 		leaf->faceData[i].saturation = .0f;
	// 		leaf->faceData[i].contrast = 2.f;
	// 	}
	// 	leaf->insertKeyframe(f, FS_KEYFRAME_FACE_MATERIALS, leaf->ALL_FACES());

	// 	for (uint32_t i = 0; i < arch->getNumberOfFaces(); ++i) {
	// 		glm::vec2 uvOffset = glm::vec2(dis(gen), dis(gen));
	// 		arch->faceData[i].uvOffset = uvOffset;
	// 		arch->faceData[i].contrast =  (dis(gen) - .2) * 1.1 + 16.f;
	// 	}
	// 	arch->insertKeyframe(f, FS_KEYFRAME_FACE_MATERIALS, arch->ALL_FACES());

	// 	auto offset = glm::vec2(((int)(dis(gen) * 8)) / 8.f, ((int)(dis(gen) * 5)) / 5.f);
	// 	for (const auto& face : {0, 2, 6, 8}) {
	// 		door->faceData[face].uvOffset = offset;
	// 		door->faceData[face].saturation = 2.f;
	// 	}
	// 	door->insertKeyframe(f, FS_KEYFRAME_FACE_MATERIALS, {0, 2, 6, 8});

	// 	for (uint32_t i = 0; i < streetLight->getNumberOfFaces(); ++i) {
	// 		streetLight->faceData[i].contrast = 1.5f;
	// 	}
	// 	streetLight->insertKeyframe(f, FS_KEYFRAME_FACE_MATERIALS, streetLight->ALL_FACES());
	// }

	//////////////////////////////////////////////////

	auto kida = FestiModel::createModelFromFile(
		festiDevice, festiMaterials, gameObjects, "models/DEFAULT/kida.obj", "models/DEFAULT", "materials/DEFAULT");
	kida->transform.translation = {0.f, -.5f, 0.f};
	kida->transform.scale = {1.f, 1.f, 1.f};
	kida->insertKeyframe(0, FS_KEYFRAME_POS_ROT_SCALE);
	// addObjectToSceneWithName("kida", kida);

	auto cube = FestiModel::createModelFromFile(
		festiDevice, festiMaterials, gameObjects, "models/DEFAULT/cube.obj", "models/DEFAULT", "materials/DEFAULT");
	cube->transform.translation = {0.f, .0f, 0.f};
	cube->transform.scale = {0.1f, 0.1f, 0.1f};
	cube->insertKeyframe(0, FS_KEYFRAME_POS_ROT_SCALE);
	// addObjectToSceneWithName("cube", cube);
	
    std::vector<glm::vec3> lightColors = {
        {1.0f, 0.0f, 0.0f}, // Red
        {0.0f, 1.0f, 0.0f}, // Green
        {0.0f, 0.0f, 1.0f}, // Blue
        {1.0f, 1.0f, 0.0f}, // Yellow
        {1.0f, 0.5f, 0.0f}, // Orange
        {0.5f, 0.0f, 1.0f}  // Purple
    };

	std::array<FS_Model, 6> pointLights;

	for (size_t i = 0; i < lightColors.size(); i++) {
		pointLights[i] = FestiModel::createPointLight(festiDevice, gameObjects, .2f, glm::vec4(lightColors[i], .5f));
		auto rotateLight = glm::rotate(glm::mat4(1.f), i * glm::two_pi<float>() / lightColors.size(), {0.f, -1.f, 0.f});
		pointLights[i]->transform.translation = glm::vec3(rotateLight * glm::vec4(-1.f, .0f, -1.f, 1.f));
	}

	auto rotateLight = glm::rotate(glm::mat4(1.f), glm::pi<float>() / 50, {0.f, -1.f, 0.f});

	FestiModel::AsInstanceData asInstanceData1{};
	asInstanceData1.parentObject = kida;
	asInstanceData1.density = (float)(20);
	asInstanceData1.randomness = .01f;
	// asInstanceData1.minOffset.scale = {1.0f, .4f, 1.0f};
	// asInstanceData1.maxOffset.scale = {1.0f, 1.f, 1.0f};
	// asInstanceData1.maxOffset.rotation = {10.0f, 10.0f, 10.0f};

	cube->asInstanceData = asInstanceData1;
	cube->insertKeyframe(0, FS_KEYFRAME_AS_INSTANCE);

	for (uint32_t f = 0; f < (uint32_t)SCENE_LENGTH; f++) {

		// for (size_t i = 0; i < 12; i++) {
		// 	gameObjects[idOf["cube"]]->faceData[i].uvOffset = {f * 0.1, f * 0.1};}
		// gameObjects[idOf["cube"]]->insertKeyframe(f, FS_KEYFRAME_FACE_MATERIALS, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11});

		// FestiModel::AsInstanceData asInstanceData1{};
		// asInstanceData1.parentObject = gameObjects[idOf["kida"]];
		// asInstanceData1.density = (float)(f * 0.1);
		// asInstanceData1.maxOffset.scale = {1.0f, 10.0f, 1.0f};
		// asInstanceData1.maxOffset.rotation = {10.0f, 10.0f, 1.0f};

		cube->asInstanceData.seed = f;
		cube->insertKeyframe(f, FS_KEYFRAME_AS_INSTANCE);

		// gameObjects[idOf["cube"]]->transform.rotation += glm::vec3(0.1f, 0.1f, 0.f);
		// gameObjects[idOf["cube"]]->insertKeyframe(f, FS_KEYFRAME_POS_ROT_SCALE);

		// gameObjects[idOf["kida"]]->transform.rotation += glm::vec3(0.005f, 0.0f, 0.f);
		// gameObjects[idOf["kida"]]->transform.scale += glm::vec3(.01f, .01f, .01f);
		// gameObjects[idOf["kida"]]->insertKeyframe(f, FS_KEYFRAME_POS_ROT_SCALE);

		// float lol = (float)f / 255.f ;
		// scene->world->colour = glm::vec4(lol, lol * 2, 238.f / 255.f, lol);
		// scene->insertKeyframe(f, FS_KEYFRAME_WORLD);

		for (size_t i = 0; i < 6; i++) {
			pointLights[i]->transform.translation = glm::vec3(rotateLight * glm::vec4(pointLights[i]->transform.translation, 1.f));
			pointLights[i]->insertKeyframe(f, FS_KEYFRAME_POS_ROT_SCALE);
		}
	}

	// gameObjects[idOf["cube"]]->asInstanceData.makeStandAlone();
	// gameObjects[idOf["cube"]]->insertKeyframe(10, FS_KEYFRAME_AS_INSTANCE_DATA);
}

void FestiApp::checkUserInput() {
    static bool upKeyWasPressed = false;
    static bool downKeyWasPressed = false;
	static bool rightKeyWasPressed = false;
	static bool leftKeyWasPressed = false;
	static bool spaceKeyWasPressed = false;

    auto executeIfKeyPressed = [this](int key, bool& keyWasPressed, std::function<void()> onPress) {
        if (glfwGetKey(festiWindow.getGLFWwindow(), key) == GLFW_PRESS) {
            if (keyWasPressed == false) {
                onPress();
                keyWasPressed = true;
            }
        } else {
            keyWasPressed = false;
        }
    };

    executeIfKeyPressed(GLFW_KEY_UP, upKeyWasPressed, [this]() { 
		sceneClockFrequency = std::floor(sceneClockFrequency / 1.2); 
		sceneClockFrequency = glm::clamp(sceneClockFrequency, (uint32_t)1, MAX_FPS);
	});
    executeIfKeyPressed(GLFW_KEY_DOWN, downKeyWasPressed, [this]() { 
		sceneClockFrequency = std::ceil(sceneClockFrequency * 1.2); 
		sceneClockFrequency = glm::clamp(sceneClockFrequency, (uint32_t)1, MAX_FPS);
	});
    executeIfKeyPressed(GLFW_KEY_SPACE, spaceKeyWasPressed, [this]() {isRunning = !isRunning;});
    executeIfKeyPressed(GLFW_KEY_RIGHT, rightKeyWasPressed, [this]() {advanceFrame = FS_AdvanceFrame::FORWARD;});
	executeIfKeyPressed(GLFW_KEY_LEFT, leftKeyWasPressed, [this]() {advanceFrame = FS_AdvanceFrame::BACKWARD;});
}

void FestiApp::setSceneToCurrentKeyFrame(
	std::vector<uint32_t>& MssboOffsets, 
	std::unique_ptr<FestiBuffer>& MssboBuffer) {
	if ((((engineFrameIdx % sceneClockFrequency == 0) && (sceneClockFrequency != 64) && isRunning )|| sceneFrameIdx == -1) 
		|| advanceFrame == FS_AdvanceFrame::FORWARD || advanceFrame == FS_AdvanceFrame::BACKWARD) {
		if (advanceFrame == FS_AdvanceFrame::BACKWARD) {
			sceneFrameIdx--; 
			if (sceneFrameIdx < 0) {sceneFrameIdx = SCENE_LENGTH - 1;}
			advanceFrame = FS_AdvanceFrame::UNSPECIFIED;
		} else {
			sceneFrameIdx++;
			if (sceneFrameIdx == SCENE_LENGTH) {sceneFrameIdx = 0;}
			advanceFrame = FS_AdvanceFrame::UNSPECIFIED;
		}
		for (size_t i = 0; i < gameObjects.size(); i++) {
			setObjectToCurrentKeyFrame(gameObjects[i], MssboOffsets[i], MssboBuffer);
		}
	}
}

void FestiApp::setObjectToCurrentKeyFrame(
	std::shared_ptr<FestiModel>& obj, 
	uint32_t MssboOffset, 
	std::unique_ptr<FestiBuffer>& MssboBuffer) {

	const uint32_t frame = sceneFrameIdx;
	const bool atEndOrStart = frame == 0 || (int)frame == (SCENE_LENGTH - 1);

	auto getObjectPropertyKeyframe = [frame](auto& propertyKeyframes) {
		auto it = propertyKeyframes.find(frame);
		if (it == propertyKeyframes.end()) {
			it = propertyKeyframes.lower_bound(frame);
			--it;
		}
		return it;
	};

	// FS_KEYFRAME_POS_ROT_SCALE
	auto posRotScaleKF = getObjectPropertyKeyframe(obj->keyframes.transforms);
	bool hasMoved = obj->transform != posRotScaleKF->second;
	if (hasMoved || atEndOrStart) {obj->transform = posRotScaleKF->second;}

	// FS_KEYFRAME_VISIBILITY
	auto visibilityKF = getObjectPropertyKeyframe(obj->keyframes.visibility);
	if (obj->visibility != visibilityKF->second || atEndOrStart) {obj->visibility = visibilityKF->second;}

	if (obj->pointLight) {
		// FS_KEYFRAME_POINT_LIGHT
		auto pointtLightKF = getObjectPropertyKeyframe(obj->keyframes.pointLightData);
		if (*obj->pointLight != pointtLightKF->second || atEndOrStart) {
			obj->pointLight = std::make_unique<FestiModel::PointLightComponent>(pointtLightKF->second);
		}
	} else if (obj->world) {
		// FS_KEYFRAME_KEYFRAME_WORLD
		auto worldKF = getObjectPropertyKeyframe(obj->keyframes.worldProperties);
		if (*obj->world != worldKF->second || atEndOrStart) {
			obj->world = std::make_unique<FestiModel::WorldProperties>(worldKF->second);
		}
	} else {
		// FS_KEYFRAME_MATERIAL
		for (auto& faceID : obj->keyframes.modifiedFaces) {
			auto materialKF = getObjectPropertyKeyframe(obj->keyframes.objFaceData[faceID]);
			if ((obj->faceData[faceID] != materialKF->second) || atEndOrStart) {obj->faceData[faceID] = materialKF->second;}
		};
		auto data = obj->faceData.data();
		auto size = obj->faceData.size() * sizeof(ObjFaceData);
		auto offset = MssboOffset * sizeof(ObjFaceData);
		MssboBuffer->writeToBuffer(data, size, offset);

		// FS_KEYFRAME_AS_INSTANCE
		auto asInstKF = getObjectPropertyKeyframe(obj->keyframes.asInstanceData);
		bool parentHasMoved = false;
		if (obj->asInstanceData.parentObject) {
			parentHasMoved = obj->asInstanceData.parentObject->keyframes.inMotion.count(frame);
		}
		if ((obj->asInstanceData != asInstKF->second) || hasMoved || parentHasMoved || atEndOrStart) {
			if (asInstKF->second.parentObject) {obj->writeToInstanceBuffer(
				asInstKF->second.parentObject->getTransformsToRndPointsOnSurface(asInstKF->second, obj->transform));
			} else {
				obj->writeToInstanceBuffer(
					std::vector<Instance>(1, {obj->transform.getModelMatrix(), obj->transform.getNormalMatrix()}));
			}
		}
	}
}

}  // namespace festi
