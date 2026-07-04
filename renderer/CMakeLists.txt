if (NOT IS_DIRECTORY $ENV{VULKAN_SDK})
    message(WARNING "Vulkan SDK not detected, if installed please set VULKAN_SDK env var or manually provide libraries")
else()
    message(STATUS "Found Vulkan SDK in $ENV{VULKAN_SDK}")
endif()

include_directories($ENV{VULKAN_SDK}/Include)
include_directories($ENV{VULKAN_SDK})
include_directories(../deps/SDL/include)
include_directories(../deps/spirv_reflect)

add_library (renderer STATIC "device.cpp" "device.h" "queue.h" "allocator.h" "vma_impl.cpp" "allocator.cpp" "surface.h" "surface.cpp" "swapchain.h" "swapchain.cpp" "image.h" "image.cpp" "shader.h" "shader.cpp" "buffer.h" "buffer.cpp" "shader_compiler.h" "shader_compiler.cpp" "../deps/spirv_reflect/spirv_reflect.h" "../deps/spirv_reflect/spirv_reflect.c"  "descriptor_set_layout.h" "descriptor_set_layout.cpp" "pipeline_layout.h" "pipeline_layout.cpp" "pipeline.h" "pipeline.cpp" "descriptor_pool.h" "descriptor_pool.cpp" "descriptor_set.h"   "image_view.h" "image_view.cpp")

target_include_directories(renderer PUBLIC $ENV{VULKAN_SDK}/Include)
target_include_directories(renderer PUBLIC $ENV{VULKAN_SDK})
target_include_directories(renderer PUBLIC ../deps/spirv_reflect)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET renderer PROPERTY CXX_STANDARD 23)
endif()

find_package(Vulkan REQUIRED COMPONENTS volk dxc)

target_link_libraries(renderer PRIVATE Vulkan::volk)
target_link_libraries(renderer PRIVATE Vulkan::dxc_lib)
