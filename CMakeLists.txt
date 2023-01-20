cmake_minimum_required(VERSION 3.10)
project(oak)

find_package(depthai CONFIG REQUIRED)

find_package(PkgConfig)
pkg_check_modules(GST REQUIRED  gstreamer-1.0>=1.2
        gstreamer-sdp-1.0>=1.2
        gstreamer-video-1.0>=1.2
        gstreamer-app-1.0>=1.2)

# add_executable(oak_test src/main.cpp)
# target_link_libraries(oak_test PRIVATE depthai::core)

function(add_target name)
    add_executable(${name} src/${name}.cpp)
    target_include_directories(${name} PRIVATE ${GST_INCLUDE_DIRS})
    target_link_libraries(${name} PRIVATE ${GST_LIBRARIES} depthai::core)
endfunction()

add_target(rgb_video)
add_target(encoded_video)
add_target(depth_basic)
