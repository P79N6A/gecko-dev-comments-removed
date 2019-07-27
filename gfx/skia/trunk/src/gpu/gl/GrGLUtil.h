






#ifndef GrGLUtil_DEFINED
#define GrGLUtil_DEFINED

#include "gl/GrGLInterface.h"
#include "GrGLDefines.h"

class SkMatrix;



typedef uint32_t GrGLVersion;
typedef uint32_t GrGLSLVersion;

#define GR_GL_VER(major, minor) ((static_cast<int>(major) << 16) | \
                                 static_cast<int>(minor))
#define GR_GLSL_VER(major, minor) ((static_cast<int>(major) << 16) | \
                                   static_cast<int>(minor))

#define GR_GL_INVALID_VER GR_GL_VER(0, 0)
#define GR_GLSL_INVALID_VER GR_GL_VER(0, 0)




enum GrGLVendor {
    kARM_GrGLVendor,
    kImagination_GrGLVendor,
    kIntel_GrGLVendor,
    kQualcomm_GrGLVendor,
    kNVIDIA_GrGLVendor,

    kOther_GrGLVendor
};

enum GrGLRenderer {
    kTegra2_GrGLRenderer,
    kTegra3_GrGLRenderer,

    kOther_GrGLRenderer
};






#define GR_GL_INIT_ZERO     0
#define GR_GL_GetIntegerv(gl, e, p)                                            \
    do {                                                                       \
        *(p) = GR_GL_INIT_ZERO;                                                \
        GR_GL_CALL(gl, GetIntegerv(e, p));                                     \
    } while (0)

#define GR_GL_GetFramebufferAttachmentParameteriv(gl, t, a, pname, p)          \
    do {                                                                       \
        *(p) = GR_GL_INIT_ZERO;                                                \
        GR_GL_CALL(gl, GetFramebufferAttachmentParameteriv(t, a, pname, p));   \
    } while (0)

#define GR_GL_GetRenderbufferParameteriv(gl, t, pname, p)                      \
    do {                                                                       \
        *(p) = GR_GL_INIT_ZERO;                                                \
        GR_GL_CALL(gl, GetRenderbufferParameteriv(t, pname, p));               \
    } while (0)
#define GR_GL_GetTexLevelParameteriv(gl, t, l, pname, p)                       \
    do {                                                                       \
        *(p) = GR_GL_INIT_ZERO;                                                \
        GR_GL_CALL(gl, GetTexLevelParameteriv(t, l, pname, p));                \
    } while (0)








GrGLVersion GrGLGetVersionFromString(const char* versionString);
GrGLStandard GrGLGetStandardInUseFromString(const char* versionString);
GrGLSLVersion GrGLGetGLSLVersionFromString(const char* versionString);
bool GrGLIsMesaFromVersionString(const char* versionString);
GrGLVendor GrGLGetVendorFromString(const char* vendorString);
GrGLRenderer GrGLGetRendererFromString(const char* rendererString);
bool GrGLIsChromiumFromRendererString(const char* rendererString);


GrGLVersion GrGLGetVersion(const GrGLInterface*);
GrGLSLVersion GrGLGetGLSLVersion(const GrGLInterface*);
GrGLVendor GrGLGetVendor(const GrGLInterface*);
GrGLRenderer GrGLGetRenderer(const GrGLInterface*);






void GrGLCheckErr(const GrGLInterface* gl,
                  const char* location,
                  const char* call);

void GrGLClearErr(const GrGLInterface* gl);




template<int MatrixSize> void GrGLGetMatrix(GrGLfloat* dest, const SkMatrix& src);









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
        (IFACE)->fFunctions.f##X;                               \
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
        (RET) = (IFACE)->fFunctions.f##X;                       \
        GR_GL_LOG_CALLS_IMPL(X);                                \
    } while (false)


#define GR_GL_GET_ERROR(IFACE) (IFACE)->fFunctions.fGetError()

#endif
