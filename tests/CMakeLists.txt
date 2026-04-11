add_executable(xps_tests
    mock_xps_server.cc
    test_controller.cc
    test_group_motion.cc
)

target_link_libraries(xps_tests PRIVATE
    newport_xps
    GTest::gtest_main
)

# Tests need the mock server header, which lives in the tests/ directory.
target_include_directories(xps_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(xps_tests PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

add_custom_command(TARGET xps_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:newport_xps>
        $<TARGET_FILE_DIR:xps_tests>
)

include(GoogleTest)
gtest_discover_tests(xps_tests)
