
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

set(aqpm_abs_SRCS
    AbsBackend.cpp
    ABSHandler.cpp
    BuildingHandler.cpp
)

set(aqpm_abs_HEADERS
    ABSHandler.h
    BuildingHandler.h
)

automoc4_add_library(aqpmabs SHARED
    ${aqpm_abs_SRCS}
)

set_target_properties(aqpmabs PROPERTIES VERSION ${MAJOR_AQPM_VERSION}.${MINOR_AQPM_VERSION}.${PATCH_AQPM_VERSION}
                                         SOVERSION ${MAJOR_AQPM_VERSION}
                      )

target_link_libraries(aqpmabs
    ${QT_QTCORE_LIBRARY}
)

install(CODE "message(STATUS \"Installing AqpmAbs...\")")
install(TARGETS aqpmabs ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES ${aqpm_abs_HEADERS} DESTINATION ${INCLUDE_INSTALL_DIR}/aqpm)

add_subdirectory(worker)
