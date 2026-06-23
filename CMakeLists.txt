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

# ---- 查找系统 SQLite3 ----
find_package(SQLite3 QUIET)
if(NOT SQLite3_FOUND)
    # 如果系统没有 SQLite3，用 FetchContent 拉取
    include(FetchContent)
    FetchContent_Declare(
        sqlite3
        URL https://www.sqlite.org/2024/sqlite-amalgamation-3450000.zip
    )
    FetchContent_MakeAvailable(sqlite3)
    add_library(sqlite3_lib STATIC ${sqlite3_SOURCE_DIR}/sqlite3.c)
    target_include_directories(sqlite3_lib PUBLIC ${sqlite3_SOURCE_DIR})
    set(SQLITE3_LIBRARIES sqlite3_lib)
    set(SQLITE3_INCLUDE_DIRS ${sqlite3_SOURCE_DIR})
else()
    set(SQLITE3_LIBRARIES SQLite::SQLite3)
    set(SQLITE3_INCLUDE_DIRS "")
endif()

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
    src/core/device/manager/device_registry.cpp
    src/core/device/protocol_adapters/mqtt_adapter/mqtt_client.cpp
    src/core/storage/sqlite_storage.cpp
    src/services/web_services/api/device_api.cpp
    src/services/web_services/api/rest_api.cpp
    src/services/web_services/api/rule_api.cpp
    src/services/web_services/api/camera_api.cpp
    src/services/web_services/websocket/websocket_server.cpp
    src/gateway/gateway_core.cpp
)
target_include_directories(iotgw_common PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/generated
)
target_link_libraries(iotgw_common PUBLIC
    mongoose_lib
    ryml
    ${SQLITE3_LIBRARIES}
)
if(SQLITE3_INCLUDE_DIRS)
    target_include_directories(iotgw_common PUBLIC ${SQLITE3_INCLUDE_DIRS})
endif()

# ---- 可执行文件 ----
add_executable(iotgw_gateway
    src/gateway/main.cpp
)
target_link_libraries(iotgw_gateway PRIVATE
    iotgw_common
    pthread
    dl
)

add_executable(iotgw_mqtt_broker
    src/tools/mqtt_broker/main.c
)
target_link_libraries(iotgw_mqtt_broker PRIVATE
    mongoose_lib
)
