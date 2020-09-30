cmake_minimum_required(VERSION 3.5)

project(httq LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 COMPONENTS Core Network WebSockets REQUIRED)

set(src
  include/httq/AbstractServer.h
  include/httq/DataStream.h
  include/httq/HttpRequest.h
  include/httq/Logger.h

  AbstractServer.cpp
  DataStream.cpp
  HttpRequest.cpp
  Logger.cpp
)

add_library(httq SHARED ${src})

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/
  DESTINATION include
  FILES_MATCHING PATTERN "*.h")

target_include_directories(httq PRIVATE include/)

include(GNUInstallDirs)
install(TARGETS httq
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

target_link_libraries(httq http_parser Qt5::Core Qt5::Network Qt5::WebSockets)
