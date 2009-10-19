
include_directories(
${CMAKE_CURRENT_BINARY_DIR}
${CMAKE_CURRENT_SOURCE_DIR}
)

set(aqpm_aur_SRCS
AurBackend.cpp
)

set(aqpm_aur_HEADERS
AurBackend.h
)

automoc4_add_library(aqpmaur SHARED
${aqpm_aur_SRCS}
)

set_target_properties(aqpmaur PROPERTIES VERSION ${MAJOR_AQPM_VERSION}.${MINOR_AQPM_VERSION}.${PATCH_AQPM_VERSION}
SOVERSION ${MAJOR_AQPM_VERSION}
)

target_link_libraries(aqpmaur
${QT_QTCORE_LIBRARY}
${QJSON_LIBRARIES}
)

install(CODE "message(\"Installing aqpmaur...\")")
install(TARGETS aqpmaur ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES ${aqpm_aur_HEADERS} DESTINATION ${INCLUDE_INSTALL_DIR}/aqpm)