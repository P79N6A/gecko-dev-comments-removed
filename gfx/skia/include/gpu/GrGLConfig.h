









#ifndef GrGLConfig_DEFINED
#define GrGLConfig_DEFINED

#include "GrTypes.h"
#include "GrGLDefines.h"




#ifdef GR_GL_CUSTOM_SETUP_HEADER
    #include GR_GL_CUSTOM_SETUP_HEADER
#endif

#if !defined(GR_GL_FUNCTION_TYPE)
    #define GR_GL_FUNCTION_TYPE
#endif























































#if !defined(GR_GL_LOG_CALLS)
    #define GR_GL_LOG_CALLS                     GR_DEBUG
#endif

#if !defined(GR_GL_LOG_CALLS_START)
    #define GR_GL_LOG_CALLS_START               0
#endif

#if !defined(GR_GL_CHECK_ERROR)
    #define GR_GL_CHECK_ERROR                   GR_DEBUG
#endif

#if !defined(GR_GL_CHECK_ERROR_START)
    #define GR_GL_CHECK_ERROR_START             1
#endif

#if !defined(GR_GL_NO_CONSTANT_ATTRIBUTES)
    #define GR_GL_NO_CONSTANT_ATTRIBUTES        0
#endif

#if !defined(GR_GL_ATTRIBUTE_MATRICES)
    #define GR_GL_ATTRIBUTE_MATRICES            0
#endif

#if !defined(GR_GL_USE_BUFFER_DATA_NULL_HINT)
    #define GR_GL_USE_BUFFER_DATA_NULL_HINT     1
#endif

#if !defined(GR_GL_PER_GL_FUNC_CALLBACK)
    #define GR_GL_PER_GL_FUNC_CALLBACK          0
#endif

#if(GR_GL_NO_CONSTANT_ATTRIBUTES) && (GR_GL_ATTRIBUTE_MATRICES)
    #error "Cannot combine GR_GL_NO_CONSTANT_ATTRIBUTES and GR_GL_ATTRIBUTE_MATRICES"
#endif








#undef GR_SUPPORT_GLDESKTOP
#undef GR_SUPPORT_GLES1
#undef GR_SUPPORT_GLES2
#undef GR_SUPPORT_GLES



#if GR_SCALAR_IS_FIXED
    #define GrGLType   GL_FIXED
#elif GR_SCALAR_IS_FLOAT
    #define GrGLType   GR_GL_FLOAT
#else
    #error "unknown GR_SCALAR type"
#endif

#if GR_TEXT_SCALAR_IS_USHORT
    #define GrGLTextType                    GR_GL_UNSIGNED_SHORT
    #define GR_GL_TEXT_TEXTURE_NORMALIZED   1
#elif GR_TEXT_SCALAR_IS_FLOAT
    #define GrGLTextType                    GR_GL_FLOAT
    #define GR_GL_TEXT_TEXTURE_NORMALIZED   0
#elif GR_TEXT_SCALAR_IS_FIXED
    #define GrGLTextType                    GR_GL_FIXED
    #define GR_GL_TEXT_TEXTURE_NORMALIZED   0
#else
    #error "unknown GR_TEXT_SCALAR type"
#endif



#ifndef GR_GL_32BPP_COLOR_FORMAT
    #if GR_WIN32_BUILD || GR_LINUX_BUILD
        #define GR_GL_32BPP_COLOR_FORMAT    GR_GL_BGRA
    #else
        #define GR_GL_32BPP_COLOR_FORMAT    GR_GL_RGBA
    #endif
#endif



struct GrGLInterface;

extern void GrGLCheckErr(const GrGLInterface* gl,
                         const char* location,
                         const char* call);

extern void GrGLClearErr(const GrGLInterface* gl);

#if GR_GL_CHECK_ERROR
    extern bool gCheckErrorGL;
    #define GR_GL_CHECK_ERROR_IMPL(IFACE, X)                    \
        if (gCheckErrorGL)                                      \
            GrGLCheckErr(IFACE, GR_FILE_AND_LINE_STR, #X)
#else
    #define GR_GL_CHECK_ERROR_IMPL(IFACE, X)
#endif

#if GR_GL_LOG_CALLS
    extern bool gLogCallsGL;
    #define GR_GL_LOG_CALLS_IMPL(X)                             \
        if (gLogCallsGL)                                        \
            GrPrintf(GR_FILE_AND_LINE_STR "GL: " #X "\n")
#else
    #define GR_GL_LOG_CALLS_IMPL(X)
#endif

#if GR_GL_PER_GL_FUNC_CALLBACK
    #define GR_GL_CALLBACK_IMPL(IFACE) (IFACE)->fCallback(IFACE)
#else
    #define GR_GL_CALLBACK_IMPL(IFACE)
#endif

#define GR_GL_CALL(IFACE, X)                                    \
    do {                                                        \
        GR_GL_CALL_NOERRCHECK(IFACE, X);                        \
        GR_GL_CHECK_ERROR_IMPL(IFACE, X);                       \
    } while (false)

#define GR_GL_CALL_NOERRCHECK(IFACE, X)                         \
    do {                                                        \
        GR_GL_CALLBACK_IMPL(IFACE);                             \
        (IFACE)->f##X;                                          \
        GR_GL_LOG_CALLS_IMPL(X);                                \
    } while (false)

#define GR_GL_CALL_RET(IFACE, RET, X)                           \
    do {                                                        \
        GR_GL_CALL_RET_NOERRCHECK(IFACE, RET, X);               \
        GR_GL_CHECK_ERROR_IMPL(IFACE, X);                       \
    } while (false)

#define GR_GL_CALL_RET_NOERRCHECK(IFACE, RET, X)                \
    do {                                                        \
        GR_GL_CALLBACK_IMPL(IFACE);                             \
        (RET) = (IFACE)->f##X;                                  \
        GR_GL_LOG_CALLS_IMPL(X);                                \
    } while (false)

#define GR_GL_GET_ERROR(IFACE) (IFACE)->fGetError()







extern void GrGLResetRowLength(const GrGLInterface*);






#define GR_GL_INIT_ZERO     0
#define GR_GL_GetIntegerv(gl, e, p)     \
    do {                            \
        *(p) = GR_GL_INIT_ZERO;     \
        GR_GL_CALL(gl, GetIntegerv(e, p));   \
    } while (0)

#define GR_GL_GetFramebufferAttachmentParameteriv(gl, t, a, pname, p)           \
    do {                                                                        \
        *(p) = GR_GL_INIT_ZERO;                                                 \
        GR_GL_CALL(gl, GetFramebufferAttachmentParameteriv(t, a, pname, p));    \
    } while (0)

#define GR_GL_GetRenderbufferParameteriv(gl, t, pname, p)                       \
    do {                                                                        \
        *(p) = GR_GL_INIT_ZERO;                                                 \
        GR_GL_CALL(gl, GetRenderbufferParameteriv(t, pname, p));                \
    } while (0)

#define GR_GL_GetTexLevelParameteriv(gl, t, l, pname, p)                        \
    do {                                                                        \
        *(p) = GR_GL_INIT_ZERO;                                                 \
        GR_GL_CALL(gl, GetTexLevelParameteriv(t, l, pname, p));                 \
    } while (0)



#endif
