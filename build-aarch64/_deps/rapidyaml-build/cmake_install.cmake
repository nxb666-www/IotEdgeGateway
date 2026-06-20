# Install script for directory: /home/nie/IotEdgeGateway/build/_deps/rapidyaml-src

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "TRUE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/aarch64-linux-gnu-objdump")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/nie/IotEdgeGateway/build-aarch64/_deps/rapidyaml-build/libryml.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/allocator.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/base64.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/blob.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/bitmask.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/charconv.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/c4_pop.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/c4_push.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/char_traits.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/common.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/compiler.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/config.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/cpu.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/ctor_dtor.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/dump.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/enum.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/error.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/export.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/format.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/gcc-4.8.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/hash.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/language.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/memory_resource.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/memory_util.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/platform.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/preprocessor.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/restrict.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/span.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4/std" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/std/std.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4/std" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/std/std_fwd.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4/std" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/std/string.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4/std" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/std/string_fwd.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4/std" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/std/string_view.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4/std" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/std/tuple.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4/std" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/std/vector.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4/std" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/std/vector_fwd.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/substr.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/substr_fwd.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/szconv.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/type_name.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/types.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/unrestrict.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/utf.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/windows.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/version.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/windows_pop.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/windows_push.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4/ext/debugbreak" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/ext/debugbreak/debugbreak.h")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4/ext/rng" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/ext/rng/rng.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4/ext/sg14" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/ext/sg14/inplace_function.h")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4/ext" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/ext/fast_float.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4/ext" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/ext/fast_float_all.h")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/ext/c4core/src/c4/c4core.natvis")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/src/ryml.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/src/ryml_std.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4/yml/detail" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/src/c4/yml/detail/checks.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4/yml/detail" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/src/c4/yml/detail/dbgprint.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4/yml/detail" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/src/c4/yml/detail/print.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4/yml/detail" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/src/c4/yml/detail/stack.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4/yml" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/src/c4/yml/common.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4/yml" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/src/c4/yml/emit.def.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4/yml" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/src/c4/yml/emit.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4/yml" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/src/c4/yml/event_handler_stack.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4/yml" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/src/c4/yml/event_handler_tree.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4/yml" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/src/c4/yml/filter_processor.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4/yml" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/src/c4/yml/fwd.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4/yml" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/src/c4/yml/export.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4/yml" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/src/c4/yml/node.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4/yml" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/src/c4/yml/node_type.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4/yml" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/src/c4/yml/parser_state.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4/yml" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/src/c4/yml/parse.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4/yml" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/src/c4/yml/parse_engine.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4/yml" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/src/c4/yml/parse_engine.def.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4/yml" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/src/c4/yml/preprocess.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4/yml" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/src/c4/yml/reference_resolver.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4/yml/std" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/src/c4/yml/std/map.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4/yml/std" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/src/c4/yml/std/std.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4/yml/std" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/src/c4/yml/std/string.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4/yml/std" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/src/c4/yml/std/vector.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4/yml" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/src/c4/yml/tag.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4/yml" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/src/c4/yml/tree.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4/yml" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/src/c4/yml/version.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4/yml" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/src/c4/yml/writer.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c4/yml" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/src/c4/yml/yml.hpp")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES "/home/nie/IotEdgeGateway/build/_deps/rapidyaml-src/src/ryml.natvis")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/ryml/rymlTargets.cmake")
    file(DIFFERENT EXPORT_FILE_CHANGED FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/ryml/rymlTargets.cmake"
         "/home/nie/IotEdgeGateway/build-aarch64/_deps/rapidyaml-build/CMakeFiles/Export/lib/cmake/ryml/rymlTargets.cmake")
    if(EXPORT_FILE_CHANGED)
      file(GLOB OLD_CONFIG_FILES "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/ryml/rymlTargets-*.cmake")
      if(OLD_CONFIG_FILES)
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/ryml/rymlTargets.cmake\" will be replaced.  Removing files [${OLD_CONFIG_FILES}].")
        file(REMOVE ${OLD_CONFIG_FILES})
      endif()
    endif()
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/ryml" TYPE FILE FILES "/home/nie/IotEdgeGateway/build-aarch64/_deps/rapidyaml-build/CMakeFiles/Export/lib/cmake/ryml/rymlTargets.cmake")
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/ryml" TYPE FILE FILES "/home/nie/IotEdgeGateway/build-aarch64/_deps/rapidyaml-build/CMakeFiles/Export/lib/cmake/ryml/rymlTargets-debug.cmake")
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/ryml" TYPE FILE FILES
    "/home/nie/IotEdgeGateway/build-aarch64/_deps/rapidyaml-build/export_cases/lib//cmake/ryml/rymlConfig.cmake"
    "/home/nie/IotEdgeGateway/build-aarch64/_deps/rapidyaml-build/export_cases/lib//cmake/ryml/rymlConfigVersion.cmake"
    )
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/nie/IotEdgeGateway/build-aarch64/_deps/rapidyaml-build/subprojects/c4core/build/cmake_install.cmake")

endif()

