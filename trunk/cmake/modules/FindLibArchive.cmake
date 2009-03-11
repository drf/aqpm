# cmake macro to see if we have libarchive

# LIBARCHIVE_INCLUDE_DIR
# LIBARCHIVE_FOUND
# Copyright (C) 2007 Brad Hards <bradh@frogmouth.net>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.


if (LIBARCHIVE_INCLUDE_DIR AND LIBARCHIVE_LIBS)
    # Already in cache, be silent
    set(LIBARCHIVE_FIND_QUIETLY TRUE)
endif (LIBARCHIVE_INCLUDE_DIR AND LIBARCHIVE_LIBS)


find_path(LIBARCHIVE_INCLUDE_DIR NAMES archive.h
        PATHS
        ${INCLUDE_INSTALL_DIR}
)

find_library(LIBARCHIVE_LIBS NAMES archive
        PATHS
        ${LIB_INSTALL_DIR}
)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LibArchive DEFAULT_MSG LIBARCHIVE_LIBS LIBARCHIVE_INCLUDE_DIR )
