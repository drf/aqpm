project(aqpm)

cmake_minimum_required(VERSION 2.6.0)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

if(COMMAND cmake_policy)
   cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy) 

include(InstallSettings)

set(MAJOR_AQPM_VERSION "1")
set(MINOR_AQPM_VERSION "3")
set(PATCH_AQPM_VERSION "3")
set(FIX_AQPM_VERSION "2")
set(AQPM_VERSION_STRING "${MAJOR_AQPM_VERSION}.${MINOR_AQPM_VERSION}.${PATCH_AQPM_VERSION}.${FIX_AQPM_VERSION}")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/aqpm.pc.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/aqpm.pc
               @ONLY )

# requires minimal Qt 4.4
set(QT_MIN_VERSION "4.4.0")

find_package(Qt4 REQUIRED)
find_package(Automoc4 REQUIRED)
find_package(Alpm REQUIRED)
set(POLKITQT_MIN_VERSION 0.9.2)
find_package(PolkitQt REQUIRED)

include(${QT_USE_FILE})

set(QT_USE_QTXML)

include_directories(
    ${QT_INCLUDES}
    ${ALPM_INCLUDE_DIR}
    ${LIBARCHIVE_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/misc
    ${CMAKE_SOURCE_DIR}/misc
    ${POLKITQT_INCLUDE_DIR}
)

if (NOT AQPM_CONFIGURATION_FILE)
    set (AQPM_CONFIGURATION_FILE "/etc/aqpm.conf")
endif (NOT AQPM_CONFIGURATION_FILE)

configure_file(
       "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/cmake_uninstall.cmake.in"
       "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
       IMMEDIATE @ONLY)
add_custom_target(uninstall "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config-aqpm.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/config-aqpm.h)

add_subdirectory(libaqpm)
add_subdirectory(libaqpmabs)
add_subdirectory(misc)

install( FILES ${CMAKE_CURRENT_BINARY_DIR}/aqpm.pc
         DESTINATION ${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}/pkgconfig
         COMPONENT Devel )
 
### Source tarball autogeneration ###
set(ARCHIVE_NAME ${CMAKE_PROJECT_NAME}-${AQPM_VERSION_STRING})
add_custom_target(dist
    COMMAND git archive --prefix=${ARCHIVE_NAME}/ HEAD
        | bzip2 > ${CMAKE_BINARY_DIR}/${ARCHIVE_NAME}.tar.bz2
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
