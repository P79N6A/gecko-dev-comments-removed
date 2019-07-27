



















#ifndef AVUTIL_VERSION_H
#define AVUTIL_VERSION_H









#define AV_STRINGIFY(s)         AV_TOSTRING(s)
#define AV_TOSTRING(s) #s

#define AV_GLUE(a, b) a ## b
#define AV_JOIN(a, b) AV_GLUE(a, b)

#define AV_PRAGMA(s) _Pragma(#s)














#define AV_VERSION_INT(a, b, c) (a<<16 | b<<8 | c)
#define AV_VERSION_DOT(a, b, c) a ##.## b ##.## c
#define AV_VERSION(a, b, c) AV_VERSION_DOT(a, b, c)





















#define LIBAVUTIL_VERSION_MAJOR  52
#define LIBAVUTIL_VERSION_MINOR   5
#define LIBAVUTIL_VERSION_MICRO 100

#define LIBAVUTIL_VERSION_INT   AV_VERSION_INT(LIBAVUTIL_VERSION_MAJOR, \
                                               LIBAVUTIL_VERSION_MINOR, \
                                               LIBAVUTIL_VERSION_MICRO)
#define LIBAVUTIL_VERSION       AV_VERSION(LIBAVUTIL_VERSION_MAJOR,     \
                                           LIBAVUTIL_VERSION_MINOR,     \
                                           LIBAVUTIL_VERSION_MICRO)
#define LIBAVUTIL_BUILD         LIBAVUTIL_VERSION_INT

#define LIBAVUTIL_IDENT         "Lavu" AV_STRINGIFY(LIBAVUTIL_VERSION)












#ifndef FF_API_OLD_EVAL_NAMES
#define FF_API_OLD_EVAL_NAMES           (LIBAVUTIL_VERSION_MAJOR < 52)
#endif
#ifndef FF_API_GET_BITS_PER_SAMPLE_FMT
#define FF_API_GET_BITS_PER_SAMPLE_FMT (LIBAVUTIL_VERSION_MAJOR < 53)
#endif
#ifndef FF_API_FIND_OPT
#define FF_API_FIND_OPT                 (LIBAVUTIL_VERSION_MAJOR < 53)
#endif
#ifndef FF_API_OLD_AVOPTIONS
#define FF_API_OLD_AVOPTIONS            (LIBAVUTIL_VERSION_MAJOR < 53)
#endif
#ifndef FF_API_PIX_FMT
#define FF_API_PIX_FMT                  (LIBAVUTIL_VERSION_MAJOR < 53)
#endif
#ifndef FF_API_CONTEXT_SIZE
#define FF_API_CONTEXT_SIZE             (LIBAVUTIL_VERSION_MAJOR < 53)
#endif
#ifndef FF_API_PIX_FMT_DESC
#define FF_API_PIX_FMT_DESC             (LIBAVUTIL_VERSION_MAJOR < 53)
#endif
#ifndef FF_API_AV_REVERSE
#define FF_API_AV_REVERSE               (LIBAVUTIL_VERSION_MAJOR < 53)
#endif





#endif 

