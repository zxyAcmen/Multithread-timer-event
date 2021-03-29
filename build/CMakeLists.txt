CMAKE_MINIMUM_REQUIRED(VERSION 2.9)
#项目名
PROJECT(timer)
#添加一个子目录并构建该子目录。
#MESSAGE( STATUS "libevent-2.1.12-stable############### = ${DIR_SRCS}.")
#查找在某个路径下的所有源文件。
AUX_SOURCE_DIRECTORY(../src DIR_SRCS)
add_definitions("-Wall -g ")
#MESSAGE( STATUS "this var key = ${DIR_SRCS}.")
#将指定目录添加到编译器的头文件搜索路径之下，指定的目录被解释成当前源码路径的相对路径。
include_directories (../include/)

#set (CMAKE_C_COMPILER "/usr/local/gcc/bin/gcc")

set (CMAKE_C_COMPILER "/usr/bin/gcc")
SET(CMAKE_BUILD_TYPE "Debug")

#if(CMAKE_COMPILER_IS_GNUCXX)
#    set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")
#    message(STATUS "optional:-std=c++11")   
#endif(CMAKE_COMPILER_IS_GNUCXX)
#上面的语句能设置可执行文件的输出目录
#set (EXECUTABLE_OUTPUT_PATH  ../bin)
ADD_EXECUTABLE(timer ${DIR_SRCS}  ../bin )
target_link_libraries( timer -lpthread )