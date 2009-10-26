
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

qt4_add_dbus_adaptor(aqpmdownloader_adaptor_SRCS org.chakraproject.aqpmdownloader.xml Downloader_p.h Aqpm::Downloader)

add_subdirectory(worker)
add_subdirectory(configurator)

set(aqpm_SRCS
    Backend.cpp
    BackendThread.cpp
    Downloader.cpp
    QueueItem.cpp
    Package.cpp
    Group.cpp
    Database.cpp
    ../misc/Loops.cpp
    ActionEvent.cpp
    Configuration.cpp
    ConfigurationThread.cpp
    Maintenance.cpp
)

set(aqpm_HEADERS
    Backend.h
    Configuration.h
    Database.h
    Globals.h
    Group.h
    Maintenance.h
    Package.h
    QueueItem.h
)

automoc4_add_library(aqpm SHARED
    ${aqpm_SRCS}
    ${aqpmdownloader_adaptor_SRCS}
)

set_target_properties(aqpm PROPERTIES VERSION ${MAJOR_AQPM_VERSION}.${MINOR_AQPM_VERSION}.${PATCH_AQPM_VERSION}
                                      SOVERSION ${MAJOR_AQPM_VERSION}
                      )

target_link_libraries(aqpm
    ${QT_QTCORE_LIBRARY}
    ${QT_QTDBUS_LIBRARY}
    ${ALPM_LIBS}
    ${POLKITQT_CORE_LIBRARY}
)

if (KDE4_INTEGRATION)
    target_link_libraries(aqpm ${KDE4_KIO_LIBRARY})
else (KDE4_INTEGRATION)
    target_link_libraries(aqpm ${QT_QTNETWORK_LIBRARY})
endif (KDE4_INTEGRATION)

install(CODE "message(\"Installing Aqpm...\")")
install(TARGETS aqpm ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES ${aqpm_HEADERS} DESTINATION ${INCLUDE_INSTALL_DIR}/aqpm)

add_subdirectory(tests)
