









#ifndef GrGLConfig_DEFINED
#define GrGLConfig_DEFINED

#include "GrTypes.h"




#ifdef GR_GL_CUSTOM_SETUP_HEADER
    #include GR_GL_CUSTOM_SETUP_HEADER
#endif

#if !defined(GR_GL_FUNCTION_TYPE)
    #define GR_GL_FUNCTION_TYPE
#endif











































































#if !defined(GR_GL_LOG_CALLS)
    #define GR_GL_LOG_CALLS                             GR_DEBUG
#endif

#if !defined(GR_GL_LOG_CALLS_START)
    #define GR_GL_LOG_CALLS_START                       0
#endif

#if !defined(GR_GL_CHECK_ERROR)
    #define GR_GL_CHECK_ERROR                           GR_DEBUG
#endif

#if !defined(GR_GL_CHECK_ERROR_START)
    #define GR_GL_CHECK_ERROR_START                     1
#endif

#if !defined(GR_GL_NO_CONSTANT_ATTRIBUTES)
    #define GR_GL_NO_CONSTANT_ATTRIBUTES                0
#endif

#if !defined(GR_GL_USE_BUFFER_DATA_NULL_HINT)
    #define GR_GL_USE_BUFFER_DATA_NULL_HINT             1
#endif

#if !defined(GR_GL_PER_GL_FUNC_CALLBACK)
    #define GR_GL_PER_GL_FUNC_CALLBACK                  0
#endif

#if !defined(GR_GL_RGBA_8888_PIXEL_OPS_SLOW)
    #define GR_GL_RGBA_8888_PIXEL_OPS_SLOW              0
#endif

#if !defined(GR_GL_FULL_READPIXELS_FASTER_THAN_PARTIAL)
    #define GR_GL_FULL_READPIXELS_FASTER_THAN_PARTIAL   0
#endif

#if !defined(GR_GL_CHECK_ALLOC_WITH_GET_ERROR)
    #define GR_GL_CHECK_ALLOC_WITH_GET_ERROR            1
#endif

#if !defined(GR_GL_CHECK_FBO_STATUS_ONCE_PER_FORMAT)
    #define GR_GL_CHECK_FBO_STATUS_ONCE_PER_FORMAT      0
#endif

#if !defined(GR_GL_USE_NV_PATH_RENDERING)
    #define GR_GL_USE_NV_PATH_RENDERING                 0
#endif


























#define GR_GL_MAC_BUFFER_OBJECT_PERFOMANCE_WORKAROUND   \
    (GR_MAC_BUILD &&                                    \
     !GR_GL_USE_BUFFER_DATA_NULL_HINT)

#endif
