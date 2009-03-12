# cmake macro to see if we have Plasma from KDE workspace

# ALPM_INCLUDE_DIR
# ALPM_FOUND
# Copyright (C) 2007 Brad Hards <bradh@frogmouth.net>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.


if (ALPM_INCLUDE_DIR AND ALPM_LIBS)
    # Already in cache, be silent
    set(ALPM_FIND_QUIETLY TRUE)
endif (ALPM_INCLUDE_DIR AND ALPM_LIBS)


find_path(ALPM_INCLUDE_DIR NAMES alpm.h
        PATHS
        ${INCLUDE_INSTALL_DIR}
)

find_library(ALPM_LIBS NAMES alpm
        PATHS
        ${LIB_INSTALL_DIR}
)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Alpm DEFAULT_MSG ALPM_LIBS ALPM_INCLUDE_DIR )
