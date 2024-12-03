#pragma once

#include "model.hpp"
#include "descriptors.hpp"
#include "device.hpp"
#include "renderer.hpp"
#include "window.hpp"
#include "camera.hpp"
#include "materials.hpp"

// std
#include <memory>
#include <vector>
#include <string>
#include <map>

namespace festi {

enum class FS_AdvanceFrame {
	FORWARD,
	BACKWARD,
	UNSPECIFIED
};

class FestiApp {
public:
    FestiApp() = default;
    ~FestiApp() = default;
    FestiApp(const FestiApp &) = delete;
    FestiApp &operator=(const FestiApp &) = delete;
	FestiApp(const FestiApp &&) = delete;
    FestiApp &operator=(const FestiApp &&) = delete;

    void run();

private:

	uint32_t material(std::string name) {return FestiModel::getMaterial(name);}
	
	// helpers
	void setScene(std::shared_ptr<FestiModel> scene);
	void checkInputsForSceneUpdates();
	void setSceneToCurrentKeyFrame(
		std::vector<uint32_t>& MssboOffset, 
		std::unique_ptr<FestiBuffer>& MssboBuffer);
	void setObjectToCurrentKeyFrame(
		std::shared_ptr<FestiModel>& obj, 
		uint32_t MssboOffset, 
		std::unique_ptr<FestiBuffer>& MssboBuffer);
	bool runOnceIfKeyPressed(int key, std::function<void()> onPress);

	uint32_t sceneClockFrequency = 1;
	bool isRunning = false;

	FS_AdvanceFrame advanceFrame = FS_AdvanceFrame::UNSPECIFIED;

	const uint32_t MAX_FPS = 120;
	const int SCENE_LENGTH = 300;

	size_t engineFrameIdx = 0;
	int sceneFrameIdx = -1;

	FestiWindow festiWindow; 
	FestiDevice festiDevice{festiWindow};
	FestiRenderer festiRenderer{festiWindow, festiDevice};
	FestiMaterials festiMaterials{festiDevice};

	FS_ModelMap gameObjects;

};

}  // namespace festi
