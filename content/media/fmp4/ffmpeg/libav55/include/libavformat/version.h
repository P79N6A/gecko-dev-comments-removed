



















#ifndef AVFORMAT_VERSION_H
#define AVFORMAT_VERSION_H







#include "libavutil/version.h"

#define LIBAVFORMAT_VERSION_MAJOR 55
#define LIBAVFORMAT_VERSION_MINOR 12
#define LIBAVFORMAT_VERSION_MICRO  0

#define LIBAVFORMAT_VERSION_INT AV_VERSION_INT(LIBAVFORMAT_VERSION_MAJOR, \
                                               LIBAVFORMAT_VERSION_MINOR, \
                                               LIBAVFORMAT_VERSION_MICRO)
#define LIBAVFORMAT_VERSION     AV_VERSION(LIBAVFORMAT_VERSION_MAJOR,   \
                                           LIBAVFORMAT_VERSION_MINOR,   \
                                           LIBAVFORMAT_VERSION_MICRO)
#define LIBAVFORMAT_BUILD       LIBAVFORMAT_VERSION_INT

#define LIBAVFORMAT_IDENT       "Lavf" AV_STRINGIFY(LIBAVFORMAT_VERSION)






#ifndef FF_API_REFERENCE_DTS
#define FF_API_REFERENCE_DTS            (LIBAVFORMAT_VERSION_MAJOR < 56)
#endif

#endif 
