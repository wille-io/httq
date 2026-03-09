add_executable(httq_tests
  tst_httq.cpp
)

target_include_directories(httq_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(httq_tests
  PRIVATE
    httq
    Qt${Qt_VERSION_MAJOR}::Core
    Qt${Qt_VERSION_MAJOR}::Network
    Qt${Qt_VERSION_MAJOR}::Test
)

add_test(NAME httq_tests COMMAND httq_tests)
