



















#ifndef AVUTIL_AVUTIL_H
#define AVUTIL_AVUTIL_H













































































































































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





#include "error.h"
#include "version.h"
#include "macros.h"






#endif 
