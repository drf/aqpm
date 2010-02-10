
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

qt4_add_dbus_adaptor(aqpmabsworker_adaptor_SRCS org.chakraproject.aqpmabsworker.xml Worker_p.h Aqpm::AbsWorker::Worker)

set(aqpmabsworker_SRCS
    main.cpp
    Worker.cpp
    ${CMAKE_SOURCE_DIR}/misc/TemporizedApplication.cpp
)

automoc4_add_executable(aqpmabsworker
    ${aqpmabsworker_SRCS}
    ${aqpmabsworker_adaptor_SRCS}
)

target_link_libraries(aqpmabsworker
    ${QT_QTCORE_LIBRARY}
    ${QT_QTDBUS_LIBRARY}
    ${POLKITQT-1_CORE_LIBRARY}
    aqpm
)

install(CODE "message(STATUS \"Installing AqpmAbs worker...\")")
install(TARGETS aqpmabsworker DESTINATION ${AQPM_HELPER_INSTALL_DIR})

dbus_add_activation_system_service(org.chakraproject.aqpmabsworker.service.in)

install(FILES org.chakraproject.aqpmabsworker.conf DESTINATION /etc/dbus-1/system.d)
