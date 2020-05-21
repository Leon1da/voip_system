cmake_minimum_required(VERSION 2.8)
project(client_server_tcp)
set(CMAKE_CXX_STANDARD 14)

set(CMAKE_THREAD_LIBS_INIT "-lpthread")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
set(CMAKE_HAVE_THREADS_LIBRARY 1)
set(CMAKE_USE_WIN32_THREADS_INIT 0)
set(CMAKE_USE_PTHREADS_INIT 1)
set(THREADS_PREFER_PTHREAD_FLAG ON)

# client
add_executable(client client.cpp send_recv.cpp methods.h)

# server
add_executable(server server.cpp utils.h send_recv.cpp methods.h)
