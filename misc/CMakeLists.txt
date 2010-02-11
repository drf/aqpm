
set(aqpm_misc_HEADERS
    Visibility.h
)

# I know, sucks extremely hard, but it's the only way.

set(policy_gen_SRCS
    policy-gen/policy-gen.cpp
    policy-gen/kauth-policy-gen-polkit1.cpp )

automoc4_add_executable(polkit-kde-helper-policy-gen ${policy_gen_SRCS})

target_link_libraries(polkit-kde-helper-policy-gen ${QT_QTCORE_LIBRARY})

## Execute it
set(_output ${CMAKE_CURRENT_BINARY_DIR}/org.chakraproject.aqpm.policy)
get_filename_component(_input org.chakraproject.aqpm.actions ABSOLUTE)

add_custom_command(OUTPUT ${_output}
                   COMMAND ${CMAKE_CURRENT_BINARY_DIR}/polkit-kde-helper-policy-gen ${_input} > ${_output}
                   MAIN_DEPENDENCY ${_input}
                   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                   COMMENT "Generating org.chakraproject.aqpm.policy"
                   DEPENDS polkit-kde-helper-policy-gen)
add_custom_target("actions for polkit-kde" ALL DEPENDS ${_output})

install(CODE "message(STATUS \"Installing System Policies for Aqpm...\")")
install(FILES ${_output} DESTINATION ${POLKITQT-1_POLICY_FILES_INSTALL_DIR})

install(CODE "message(STATUS \"Installing additional header files...\")")
install(FILES ${aqpm_misc_HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/aqpm)
if (KDE4_INTEGRATION)
    install( FILES settings-package-management.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
endif (KDE4_INTEGRATION)

