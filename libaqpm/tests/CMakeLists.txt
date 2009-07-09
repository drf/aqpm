SET(sync_test_SRCS 
    main_synchronous.cpp
)
                      
automoc4_add_executable(synctest
    ${sync_test_SRCS}
)

TARGET_LINK_LIBRARIES(synctest
    ${QT_QTCORE_LIBRARY}
    aqpm
)
