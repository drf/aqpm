prefix=@CMAKE_INSTALL_PREFIX@
exec_prefix=@CMAKE_INSTALL_PREFIX@
libdir=@CMAKE_INSTALL_PREFIX@/lib@LIB_SUFFIX@
includedir=@CMAKE_INSTALL_PREFIX@/include/aqpm

Name: aqpmaur
Description: Qt Wrapper around Aur
Version: @AQPM_VERSION_STRING@

Libs: -L${libdir} -laqpmaur
Cflags: -I${includedir}
