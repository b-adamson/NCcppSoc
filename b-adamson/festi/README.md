Vulkan Game Engine for hyperlapses

Currently only for windows (working on it for all OS coming Soon™ !!!!!)

To run download the repo and config Makefile (CMake coming Soon™ !!!!!)

Need:
-vulkan sdk https://www.lunarg.com/vulkan-sdk (vulkan)
-glfw https://www.glfw.org (64bit) (to create window)
-glm https://github.com/g-truc/glm (for better maths containers)
-stb https://github.com/nothings/stb (to read PNG files)
-tinyobjloader https://github.com/tinyobjloader/tinyobjloader (to read OBJ files)
-pybind11

Makefile:
Set "INCLUDE_DIRS" to have the right paths (ignore -Isrc)
Set "LIB_DIRS" so it connects with the glfw lib and the vulkan lib dlls
Set "VENV_PYTHON_DIR" also to correct absolute path of dir you want to add packages to

app.cpp
Set "PYTHON_PACKAGES_DIR" in app.hpp to the correct dir you want to add python packages to

config.txt coming Soon™ !!!!