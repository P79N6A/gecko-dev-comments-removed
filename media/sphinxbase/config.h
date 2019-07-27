#if (  defined(_WIN32) || defined(__CYGWIN__) )









#define ENABLE_THREADS 


#define SPHINXBASE_TLS __declspec(thread)








#define WITH_LAPACK


#define SIZEOF_LONG 4





#define HAVE_PERROR 1


#define HAVE_SYS_STAT_H 1


#define YY_NO_UNISTD_H 1


#define EXEEXT ".exe"
#else







#define ENABLE_THREADS








#define HAVE_DLFCN_H 1





#define HAVE_INTTYPES_H 1











#define HAVE_LIBM 1


#define HAVE_LIBPTHREAD 1


#define HAVE_LONG_LONG 1


#define HAVE_MEMORY_H 1


#define HAVE_PERROR 1


#define HAVE_POPEN 1


#define HAVE_PTHREAD_H 1


#define HAVE_SNDFILE_H 1


#define HAVE_SNPRINTF 1


#define HAVE_STDINT_H 1


#define HAVE_STDLIB_H 1


#define HAVE_STRINGS_H 1


#define HAVE_STRING_H 1


#define HAVE_SYS_STAT_H 1


#define HAVE_SYS_TYPES_H 1


#define HAVE_UNISTD_H 1


#define ICONV_CONST 



#define LT_OBJDIR ".libs/"


#define RETSIGTYPE void


#define SIZEOF_LONG 8


#define SIZEOF_LONG_LONG 8





#define STDC_HEADERS 1






#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN

# endif
#endif
#endif
