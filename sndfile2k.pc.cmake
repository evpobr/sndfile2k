prefix=@PC_PREFIX@
exec_prefix=@PC_EXEC_PREFIX@
libdir=@PC_LIBDIR@
includedir=@PC_INCLUDEDIR@

Name: sndfile2k
Description: A library for reading and writing audio files
Requires: 
Version: @PC_VERSION@
Libs: -L${libdir} -lsndfile2k
Libs.private: @PC_PRIVATE_LIBS@
Cflags: -I${includedir} 
