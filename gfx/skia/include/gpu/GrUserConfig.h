








#ifndef GrUserConfig_DEFINED
#define GrUserConfig_DEFINED

#if defined(GR_USER_CONFIG_FILE)
    #error "default user config pulled in but GR_USER_CONFIG_FILE is defined."
#endif

#if 0
    #undef GR_RELEASE
    #undef GR_DEBUG
    #define GR_RELEASE  0
    #define GR_DEBUG    1
#endif





































#define GR_SCALAR_IS_FIXED          0
#define GR_SCALAR_IS_FLOAT          1

#define GR_TEXT_SCALAR_IS_USHORT    0
#define GR_TEXT_SCALAR_IS_FIXED     0
#define GR_TEXT_SCALAR_IS_FLOAT     1





#define GR_GL_PER_GL_FUNC_CALLBACK  1

#endif


