if (NOT IS_DIRECTORY $ENV{VULKAN_SDK})
    message(WARNING "Vulkan SDK not detected, if installed please set VULKAN_SDK env var or manually provide libraries")
else()
    message(STATUS "Found Vulkan SDK in $ENV{VULKAN_SDK}")
endif()

include_directories($ENV{VULKAN_SDK}/Include)
include_directories(../deps/SDL/include)

add_library (renderer STATIC "device.cpp" "device.h" "queue.h" "allocator.h" "vma_impl.cpp" "allocator.cpp" "surface.h" "surface.cpp")

target_include_directories(renderer PUBLIC $ENV{VULKAN_SDK}/Include)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET renderer PROPERTY CXX_STANDARD 23)
endif()

find_package(Vulkan REQUIRED COMPONENTS volk)

target_link_libraries(renderer PRIVATE Vulkan::volk)
