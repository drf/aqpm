
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

set(sync_test_SRCS
    main_synchronous.cpp
)

automoc4_add_executable(synctest
    ${sync_test_SRCS}
)

target_link_libraries(synctest
    ${QT_QTCORE_LIBRARY}
    aqpm
)

#############

set(aqpm_benchmark_SRCS
    AqpmBenchmark.cpp
)

automoc4_add_executable(aqpmbenchmark
    ${aqpm_benchmark_SRCS}
)

target_link_libraries(aqpmbenchmark
    ${QT_QTCORE_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    ${QT_QTTEST_LIBRARY}
    aqpm
)

