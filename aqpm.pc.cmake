prefix=@CMAKE_INSTALL_PREFIX@
exec_prefix=@CMAKE_INSTALL_PREFIX@
libdir=@CMAKE_INSTALL_PREFIX@/lib@LIB_SUFFIX@
includedir=@CMAKE_INSTALL_PREFIX@/include/aqpm

Name: aqpm
Description: Qt Wrapper around Alpm, with extra functionalities
Version: @AQPM_VERSION_STRING@

Libs: -L${libdir} -laqpm
Cflags: -I${includedir}
