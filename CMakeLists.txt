cmake_minimum_required(VERSION 3.5)

project(httq VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(HTTQ_BUILD_TESTS "Build httq tests" OFF)

find_package(Qt NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Network WebSockets)
message(STATUS "Using Qt${Qt_VERSION_MAJOR}")
find_package(Qt${Qt_VERSION_MAJOR} COMPONENTS Core Network WebSockets REQUIRED)


# http-parser as own, c-only target
add_library(http_parser STATIC
  http-parser/http_parser.c
)
set_target_properties(http_parser PROPERTIES LINKER_LANGUAGE C)
target_include_directories(http_parser
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/http-parser>
    $<INSTALL_INTERFACE:include>
)


# httq
set(src
  include/httq/AbstractServer.h
  include/httq/AbstractHandler.h
  include/httq/AbstractBodyHandler.h
  include/httq/BodyReader.h
  include/httq/DataStream.h
  include/httq/HandlerServer.h
  include/httq/HttpRequest.h
  include/httq/Logger.h

  AbstractServer.cpp
  AbstractHandler.cpp
  AbstractBodyHandler.cpp
  BodyReader.cpp
  DataStream.cpp
  HandlerServer.cpp
  HttpRequest.cpp
  Logger.cpp
)

add_library(httq SHARED ${src})
add_library(httq::httq ALIAS httq)

include(GNUInstallDirs)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING PATTERN "*.h")
install(FILES ${CMAKE_SOURCE_DIR}/http-parser/http_parser.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

target_include_directories(httq
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/http-parser>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

install(TARGETS httq
  EXPORT httqTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(TARGETS http_parser
  EXPORT httqTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

target_link_libraries(httq http_parser Qt${Qt_VERSION_MAJOR}::Core Qt${Qt_VERSION_MAJOR}::Network Qt${Qt_VERSION_MAJOR}::WebSockets)


# tests
if(HTTQ_BUILD_TESTS)
  find_package(Qt${Qt_VERSION_MAJOR} COMPONENTS Test REQUIRED)
  enable_testing()
  add_subdirectory(tests)
endif()


# cmake find_package
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/httqConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(EXPORT httqTargets
  FILE httqTargets.cmake
  NAMESPACE httq::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/httq)

configure_package_config_file(
  ${CMAKE_SOURCE_DIR}/cmake/httqConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/httqConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/httq
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/httqConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/httqConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/httq)

