
set(aqpm_misc_HEADERS
    Visibility.h
)

install(CODE "message(\"Installing additional header files...\")")
install(FILES ${aqpm_misc_HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/aqpm)
install(CODE "message(\"Installing System Policies for Aqpm...\")")
install(FILES org.chakraproject.aqpm.policy DESTINATION ${POLKITQT_POLICY_FILES_INSTALL_DIR})
