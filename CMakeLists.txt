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
add_executable(client client.cpp config.h client.h MessageManager.cpp MessageManager.h AudioManager.cpp AudioManager.h ipify.cpp ipify.h )

# server
add_executable(server server.cpp config.h server.h MessageManager.cpp MessageManager.h)


find_package(ALSA REQUIRED)
if (ALSA_FOUND)
    include_directories(${ALSA_INCLUDE_DIRS})
    target_link_libraries (client ${ALSA_LIBRARIES})

endif(ALSA_FOUND)


