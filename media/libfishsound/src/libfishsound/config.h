






#define FS_DECODE 1


#define FS_ENCODE 0





#define HAVE_DLFCN_H 1


#define HAVE_FLAC 0








#define HAVE_INTTYPES_H 1





#define HAVE_MEMORY_H 1





#define HAVE_SPEEX 0











#define HAVE_STDLIB_H 1


#define HAVE_STRINGS_H 1


#define HAVE_STRING_H 1


#define HAVE_SYS_STAT_H 1


#define HAVE_SYS_TYPES_H 1





#define HAVE_UNISTD_H 1


#define HAVE_VORBIS 1


#define HAVE_VORBISENC 0



#define LT_OBJDIR ".libs/"


#define PACKAGE "libfishsound"


#define PACKAGE_BUGREPORT ""


#define PACKAGE_NAME ""


#define PACKAGE_STRING ""


#define PACKAGE_TARNAME ""


#define PACKAGE_VERSION ""


#define STDC_HEADERS 1


#define VERSION "0.9.2"



#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN

# endif
#endif



#undef FS_ENCODE
#define FS_ENCODE 0
#undef HAVE_FLAC
#define HAVE_FLAC 0
#undef HAVE_OGGZ
#define HAVE_OGGZ 1
#undef HAVE_SPEEX
#define HAVE_SPEEX 0
#undef HAVE_VORBIS
#define HAVE_VORBIS 1
#undef HAVE_VORBISENC
#define HAVE_VORBISENC 0
#undef DEBUG

#include "prcpucfg.h"
#ifdef IS_BIG_ENDIAN
#define WORDS_BIGENDIAN
#endif
