cmake_minimum_required(VERSION 3.7)
project(CPUSender)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/")

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_package(LibEvent REQUIRED)
set (INCLUDE_DIRECTORIES ${INCLUDE_DIRECTORIES} ${LIBEVENT_INCLUDE_DIR})

include_directories(${INCLUDE_DIRECTORIES})

add_executable(SenderIncast sender_incast.cpp
        utils/ThreadPool.h utils/ThreadPool.cpp utils/containers.h
        basic_sender.cpp basic_sender.h)

add_executable(SenderBatching sender_batching.cpp
        utils/ThreadPool.h utils/ThreadPool.cpp utils/containers.h
        basic_sender.cpp basic_sender.h)

add_executable(SenderRate sender_rate.cpp
        utils/ThreadPool.h utils/ThreadPool.cpp utils/containers.h utils/string_utils.h utils/string_utils.cpp
        utils/rate-control-info.h
        basic_sender.cpp basic_sender.h)

target_link_libraries(SenderIncast Threads::Threads ${LIBEVENT_LIBRARY} ${LIBEVENT_THREAD})
target_link_libraries(SenderBatching Threads::Threads ${LIBEVENT_LIBRARY} ${LIBEVENT_THREAD})
target_link_libraries(SenderRate Threads::Threads ${LIBEVENT_LIBRARY} ${LIBEVENT_THREAD})