PROJECT(aqpm)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6.0)
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

MACRO (MACRO_OPTIONAL_FIND_PACKAGE _name )
   OPTION(WITH_${_name} "Search for ${_name} package" ON)
   if (WITH_${_name})
      FIND_PACKAGE(${_name} ${ARGN})
   else (WITH_${_name})
      set(${_name}_FOUND)
      set(${_name}_INCLUDE_DIR)
      set(${_name}_INCLUDES)
      set(${_name}_LIBRARY)
      set(${_name}_LIBRARIES)
   endif (WITH_${_name})
ENDMACRO (MACRO_OPTIONAL_FIND_PACKAGE)

# requires minimal Qt 4.4
SET(QT_MIN_VERSION "4.4.0")

FIND_PACKAGE(Qt4 REQUIRED)
FIND_PACKAGE(Automoc4 REQUIRED)
FIND_PACKAGE(Alpm REQUIRED)
#FIND_PACKAGE(LibArchive REQUIRED)
SET(POLKITQT_MIN_VERSION 0.9.2)
FIND_PACKAGE(PolkitQt REQUIRED)

INCLUDE(${QT_USE_FILE})

SET(QT_USE_QTXML)

INCLUDE_DIRECTORIES(
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

CONFIGURE_FILE(
       "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/cmake_uninstall.cmake.in"
       "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
       IMMEDIATE @ONLY)
ADD_CUSTOM_TARGET(uninstall "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config-aqpm.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/config-aqpm.h)

add_subdirectory(libaqpm)
add_subdirectory(libaqpmabs)
add_subdirectory(misc)

install( FILES ${CMAKE_CURRENT_BINARY_DIR}/aqpm.pc
         DESTINATION ${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}/pkgconfig
         COMPONENT Devel )

# verbose - on
# SET(CMAKE_VERBOSE_MAKEFILE ON)

#FILE(GLOB helpfiles "${CMAKE_CURRENT_SOURCE_DIR}/help/*.png")
#INSTALL(FILES ${helpfiles} DESTINATION share/shaman/help)

#INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/help/main.htm DESTINATION share/shaman/help)

#FILE(GLOB helpfiles_de "${CMAKE_CURRENT_SOURCE_DIR}/help/de/*.png")
#INSTALL(FILES ${helpfiles_de} DESTINATION share/shaman/help/de)

#INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/help/de/main.htm DESTINATION share/shaman/help/de)
 
