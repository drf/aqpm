
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

set(aqpm_builder_SRCS
    BuilderBackend.cpp
)

set(aqpm_builder_HEADERS
    BuilderBackend.h
)

automoc4_add_library(aqpmbuilder SHARED
    ${aqpm_builder_SRCS}
)

set_target_properties(aqpmbuilder PROPERTIES VERSION ${MAJOR_AQPM_VERSION}.${MINOR_AQPM_VERSION}.${PATCH_AQPM_VERSION}
                                         SOVERSION ${MAJOR_AQPM_VERSION}
                      )

target_link_libraries(aqpmbuilder
    ${QT_QTCORE_LIBRARY}
)

install(CODE "message(STATUS \"Installing AqpmBuilder...\")")
install(TARGETS aqpmbuilder ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES ${aqpm_builder_HEADERS} DESTINATION ${INCLUDE_INSTALL_DIR}/aqpm)
