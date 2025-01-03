# Compiler and tools
CXX = g++ -g 
CFLAGS := -std=c++17 -Wall -Wextra
GLSLC = C:/VulkanSDK/1.3.290.0/Bin/glslc.exe

VENV_PYTHON_DIR = "C:/Users/beada/dev/Projects/festi/b-adamson/festi/.venv/lib/python3.12/site-packages"

# Paths and flags
LIB_PATH = C:/Users/beada/dev/C++/libraries
VULKAN_SDK = C:/VulkanSDK/1.3.290.0
INCLUDE_DIRS =  -Isrc \
                -I$(VULKAN_SDK)/Include \
                -I$(LIB_PATH)/glm-master \
                -I$(LIB_PATH)/glfw-3.4.bin.WIN64/include \
                -I$(LIB_PATH)/tinyobjloader \
                -I$(LIB_PATH)/stb-master \
                -I$(LIB_PATH)/pybind11-master/include \
                -IC:/msys64/mingw64/include/python3.12

LIB_DIRS =  -L$(VULKAN_SDK)/Lib \
            -L$(LIB_PATH)/glfw-3.4.bin.WIN64/lib-mingw-w64 \
			-LC:/msys64/mingw64/lib

LIBS = -lvulkan-1 -lglfw3 -luser32 -lgdi32 -lshell32 -lpython3.12.dll
DEFINES = -DDEBUG

# Directories
OBJ_DIR = bin
SHADER_DIR = src/shaders
SRC_DIR = src

# Source and object files
CPP_FILES = $(wildcard $(SRC_DIR)/*.cpp) $(wildcard $(SRC_DIR)/*/*.cpp)
OBJ_FILES = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(CPP_FILES))
VERT_FILES = $(wildcard $(SHADER_DIR)/*.vert)
FRAG_FILES = $(wildcard $(SHADER_DIR)/*.frag)
SPV_FILES = $(VERT_FILES:$(SHADER_DIR)/%.vert=$(OBJ_DIR)/%.vert.spv) $(FRAG_FILES:$(SHADER_DIR)/%.frag=$(OBJ_DIR)/%.frag.spv)

# Targets
.PHONY: all clean shaders python_module

# Default target
all: shaders festi.exe python_module

# Ensure OBJ_DIR exists
$(OBJ_DIR):
	mkdir $(OBJ_DIR)

# Compile shaders
$(OBJ_DIR)/%.vert.spv: $(SHADER_DIR)/%.vert
	@echo "Compiling vertex shader $<..."
	@$(GLSLC) $< -o $@ || (echo "Failed to compile $<" && exit 1)

$(OBJ_DIR)/%.frag.spv: $(SHADER_DIR)/%.frag
	@echo "Compiling fragment shader $<..."
	@$(GLSLC) $< -o $@ || (echo "Failed to compile $<" && exit 1)

# Compile source files into object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo "Compiling $<..."
	@mkdir -p $(dir $@)
	$(CXX) $(CFLAGS) -c $< -o $@ $(INCLUDE_DIRS)

# Link object files to create the executable
festi.exe: $(OBJ_FILES) $(SPV_FILES)
	@echo "Linking object files..."
	@mkdir -p $(OBJ_DIR)
	@$(CXX) -o $@ $(OBJ_FILES) $(LIB_DIRS) $(LIBS) || (echo "Failed to link $@" && exit 1)

# Build the Python extension module (.pyd file)
python_module: $(OBJ_FILES) | $(OBJ_DIR)
	@echo "Creating Python extension module..."
	$(CXX) -shared -o $(VENV_PYTHON_DIR)/festi.pyd $(OBJ_FILES) $(LIB_DIRS) $(LIBS) || (echo "Failed to build Python extension" && exit 1)

# Clean target
clean:
	@echo "Cleaning up..."
	rm -f $(OBJ_DIR)/*.o
	rm -f festi.exe
	rm -f $(SHADER_DIR)/*.spv
	rm -f $(VENV_PYTHON_DIR)/festi.pyd
	rm -rf $(OBJ_DIR)
