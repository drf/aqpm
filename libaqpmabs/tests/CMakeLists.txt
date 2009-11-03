
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

set(updateall_test_SRCS
    main_updateall.cpp
)

automoc4_add_executable(updatealltest
    ${updateall_test_SRCS}
)

target_link_libraries(updatealltest
    ${QT_QTCORE_LIBRARY}
    aqpmabs
)

