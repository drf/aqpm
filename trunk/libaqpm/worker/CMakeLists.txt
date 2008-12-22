
INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

qt4_add_dbus_adaptor(aqpmworker_adaptor_SRCS org.chakraproject.aqpmworker.xml Worker.h AqpmWorker::Worker)

SET(aqpmworker_SRCS 
    main.cpp
    Worker.cpp
    w_callbacks.cpp
)

SET(aqpmworker_MOC_HDRS
    Worker.h
    w_callbacks.h
)

QT4_WRAP_CPP(aqpmworker_MOC_SRCS ${aqpmworker_MOC_HDRS})
QT4_AUTOMOC(${aqpmworker_SRCS})
                      
add_executable(aqpmworker
    ${aqpmworker_SRCS}
    ${aqpmworker_adaptor_SRCS}
    ${aqpmworker_MOC_SRCS}
)

TARGET_LINK_LIBRARIES(aqpmworker
    ${QT_QTCORE_LIBRARY}
    ${QT_QTDBUS_LIBRARY}
    ${ALPM_LIBS}
    ${LIBARCHIVE_LIBS}
    aqpm
)

INSTALL(CODE "MESSAGE(\"Installing Aqpm worker...\")")
install(TARGETS aqpmworker RUNTIME DESTINATION bin
                     LIBRARY DESTINATION lib
                     ARCHIVE DESTINATION lib)
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/aqpm.conf DESTINATION /etc/dbus-1/system.d)

