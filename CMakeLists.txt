cmake_minimum_required(VERSION 3.16)
project(IotEdgeGateway VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---- 生成版本头文件 ----
set(IOTGW_VERSION "${PROJECT_VERSION}")
configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/version.hpp.in
    ${CMAKE_BINARY_DIR}/generated/version.hpp
)

# ---- FetchContent 拉依赖 ----
include(FetchContent)

# Mongoose（C库，单文件）
FetchContent_Declare(
    mongoose
    GIT_REPOSITORY https://github.com/cesanta/mongoose.git
    GIT_TAG        7.15
)
FetchContent_MakeAvailable(mongoose)

# rapidyaml
set(RYML_WITH_DEFAULT_CALLBACKS ON CACHE BOOL "" FORCE)
FetchContent_Declare(
    rapidyaml
    GIT_REPOSITORY https://github.com/biojppm/rapidyaml.git
    GIT_TAG        v0.10.0
)
FetchContent_MakeAvailable(rapidyaml)

# ---- Mongoose 静态库（C代码单独编译）----
add_library(mongoose_lib STATIC
    ${mongoose_SOURCE_DIR}/mongoose.c
)
target_include_directories(mongoose_lib PUBLIC
    ${mongoose_SOURCE_DIR}
)

# ---- 通用静态库（业务代码）----
add_library(iotgw_common STATIC
    src/core/common/logger/file_logger.cpp
    src/core/common/config/config_manager.cpp
    src/services/web_services/api/system_api.cpp
    src/gateway/gateway_core.cpp
)
target_include_directories(iotgw_common PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/generated
)
target_link_libraries(iotgw_common PUBLIC
    mongoose_lib
    ryml
)

# ---- 可执行文件 ----
add_executable(iotgw_gateway
    src/gateway/main.cpp
)
target_link_libraries(iotgw_gateway PRIVATE
    iotgw_common
)
