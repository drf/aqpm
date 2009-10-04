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
