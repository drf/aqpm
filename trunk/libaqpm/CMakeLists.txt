
add_subdirectory(worker)

SET(aqpm_SRCS 
    Backend.cpp
    BackendThread.cpp
    ConfigurationParser.cpp
    callbacks.cpp
)

SET(aqpm_MOC_HDRS
    Backend.h
    BackendThread.h
    callbacks.h
)

SET(aqpm_HEADERS
	Backend.h
	ConfigurationParser.h
)

QT4_WRAP_CPP(aqpm_MOC_SRCS ${aqpm_MOC_HDRS})
QT4_AUTOMOC(${aqpm_SRCS})
                      
add_library(aqpm SHARED
    ${aqpm_SRCS}
    ${aqpm_MOC_SRCS}
)

set_target_properties(aqpm PROPERTIES VERSION ${MAJOR_AQPM_VERSION}.${MINOR_AQPM_VERSION}.${PATCH_AQPM_VERSION}
                                      SOVERSION ${MAJOR_AQPM_VERSION}
                      )

TARGET_LINK_LIBRARIES(aqpm
    ${QT_QTCORE_LIBRARY}
    ${QT_QTDBUS_LIBRARY}
    ${ALPM_LIBS}
    ${LIBARCHIVE_LIBS}
)

INSTALL(CODE "MESSAGE(\"Installing Aqpm...\")")
install(TARGETS aqpm RUNTIME DESTINATION bin
                     LIBRARY DESTINATION lib
                     ARCHIVE DESTINATION lib)
INSTALL(FILES ${aqpm_HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/aqpm)
