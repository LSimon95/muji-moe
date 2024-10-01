cmake_minimum_required(VERSION 3.16)

# Set app name.
set(APP_NAME muji_moe)
# Set directory paths.
set(SDK_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../lib/cubsim)
set(CORE_PATH ${SDK_ROOT_PATH}/Core)
set(FRAMEWORK_PATH ${SDK_ROOT_PATH}/Framework)
set(THIRD_PARTY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../lib)
set(STB_PATH ${THIRD_PARTY_PATH}/stb)
set(GLEW_PATH ${THIRD_PARTY_PATH}/glew)
set(GLFW_PATH ${THIRD_PARTY_PATH}/glfw)
set(CPR_PATH ${THIRD_PARTY_PATH}/cpr)
set(ASIO_PATH ${THIRD_PARTY_PATH}/asio-1.30.2/)
set(LIBSOUNDIO ${THIRD_PARTY_PATH}/libsoundio)
set(RES_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../Resources)

# Set project.
project(${APP_NAME} VERSION 0.1.0)

# Define output directory.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
# Set configuration (Release and Debug only).
set(CMAKE_CONFIGURATION_TYPES Debug Release
  CACHE STRING "Configurations" FORCE
)
# Suppress generation of ZERO_CHECK project.
set(CMAKE_SUPPRESS_REGENERATION ON)

# Surpress GLEW and GLFW process.
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(BUILD_UTILS OFF CACHE BOOL "" FORCE)

# Specify version of compiler.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add Cubism Core.
# Import as static library.
add_library(Live2DCubismCore STATIC IMPORTED)
# Get architecture.
EXECUTE_PROCESS( COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE ARCHITECTURE )
# Set library path and inlude path.
set_target_properties(Live2DCubismCore
  PROPERTIES
    IMPORTED_LOCATION ${CORE_PATH}/lib/macos/${ARCHITECTURE}/libLive2DCubismCore.a
    INTERFACE_INCLUDE_DIRECTORIES ${CORE_PATH}/include
)

# Add GLEW ,GLFW.
add_subdirectory(${GLEW_PATH}/build/cmake ${CMAKE_CURRENT_BINARY_DIR}/build/glew)
add_subdirectory(${GLFW_PATH} ${CMAKE_CURRENT_BINARY_DIR}/build/glfw)

# Add Cpr
add_subdirectory(${CPR_PATH} ${CMAKE_CURRENT_BINARY_DIR}/build/cpr)

# Specify Cubism Framework rendering.
set(FRAMEWORK_SOURCE OpenGL)
# Add Cubism Native Framework.
add_subdirectory(${FRAMEWORK_PATH} ${CMAKE_CURRENT_BINARY_DIR}/build/Framework)
# Add rendering definition to framework.
target_compile_definitions(Framework PUBLIC CSM_TARGET_MAC_GL)
# Add include path of GLEW to framework.
target_include_directories(Framework PUBLIC ${GLEW_PATH}/include)
# Link libraries to framework.
target_link_libraries(Framework Live2DCubismCore glew_s)

# Add asio.
add_library(asio INTERFACE)
target_include_directories(asio INTERFACE ${ASIO_PATH}/include)
target_link_libraries(asio INTERFACE pthread)

# Add libsoundio.
set(BUILD_EXAMPLE_PROGRAMS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(${LIBSOUNDIO} ${CMAKE_CURRENT_BINARY_DIR}/build/libsoundio)

# Find opengl libraries.
find_package(OpenGL REQUIRED)

# Make executable app.
add_executable(${APP_NAME})
# Add source files.
add_subdirectory(../src ${CMAKE_CURRENT_BINARY_DIR}/build/src)
set(VOICE_CHAT ${CMAKE_PROJECT_VERSION})
configure_file(../src/version.h.cofig ../src/version.h @ONLY)
# Link libraries to app.
target_link_libraries(${APP_NAME}
  Framework
  glfw
  cpr
  asio
  libsoundio_shared
  ${LIBSOUNDIO_LIBS}
  ${OPENGL_LIBRARIES}
  ${THIRD_PARTY_PATH}/jsoncpp/lib/libjsoncpp.a
)
# Specify include directories.
target_include_directories(${APP_NAME} PRIVATE ${STB_PATH})
target_include_directories(${APP_NAME} PRIVATE ${CPR_PATH}/include)
target_include_directories(${APP_NAME} PRIVATE ${THIRD_PARTY_PATH}/jsoncpp/include)
target_include_directories(${APP_NAME} PRIVATE ${LIBSOUNDIO})


file(COPY ${CMAKE_CURRENT_BINARY_DIR}/build/cpr/cpr_generated_includes/cpr/cprver.h DESTINATION ${THIRD_PARTY_PATH}/cpr/include/cpr)

# Copy resource directory to build directory.
add_custom_command(
  TARGET ${APP_NAME}
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND} -E
      copy_directory ${RES_PATH} $<TARGET_FILE_DIR:${APP_NAME}>/Resources
)

# You can change target that renderer draws by enabling following definition.
#
# * USE_RENDER_TARGET
#   Renderer draws to target of LAppView.
# * USE_MODEL_RENDER_TARGET
#   Renderer draws to target of each LAppModel.
# * default
#   Renderer draws to default main framebuffer.
#
# INFO: USE_RENDER_TARGET has higher priority than USE_MODEL_RENDER_TARGET.
#
# target_compile_definitions(${APP_NAME}
#   PRIVATE
#     USE_RENDER_TARGET
#     USE_MODEL_RENDER_TARGET
# )
