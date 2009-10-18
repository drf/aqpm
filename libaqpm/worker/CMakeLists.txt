
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

qt4_add_dbus_adaptor(aqpmworker_adaptor_SRCS org.chakraproject.aqpmworker.xml Worker.h AqpmWorker::Worker)

set(aqpmworker_SRCS
    main.cpp
    Worker.cpp
    w_callbacks.cpp
    ${CMAKE_SOURCE_DIR}/misc/TemporizedApplication.cpp
)

automoc4_add_executable(aqpmworker
    ${aqpmworker_SRCS}
    ${aqpmworker_adaptor_SRCS}
)

target_link_libraries(aqpmworker
    ${QT_QTCORE_LIBRARY}
    ${QT_QTDBUS_LIBRARY}
    ${ALPM_LIBS}
    ${POLKITQT_CORE_LIBRARY}
    aqpm
)

install(CODE "message(\"Installing Aqpm worker...\")")
install(TARGETS aqpmworker DESTINATION ${AQPM_HELPER_INSTALL_DIR})

dbus_add_activation_system_service(org.chakraproject.aqpmworker.service.in)

install(FILES org.chakraproject.aqpmworker.conf DESTINATION /etc/dbus-1/system.d)
install(FILES org.chakraproject.aqpmdownloader.conf DESTINATION /etc/dbus-1/system.d)
