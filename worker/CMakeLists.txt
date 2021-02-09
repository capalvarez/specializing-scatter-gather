cmake_minimum_required(VERSION 3.7)
project(Worker)

set(CMAKE_CXX_STANDARD 11)
find_package(Threads REQUIRED)

add_executable(WorkerIncast base_worker.h base_worker.cpp worker_incast.cpp config/base_config.h config/string_utils.h config/string_utils.cpp)
add_executable(WorkerRate base_worker.h base_worker.cpp worker_rate.cpp config/base_config.h config/rate_config.h config/string_utils.h config/string_utils.cpp)
add_executable(WorkerBackground base_worker.h base_worker.cpp worker_background.cpp config/base_config.h config/background_config.h config/string_utils.h config/string_utils.cpp)

target_link_libraries(WorkerIncast Threads::Threads)
target_link_libraries(WorkerRate Threads::Threads)
target_link_libraries(WorkerBackground Threads::Threads)