cmake_minimum_required(VERSION 3.15)

cmake_policy(SET CMP0091 NEW)
cmake_policy(SET CMP0077 NEW)

project(QuantumVk LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded$<$<CONFIG:Debug>:Debug>)

set(QM_INSTALL OFF)
set(GLFW_INSTALL OFF)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/QuantumVk)

set(GLFW_BUILD_DOCS OFF)
set(BUILD_SHARED_LIBS OFF)
set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(USE_MSVC_RUNTIME_LIBRARY_DLL OFF)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/dependencies/glfw)

macro(add_example name sources)

add_executable(${name})

target_link_libraries(${name} PRIVATE QuantumVk)
target_link_libraries(${name} PRIVATE glfw)

target_include_directories(${name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/QuantumVk)
target_include_directories(${name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/glfw/include)
target_include_directories(${name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/glm)

target_sources(${name} PUBLIC ${sources})

include(GNUInstallDirs)	
install(TARGETS ${name} CONFIGURATIONS Debug DESTINATION ${name}_debug)
install(TARGETS ${name} CONFIGURATIONS Release DESTINATION ${name}_release)

endmacro()

add_example(noise examples/noise/main.cpp)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/examples/noise/spirv/vertex.spv ${CMAKE_CURRENT_SOURCE_DIR}/examples/noise/spirv/fragment.spv CONFIGURATIONS Debug DESTINATION noise_debug/spirv)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/examples/noise/spirv/vertex.spv ${CMAKE_CURRENT_SOURCE_DIR}/examples/noise/spirv/fragment.spv CONFIGURATIONS Release DESTINATION noise_release/spirv)

add_example(mesh_viewer examples/mesh_viewer/main.cpp)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/examples/mesh_viewer/spirv/vertex.spv ${CMAKE_CURRENT_SOURCE_DIR}/examples/mesh_viewer/spirv/fragment.spv CONFIGURATIONS Debug DESTINATION mesh_viewer_debug/spirv)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/examples/mesh_viewer/spirv/vertex.spv ${CMAKE_CURRENT_SOURCE_DIR}/examples/mesh_viewer/spirv/fragment.spv CONFIGURATIONS Release DESTINATION mesh_viewer_release/spirv)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/examples/mesh_viewer/model.obj ${CMAKE_CURRENT_SOURCE_DIR}/examples/mesh_viewer/diffuse.png CONFIGURATIONS Debug DESTINATION mesh_viewer_debug)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/examples/mesh_viewer/model.obj ${CMAKE_CURRENT_SOURCE_DIR}/examples/mesh_viewer/diffuse.png CONFIGURATIONS Release DESTINATION mesh_viewer_release)
