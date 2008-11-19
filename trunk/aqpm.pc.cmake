prefix=@CMAKE_INSTALL_PREFIX@
exec_prefix=@CMAKE_INSTALL_PREFIX@
libdir=@CMAKE_INSTALL_PREFIX@/lib@LIB_SUFFIX@
includedir=@CMAKE_INSTALL_PREFIX@/include/aqpm

Name: aqpm
Description: Qt Wrapper around Alpm, with extra functionalities
Version: @MAJOR_AQPM_VERSION@.@MINOR_AQPM_VERSION@.@PATCH_AQPM_VERSION@

Requires: libalpm
Libs: -L${libdir} -laqpm
Cflags: -I${includedir}
