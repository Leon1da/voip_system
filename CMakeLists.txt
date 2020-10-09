cmake_minimum_required(VERSION 3.16)
project(chat_udp)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_THREAD_LIBS_INIT "-lpthread")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
set(CMAKE_HAVE_THREADS_LIBRARY 1)
set(CMAKE_USE_WIN32_THREADS_INIT 0)
set(CMAKE_USE_PTHREADS_INIT 1)
set(THREADS_PREFER_PTHREAD_FLAG ON)

# client
add_executable(client Client.cpp common.h client.h ConnectionManager.cpp ConnectionManager.h AudioManager.cpp AudioManager.h ipify.cpp ipify.h user.h peer.h)

# server
add_executable(server Server.cpp common.h server.h ConnectionManager.cpp ConnectionManager.h user.h)


find_package(ALSA REQUIRED)
if (ALSA_FOUND)
    include_directories(${ALSA_INCLUDE_DIRS})
    target_link_libraries (client ${ALSA_LIBRARIES})

endif(ALSA_FOUND)


