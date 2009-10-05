
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

qt4_add_dbus_adaptor(aqpmconfigurator_adaptor_SRCS org.chakraproject.aqpmconfigurator.xml Configurator.h
                     AqpmConfigurator::Configurator)

set(aqpmconfigurator_SRCS
    main.cpp
    Configurator.cpp
    ${CMAKE_SOURCE_DIR}/misc/TemporizedApplication.cpp
)

automoc4_add_executable(aqpmconfigurator
    ${aqpmconfigurator_SRCS}
    ${aqpmconfigurator_adaptor_SRCS}
)

target_link_libraries(aqpmconfigurator
    ${QT_QTCORE_LIBRARY}
    ${QT_QTDBUS_LIBRARY}
    ${POLKITQT_CORE_LIBRARY}
)

install(CODE "message(\"Installing Aqpm configurator...\")")
install(TARGETS aqpmconfigurator DESTINATION ${AQPM_HELPER_INSTALL_DIR})

dbus_add_activation_system_service(org.chakraproject.aqpmconfigurator.service.in)

install(FILES org.chakraproject.aqpmconfigurator.conf DESTINATION /etc/dbus-1/system.d)

