
SET(aqpm_SRCS 
    Backend.cpp
    ConfigurationParser.cpp
    callbacks.cpp
)

SET(aqpm_MOC_HDRS
    Backend.h
    callbacks.h
)

QT4_WRAP_CPP(aqpm_MOC_SRCS ${aqpm_MOC_HDRS})
QT4_AUTOMOC(${aqpm_SRCS})
                      
add_library(aqpm SHARED
    ${aqpm_SRCS}
    ${aqpm_MOC_SRCS}
)

set_target_properties(aqpm PROPERTIES VERSION 0.1.0
                                     SOVERSION 1
                      )

TARGET_LINK_LIBRARIES(aqpm
    ${QT_QTCORE_LIBRARY}
    ${ALPM_LIBS}
    ${LIBARCHIVE_LIBS}
)

INSTALL(CODE "MESSAGE(\"Installing Aqpm...\")")
INSTALL(TARGETS aqpm DESTINATION lib)
