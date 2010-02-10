include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

set(groupdep_SRCS
    main_groupdep.cpp
)

automoc4_add_executable(groupdep
    ${groupdep_SRCS}
)

target_link_libraries(groupdep
    aqpm
)
