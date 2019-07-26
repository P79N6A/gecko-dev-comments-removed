






#ifndef GrGLConfig_chrome_DEFINED
#define GrGLConfig_chrome_DEFINED


#define GR_GL_CHECK_ERROR_START                     0

#if defined(SK_BUILD_FOR_WIN32)

#define GR_GL_NO_CONSTANT_ATTRIBUTES                1


#define GR_GL_RGBA_8888_PIXEL_OPS_SLOW              1


#define GR_GL_FULL_READPIXELS_FASTER_THAN_PARTIAL   1
#else
#define GR_GL_NO_CONSTANT_ATTRIBUTES                0
#define GR_GL_RGBA_8888_PIXEL_OPS_SLOW              0
#define GR_GL_FULL_READPIXELS_FASTER_THAN_PARTIAL   0
#endif



#define GR_GL_USE_BUFFER_DATA_NULL_HINT             0


#define GR_GL_PER_GL_FUNC_CALLBACK                  1



#define GR_GL_CHECK_ALLOC_WITH_GET_ERROR            0


#define GR_GL_CHECK_FBO_STATUS_ONCE_PER_FORMAT      1


#define GR_GL_MUST_USE_VBO                          1



#define GR_GL_USE_NEW_SHADER_SOURCE_SIGNATURE       1

#if !defined(GR_GL_IGNORE_ES3_MSAA)
    #define GR_GL_IGNORE_ES3_MSAA 1
#endif

#endif
