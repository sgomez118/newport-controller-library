add_executable(xps_tests
    mock_xps_server.cc
    # Internal classes are compiled directly into the test executable so tests
    # can reach them without exporting symbols from the DLL.
    ${CMAKE_SOURCE_DIR}/src/internal/tcp_socket.cc
)

target_link_libraries(xps_tests PRIVATE
    newport_xps
    GTest::gtest_main
)

target_include_directories(xps_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    # Exposes src/internal/tcp_socket.h to tests via #include "internal/..."
    ${CMAKE_SOURCE_DIR}/src
)

target_compile_options(xps_tests PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

if(WIN32)
    target_link_libraries(xps_tests PRIVATE Ws2_32)
endif()

add_custom_command(TARGET xps_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:newport_xps>
        $<TARGET_FILE_DIR:xps_tests>
)

include(GoogleTest)
gtest_discover_tests(xps_tests)
