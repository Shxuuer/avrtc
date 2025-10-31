cmake_minimum_required(VERSION 3.10)
project(AVRTC VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM REQUIRED gtkmm-3.0)

add_subdirectory(third_party/googletest)
add_subdirectory(third_party/glog)

file(GLOB BASE_SRCS base/*.cc)

# client and server
function(add_avrtc_target TARGET_NAME MAIN_SRCS)
    add_executable(${TARGET_NAME}
        ${BASE_SRCS} ${MAIN_SRCS}
    )
    target_link_libraries(${TARGET_NAME}
        PRIVATE
            glog ${GTKMM_LIBRARIES}
    )
    target_include_directories(${TARGET_NAME}
        PRIVATE
            . ${GTKMM_INCLUDE_DIRS}
    )
endfunction()

set(CLIENT_MAIN_SRCS
    example/client.cc
    example/ui.cc
)
set(SERVER_MAIN_SRCS
    example/server.cc
    example/ui.cc
)
add_avrtc_target(client "${CLIENT_MAIN_SRCS}")
add_avrtc_target(server "${SERVER_MAIN_SRCS}")

# tests
include(GoogleTest)
enable_testing()
function(add_single_test_target TEST_FILE)
    # 将文件名xx.cc编译后的名字改为test_xx
    get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
    set(TARGET_NAME "test_${TEST_NAME}")
    add_executable(${TARGET_NAME} 
        ${BASE_SRCS} ${TEST_FILE}
    )
    target_link_libraries(${TARGET_NAME}
        PRIVATE
            gtest gtest_main glog ${GTKMM_LIBRARIES}
    )
    target_include_directories(${TARGET_NAME}
        PRIVATE
            . ${GTKMM_INCLUDE_DIRS}
    )
    gtest_discover_tests(${TARGET_NAME})
endfunction()

file(GLOB TEST_SRCS "tests/*.cc")
foreach(TEST_SRC_FILE IN LISTS TEST_SRCS)
    add_single_test_target(${TEST_SRC_FILE})
endforeach()