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

add_executable(basic_example)

target_link_libraries(basic_example PRIVATE QuantumVk)
target_link_libraries(basic_example PRIVATE glfw)

target_include_directories(basic_example PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/QuantumVk)
target_include_directories(basic_example PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/glfw/include)

set(BASIC_EXAMPLE_SRC examples/basic/main.cpp)

target_sources(basic_example PUBLIC ${BASIC_EXAMPLE_SRC})

include(GNUInstallDirs)	
install(TARGETS basic_example CONFIGURATIONS Debug DESTINATION basic_example_debug)
install(TARGETS basic_example CONFIGURATIONS Release DESTINATION basic_example_release)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/examples/basic/spirv/vertex.spv ${CMAKE_CURRENT_SOURCE_DIR}/examples/basic/spirv/fragment.spv CONFIGURATIONS Debug DESTINATION basic_example_debug/spirv)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/examples/basic/spirv/vertex.spv ${CMAKE_CURRENT_SOURCE_DIR}/examples/basic/spirv/fragment.spv CONFIGURATIONS Release DESTINATION basic_example_release/spirv)
