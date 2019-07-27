



















#ifndef AVUTIL_AVUTIL_H
#define AVUTIL_AVUTIL_H

































































































#define AV_STRINGIFY(s)         AV_TOSTRING(s)
#define AV_TOSTRING(s) #s

#define AV_GLUE(a, b) a ## b
#define AV_JOIN(a, b) AV_GLUE(a, b)

#define AV_PRAGMA(s) _Pragma(#s)














#define AV_VERSION_INT(a, b, c) (a<<16 | b<<8 | c)
#define AV_VERSION_DOT(a, b, c) a ##.## b ##.## c
#define AV_VERSION(a, b, c) AV_VERSION_DOT(a, b, c)












#define LIBAVUTIL_VERSION_MAJOR 51
#define LIBAVUTIL_VERSION_MINOR 22
#define LIBAVUTIL_VERSION_MICRO  1

#define LIBAVUTIL_VERSION_INT   AV_VERSION_INT(LIBAVUTIL_VERSION_MAJOR, \
                                               LIBAVUTIL_VERSION_MINOR, \
                                               LIBAVUTIL_VERSION_MICRO)
#define LIBAVUTIL_VERSION       AV_VERSION(LIBAVUTIL_VERSION_MAJOR,     \
                                           LIBAVUTIL_VERSION_MINOR,     \
                                           LIBAVUTIL_VERSION_MICRO)
#define LIBAVUTIL_BUILD         LIBAVUTIL_VERSION_INT

#define LIBAVUTIL_IDENT         "Lavu" AV_STRINGIFY(LIBAVUTIL_VERSION)













#ifndef FF_API_GET_BITS_PER_SAMPLE_FMT
#define FF_API_GET_BITS_PER_SAMPLE_FMT (LIBAVUTIL_VERSION_MAJOR < 52)
#endif
#ifndef FF_API_FIND_OPT
#define FF_API_FIND_OPT                 (LIBAVUTIL_VERSION_MAJOR < 52)
#endif
#ifndef FF_API_AV_FIFO_PEEK
#define FF_API_AV_FIFO_PEEK             (LIBAVUTIL_VERSION_MAJOR < 52)
#endif
#ifndef FF_API_OLD_AVOPTIONS
#define FF_API_OLD_AVOPTIONS            (LIBAVUTIL_VERSION_MAJOR < 52)
#endif













unsigned avutil_version(void);




const char *avutil_configuration(void);




const char *avutil_license(void);










enum AVMediaType {
    AVMEDIA_TYPE_UNKNOWN = -1,  
    AVMEDIA_TYPE_VIDEO,
    AVMEDIA_TYPE_AUDIO,
    AVMEDIA_TYPE_DATA,          
    AVMEDIA_TYPE_SUBTITLE,
    AVMEDIA_TYPE_ATTACHMENT,    
    AVMEDIA_TYPE_NB
};











#define FF_LAMBDA_SHIFT 7
#define FF_LAMBDA_SCALE (1<<FF_LAMBDA_SHIFT)
#define FF_QP2LAMBDA 118 ///< factor to convert from H.263 QP to lambda
#define FF_LAMBDA_MAX (256*128-1)

#define FF_QUALITY_SCALE FF_LAMBDA_SCALE //FIXME maybe remove

















#define AV_NOPTS_VALUE          INT64_C(0x8000000000000000)





#define AV_TIME_BASE            1000000





#define AV_TIME_BASE_Q          (AVRational){1, AV_TIME_BASE}











enum AVPictureType {
    AV_PICTURE_TYPE_I = 1, 
    AV_PICTURE_TYPE_P,     
    AV_PICTURE_TYPE_B,     
    AV_PICTURE_TYPE_S,     
    AV_PICTURE_TYPE_SI,    
    AV_PICTURE_TYPE_SP,    
    AV_PICTURE_TYPE_BI,    
};








char av_get_picture_type_char(enum AVPictureType pict_type);





#include "common.h"
#include "error.h"






#endif 
