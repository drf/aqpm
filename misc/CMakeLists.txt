
set(aqpm_misc_HEADERS
    Visibility.h
)

install(CODE "message(STATUS \"Installing additional header files...\")")
install(FILES ${aqpm_misc_HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/aqpm)
install(CODE "message(STATUS \"Installing System Policies for Aqpm...\")")
install(FILES org.chakraproject.aqpm.policy DESTINATION ${POLKITQT-1_POLICY_FILES_INSTALL_DIR})
if (KDE4_INTEGRATION)
    install( FILES settings-package-management.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
endif (KDE4_INTEGRATION)

