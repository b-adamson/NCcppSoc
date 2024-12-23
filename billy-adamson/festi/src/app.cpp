#include "app.hpp"

#include "swap_chain.hpp"
#include "buffer.hpp"
#include "systems/point_light_system.hpp"
#include "systems/main_system.hpp"
#include "bindings.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <pybind11/embed.h>
#include <windows.h>

// std
#include <cassert>
#include <chrono>
#include <stdexcept>
#include <filesystem>
#include <iostream>
#include <array>
#include <random>
#include <cstdlib>
#include <thread>

namespace py = pybind11;
namespace festi {

const std::string FS_APP_NAME = "buildings";

void FestiApp::run() {

	// Instantiate world object
	auto worldObj = std::make_shared<FestiWorld>();

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
	FestiCamera mainLight{festiWindow};
	FestiCamera camera{festiWindow};

	auto lastFrameTime = std::chrono::high_resolution_clock::now();
	std::chrono::duration<float> frameDuration{1.0f / FS_MAX_FPS};

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
		checkInputsForSceneUpdates();
		
		// Check for key presses and adjust viewerObject in world space
		camera.updateCameraFromKeyPresses(dt.count());

		if (auto commandBuffer = festiRenderer.beginFrame()) {
			uint32_t frameBufferIndex = festiRenderer.getFrameBufferIdx();

			// Set scene to current keyframe
			setSceneToCurrentKeyFrame(Mssbo.offsets, MssboBuffer, worldObj);
			
			// Set light direction and clipping distance
			glm::vec3 lightDir = glm::vec3(worldObj->world.mainLightDirection, 0.f);
			mainLight.transform.translation = worldObj->world.getDirectionVector() * worldObj->world.lightClip[0];
			mainLight.transform.rotation = lightDir;
			mainLight.setOrthographicProjection(-10., 10., -10., 10., .1f, worldObj->world.lightClip[1]);

			// Set camera to keyframed posrot
			camera.transform.rotation = worldObj->world.cameraRotation;
    		camera.transform.translation = worldObj->world.cameraPosition;
			camera.setPerspectiveProjection(
				worldObj->world.fov, festiRenderer.getAspectRatio(), worldObj->world.clip[0], worldObj->world.clip[1]);

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
				gameObjects,
				pointLights
				};

			// Update Gubo based on frame details
			GlobalUBO Gubo{};
			Gubo.directionalColour = worldObj->world.mainLightColour;
			Gubo.ambientLightColor = worldObj->world.ambientColour;
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

void FestiApp::setScene(std::shared_ptr<FestiWorld> scene) {

	// TEST SCENE:

	// auto kida = FestiModel::createModelFromFile(
	// 	festiDevice, festiMaterials, gameObjects, "models/TEST/kida.obj", "models/TEST", "materials/TEST");
	// kida->transform.translation = {0.f, -.5f, 0.f};
	// kida->transform.scale = {1.f, 1.f, 1.f};
	// kida->insertKeyframe(0, FS_KEYFRAME_POS_ROT_SCALE);

	// auto cube = FestiModel::createModelFromFile(
	// 	festiDevice, festiMaterials, gameObjects, "models/TEST/cube.obj", "models/TEST", "materials/TEST");
	// cube->transform.translation = {0.f, .0f, 0.f};
	// cube->transform.scale = {0.1f, 0.1f, 0.1f};
	// cube->insertKeyframe(0, FS_KEYFRAME_POS_ROT_SCALE);
	
    // std::vector<glm::vec3> lightColors = {
    //     {1.0f, 0.0f, 0.0f}, // Red
    //     {0.0f, 1.0f, 0.0f}, // Green
    //     {0.0f, 0.0f, 1.0f}, // Blue
    //     {1.0f, 1.0f, 0.0f}, // Yellow
    //     {1.0f, 0.5f, 0.0f}, // Orange
    //     {0.5f, 0.0f, 1.0f}  // Purple
    // };

	// std::array<FS_PointLight, 6> lights;

	// for (size_t i = 0; i < lightColors.size(); i++) {
	// 	lights[i] = FestiPointLight::createPointLight(pointLights, .2f, glm::vec4(lightColors[i], .5f));
	// 	auto rotateLight = glm::rotate(glm::mat4(1.f), i * glm::two_pi<float>() / lightColors.size(), {0.f, -1.f, 0.f});
	// 	lights[i]->transform.translation = glm::vec3(rotateLight * glm::vec4(-1.f, .0f, -1.f, 1.f));
	// }

	// auto rotateLight = glm::rotate(glm::mat4(1.f), glm::pi<float>() / 50, {0.f, -1.f, 0.f});

	// FestiModel::AsInstanceData asInstanceData1{};
	// asInstanceData1.parentObject = kida;
	// asInstanceData1.random.density = 0.f;
	// asInstanceData1.random.randomness = .3f;
	// asInstanceData1.layers = 1;
	// asInstanceData1.layerSeparation = 5.f;
	// asInstanceData1.random.solidity = 1.f;
	// asInstanceData1.building.columnDensity = 3.f;
	// asInstanceData1.building.alignToEdgeIdx = 0;
	// // asInstanceData1.random.minOffset.scale = {4.0f, 1.f, 5.0f};
	// // asInstanceData1.random.maxOffset.scale = {4.0f, 1.f, 5.0f};
	// // asInstanceData1.maxOffset.rotation = {10.0f, 10.0f, 10.0f};
	// asInstanceData1.building.strutsPerColumnRange = {3,3};
	// asInstanceData1.building.jengaFactor = 0.f;

	// cube->asInstanceData = asInstanceData1;
	// cube->insertKeyframe(0, FS_KEYFRAME_AS_INSTANCE);

	// for (uint32_t f = 0; f < (uint32_t)SCENE_LENGTH; f++) {

	// 	// for (size_t i = 0; i < 12; i++) {
	// 	// 	gameObjects[idOf["cube"]]->faceData[i].uvOffset = {f * 0.1, f * 0.1};}
	// 	// gameObjects[idOf["cube"]]->insertKeyframe(f, FS_KEYFRAME_FACE_MATERIALS, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11});

	// 	// FestiModel::AsInstanceData asInstanceData1{};
	// 	// asInstanceData1.parentObject = gameObjects[idOf["kida"]];
	// 	// asInstanceData1.density = (float)(f * 0.1);
	// 	// asInstanceData1.maxOffset.scale = {1.0f, 10.0f, 1.0f};
	// 	// asInstanceData1.maxOffset.rotation = {10.0f, 10.0f, 1.0f};

	// 	cube->asInstanceData.random.seed = f;
	// 	// cube->insertKeyframe(f, FS_KEYFRAME_AS_INSTANCE);

	// 	kida->transform.rotation.y += 0.02;
	// 	// kida->transform.rotation.x += 0.02;
	// 	// kida->transform.rotation.z += 0.02;
	// 	// kida->transform.translation.z += 0.001;
	// 	// kida->transform.translation.y += 0.01;
	// 	kida->insertKeyframe(f, FS_KEYFRAME_POS_ROT_SCALE);

	// 	cube->transform.translation.y += 0.01f;
	// 	// cube->insertKeyframe(f, FS_KEYFRAME_POS_ROT_SCALE);

	// 	// float lol = (float)f / 255.f ;
	// 	// scene->world->colour = glm::vec4(lol, lol * 2, 238.f / 255.f, lol);
	// 	// scene->insertKeyframe(f, FS_KEYFRAME_WORLD);

	// 	for (size_t i = 0; i < 6; i++) {
	// 		lights[i]->transform.translation = glm::vec3(rotateLight * glm::vec4(lights[i]->transform.translation, 1.f));
	// 		lights[i]->insertKeyframe(f, FS_KEYFRAME_POS_ROT_SCALE);
	// 	}
	// }

	// binding

	// Add relevant environment variables
	std::string PYTHONHOME = "C:/msys64/mingw64";
	std::string PYTHONPATH = "C:/Users/beada/dev/Projects/festi/billy-adamson/festi/.venv/lib/python3.12/site-packages";

	if (_putenv(std::string("PYTHONHOME=" + PYTHONHOME).c_str()) != 0) throw std::runtime_error("Failed to set PYTHONHOME environment variable");
	if (_putenv(std::string("PYTHONPATH=" + PYTHONPATH).c_str()) != 0) throw std::runtime_error("Failed to set PYTHONPATH environment variable");

	// Add additional DLL dir (this caused issues without for some reason with mingw)
	std::string pythonBinPath = std::string(PYTHONHOME) + "\\bin";
	if (!SetDllDirectoryA(pythonBinPath.c_str())) {
		std::cerr << "Failed to find /bin/ in PYTHONHOME. Some python libraries may not import correctly";
	}

	FestiBindings::festiMaterials = &festiMaterials;
	FestiBindings::gameObjects = &gameObjects;
	FestiBindings::festiDevice = &festiDevice;
	FestiBindings::pointLights = &pointLights;
	FestiBindings::scene = &scene;

	// Initialize Python interpreter
	py::scoped_interpreter guard{};

	// Add relevant system paths for python
	py::module sys = py::module::import("sys");
	std::string bin_dir = "bin";
	std::string script_dir = "src/scripts";
	sys.attr("path").attr("append")(bin_dir); 
	sys.attr("path").attr("append")(script_dir);
	sys.attr("path").attr("append")(FS_PYTHON_PACKAGES_DIR);

	std::string script_path = script_dir + "/" + FS_APP_NAME + ".py";
	if (std::filesystem::exists(script_path)) {
		try {
			// Import the script and run it
			py::module script = py::module::import(FS_APP_NAME.c_str());
		} catch (const py::error_already_set& e) {
			std::cerr << "Failed to run script: " << script_path << " " << e.what() << '\n';
			throw std::runtime_error("Failed to execute script.");
		}
	} else {
		throw std::runtime_error("Script file not found: " + script_path);
	}
}
	
void FestiApp::checkInputsForSceneUpdates() {
    runOnceIfKeyPressed(GLFW_KEY_UP, [this]() { 
		sceneClockFrequency = std::floor(sceneClockFrequency / 1.2); 
		sceneClockFrequency = glm::clamp(sceneClockFrequency, (uint32_t)1, FS_MAX_FPS);
	});
    runOnceIfKeyPressed(GLFW_KEY_DOWN, [this]() { 
		sceneClockFrequency = std::ceil(sceneClockFrequency * 1.2); 
		sceneClockFrequency = glm::clamp(sceneClockFrequency, (uint32_t)1, FS_MAX_FPS);
	});
    runOnceIfKeyPressed(GLFW_KEY_SPACE, [this]() {isRunning = !isRunning;});
}

void FestiApp::setSceneToCurrentKeyFrame(
	std::vector<uint32_t>& MssboOffsets, 
	std::unique_ptr<FestiBuffer>& MssboBuffer,
	FS_World world
	) {

	bool right = runOnceIfKeyPressed(GLFW_KEY_RIGHT, [this]() {});
	bool left = runOnceIfKeyPressed(GLFW_KEY_LEFT, [this]() {});

	if ((((engineFrameIdx % sceneClockFrequency == 0) && (sceneClockFrequency != 64) && isRunning )|| sceneFrameIdx == -1) 
		|| right || left) {
		if (right) {
			sceneFrameIdx--; 
			if (sceneFrameIdx < 0) {sceneFrameIdx = FS_SCENE_LENGTH - 1;}
		} else {
			sceneFrameIdx++;
			if (sceneFrameIdx == FS_SCENE_LENGTH) {sceneFrameIdx = 0;}
		}

		for (size_t i = 0; i < gameObjects.size(); i++) {
			gameObjects[i]->setObjectToCurrentKeyFrame(MssboOffsets[i], MssboBuffer, sceneFrameIdx);
		}
		for (size_t j = 0; j < pointLights.size(); j++) {
			pointLights[j]->setPointLightToCurrentKeyFrame(sceneFrameIdx);
		}
		world->setWorldToCurrentKeyFrame(sceneFrameIdx);
	}
}

bool FestiApp::runOnceIfKeyPressed(int key, std::function<void()> onPress) {
	static std::unordered_map<int, bool> keyWasPressedMap;
	bool& keyWasPressed = keyWasPressedMap[key];

	if (glfwGetKey(festiWindow.getGLFWwindow(), key) == GLFW_PRESS) {
		if (keyWasPressed == false) {
			onPress();
			keyWasPressed = true;
			return true; 
		}
	} else {
		keyWasPressed = false;
	}
	return false;
};

}  // namespace festi
