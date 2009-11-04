project(aqpm)

cmake_minimum_required(VERSION 2.6.0)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

if(COMMAND cmake_policy)
   cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy) 

include(InstallSettings)
# Documentation
include(TargetDoc)

set(MAJOR_AQPM_VERSION "1")
set(MINOR_AQPM_VERSION "3")
set(PATCH_AQPM_VERSION "3")
set(FIX_AQPM_VERSION "2")
set(AQPM_VERSION_STRING "${MAJOR_AQPM_VERSION}.${MINOR_AQPM_VERSION}.${PATCH_AQPM_VERSION}.${FIX_AQPM_VERSION}")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/aqpm.pc.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/aqpm.pc
               @ONLY )
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/aqpmaur.pc.cmake
${CMAKE_CURRENT_BINARY_DIR}/aqpmaur.pc
@ONLY )

# requires minimal Qt 4.4
set(QT_MIN_VERSION "4.4.0")

if (KDE4_INTEGRATION)
    find_package(KDE4 REQUIRED)

    include (MacroLibrary)
    include (FindPackageHandleStandardArgs)
    include (KDE4Defaults)

    include_directories(${KDE4_INCLUDES})
else (KDE4_INTEGRATION)
    find_package(Qt4 REQUIRED)
    find_package(Automoc4 REQUIRED)
endif (KDE4_INTEGRATION)

find_package(Alpm REQUIRED)
set(POLKITQT_MIN_VERSION 0.9.2)
find_package(PolkitQt REQUIRED)
find_package(QJSON)

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

set (AQPM_CONFIGURATION_FILE "/etc/aqpm.conf" CACHE STRING "The configuration file aqpm will use. Change only if 
                                                           you know what you're doing")
set (AUR_JSON_URL "http://aur.archlinux.org/rpc.php?" CACHE STRING "The URL that will be used by aqpmaur to access the
                                                                   aur. Change if you want to use your own installation. 
                                                                   It needs to end with an '?'")
set (AUR_BASE_URL "http://aur.archlinux.org" CACHE STRING "The URL that will be used by aqpmaur to download pkgbuilds from
                                                           aur. Change if you want to use your own installation. It should not
                                                           end with an '/'")

configure_file(
       "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/cmake_uninstall.cmake.in"
       "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
       IMMEDIATE @ONLY)
add_custom_target(uninstall "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config-aqpm.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/config-aqpm.h)

add_subdirectory(libaqpm)
add_subdirectory(misc)

option(BUILD_AQPM_AUR "Builds libaqpmaur, a library which can be used to access AUR" OFF)
option(BUILD_AQPM_ABS "Builds libaqpmabs, a wrapper around abs" ON)
option(BUILD_AQPM_BUILDER "Builds libaqpmbuilder, a wrapper around makepkg" ON)
option(BUILD_TESTS "Builds various tests and examples for the libraries" ON)

if (BUILD_AQPM_ABS)
    add_subdirectory(libaqpmabs)
endif (BUILD_AQPM_ABS)

if (BUILD_AQPM_BUILDER)
    add_subdirectory(libaqpmbuilder)
endif (BUILD_AQPM_BUILDER)

if (BUILD_AQPM_AUR AND QJSON_FOUND)
    add_subdirectory(libaqpmaur)
    message(STATUS "aqpmaur will be built")
else (BUILD_AQPM_AUR AND QJSON_FOUND)
    message(STATUS "aqpmaur will NOT be built")
endif (BUILD_AQPM_AUR AND QJSON_FOUND)

install( FILES ${CMAKE_CURRENT_BINARY_DIR}/aqpm.pc
         DESTINATION ${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}/pkgconfig
         COMPONENT Devel )
install( FILES ${CMAKE_CURRENT_BINARY_DIR}/aqpmaur.pc
         DESTINATION ${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}/pkgconfig
         COMPONENT Devel )
 
### Source tarball autogeneration ###
set(ARCHIVE_NAME ${CMAKE_PROJECT_NAME}-${AQPM_VERSION_STRING})
add_custom_target(dist
    COMMAND git archive --prefix=${ARCHIVE_NAME}/ HEAD
        | bzip2 > ${CMAKE_BINARY_DIR}/${ARCHIVE_NAME}.tar.bz2
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})


