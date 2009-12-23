# - Try to find Aqpm
# Once done this will define
#
#  AQPM_FOUND - system has Polkit-qt
#  AQPM_INCLUDE_DIR - the Polkit-qt include directory
#  AQPM_LIBRARIES - Link these to use all Polkit-qt libs
#  AQPM_CORE_LIBRARY - Link this to use the polkit-qt-core library only
#  AQPM_GUI_LIBRARY - Link this to use GUI elements in polkit-qt (polkit-qt-gui)
#  AQPM_AGENT_LIBRARY - Link this to use the agent wrapper in polkit-qt
#  AQPM_DEFINITIONS - Compiler switches required for using Polkit-qt

# Copyright (c) 2009, Dario Freddi, <drf@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if (AQPM_INCLUDE_DIR AND AQPM_LIB)
    set(AQPM_FIND_QUIETLY TRUE)
endif (AQPM_INCLUDE_DIR AND AQPM_LIB)

if (NOT AQPM_MIN_VERSION)
  set(AQPM_MIN_VERSION "1.3.3.5")
endif (NOT AQPM_MIN_VERSION)

if (NOT WIN32)
   # use pkg-config to get the directories and then use these values
   # in the FIND_PATH() and FIND_LIBRARY() calls
   find_package(PkgConfig)
   pkg_check_modules(PC_AQPM QUIET aqpm)
   set(AQPM_DEFINITIONS ${PC_AQPM_CFLAGS_OTHER})
endif (NOT WIN32)

find_path( AQPM_INCLUDE_DIR
     NAMES aqpm/aqpmversion.h
     PATH_SUFFIXES polkit-qt-1
)

set(AQPM_VERSION_OK TRUE)
if(AQPM_INCLUDE_DIR)
  file(READ ${AQPM_INCLUDE_DIR}/aqpm/aqpmversion.h AQPM_VERSION_CONTENT)
  string (REGEX MATCH "AQPM_VERSION_STRING \".*\"\n" AQPM_VERSION_MATCH "${AQPM_VERSION_CONTENT}")

  if(AQPM_VERSION_MATCH)
    string(REGEX REPLACE "AQPM_VERSION_STRING \"(.*)\"\n" "\\1" AQPM_VERSION ${AQPM_VERSION_MATCH})
    if(AQPM_VERSION STRLESS "${AQPM_MIN_VERSION}")
      set(AQPM_VERSION_OK FALSE)
      if(Aqpm_FIND_REQUIRED)
        message(FATAL_ERROR "Aqpm version ${AQPM_VERSION} was found, but it is too old. Please install ${AQPM_MIN_VERSION} or
newer.")
      else(Aqpm_FIND_REQUIRED)
        message(STATUS "Aqpm version ${AQPM_VERSION} is too old. Please install ${AQPM_MIN_VERSION} or newer.")
      endif(Aqpm_FIND_REQUIRED)
    endif(AQPM_VERSION STRLESS "${AQPM_MIN_VERSION}")
  endif(AQPM_VERSION_MATCH)
elseif(AQPM_INCLUDE_DIR)
  # The version is so old that it does not even have the file
  set(AQPM_VERSION_OK FALSE)
  if(Aqpm_FIND_REQUIRED)
    message(FATAL_ERROR "It looks like Aqpm is too old. Please install Aqpm version ${AQPM_MIN_VERSION} or newer.")
  else(Aqpm_FIND_REQUIRED)
    message(STATUS "It looks like Aqpm is too old. Please install Aqpm version ${AQPM_MIN_VERSION} or newer.")
  endif(Aqpm_FIND_REQUIRED)
endif(AQPM_INCLUDE_DIR)

find_library( AQPM_LIBRARY
    NAMES aqpm
    HINTS ${PC_AQPM_LIBDIR}
)
find_library( AQPM_ABS_LIBRARY
    NAMES aqpmabs
    HINTS ${PC_AQPM_LIBDIR}
)
find_library( AQPM_BUILDER_LIBRARY
    NAMES aqpmbuilder
    HINTS ${PC_AQPM_LIBDIR}
)
find_library( AQPM_AUR_LIBRARY
    NAMES aqpmaur
    HINTS ${PC_AQPM_LIBDIR}
)

include(FindPackageHandleStandardArgs)

# handle the QUIETLY and REQUIRED arguments and set AQPM_FOUND to TRUE if
# all listed variables are TRUE
find_package_handle_standard_args(Aqpm DEFAULT_MSG AQPM_LIBRARY AQPM_INCLUDE_DIR AQPM_VERSION_OK)
find_package_handle_standard_args(AqpmAbs DEFAULT_MSG AQPM_ABS_LIBRARY AQPM_INCLUDE_DIR AQPM_VERSION_OK)
find_package_handle_standard_args(AqpmBuilder DEFAULT_MSG AQPM_BUILDER_LIBRARY AQPM_INCLUDE_DIR AQPM_VERSION_OK)
find_package_handle_standard_args(AqpmAur DEFAULT_MSG AQPM_AUR_LIBRARY AQPM_INCLUDE_DIR AQPM_VERSION_OK)

mark_as_advanced(AQPM_INCLUDE_DIR AQPM_LIBRARY AQPM_ABS_LIBRARY AQPM_BUILDER_LIBRARY AQPM_AUR_LIBRARY AQPM_VERSION_OK)

