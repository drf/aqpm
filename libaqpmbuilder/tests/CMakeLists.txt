
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

set(builder_test_SRCS
    main_builder.cpp
)

automoc4_add_executable(builder
    ${builder_test_SRCS}
)

target_link_libraries(builder
    ${QT_QTCORE_LIBRARY}
    aqpmbuilder
)

