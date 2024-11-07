#pragma once

#include "tga/tga.hpp"
#include "tga/tga_utils.hpp"
#include "tga/tga_vulkan/tga_vulkan.hpp"
#include "tga/tga_math.hpp"

#include <glm/gtc/random.hpp>

#define toui8 reinterpret_cast<uint8_t*>
#define ref std::addressof

#include "gpro/data.hpp"
#include "gpro/utils.hpp"
#include "gpro/mesh.hpp"
#include "gpro/file.hpp"
#include "gpro/camera_controller.hpp"