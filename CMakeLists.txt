# CorridorKey 2 for Baselight - build (Linux x86_64; also builds on macOS for
# development, where only the tests are meaningful).
# Based on CorridorKey 2 for Resolve by Creative Video Tips.
# Licence: CC BY-NC-SA 4.0 with the CorridorKey additional terms (see LICENSE).
cmake_minimum_required(VERSION 3.20)
project(CorridorKey2 CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Source paths compiled into the binary (asserts, error messages) say "~"
# instead of the builder's home folder, so no user name ships.
add_compile_options("-ffile-prefix-map=$ENV{HOME}=~")

set(OFX ${CMAKE_SOURCE_DIR}/third_party/openfx)
file(GLOB OFX_SUPPORT ${OFX}/Support/Library/*.cpp)

# OpenEXR + Imath, built static by third_party/build_openexr.sh (never the system's).
set(CK2_DEPS ${CMAKE_SOURCE_DIR}/build/deps/install)
find_package(Imath CONFIG REQUIRED PATHS ${CK2_DEPS} NO_DEFAULT_PATH)
find_package(OpenEXR CONFIG REQUIRED PATHS ${CK2_DEPS} NO_DEFAULT_PATH)
find_package(Threads REQUIRED)

add_library(ck2core STATIC src/ck2_exr.cpp src/ck2_job.cpp)
target_include_directories(ck2core PUBLIC src)
target_link_libraries(ck2core PUBLIC OpenEXR::OpenEXR Threads::Threads)
target_compile_options(ck2core PRIVATE -Wall -Wextra)

add_library(CorridorKey2 MODULE src/ck2_plugin.cpp ${OFX_SUPPORT})
target_include_directories(CorridorKey2 PRIVATE ${OFX}/include ${OFX}/Support/include
                           ${OFX}/Support/Library)
target_link_libraries(CorridorKey2 PRIVATE ck2core ${CMAKE_DL_LIBS})
set_target_properties(CorridorKey2 PROPERTIES PREFIX "" SUFFIX ".ofx")

# Only the OFX entry points are exported.
if(APPLE)
  set_target_properties(CorridorKey2 PROPERTIES CXX_VISIBILITY_PRESET hidden)
  target_link_options(CorridorKey2 PRIVATE
    -Wl,-exported_symbol,_OfxGetNumberOfPlugins -Wl,-exported_symbol,_OfxGetPlugin)
else()
  # OfxExport doesn't mark the entry points visible on Linux, so a version
  # script keeps them global and everything else (OpenEXR included) local.
  # Static libstdc++/libgcc: the plugin doesn't depend on the host's C++ runtime.
  target_link_options(CorridorKey2 PRIVATE
    -Wl,--version-script=${CMAKE_SOURCE_DIR}/src/exports.map
    -static-libstdc++ -static-libgcc -Wl,--no-undefined)
endif()

add_executable(ck2_tests tests/test_core.cpp)
target_link_libraries(ck2_tests PRIVATE ck2core)
