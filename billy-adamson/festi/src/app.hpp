#pragma once

#include "model.hpp"
#include "descriptors.hpp"
#include "device.hpp"
#include "renderer.hpp"
#include "window.hpp"
#include "camera.hpp"
#include "materials.hpp"
#include "bindings.hpp"

// std
#include <memory>
#include <vector>
#include <string>
#include <map>

namespace festi {

class FestiApp {
public:

    FestiApp() = default;
    ~FestiApp() = default;
    FestiApp(const FestiApp &) = delete;
    FestiApp &operator=(const FestiApp &) = delete;
	FestiApp(const FestiApp &&) = delete;
    FestiApp &operator=(const FestiApp &&) = delete;

    void run();

	FestiDevice& getDevice() {return festiDevice;}
	FestiMaterials& getMaterials() {return festiMaterials;}
	FS_ModelMap& getGameObjects() {return gameObjects;}

	const std::string APP_NAME = "script";
	const std::string PYTHON_PACKAGES_DIR = ".venv/lib/python3.12/site-packages";

private:

	uint32_t material(std::string name) {return FestiModel::getMaterial(name);}
	
	// helpers
	void setScene(std::shared_ptr<FestiWorld> scene);
	void checkInputsForSceneUpdates();
	void setSceneToCurrentKeyFrame(
		std::vector<uint32_t>& MssboOffset, 
		std::unique_ptr<FestiBuffer>& MssboBuffer,
		FS_World world
	);
	void setObjectToCurrentKeyFrame(
		FS_Model& obj, 
		uint32_t MssboOffset, 
		std::unique_ptr<FestiBuffer>& MssboBuffer
	);
	void setPointLightToCurrentKeyFrame(FS_PointLight& obj);
	void setWorldToCurrentKeyFrame(FS_World& obj);
	bool runOnceIfKeyPressed(int key, std::function<void()> onPress);

	uint32_t sceneClockFrequency = 1;
	bool isRunning = false;

	const uint32_t MAX_FPS = 120;
	const int SCENE_LENGTH = 300;

	size_t engineFrameIdx = 0;
	int sceneFrameIdx = -1;

	FestiWindow festiWindow; 
	FestiDevice festiDevice{festiWindow};
	FestiRenderer festiRenderer{festiWindow, festiDevice};
	FestiMaterials festiMaterials{festiDevice};

	FS_ModelMap gameObjects;
	FS_PointLightMap pointLights;
};

}  // namespace festi
