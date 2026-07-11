# CMakeList.txt : CMake project for app, include source and define
# project specific logic here.
#

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)

add_subdirectory(../deps/SDL ../deps/SDL/build EXCLUDE_FROM_ALL)
add_subdirectory(../deps/glm ../deps/glm/build EXCLUDE_FROM_ALL)

# Make it easier to include stuff like renderer/...
include_directories(..)

# Add source to this project's executable.
add_executable (app "app.cpp" "app.h" "main.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET app PROPERTY CXX_STANDARD 23)
endif()

target_link_libraries(app PRIVATE SDL3::SDL3-static)
target_link_libraries(app PRIVATE glm::glm)
target_link_libraries(app PRIVATE renderer)
