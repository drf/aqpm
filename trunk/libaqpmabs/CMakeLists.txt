
SET(aqpm_abs_SRCS 
    ABSHandler.cpp
    BuildingHandler.cpp
)

SET(aqpm_abs_MOC_HDRS
    ABSHandler.h
    BuildingHandler.h
)

SET(aqpm_abs_HEADERS
	ABSHandler.h
	BuildingHandler.h
)

QT4_WRAP_CPP(aqpm_abs_MOC_SRCS ${aqpm_abs_MOC_HDRS})
QT4_AUTOMOC(${aqpm_abs_SRCS})
                      
add_library(aqpmabs SHARED
    ${aqpm_abs_SRCS}
    ${aqpm_abs_MOC_SRCS}
)

set_target_properties(aqpmabs PROPERTIES VERSION ${MAJOR_AQPM_VERSION}.${MINOR_AQPM_VERSION}.${PATCH_AQPM_VERSION}
                                         SOVERSION ${MAJOR_AQPM_VERSION}
                      )

TARGET_LINK_LIBRARIES(aqpmabs
    ${QT_QTCORE_LIBRARY}
)

INSTALL(CODE "MESSAGE(\"Installing aqpm_abs...\")")
install(TARGETS aqpmabs RUNTIME DESTINATION bin
                     LIBRARY DESTINATION lib
                     ARCHIVE DESTINATION lib)
INSTALL(FILES ${aqpm_abs_HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/aqpm)
