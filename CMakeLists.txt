cmake_minimum_required(VERSION 3.16)
project(streaming_ingest_mvp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define a Windows version for cpp-httplib (Windows 10 = 0x0A00)
add_compile_definitions(_WIN32_WINNT=0x0A00)
add_compile_definitions(_GNU_SOURCE)

add_executable(ingest_service
  src/main.cpp
  src/ingest_queue.hpp
  src/http_ingest_server.h
  src/http_ingest_server.cpp
  src/parser_worker.h
  src/parser_worker.cpp
  src/batcher.h
  src/batcher.cpp
  src/writer.h
  src/writer.cpp
)

target_include_directories(ingest_service PRIVATE 
  ${CMAKE_CURRENT_SOURCE_DIR}/src 
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(WIN32)
  target_link_libraries(ingest_service PRIVATE pthread ws2_32 wsock32)
else()
  target_link_libraries(ingest_service PRIVATE pthread)
endif()