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

add_executable(rgb_video src/rgb_video.cpp)
target_include_directories(rgb_video PRIVATE ${GST_INCLUDE_DIRS})
target_link_libraries(rgb_video PRIVATE ${GST_LIBRARIES} depthai::core)

add_executable(encoded_video src/encoded_video.cpp)
target_include_directories(encoded_video PRIVATE ${GST_INCLUDE_DIRS})
target_link_libraries(encoded_video PRIVATE ${GST_LIBRARIES} depthai::core)