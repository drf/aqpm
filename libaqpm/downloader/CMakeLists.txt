
INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

set (AQPM_HELPER_INSTALL_DIR ${LIB_INSTALL_DIR}/aqpm)

qt4_add_dbus_adaptor(aqpmdownloader_adaptor_SRCS org.chakraproject.aqpmdownloader.xml Downloader.h AqpmDownloader::Downloader)

SET(aqpmdownloader_SRCS 
    main.cpp
    Downloader.cpp
)
                      
automoc4_add_executable(aqpmdownloader
    ${aqpmdownloader_SRCS}
    ${aqpmdownloader_adaptor_SRCS}
)

TARGET_LINK_LIBRARIES(aqpmdownloader
    ${QT_QTCORE_LIBRARY}
    ${QT_QTDBUS_LIBRARY}
    ${QT_QTNETWORK_LIBRARY}
)

INSTALL(CODE "MESSAGE(\"Installing Aqpm downloader...\")")
install(TARGETS aqpmdownloader DESTINATION ${AQPM_HELPER_INSTALL_DIR})

dbus_add_activation_system_service(org.chakraproject.aqpmdownloader.service.in)

install(FILES org.chakraproject.aqpmdownloader.conf DESTINATION /etc/dbus-1/system.d)

