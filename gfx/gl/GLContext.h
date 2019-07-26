





#ifndef GLCONTEXT_H_
#define GLCONTEXT_H_

#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include <map>
#include <bitset>

#ifdef DEBUG
#include <string.h>
#endif

#ifdef WIN32
#include <windows.h>
#endif

#ifdef GetClassName
#undef GetClassName
#endif

#include "GLDefs.h"
#include "GLLibraryLoader.h"
#include "gfxImageSurface.h"
#include "gfx3DMatrix.h"
#include "nsISupportsImpl.h"
#include "plstr.h"
#include "nsDataHashtable.h"
#include "nsHashKeys.h"
#include "nsAutoPtr.h"
#include "GLContextTypes.h"
#include "GLTextureImage.h"
#include "SurfaceTypes.h"
#include "GLScreenBuffer.h"
#include "GLContextSymbols.h"
#include "mozilla/GenericRefCounted.h"
#include "mozilla/Scoped.h"
#include "gfx2DGlue.h"

#ifdef DEBUG
#define MOZ_ENABLE_GL_TRACKING 1
#endif

class nsIntRegion;
class nsIRunnable;
class nsIThread;

namespace android {
    class GraphicBuffer;
}

namespace mozilla {
    namespace gfx {
        class SharedSurface;
        class SourceSurface;
        class DataSourceSurface;
        struct SurfaceCaps;
    }

    namespace gl {
        class GLContext;
        class GLLibraryEGL;
        class GLScreenBuffer;
        class TextureGarbageBin;
        class GLBlitHelper;
        class GLBlitTextureImageHelper;
        class GLReadTexImageHelper;
    }

    namespace layers {
        class ColorTextureLayerProgram;
    }
}

namespace mozilla {
namespace gl {





namespace GLFeature {
    enum Enum {
        bind_buffer_offset,
        blend_minmax,
        depth_texture,
        draw_buffers,
        draw_instanced,
        element_index_uint,
        ES2_compatibility,
        ES3_compatibility,
        framebuffer_blit,
        framebuffer_multisample,
        framebuffer_object,
        get_query_object_iv,
        instanced_arrays,
        instanced_non_arrays,
        occlusion_query,
        occlusion_query_boolean,
        occlusion_query2,
        packed_depth_stencil,
        query_objects,
        robustness,
        sRGB,
        standard_derivatives,
        texture_float,
        texture_float_linear,
        texture_non_power_of_two,
        transform_feedback,
        vertex_array_object,
        EnumMax
    };
}

MOZ_BEGIN_ENUM_CLASS(ContextProfile, uint8_t)
    Unknown = 0,
    OpenGL, 
    OpenGLCore,
    OpenGLCompatibility,
    OpenGLES
MOZ_END_ENUM_CLASS(ContextProfile)

class GLContext
    : public GLLibraryLoader
    , public GenericAtomicRefCounted
{


public:

    enum {
        VendorIntel,
        VendorNVIDIA,
        VendorATI,
        VendorQualcomm,
        VendorImagination,
        VendorNouveau,
        VendorOther
    };

    enum {
        RendererAdreno200,
        RendererAdreno205,
        RendererAdrenoTM205,
        RendererAdrenoTM320,
        RendererSGX530,
        RendererSGX540,
        RendererTegra,
        RendererAndroidEmulator,
        RendererOther
    };




public:

    



    virtual bool IsANGLE() {
        return false;
    }

    


    inline bool IsCoreProfile() const {
        MOZ_ASSERT(mProfile != ContextProfile::Unknown, "unknown context profile");

        return mProfile == ContextProfile::OpenGLCore;
    }

    



    inline bool IsCompatibilityProfile() const {
        MOZ_ASSERT(mProfile != ContextProfile::Unknown, "unknown context profile");

        return mProfile == ContextProfile::OpenGLCompatibility;
    }

    


    inline bool IsGLES() const {
        MOZ_ASSERT(mProfile != ContextProfile::Unknown, "unknown context profile");

        return mProfile == ContextProfile::OpenGLES;
    }

    static const char* GetProfileName(ContextProfile profile)
    {
        switch (profile)
        {
            case ContextProfile::OpenGL:
                return "OpenGL";
            case ContextProfile::OpenGLCore:
                return "OpenGL Core";
            case ContextProfile::OpenGLCompatibility:
                return "OpenGL Compatibility";
            case ContextProfile::OpenGLES:
                return "OpenGL ES";
            default:
                break;
        }

        MOZ_ASSERT(profile != ContextProfile::Unknown, "unknown context profile");
        return "OpenGL unknown profile";
    }

    


    const char* ProfileString() const {
        return GetProfileName(mProfile);
    }

    





    inline bool IsAtLeast(ContextProfile profile, unsigned int version) const
    {
        MOZ_ASSERT(profile != ContextProfile::Unknown, "IsAtLeast: bad <profile> parameter");
        MOZ_ASSERT(mProfile != ContextProfile::Unknown, "unknown context profile");
        MOZ_ASSERT(mVersion != 0, "unknown context version");

        if (version > mVersion) {
            return false;
        }

        if (profile == ContextProfile::OpenGL) {
            return profile == ContextProfile::OpenGLCore ||
                   profile == ContextProfile::OpenGLCompatibility;
        }

        return profile == mProfile;
    }

    




    inline unsigned int Version() const {
        return mVersion;
    }

    const char* VersionString() const {
        return mVersionString.get();
    }

    int Vendor() const {
        return mVendor;
    }

    int Renderer() const {
        return mRenderer;
    }

    bool IsContextLost() const {
        return mContextLost;
    }

    


    virtual bool IsDoubleBuffered() {
        return false;
    }

    virtual GLContextType GetContextType() {
        return ContextTypeUnknown;
    }

    virtual bool IsCurrent() = 0;

    




    inline bool IsGLES2() const {
        return IsAtLeast(ContextProfile::OpenGLES, 200);
    }


protected:

    bool mInitialized;
    bool mIsOffscreen;
    bool mIsGlobalSharedContext;
    bool mContextLost;

    



    unsigned int mVersion;
    nsCString mVersionString;
    ContextProfile mProfile;

    int32_t mVendor;
    int32_t mRenderer;

    inline void SetProfileVersion(ContextProfile profile, unsigned int version) {
        MOZ_ASSERT(!mInitialized, "SetProfileVersion can only be called before initialization!");
        MOZ_ASSERT(profile != ContextProfile::Unknown && profile != ContextProfile::OpenGL, "Invalid `profile` for SetProfileVersion");
        MOZ_ASSERT(version >= 100, "Invalid `version` for SetProfileVersion");

        mVersion = version;
        mProfile = profile;
    }










public:

    






    enum GLExtensions {
        EXT_framebuffer_object,
        ARB_framebuffer_object,
        ARB_texture_rectangle,
        EXT_bgra,
        EXT_texture_format_BGRA8888,
        OES_depth24,
        OES_depth32,
        OES_stencil8,
        OES_texture_npot,
        ARB_depth_texture,
        OES_depth_texture,
        OES_packed_depth_stencil,
        IMG_read_format,
        EXT_read_format_bgra,
        APPLE_client_storage,
        ARB_texture_non_power_of_two,
        ARB_pixel_buffer_object,
        ARB_ES2_compatibility,
        ARB_ES3_compatibility,
        OES_texture_float,
        OES_texture_float_linear,
        ARB_texture_float,
        EXT_unpack_subimage,
        OES_standard_derivatives,
        EXT_texture_filter_anisotropic,
        EXT_texture_compression_s3tc,
        EXT_texture_compression_dxt1,
        ANGLE_texture_compression_dxt3,
        ANGLE_texture_compression_dxt5,
        AMD_compressed_ATC_texture,
        IMG_texture_compression_pvrtc,
        EXT_framebuffer_blit,
        ANGLE_framebuffer_blit,
        EXT_framebuffer_multisample,
        ANGLE_framebuffer_multisample,
        OES_rgb8_rgba8,
        ARB_robustness,
        EXT_robustness,
        ARB_sync,
        OES_EGL_image,
        OES_EGL_sync,
        OES_EGL_image_external,
        EXT_packed_depth_stencil,
        OES_element_index_uint,
        OES_vertex_array_object,
        ARB_vertex_array_object,
        APPLE_vertex_array_object,
        ARB_draw_buffers,
        EXT_draw_buffers,
        EXT_gpu_shader4,
        EXT_blend_minmax,
        ARB_draw_instanced,
        EXT_draw_instanced,
        NV_draw_instanced,
        ARB_instanced_arrays,
        NV_instanced_arrays,
        ANGLE_instanced_arrays,
        EXT_occlusion_query_boolean,
        ARB_occlusion_query2,
        EXT_transform_feedback,
        NV_transform_feedback,
        ANGLE_depth_texture,
        EXT_sRGB,
        EXT_texture_sRGB,
        ARB_framebuffer_sRGB,
        EXT_framebuffer_sRGB,
        KHR_debug,
        Extensions_Max,
        Extensions_End
    };

    bool IsExtensionSupported(GLExtensions aKnownExtension) const {
        return mAvailableExtensions[aKnownExtension];
    }

    void MarkExtensionUnsupported(GLExtensions aKnownExtension) {
        mAvailableExtensions[aKnownExtension] = 0;
    }

    void MarkExtensionSupported(GLExtensions aKnownExtension) {
        mAvailableExtensions[aKnownExtension] = 1;
    }


public:

    template<size_t N>
    static void InitializeExtensionsBitSet(std::bitset<N>& extensionsBitset, const char* extStr, const char** extList, bool verbose = false)
    {
        char* exts = strdup(extStr);

        if (verbose)
            printf_stderr("Extensions: %s\n", exts);

        char* cur = exts;
        bool done = false;
        while (!done) {
            char* space = strchr(cur, ' ');
            if (space) {
                *space = '\0';
            } else {
                done = true;
            }

            for (int i = 0; extList[i]; ++i) {
                if (PL_strcasecmp(cur, extList[i]) == 0) {
                    if (verbose)
                        printf_stderr("Found extension %s\n", cur);
                    extensionsBitset[i] = true;
                }
            }

            cur = space + 1;
        }

        free(exts);
    }


protected:
    std::bitset<Extensions_Max> mAvailableExtensions;









public:
    bool IsSupported(GLFeature::Enum feature) const {
        return mAvailableFeatures[feature];
    }

    static const char* GetFeatureName(GLFeature::Enum feature);


private:
    std::bitset<GLFeature::EnumMax> mAvailableFeatures;

    


    void InitFeatures();

    


    void MarkUnsupported(GLFeature::Enum feature);



public:

    bool HasRobustness() {
        return mHasRobustness;
    }

    



    virtual bool SupportsRobustness() = 0;


private:
    bool mHasRobustness;




public:

    static const char* GLErrorToString(GLenum aError)
    {
        switch (aError) {
            case LOCAL_GL_INVALID_ENUM:
                return "GL_INVALID_ENUM";
            case LOCAL_GL_INVALID_VALUE:
                return "GL_INVALID_VALUE";
            case LOCAL_GL_INVALID_OPERATION:
                return "GL_INVALID_OPERATION";
            case LOCAL_GL_STACK_OVERFLOW:
                return "GL_STACK_OVERFLOW";
            case LOCAL_GL_STACK_UNDERFLOW:
                return "GL_STACK_UNDERFLOW";
            case LOCAL_GL_OUT_OF_MEMORY:
                return "GL_OUT_OF_MEMORY";
            case LOCAL_GL_TABLE_TOO_LARGE:
                return "GL_TABLE_TOO_LARGE";
            case LOCAL_GL_INVALID_FRAMEBUFFER_OPERATION:
                return "GL_INVALID_FRAMEBUFFER_OPERATION";
            default:
                return "";
        }
    }


    


    GLenum GetAndClearError()
    {
        
        GLenum error = fGetError();

        if (error) {
            
            while(fGetError()) {}
        }

        return error;
    }


    

    GLenum fGetError()
    {
#ifdef DEBUG
        
        if (DebugMode()) {
            GLenum err = mGLError;
            mGLError = LOCAL_GL_NO_ERROR;
            return err;
        }
#endif 

        return mSymbols.fGetError();
    }


#ifdef DEBUG
private:

    GLenum mGLError;
#endif 




private:

#undef BEFORE_GL_CALL
#undef AFTER_GL_CALL

#ifdef MOZ_ENABLE_GL_TRACKING

#ifndef MOZ_FUNCTION_NAME
# ifdef __GNUC__
#  define MOZ_FUNCTION_NAME __PRETTY_FUNCTION__
# elif defined(_MSC_VER)
#  define MOZ_FUNCTION_NAME __FUNCTION__
# else
#  define MOZ_FUNCTION_NAME __func__  // defined in C99, supported in various C++ compilers. Just raw function name.
# endif
#endif

    void BeforeGLCall(const char* glFunction)
    {
        MOZ_ASSERT(IsCurrent());
        if (DebugMode()) {
            GLContext *currentGLContext = nullptr;

            currentGLContext = (GLContext*)PR_GetThreadPrivate(sCurrentGLContextTLS);

            if (DebugMode() & DebugTrace)
                printf_stderr("[gl:%p] > %s\n", this, glFunction);
            if (this != currentGLContext) {
                printf_stderr("Fatal: %s called on non-current context %p. "
                              "The current context for this thread is %p.\n",
                              glFunction, this, currentGLContext);
                NS_ABORT();
            }
        }
    }

    void AfterGLCall(const char* glFunction)
    {
        if (DebugMode()) {
            
            
            
            mSymbols.fFinish();
            mGLError = mSymbols.fGetError();
            if (DebugMode() & DebugTrace)
                printf_stderr("[gl:%p] < %s [0x%04x]\n", this, glFunction, mGLError);
            if (mGLError != LOCAL_GL_NO_ERROR) {
                printf_stderr("GL ERROR: %s generated GL error %s(0x%04x)\n",
                              glFunction,
                              GLErrorToString(mGLError),
                              mGLError);
                if (DebugMode() & DebugAbortOnError)
                    NS_ABORT();
            }
        }
    }

    GLContext *TrackingContext()
    {
        GLContext *tip = this;
        while (tip->mSharedContext)
            tip = tip->mSharedContext;
        return tip;
    }

#define BEFORE_GL_CALL                              \
            do {                                    \
                BeforeGLCall(MOZ_FUNCTION_NAME);    \
            } while (0)

#define AFTER_GL_CALL                               \
            do {                                    \
                AfterGLCall(MOZ_FUNCTION_NAME);     \
            } while (0)

#define TRACKING_CONTEXT(a)                         \
            do {                                    \
                TrackingContext()->a;               \
            } while (0)

#else 

#define BEFORE_GL_CALL do { } while (0)
#define AFTER_GL_CALL do { } while (0)
#define TRACKING_CONTEXT(a) do {} while (0)

#endif 

#define ASSERT_SYMBOL_PRESENT(func) \
            do {\
                MOZ_ASSERT(strstr(MOZ_FUNCTION_NAME, #func) != nullptr, "Mismatched symbol check.");\
                if (MOZ_UNLIKELY(!mSymbols.func)) {\
                    printf_stderr("RUNTIME ASSERT: Uninitialized GL function: %s\n", #func);\
                    MOZ_CRASH();\
                }\
            } while (0)

    
    
    void BeforeGLDrawCall() {
    }

    
    
    void AfterGLDrawCall()
    {
        if (mScreen)
            mScreen->AfterDrawCall();
    }

    
    
    void BeforeGLReadCall()
    {
        if (mScreen)
            mScreen->BeforeReadCall();
    }

    
    
    void AfterGLReadCall() {
    }




public:

    void fActiveTexture(GLenum texture) {
        BEFORE_GL_CALL;
        mSymbols.fActiveTexture(texture);
        AFTER_GL_CALL;
    }

    void fAttachShader(GLuint program, GLuint shader) {
        BEFORE_GL_CALL;
        mSymbols.fAttachShader(program, shader);
        AFTER_GL_CALL;
    }

    void fBeginQuery(GLenum target, GLuint id) {
        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fBeginQuery);
        mSymbols.fBeginQuery(target, id);
        AFTER_GL_CALL;
    }

    void fBindAttribLocation(GLuint program, GLuint index, const GLchar* name) {
        BEFORE_GL_CALL;
        mSymbols.fBindAttribLocation(program, index, name);
        AFTER_GL_CALL;
    }

    void fBindBuffer(GLenum target, GLuint buffer) {
        BEFORE_GL_CALL;
        mSymbols.fBindBuffer(target, buffer);
        AFTER_GL_CALL;
    }

    void fBindFramebuffer(GLenum target, GLuint framebuffer) {
        if (!mScreen) {
            raw_fBindFramebuffer(target, framebuffer);
            return;
        }

        switch (target) {
            case LOCAL_GL_DRAW_FRAMEBUFFER_EXT:
                mScreen->BindDrawFB(framebuffer);
                return;

            case LOCAL_GL_READ_FRAMEBUFFER_EXT:
                mScreen->BindReadFB(framebuffer);
                return;

            case LOCAL_GL_FRAMEBUFFER:
                mScreen->BindFB(framebuffer);
                return;

            default:
                
                break;
        }

        raw_fBindFramebuffer(target, framebuffer);
    }

    void fBindTexture(GLenum target, GLuint texture) {
        BEFORE_GL_CALL;
        mSymbols.fBindTexture(target, texture);
        AFTER_GL_CALL;
    }

    void fBlendColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {
        BEFORE_GL_CALL;
        mSymbols.fBlendColor(red, green, blue, alpha);
        AFTER_GL_CALL;
    }

    void fBlendEquation(GLenum mode) {
        BEFORE_GL_CALL;
        mSymbols.fBlendEquation(mode);
        AFTER_GL_CALL;
    }

    void fBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha) {
        BEFORE_GL_CALL;
        mSymbols.fBlendEquationSeparate(modeRGB, modeAlpha);
        AFTER_GL_CALL;
    }

    void fBlendFunc(GLenum sfactor, GLenum dfactor) {
        BEFORE_GL_CALL;
        mSymbols.fBlendFunc(sfactor, dfactor);
        AFTER_GL_CALL;
    }

    void fBlendFuncSeparate(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha) {
        BEFORE_GL_CALL;
        mSymbols.fBlendFuncSeparate(sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha);
        AFTER_GL_CALL;
    }

private:
    void raw_fBufferData(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage) {
        BEFORE_GL_CALL;
        mSymbols.fBufferData(target, size, data, usage);
        AFTER_GL_CALL;
    }

public:
    void fBufferData(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage) {
        raw_fBufferData(target, size, data, usage);

        
        if (WorkAroundDriverBugs() &&
            !data &&
            Vendor() == VendorNVIDIA)
        {
            char c = 0;
            fBufferSubData(target, size-1, 1, &c);
        }
    }

    void fBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data) {
        BEFORE_GL_CALL;
        mSymbols.fBufferSubData(target, offset, size, data);
        AFTER_GL_CALL;
    }

private:
    void raw_fClear(GLbitfield mask) {
        BEFORE_GL_CALL;
        mSymbols.fClear(mask);
        AFTER_GL_CALL;
    }

public:
    void fClear(GLbitfield mask) {
        BeforeGLDrawCall();
        raw_fClear(mask);
        AfterGLDrawCall();
    }

    void fClearColor(GLclampf r, GLclampf g, GLclampf b, GLclampf a) {
        BEFORE_GL_CALL;
        mSymbols.fClearColor(r, g, b, a);
        AFTER_GL_CALL;
    }

    void fClearStencil(GLint s) {
        BEFORE_GL_CALL;
        mSymbols.fClearStencil(s);
        AFTER_GL_CALL;
    }

    void fColorMask(realGLboolean red, realGLboolean green, realGLboolean blue, realGLboolean alpha) {
        BEFORE_GL_CALL;
        mSymbols.fColorMask(red, green, blue, alpha);
        AFTER_GL_CALL;
    }

    void fCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *pixels) {
        BEFORE_GL_CALL;
        mSymbols.fCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, pixels);
        AFTER_GL_CALL;
    }

    void fCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid *pixels) {
        BEFORE_GL_CALL;
        mSymbols.fCompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, pixels);
        AFTER_GL_CALL;
    }

    void fCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border) {
        y = FixYValue(y, height);

        if (!IsTextureSizeSafeToPassToDriver(target, width, height)) {
            
            
            level = -1;
            width = -1;
            height = -1;
            border = -1;
        }

        BeforeGLReadCall();
        raw_fCopyTexImage2D(target, level, internalformat,
                            x, y, width, height, border);
        AfterGLReadCall();
    }

    void fCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) {
        y = FixYValue(y, height);

        BeforeGLReadCall();
        raw_fCopyTexSubImage2D(target, level, xoffset, yoffset,
                               x, y, width, height);
        AfterGLReadCall();
    }

    void fCullFace(GLenum mode) {
        BEFORE_GL_CALL;
        mSymbols.fCullFace(mode);
        AFTER_GL_CALL;
    }

    void fDebugMessageCallback(GLDEBUGPROC callback, const GLvoid* userParam) {
        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fDebugMessageCallback);
        mSymbols.fDebugMessageCallback(callback, userParam);
        AFTER_GL_CALL;
    }

    void fDebugMessageControl(GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint* ids, realGLboolean enabled) {
        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fDebugMessageControl);
        mSymbols.fDebugMessageControl(source, type, severity, count, ids, enabled);
        AFTER_GL_CALL;
    }

    void fDebugMessageInsert(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* buf) {
        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fDebugMessageInsert);
        mSymbols.fDebugMessageInsert(source, type, id, severity, length, buf);
        AFTER_GL_CALL;
    }

    void fDetachShader(GLuint program, GLuint shader) {
        BEFORE_GL_CALL;
        mSymbols.fDetachShader(program, shader);
        AFTER_GL_CALL;
    }

    void fDepthFunc(GLenum func) {
        BEFORE_GL_CALL;
        mSymbols.fDepthFunc(func);
        AFTER_GL_CALL;
    }

    void fDepthMask(realGLboolean flag) {
        BEFORE_GL_CALL;
        mSymbols.fDepthMask(flag);
        AFTER_GL_CALL;
    }

    void fDisable(GLenum capability) {
        BEFORE_GL_CALL;
        mSymbols.fDisable(capability);
        AFTER_GL_CALL;
    }

    void fDisableVertexAttribArray(GLuint index) {
        BEFORE_GL_CALL;
        mSymbols.fDisableVertexAttribArray(index);
        AFTER_GL_CALL;
    }

    void fDrawBuffer(GLenum mode) {
        BEFORE_GL_CALL;
        mSymbols.fDrawBuffer(mode);
        AFTER_GL_CALL;
    }

private:
    void raw_fDrawArrays(GLenum mode, GLint first, GLsizei count) {
        BEFORE_GL_CALL;
        mSymbols.fDrawArrays(mode, first, count);
        AFTER_GL_CALL;
    }

    void raw_fDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices) {
        BEFORE_GL_CALL;
        mSymbols.fDrawElements(mode, count, type, indices);
        AFTER_GL_CALL;
    }

public:
    void fDrawArrays(GLenum mode, GLint first, GLsizei count) {
        BeforeGLDrawCall();
        raw_fDrawArrays(mode, first, count);
        AfterGLDrawCall();
    }

    void fDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices) {
        BeforeGLDrawCall();
        raw_fDrawElements(mode, count, type, indices);
        AfterGLDrawCall();
    }

    void fEnable(GLenum capability) {
        BEFORE_GL_CALL;
        mSymbols.fEnable(capability);
        AFTER_GL_CALL;
    }

    void fEnableVertexAttribArray(GLuint index) {
        BEFORE_GL_CALL;
        mSymbols.fEnableVertexAttribArray(index);
        AFTER_GL_CALL;
    }

    void fEndQuery(GLenum target) {
        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fEndQuery);
        mSymbols.fEndQuery(target);
        AFTER_GL_CALL;
    }

    void fFinish() {
        BEFORE_GL_CALL;
        mSymbols.fFinish();
        AFTER_GL_CALL;
    }

    void fFlush() {
        BEFORE_GL_CALL;
        mSymbols.fFlush();
        AFTER_GL_CALL;
    }

    void fFrontFace(GLenum face) {
        BEFORE_GL_CALL;
        mSymbols.fFrontFace(face);
        AFTER_GL_CALL;
    }

    void fGetActiveAttrib(GLuint program, GLuint index, GLsizei maxLength, GLsizei* length, GLint* size, GLenum* type, GLchar* name) {
        BEFORE_GL_CALL;
        mSymbols.fGetActiveAttrib(program, index, maxLength, length, size, type, name);
        AFTER_GL_CALL;
    }

    void fGetActiveUniform(GLuint program, GLuint index, GLsizei maxLength, GLsizei* length, GLint* size, GLenum* type, GLchar* name) {
        BEFORE_GL_CALL;
        mSymbols.fGetActiveUniform(program, index, maxLength, length, size, type, name);
        AFTER_GL_CALL;
    }

    void fGetAttachedShaders(GLuint program, GLsizei maxCount, GLsizei* count, GLuint* shaders) {
        BEFORE_GL_CALL;
        mSymbols.fGetAttachedShaders(program, maxCount, count, shaders);
        AFTER_GL_CALL;
    }

    GLint fGetAttribLocation (GLuint program, const GLchar* name) {
        BEFORE_GL_CALL;
        GLint retval = mSymbols.fGetAttribLocation(program, name);
        AFTER_GL_CALL;
        return retval;
    }

private:
    void raw_fGetIntegerv(GLenum pname, GLint *params) {
        BEFORE_GL_CALL;
        mSymbols.fGetIntegerv(pname, params);
        AFTER_GL_CALL;
    }

public:

    void fGetIntegerv(GLenum pname, GLint *params) {
        switch (pname)
        {
            
            
            
            case LOCAL_GL_DRAW_FRAMEBUFFER_BINDING_EXT:
                if (mScreen) {
                    *params = mScreen->GetDrawFB();
                } else {
                    raw_fGetIntegerv(pname, params);
                }
                break;

            case LOCAL_GL_READ_FRAMEBUFFER_BINDING_EXT:
                if (mScreen) {
                    *params = mScreen->GetReadFB();
                } else {
                    raw_fGetIntegerv(pname, params);
                }
                break;

            case LOCAL_GL_MAX_TEXTURE_SIZE:
                MOZ_ASSERT(mMaxTextureSize>0);
                *params = mMaxTextureSize;
                break;

            case LOCAL_GL_MAX_CUBE_MAP_TEXTURE_SIZE:
                MOZ_ASSERT(mMaxCubeMapTextureSize>0);
                *params = mMaxCubeMapTextureSize;
                break;

            case LOCAL_GL_MAX_RENDERBUFFER_SIZE:
                MOZ_ASSERT(mMaxRenderbufferSize>0);
                *params = mMaxRenderbufferSize;
                break;

            default:
                raw_fGetIntegerv(pname, params);
                break;
        }
    }

    void GetUIntegerv(GLenum pname, GLuint *params) {
        fGetIntegerv(pname, reinterpret_cast<GLint*>(params));
    }

    void fGetFloatv(GLenum pname, GLfloat *params) {
        BEFORE_GL_CALL;
        mSymbols.fGetFloatv(pname, params);
        AFTER_GL_CALL;
    }

    void fGetBooleanv(GLenum pname, realGLboolean *params) {
        BEFORE_GL_CALL;
        mSymbols.fGetBooleanv(pname, params);
        AFTER_GL_CALL;
    }

    void fGetBufferParameteriv(GLenum target, GLenum pname, GLint* params) {
        BEFORE_GL_CALL;
        mSymbols.fGetBufferParameteriv(target, pname, params);
        AFTER_GL_CALL;
    }

    GLuint fGetDebugMessageLog(GLuint count, GLsizei bufsize, GLenum* sources, GLenum* types, GLuint* ids, GLenum* severities, GLsizei* lengths, GLchar* messageLog) {
        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fGetDebugMessageLog);
        GLuint ret = mSymbols.fGetDebugMessageLog(count, bufsize, sources, types, ids, severities, lengths, messageLog);
        AFTER_GL_CALL;
        return ret;
    }

    void fGetPointerv(GLenum pname, GLvoid** params) {
        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fGetPointerv);
        mSymbols.fGetPointerv(pname, params);
        AFTER_GL_CALL;
    }

    void fGetObjectLabel(GLenum identifier, GLuint name, GLsizei bufSize, GLsizei* length, GLchar* label) {
        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fGetObjectLabel);
        mSymbols.fGetObjectLabel(identifier, name, bufSize, length, label);
        AFTER_GL_CALL;
    }

    void fGetObjectPtrLabel(GLvoid* ptr, GLsizei bufSize, GLsizei* length, GLchar* label) {
        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fGetObjectPtrLabel);
        mSymbols.fGetObjectPtrLabel(ptr, bufSize, length, label);
        AFTER_GL_CALL;
    }

    void fGenerateMipmap(GLenum target) {
        BEFORE_GL_CALL;
        mSymbols.fGenerateMipmap(target);
        AFTER_GL_CALL;
    }

    void fGetProgramiv(GLuint program, GLenum pname, GLint* param) {
        BEFORE_GL_CALL;
        mSymbols.fGetProgramiv(program, pname, param);
        AFTER_GL_CALL;
    }

    void fGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog) {
        BEFORE_GL_CALL;
        mSymbols.fGetProgramInfoLog(program, bufSize, length, infoLog);
        AFTER_GL_CALL;
    }

    void fTexParameteri(GLenum target, GLenum pname, GLint param) {
        BEFORE_GL_CALL;
        mSymbols.fTexParameteri(target, pname, param);
        AFTER_GL_CALL;
    }

    void fTexParameteriv(GLenum target, GLenum pname, GLint* params) {
        BEFORE_GL_CALL;
        mSymbols.fTexParameteriv(target, pname, params);
        AFTER_GL_CALL;
    }

    void fTexParameterf(GLenum target, GLenum pname, GLfloat param) {
        BEFORE_GL_CALL;
        mSymbols.fTexParameterf(target, pname, param);
        AFTER_GL_CALL;
    }

    const GLubyte* fGetString(GLenum name) {
        BEFORE_GL_CALL;
        const GLubyte *result = mSymbols.fGetString(name);
        AFTER_GL_CALL;
        return result;
    }

    void fGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *img) {
        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fGetTexImage);
        mSymbols.fGetTexImage(target, level, format, type, img);
        AFTER_GL_CALL;
    }

    void fGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint *params)
    {
        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fGetTexLevelParameteriv);
        mSymbols.fGetTexLevelParameteriv(target, level, pname, params);
        AFTER_GL_CALL;
    }

    void fGetTexParameterfv(GLenum target, GLenum pname, const GLfloat *params) {
        BEFORE_GL_CALL;
        mSymbols.fGetTexParameterfv(target, pname, params);
        AFTER_GL_CALL;
    }

    void fGetTexParameteriv(GLenum target, GLenum pname, const GLint *params) {
        BEFORE_GL_CALL;
        mSymbols.fGetTexParameteriv(target, pname, params);
        AFTER_GL_CALL;
    }

    void fGetUniformfv(GLuint program, GLint location, GLfloat* params) {
        BEFORE_GL_CALL;
        mSymbols.fGetUniformfv(program, location, params);
        AFTER_GL_CALL;
    }

    void fGetUniformiv(GLuint program, GLint location, GLint* params) {
        BEFORE_GL_CALL;
        mSymbols.fGetUniformiv(program, location, params);
        AFTER_GL_CALL;
    }

    GLint fGetUniformLocation (GLint programObj, const GLchar* name) {
        BEFORE_GL_CALL;
        GLint retval = mSymbols.fGetUniformLocation(programObj, name);
        AFTER_GL_CALL;
        return retval;
    }

    void fGetVertexAttribfv(GLuint index, GLenum pname, GLfloat* retval) {
        BEFORE_GL_CALL;
        mSymbols.fGetVertexAttribfv(index, pname, retval);
        AFTER_GL_CALL;
    }

    void fGetVertexAttribiv(GLuint index, GLenum pname, GLint* retval) {
        BEFORE_GL_CALL;
        mSymbols.fGetVertexAttribiv(index, pname, retval);
        AFTER_GL_CALL;
    }

    void fGetVertexAttribPointerv(GLuint index, GLenum pname, GLvoid** retval) {
        BEFORE_GL_CALL;
        mSymbols.fGetVertexAttribPointerv(index, pname, retval);
        AFTER_GL_CALL;
    }

    void fHint(GLenum target, GLenum mode) {
        BEFORE_GL_CALL;
        mSymbols.fHint(target, mode);
        AFTER_GL_CALL;
    }

    realGLboolean fIsBuffer(GLuint buffer) {
        BEFORE_GL_CALL;
        realGLboolean retval = mSymbols.fIsBuffer(buffer);
        AFTER_GL_CALL;
        return retval;
    }

    realGLboolean fIsEnabled(GLenum capability) {
        BEFORE_GL_CALL;
        realGLboolean retval = mSymbols.fIsEnabled(capability);
        AFTER_GL_CALL;
        return retval;
    }

    realGLboolean fIsProgram(GLuint program) {
        BEFORE_GL_CALL;
        realGLboolean retval = mSymbols.fIsProgram(program);
        AFTER_GL_CALL;
        return retval;
    }

    realGLboolean fIsShader(GLuint shader) {
        BEFORE_GL_CALL;
        realGLboolean retval = mSymbols.fIsShader(shader);
        AFTER_GL_CALL;
        return retval;
    }

    realGLboolean fIsTexture(GLuint texture) {
        BEFORE_GL_CALL;
        realGLboolean retval = mSymbols.fIsTexture(texture);
        AFTER_GL_CALL;
        return retval;
    }

    void fLineWidth(GLfloat width) {
        BEFORE_GL_CALL;
        mSymbols.fLineWidth(width);
        AFTER_GL_CALL;
    }

    void fLinkProgram(GLuint program) {
        BEFORE_GL_CALL;
        mSymbols.fLinkProgram(program);
        AFTER_GL_CALL;
    }

    void fObjectLabel(GLenum identifier, GLuint name, GLsizei length, const GLchar* label) {
        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fObjectLabel);
        mSymbols.fObjectLabel(identifier, name, length, label);
        AFTER_GL_CALL;
    }

    void fObjectPtrLabel(GLvoid* ptr, GLsizei length, const GLchar* label) {
        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fObjectPtrLabel);
        mSymbols.fObjectPtrLabel(ptr, length, label);
        AFTER_GL_CALL;
    }

    void fPixelStorei(GLenum pname, GLint param) {
        BEFORE_GL_CALL;
        mSymbols.fPixelStorei(pname, param);
        AFTER_GL_CALL;
    }

    void fPointParameterf(GLenum pname, GLfloat param) {
        BEFORE_GL_CALL;
        mSymbols.fPointParameterf(pname, param);
        AFTER_GL_CALL;
    }

    void fPolygonOffset(GLfloat factor, GLfloat bias) {
        BEFORE_GL_CALL;
        mSymbols.fPolygonOffset(factor, bias);
        AFTER_GL_CALL;
    }

    void fPopDebugGroup() {
        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fPopDebugGroup);
        mSymbols.fPopDebugGroup();
        AFTER_GL_CALL;
    }

    void fPushDebugGroup(GLenum source, GLuint id, GLsizei length, const GLchar* message) {
        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fPushDebugGroup);
        mSymbols.fPushDebugGroup(source, id, length, message);
        AFTER_GL_CALL;
    }

    void fReadBuffer(GLenum mode) {
        BEFORE_GL_CALL;
        mSymbols.fReadBuffer(mode);
        AFTER_GL_CALL;
    }

private:
    void raw_fReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels) {
        BEFORE_GL_CALL;
        mSymbols.fReadPixels(x, FixYValue(y, height), width, height, format, type, pixels);
        AFTER_GL_CALL;
    }

public:
    void fReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels) {
        y = FixYValue(y, height);

        BeforeGLReadCall();

        bool didReadPixels = false;
        if (mScreen) {
            didReadPixels = mScreen->ReadPixels(x, y, width, height, format, type, pixels);
        }

        if (!didReadPixels) {
            raw_fReadPixels(x, y, width, height, format, type, pixels);
        }

        AfterGLReadCall();
    }

public:
    void fSampleCoverage(GLclampf value, realGLboolean invert) {
        BEFORE_GL_CALL;
        mSymbols.fSampleCoverage(value, invert);
        AFTER_GL_CALL;
    }

private:
    void raw_fScissor(GLint x, GLint y, GLsizei width, GLsizei height) {
        BEFORE_GL_CALL;
        mSymbols.fScissor(x, y, width, height);
        AFTER_GL_CALL;
    }

public:
    void fStencilFunc(GLenum func, GLint ref, GLuint mask) {
        BEFORE_GL_CALL;
        mSymbols.fStencilFunc(func, ref, mask);
        AFTER_GL_CALL;
    }

    void fStencilFuncSeparate(GLenum frontfunc, GLenum backfunc, GLint ref, GLuint mask) {
        BEFORE_GL_CALL;
        mSymbols.fStencilFuncSeparate(frontfunc, backfunc, ref, mask);
        AFTER_GL_CALL;
    }

    void fStencilMask(GLuint mask) {
        BEFORE_GL_CALL;
        mSymbols.fStencilMask(mask);
        AFTER_GL_CALL;
    }

    void fStencilMaskSeparate(GLenum face, GLuint mask) {
        BEFORE_GL_CALL;
        mSymbols.fStencilMaskSeparate(face, mask);
        AFTER_GL_CALL;
    }

    void fStencilOp(GLenum fail, GLenum zfail, GLenum zpass) {
        BEFORE_GL_CALL;
        mSymbols.fStencilOp(fail, zfail, zpass);
        AFTER_GL_CALL;
    }

    void fStencilOpSeparate(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass) {
        BEFORE_GL_CALL;
        mSymbols.fStencilOpSeparate(face, sfail, dpfail, dppass);
        AFTER_GL_CALL;
    }

private:
    void raw_fTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels) {
        BEFORE_GL_CALL;
        mSymbols.fTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
        AFTER_GL_CALL;
    }

public:
    void fTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels) {
        if (!IsTextureSizeSafeToPassToDriver(target, width, height)) {
            
            
            level = -1;
            width = -1;
            height = -1;
            border = -1;
        }
        raw_fTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
    }

    void fTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels) {
        BEFORE_GL_CALL;
        mSymbols.fTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
        AFTER_GL_CALL;
    }

    void fUniform1f(GLint location, GLfloat v0) {
        BEFORE_GL_CALL;
        mSymbols.fUniform1f(location, v0);
        AFTER_GL_CALL;
    }

    void fUniform1fv(GLint location, GLsizei count, const GLfloat* value) {
        BEFORE_GL_CALL;
        mSymbols.fUniform1fv(location, count, value);
        AFTER_GL_CALL;
    }

    void fUniform1i(GLint location, GLint v0) {
        BEFORE_GL_CALL;
        mSymbols.fUniform1i(location, v0);
        AFTER_GL_CALL;
    }

    void fUniform1iv(GLint location, GLsizei count, const GLint* value) {
        BEFORE_GL_CALL;
        mSymbols.fUniform1iv(location, count, value);
        AFTER_GL_CALL;
    }

    void fUniform2f(GLint location, GLfloat v0, GLfloat v1) {
        BEFORE_GL_CALL;
        mSymbols.fUniform2f(location, v0, v1);
        AFTER_GL_CALL;
    }

    void fUniform2fv(GLint location, GLsizei count, const GLfloat* value) {
        BEFORE_GL_CALL;
        mSymbols.fUniform2fv(location, count, value);
        AFTER_GL_CALL;
    }

    void fUniform2i(GLint location, GLint v0, GLint v1) {
        BEFORE_GL_CALL;
        mSymbols.fUniform2i(location, v0, v1);
        AFTER_GL_CALL;
    }

    void fUniform2iv(GLint location, GLsizei count, const GLint* value) {
        BEFORE_GL_CALL;
        mSymbols.fUniform2iv(location, count, value);
        AFTER_GL_CALL;
    }

    void fUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {
        BEFORE_GL_CALL;
        mSymbols.fUniform3f(location, v0, v1, v2);
        AFTER_GL_CALL;
    }

    void fUniform3fv(GLint location, GLsizei count, const GLfloat* value) {
        BEFORE_GL_CALL;
        mSymbols.fUniform3fv(location, count, value);
        AFTER_GL_CALL;
    }

    void fUniform3i(GLint location, GLint v0, GLint v1, GLint v2) {
        BEFORE_GL_CALL;
        mSymbols.fUniform3i(location, v0, v1, v2);
        AFTER_GL_CALL;
    }

    void fUniform3iv(GLint location, GLsizei count, const GLint* value) {
        BEFORE_GL_CALL;
        mSymbols.fUniform3iv(location, count, value);
        AFTER_GL_CALL;
    }

    void fUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
        BEFORE_GL_CALL;
        mSymbols.fUniform4f(location, v0, v1, v2, v3);
        AFTER_GL_CALL;
    }

    void fUniform4fv(GLint location, GLsizei count, const GLfloat* value) {
        BEFORE_GL_CALL;
        mSymbols.fUniform4fv(location, count, value);
        AFTER_GL_CALL;
    }

    void fUniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3) {
        BEFORE_GL_CALL;
        mSymbols.fUniform4i(location, v0, v1, v2, v3);
        AFTER_GL_CALL;
    }

    void fUniform4iv(GLint location, GLsizei count, const GLint* value) {
        BEFORE_GL_CALL;
        mSymbols.fUniform4iv(location, count, value);
        AFTER_GL_CALL;
    }

    void fUniformMatrix2fv(GLint location, GLsizei count, realGLboolean transpose, const GLfloat* value) {
        BEFORE_GL_CALL;
        mSymbols.fUniformMatrix2fv(location, count, transpose, value);
        AFTER_GL_CALL;
    }

    void fUniformMatrix3fv(GLint location, GLsizei count, realGLboolean transpose, const GLfloat* value) {
        BEFORE_GL_CALL;
        mSymbols.fUniformMatrix3fv(location, count, transpose, value);
        AFTER_GL_CALL;
    }

    void fUniformMatrix4fv(GLint location, GLsizei count, realGLboolean transpose, const GLfloat* value) {
        BEFORE_GL_CALL;
        mSymbols.fUniformMatrix4fv(location, count, transpose, value);
        AFTER_GL_CALL;
    }

    void fUseProgram(GLuint program) {
        BEFORE_GL_CALL;
        mSymbols.fUseProgram(program);
        AFTER_GL_CALL;
    }

    void fValidateProgram(GLuint program) {
        BEFORE_GL_CALL;
        mSymbols.fValidateProgram(program);
        AFTER_GL_CALL;
    }

    void fVertexAttribPointer(GLuint index, GLint size, GLenum type, realGLboolean normalized, GLsizei stride, const GLvoid* pointer) {
        BEFORE_GL_CALL;
        mSymbols.fVertexAttribPointer(index, size, type, normalized, stride, pointer);
        AFTER_GL_CALL;
    }

    void fVertexAttrib1f(GLuint index, GLfloat x) {
        BEFORE_GL_CALL;
        mSymbols.fVertexAttrib1f(index, x);
        AFTER_GL_CALL;
    }

    void fVertexAttrib2f(GLuint index, GLfloat x, GLfloat y) {
        BEFORE_GL_CALL;
        mSymbols.fVertexAttrib2f(index, x, y);
        AFTER_GL_CALL;
    }

    void fVertexAttrib3f(GLuint index, GLfloat x, GLfloat y, GLfloat z) {
        BEFORE_GL_CALL;
        mSymbols.fVertexAttrib3f(index, x, y, z);
        AFTER_GL_CALL;
    }

    void fVertexAttrib4f(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
        BEFORE_GL_CALL;
        mSymbols.fVertexAttrib4f(index, x, y, z, w);
        AFTER_GL_CALL;
    }

    void fVertexAttrib1fv(GLuint index, const GLfloat* v) {
        BEFORE_GL_CALL;
        mSymbols.fVertexAttrib1fv(index, v);
        AFTER_GL_CALL;
    }

    void fVertexAttrib2fv(GLuint index, const GLfloat* v) {
        BEFORE_GL_CALL;
        mSymbols.fVertexAttrib2fv(index, v);
        AFTER_GL_CALL;
    }

    void fVertexAttrib3fv(GLuint index, const GLfloat* v) {
        BEFORE_GL_CALL;
        mSymbols.fVertexAttrib3fv(index, v);
        AFTER_GL_CALL;
    }

    void fVertexAttrib4fv(GLuint index, const GLfloat* v) {
        BEFORE_GL_CALL;
        mSymbols.fVertexAttrib4fv(index, v);
        AFTER_GL_CALL;
    }

    void fCompileShader(GLuint shader) {
        BEFORE_GL_CALL;
        mSymbols.fCompileShader(shader);
        AFTER_GL_CALL;
    }

private:
    void raw_fCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border) {
        BEFORE_GL_CALL;
        mSymbols.fCopyTexImage2D(target, level, internalformat, x, y, width, height, border);
        AFTER_GL_CALL;
    }

    void raw_fCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) {
        BEFORE_GL_CALL;
        mSymbols.fCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);
        AFTER_GL_CALL;
    }

public:
    void fGetShaderiv(GLuint shader, GLenum pname, GLint* param) {
        BEFORE_GL_CALL;
        mSymbols.fGetShaderiv(shader, pname, param);
        AFTER_GL_CALL;
    }

    void fGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog) {
        BEFORE_GL_CALL;
        mSymbols.fGetShaderInfoLog(shader, bufSize, length, infoLog);
        AFTER_GL_CALL;
    }

private:
    void raw_fGetShaderPrecisionFormat(GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision) {
        MOZ_ASSERT(IsGLES2());

        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fGetShaderPrecisionFormat);
        mSymbols.fGetShaderPrecisionFormat(shadertype, precisiontype, range, precision);
        AFTER_GL_CALL;
    }

public:
    void fGetShaderPrecisionFormat(GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision) {
        if (IsGLES2()) {
            raw_fGetShaderPrecisionFormat(shadertype, precisiontype, range, precision);
        } else {
            
            GetShaderPrecisionFormatNonES2(shadertype, precisiontype, range, precision);
        }
    }

    void fGetShaderSource(GLint obj, GLsizei maxLength, GLsizei* length, GLchar* source) {
        BEFORE_GL_CALL;
        mSymbols.fGetShaderSource(obj, maxLength, length, source);
        AFTER_GL_CALL;
    }

    void fShaderSource(GLuint shader, GLsizei count, const GLchar** strings, const GLint* lengths) {
        BEFORE_GL_CALL;
        mSymbols.fShaderSource(shader, count, strings, lengths);
        AFTER_GL_CALL;
    }

private:
    void raw_fBindFramebuffer(GLenum target, GLuint framebuffer) {
        BEFORE_GL_CALL;
        mSymbols.fBindFramebuffer(target, framebuffer);
        AFTER_GL_CALL;
    }

public:
    void fBindRenderbuffer(GLenum target, GLuint renderbuffer) {
        BEFORE_GL_CALL;
        mSymbols.fBindRenderbuffer(target, renderbuffer);
        AFTER_GL_CALL;
    }

    GLenum fCheckFramebufferStatus(GLenum target) {
        BEFORE_GL_CALL;
        GLenum retval = mSymbols.fCheckFramebufferStatus(target);
        AFTER_GL_CALL;
        return retval;
    }

    void fFramebufferRenderbuffer(GLenum target, GLenum attachmentPoint, GLenum renderbufferTarget, GLuint renderbuffer) {
        BEFORE_GL_CALL;
        mSymbols.fFramebufferRenderbuffer(target, attachmentPoint, renderbufferTarget, renderbuffer);
        AFTER_GL_CALL;
    }

    void fFramebufferTexture2D(GLenum target, GLenum attachmentPoint, GLenum textureTarget, GLuint texture, GLint level) {
        BEFORE_GL_CALL;
        mSymbols.fFramebufferTexture2D(target, attachmentPoint, textureTarget, texture, level);
        AFTER_GL_CALL;
    }

    void fGetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint* value) {
        BEFORE_GL_CALL;
        mSymbols.fGetFramebufferAttachmentParameteriv(target, attachment, pname, value);
        AFTER_GL_CALL;
    }

    void fGetRenderbufferParameteriv(GLenum target, GLenum pname, GLint* value) {
        BEFORE_GL_CALL;
        mSymbols.fGetRenderbufferParameteriv(target, pname, value);
        AFTER_GL_CALL;
    }

    realGLboolean fIsFramebuffer (GLuint framebuffer) {
        BEFORE_GL_CALL;
        realGLboolean retval = mSymbols.fIsFramebuffer(framebuffer);
        AFTER_GL_CALL;
        return retval;
    }

public:
    realGLboolean fIsRenderbuffer (GLuint renderbuffer) {
        BEFORE_GL_CALL;
        realGLboolean retval = mSymbols.fIsRenderbuffer(renderbuffer);
        AFTER_GL_CALL;
        return retval;
    }

    void fRenderbufferStorage(GLenum target, GLenum internalFormat, GLsizei width, GLsizei height) {
        BEFORE_GL_CALL;
        mSymbols.fRenderbufferStorage(target, internalFormat, width, height);
        AFTER_GL_CALL;
    }

private:
    void raw_fDepthRange(GLclampf a, GLclampf b) {
        MOZ_ASSERT(!IsGLES2());

        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fDepthRange);
        mSymbols.fDepthRange(a, b);
        AFTER_GL_CALL;
    }

    void raw_fDepthRangef(GLclampf a, GLclampf b) {
        MOZ_ASSERT(IsGLES2());

        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fDepthRangef);
        mSymbols.fDepthRangef(a, b);
        AFTER_GL_CALL;
    }

    void raw_fClearDepth(GLclampf v) {
        MOZ_ASSERT(!IsGLES2());

        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fClearDepth);
        mSymbols.fClearDepth(v);
        AFTER_GL_CALL;
    }

    void raw_fClearDepthf(GLclampf v) {
        MOZ_ASSERT(IsGLES2());

        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fClearDepthf);
        mSymbols.fClearDepthf(v);
        AFTER_GL_CALL;
    }

public:
    void fDepthRange(GLclampf a, GLclampf b) {
        if (IsGLES2()) {
            raw_fDepthRangef(a, b);
        } else {
            raw_fDepthRange(a, b);
        }
    }

    void fClearDepth(GLclampf v) {
        if (IsGLES2()) {
            raw_fClearDepthf(v);
        } else {
            raw_fClearDepth(v);
        }
    }

    void* fMapBuffer(GLenum target, GLenum access) {
        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fMapBuffer);
        void *ret = mSymbols.fMapBuffer(target, access);
        AFTER_GL_CALL;
        return ret;
    }

    realGLboolean fUnmapBuffer(GLenum target) {
        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fUnmapBuffer);
        realGLboolean ret = mSymbols.fUnmapBuffer(target);
        AFTER_GL_CALL;
        return ret;
    }


private:
    GLuint GLAPIENTRY raw_fCreateProgram() {
        BEFORE_GL_CALL;
        GLuint ret = mSymbols.fCreateProgram();
        AFTER_GL_CALL;
        return ret;
    }

    GLuint GLAPIENTRY raw_fCreateShader(GLenum t) {
        BEFORE_GL_CALL;
        GLuint ret = mSymbols.fCreateShader(t);
        AFTER_GL_CALL;
        return ret;
    }

    void GLAPIENTRY raw_fGenBuffers(GLsizei n, GLuint* names) {
        BEFORE_GL_CALL;
        mSymbols.fGenBuffers(n, names);
        AFTER_GL_CALL;
    }

    void GLAPIENTRY raw_fGenFramebuffers(GLsizei n, GLuint* names) {
        BEFORE_GL_CALL;
        mSymbols.fGenFramebuffers(n, names);
        AFTER_GL_CALL;
    }

    void GLAPIENTRY raw_fGenRenderbuffers(GLsizei n, GLuint* names) {
        BEFORE_GL_CALL;
        mSymbols.fGenRenderbuffers(n, names);
        AFTER_GL_CALL;
    }

    void GLAPIENTRY raw_fGenTextures(GLsizei n, GLuint* names) {
        BEFORE_GL_CALL;
        mSymbols.fGenTextures(n, names);
        AFTER_GL_CALL;
    }

public:
    GLuint fCreateProgram() {
        GLuint ret = raw_fCreateProgram();
        TRACKING_CONTEXT(CreatedProgram(this, ret));
        return ret;
    }

    GLuint fCreateShader(GLenum t) {
        GLuint ret = raw_fCreateShader(t);
        TRACKING_CONTEXT(CreatedShader(this, ret));
        return ret;
    }

    void fGenBuffers(GLsizei n, GLuint* names) {
        raw_fGenBuffers(n, names);
        TRACKING_CONTEXT(CreatedBuffers(this, n, names));
    }

    void fGenFramebuffers(GLsizei n, GLuint* names) {
        raw_fGenFramebuffers(n, names);
        TRACKING_CONTEXT(CreatedFramebuffers(this, n, names));
    }

    void fGenRenderbuffers(GLsizei n, GLuint* names) {
        raw_fGenRenderbuffers(n, names);
        TRACKING_CONTEXT(CreatedRenderbuffers(this, n, names));
    }

    void fGenTextures(GLsizei n, GLuint* names) {
        raw_fGenTextures(n, names);
        TRACKING_CONTEXT(CreatedTextures(this, n, names));
    }

private:
    void GLAPIENTRY raw_fDeleteProgram(GLuint program) {
        BEFORE_GL_CALL;
        mSymbols.fDeleteProgram(program);
        AFTER_GL_CALL;
    }

    void GLAPIENTRY raw_fDeleteShader(GLuint shader) {
        BEFORE_GL_CALL;
        mSymbols.fDeleteShader(shader);
        AFTER_GL_CALL;
    }

    void GLAPIENTRY raw_fDeleteBuffers(GLsizei n, GLuint *names) {
        BEFORE_GL_CALL;
        mSymbols.fDeleteBuffers(n, names);
        AFTER_GL_CALL;
    }

    void GLAPIENTRY raw_fDeleteFramebuffers(GLsizei n, GLuint *names) {
        BEFORE_GL_CALL;
        mSymbols.fDeleteFramebuffers(n, names);
        AFTER_GL_CALL;
    }

    void GLAPIENTRY raw_fDeleteRenderbuffers(GLsizei n, GLuint *names) {
        BEFORE_GL_CALL;
        mSymbols.fDeleteRenderbuffers(n, names);
        AFTER_GL_CALL;
    }

    void GLAPIENTRY raw_fDeleteTextures(GLsizei n, GLuint *names) {
        BEFORE_GL_CALL;
        mSymbols.fDeleteTextures(n, names);
        AFTER_GL_CALL;
    }

public:

    void fDeleteProgram(GLuint program) {
        raw_fDeleteProgram(program);
        TRACKING_CONTEXT(DeletedProgram(this, program));
    }

    void fDeleteShader(GLuint shader) {
        raw_fDeleteShader(shader);
        TRACKING_CONTEXT(DeletedShader(this, shader));
    }

    void fDeleteBuffers(GLsizei n, GLuint *names) {
        raw_fDeleteBuffers(n, names);
        TRACKING_CONTEXT(DeletedBuffers(this, n, names));
    }

    void fDeleteFramebuffers(GLsizei n, GLuint *names) {
        if (mScreen) {
            
            
            for (int i = 0; i < n; i++) {
                mScreen->DeletingFB(names[i]);
            }
        }

        if (n == 1 && *names == 0) {
            
        } else {
            raw_fDeleteFramebuffers(n, names);
        }
        TRACKING_CONTEXT(DeletedFramebuffers(this, n, names));
    }

    void fDeleteRenderbuffers(GLsizei n, GLuint *names) {
        raw_fDeleteRenderbuffers(n, names);
        TRACKING_CONTEXT(DeletedRenderbuffers(this, n, names));
    }

    void fDeleteTextures(GLsizei n, GLuint *names) {
        raw_fDeleteTextures(n, names);
        TRACKING_CONTEXT(DeletedTextures(this, n, names));
    }

    GLenum GLAPIENTRY fGetGraphicsResetStatus() {
        MOZ_ASSERT(mHasRobustness);

        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fGetGraphicsResetStatus);
        GLenum ret = mSymbols.fGetGraphicsResetStatus();
        AFTER_GL_CALL;
        return ret;
    }




public:
    GLsync GLAPIENTRY fFenceSync(GLenum condition, GLbitfield flags) {
        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fFenceSync);
        GLsync ret = mSymbols.fFenceSync(condition, flags);
        AFTER_GL_CALL;
        return ret;
    }

    realGLboolean GLAPIENTRY fIsSync(GLsync sync) {
        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fIsSync);
        realGLboolean ret = mSymbols.fIsSync(sync);
        AFTER_GL_CALL;
        return ret;
    }

    void GLAPIENTRY fDeleteSync(GLsync sync) {
        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fDeleteSync);
        mSymbols.fDeleteSync(sync);
        AFTER_GL_CALL;
    }

    GLenum GLAPIENTRY fClientWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout) {
        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fClientWaitSync);
        GLenum ret = mSymbols.fClientWaitSync(sync, flags, timeout);
        AFTER_GL_CALL;
        return ret;
    }

    void GLAPIENTRY fWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout) {
        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fWaitSync);
        mSymbols.fWaitSync(sync, flags, timeout);
        AFTER_GL_CALL;
    }

    void GLAPIENTRY fGetInteger64v(GLenum pname, GLint64 *params) {
        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fGetInteger64v);
        mSymbols.fGetInteger64v(pname, params);
        AFTER_GL_CALL;
    }

    void GLAPIENTRY fGetSynciv(GLsync sync, GLenum pname, GLsizei bufSize, GLsizei *length, GLint *values) {
        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fGetSynciv);
        mSymbols.fGetSynciv(sync, pname, bufSize, length, values);
        AFTER_GL_CALL;
    }




public:
    void fEGLImageTargetTexture2D(GLenum target, GLeglImage image) {
        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fEGLImageTargetTexture2D);
        mSymbols.fEGLImageTargetTexture2D(target, image);
        AFTER_GL_CALL;
    }

    void fEGLImageTargetRenderbufferStorage(GLenum target, GLeglImage image)
    {
        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fEGLImageTargetRenderbufferStorage);
        mSymbols.fEGLImageTargetRenderbufferStorage(target, image);
        AFTER_GL_CALL;
    }




public:
    void fBindBufferOffset(GLenum target, GLuint index, GLuint buffer, GLintptr offset)
    {
        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fBindBufferOffset);
        mSymbols.fBindBufferOffset(target, index, buffer, offset);
        AFTER_GL_CALL;
    }




public:
    void fDrawBuffers(GLsizei n, const GLenum* bufs) {
        BEFORE_GL_CALL;
        mSymbols.fDrawBuffers(n, bufs);
        AFTER_GL_CALL;
    }




public:
    void fDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei primcount)
    {
        BeforeGLDrawCall();
        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fDrawArraysInstanced);
        mSymbols.fDrawArraysInstanced(mode, first, count, primcount);
        AFTER_GL_CALL;
        AfterGLDrawCall();
    }

    void fDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices, GLsizei primcount)
    {
        BeforeGLDrawCall();
        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fDrawElementsInstanced);
        mSymbols.fDrawElementsInstanced(mode, count, type, indices, primcount);
        AFTER_GL_CALL;
        AfterGLDrawCall();
    }




public:
    
    void fBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) {
        BeforeGLDrawCall();
        BeforeGLReadCall();
        raw_fBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
        AfterGLReadCall();
        AfterGLDrawCall();
    }


private:
    void raw_fBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) {
        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fBlitFramebuffer);
        mSymbols.fBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
        AFTER_GL_CALL;
    }




public:
    void fRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height) {
        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fRenderbufferStorageMultisample);
        mSymbols.fRenderbufferStorageMultisample(target, samples, internalFormat, width, height);
        AFTER_GL_CALL;
    }




public:
    void fVertexAttribDivisor(GLuint index, GLuint divisor)
    {
        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fVertexAttribDivisor);
        mSymbols.fVertexAttribDivisor(index, divisor);
        AFTER_GL_CALL;
    }
















public:
    void fDeleteQueries(GLsizei n, const GLuint* names) {
        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fDeleteQueries);
        mSymbols.fDeleteQueries(n, names);
        AFTER_GL_CALL;
        TRACKING_CONTEXT(DeletedQueries(this, n, names));
    }

    void fGenQueries(GLsizei n, GLuint* names) {
        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fGenQueries);
        mSymbols.fGenQueries(n, names);
        AFTER_GL_CALL;
        TRACKING_CONTEXT(CreatedQueries(this, n, names));
    }

    void fGetQueryiv(GLenum target, GLenum pname, GLint* params) {
        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fGetQueryiv);
        mSymbols.fGetQueryiv(target, pname, params);
        AFTER_GL_CALL;
    }

    void fGetQueryObjectuiv(GLuint id, GLenum pname, GLuint* params) {
        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fGetQueryObjectuiv);
        mSymbols.fGetQueryObjectuiv(id, pname, params);
        AFTER_GL_CALL;
    }

    realGLboolean fIsQuery(GLuint query) {
        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fIsQuery);
        realGLboolean retval = mSymbols.fIsQuery(query);
        AFTER_GL_CALL;
        return retval;
    }













public:
    void fGetQueryObjectiv(GLuint id, GLenum pname, GLint* params) {
        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fGetQueryObjectiv);
        mSymbols.fGetQueryObjectiv(id, pname, params);
        AFTER_GL_CALL;
    }




public:
    void fBindBufferBase(GLenum target, GLuint index, GLuint buffer)
    {
        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fBindBufferBase);
        mSymbols.fBindBufferBase(target, index, buffer);
        AFTER_GL_CALL;
    }

    void fBindBufferRange(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size)
    {
        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fBindBufferRange);
        mSymbols.fBindBufferRange(target, index, buffer, offset, size);
        AFTER_GL_CALL;
    }

    void fBeginTransformFeedback(GLenum primitiveMode)
    {
        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fBeginTransformFeedback);
        mSymbols.fBeginTransformFeedback(primitiveMode);
        AFTER_GL_CALL;
    }

    void fEndTransformFeedback()
    {
        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fEndTransformFeedback);
        mSymbols.fEndTransformFeedback();
        AFTER_GL_CALL;
    }

    void fTransformFeedbackVaryings(GLuint program, GLsizei count, const GLchar* const* varyings, GLenum bufferMode)
    {
        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fTransformFeedbackVaryings);
        mSymbols.fTransformFeedbackVaryings(program, count, varyings, bufferMode);
        AFTER_GL_CALL;
    }

    void fGetTransformFeedbackVarying(GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLsizei* size, GLenum* type, GLchar* name)
    {
        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fGetTransformFeedbackVarying);
        mSymbols.fGetTransformFeedbackVarying(program, index, bufSize, length, size, type, name);
        AFTER_GL_CALL;
    }

    void fGetIntegeri_v(GLenum param, GLuint index, GLint* values)
    {
        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fGetIntegeri_v);
        mSymbols.fGetIntegeri_v(param, index, values);
        AFTER_GL_CALL;
    }




public:
    void GLAPIENTRY fBindVertexArray(GLuint array)
    {
        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fBindVertexArray);
        mSymbols.fBindVertexArray(array);
        AFTER_GL_CALL;
    }

    void GLAPIENTRY fDeleteVertexArrays(GLsizei n, const GLuint *arrays)
    {
        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fDeleteVertexArrays);
        mSymbols.fDeleteVertexArrays(n, arrays);
        AFTER_GL_CALL;
    }

    void GLAPIENTRY fGenVertexArrays(GLsizei n, GLuint *arrays)
    {
        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fGenVertexArrays);
        mSymbols.fGenVertexArrays(n, arrays);
        AFTER_GL_CALL;
    }

    realGLboolean GLAPIENTRY fIsVertexArray(GLuint array)
    {
        BEFORE_GL_CALL;
        ASSERT_SYMBOL_PRESENT(fIsVertexArray);
        realGLboolean ret = mSymbols.fIsVertexArray(array);
        AFTER_GL_CALL;
        return ret;
    }




public:

    typedef struct gfx::SurfaceCaps SurfaceCaps;


protected:
    GLContext(const SurfaceCaps& caps,
              GLContext* sharedContext = nullptr,
              bool isOffscreen = false);




public:
    virtual ~GLContext();




protected:

    typedef class gfx::SharedSurface SharedSurface;
    typedef gfx::SharedSurfaceType SharedSurfaceType;
    typedef gfx::SurfaceFormat SurfaceFormat;

public:

    virtual bool MakeCurrentImpl(bool aForce = false) = 0;

#ifdef MOZ_ENABLE_GL_TRACKING
    static void StaticInit() {
        PR_NewThreadPrivateIndex(&sCurrentGLContextTLS, nullptr);
    }
#endif

    bool MakeCurrent(bool aForce = false) {
        if (IsDestroyed()) {
            return false;
        }
#ifdef MOZ_ENABLE_GL_TRACKING
    PR_SetThreadPrivate(sCurrentGLContextTLS, this);

    
    
#if 0
        
        
        
        
        NS_ASSERTION(IsOwningThreadCurrent(),
                     "MakeCurrent() called on different thread than this context was created on!");
#endif
#endif
        return MakeCurrentImpl(aForce);
    }

    virtual bool SetupLookupFunction() = 0;

    virtual void ReleaseSurface() {}

    
    
    void MarkDestroyed();

    bool IsDestroyed() {
        
        return mSymbols.fUseProgram == nullptr;
    }

    enum NativeDataType {
      NativeGLContext,
      NativeImageSurface,
      NativeThebesSurface,
      NativeCGLContext,
      NativeDataTypeMax
    };

    virtual void *GetNativeData(NativeDataType aType) { return nullptr; }
    GLContext *GetSharedContext() { return mSharedContext; }

    



    bool IsOwningThreadCurrent();
    void DispatchToOwningThread(nsIRunnable *event);

    static void PlatformStartup();

public:
    




    virtual bool SwapBuffers() { return false; }

    


    virtual bool BindTexImage() { return false; }
    


    virtual bool ReleaseTexImage() { return false; }

    
    void GuaranteeResolve();

    







    virtual bool ResizeOffscreen(const gfx::IntSize& size) {
        return ResizeScreenBuffer(size);
    }

    




    const gfx::IntSize& OffscreenSize() const;

    void BindFB(GLuint fb) {
        fBindFramebuffer(LOCAL_GL_FRAMEBUFFER, fb);
        MOZ_ASSERT(!fb || fIsFramebuffer(fb));
    }

    void BindDrawFB(GLuint fb) {
        fBindFramebuffer(LOCAL_GL_DRAW_FRAMEBUFFER_EXT, fb);
    }

    void BindReadFB(GLuint fb) {
        fBindFramebuffer(LOCAL_GL_READ_FRAMEBUFFER_EXT, fb);
    }

    GLuint GetDrawFB() {
        if (mScreen)
            return mScreen->GetDrawFB();

        GLuint ret = 0;
        GetUIntegerv(LOCAL_GL_DRAW_FRAMEBUFFER_BINDING_EXT, &ret);
        return ret;
    }

    GLuint GetReadFB() {
        if (mScreen)
            return mScreen->GetReadFB();

        GLenum bindEnum = IsSupported(GLFeature::framebuffer_blit)
                            ? LOCAL_GL_READ_FRAMEBUFFER_BINDING_EXT
                            : LOCAL_GL_FRAMEBUFFER_BINDING;

        GLuint ret = 0;
        GetUIntegerv(bindEnum, &ret);
        return ret;
    }

    GLuint GetFB() {
        if (mScreen) {
            
            
            
            return mScreen->GetFB();
        }

        GLuint ret = 0;
        GetUIntegerv(LOCAL_GL_FRAMEBUFFER_BINDING, &ret);
        return ret;
    }

private:
    void GetShaderPrecisionFormatNonES2(GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision) {
        switch (precisiontype) {
            case LOCAL_GL_LOW_FLOAT:
            case LOCAL_GL_MEDIUM_FLOAT:
            case LOCAL_GL_HIGH_FLOAT:
                
                range[0] = 127;
                range[1] = 127;
                *precision = 23;
                break;
            case LOCAL_GL_LOW_INT:
            case LOCAL_GL_MEDIUM_INT:
            case LOCAL_GL_HIGH_INT:
                
                
                range[0] = 24;
                range[1] = 24;
                *precision = 0;
                break;
        }
    }

public:

    void ForceDirtyScreen();
    void CleanDirtyScreen();

    virtual GLenum GetPreferredARGB32Format() { return LOCAL_GL_RGBA; }

    virtual bool RenewSurface() { return false; }

    
    static bool ListHasExtension(const GLubyte *extensions,
                                 const char *extension);

    GLint GetMaxTextureImageSize() { return mMaxTextureImageSize; }
    void SetFlipped(bool aFlipped) { mFlipped = aFlipped; }


public:
    




    enum ContextResetARB {
        CONTEXT_NO_ERROR = 0,
        CONTEXT_GUILTY_CONTEXT_RESET_ARB = 0x8253,
        CONTEXT_INNOCENT_CONTEXT_RESET_ARB = 0x8254,
        CONTEXT_UNKNOWN_CONTEXT_RESET_ARB = 0x8255
    };

public:
    std::map<GLuint, SharedSurface_GL*> mFBOMapping;

    enum {
        DebugEnabled = 1 << 0,
        DebugTrace = 1 << 1,
        DebugAbortOnError = 1 << 2
    };

    static uint32_t sDebugMode;

    static uint32_t DebugMode() {
#ifdef DEBUG
        return sDebugMode;
#else
        return 0;
#endif
    }

protected:
    nsRefPtr<GLContext> mSharedContext;

    
    nsCOMPtr<nsIThread> mOwningThread;

    GLContextSymbols mSymbols;

#ifdef DEBUG
    
    
    
    
    
    static unsigned sCurrentGLContextTLS;
#endif
    bool mFlipped;

    ScopedDeletePtr<GLBlitHelper> mBlitHelper;
    ScopedDeletePtr<GLBlitTextureImageHelper> mBlitTextureImageHelper;
    ScopedDeletePtr<GLReadTexImageHelper> mReadTexImageHelper;

public:

    GLBlitHelper* BlitHelper();
    GLBlitTextureImageHelper* BlitTextureImageHelper();
    GLReadTexImageHelper* ReadTexImageHelper();

    
    bool SharesWith(const GLContext* other) const {
        MOZ_ASSERT(!this->mSharedContext || !this->mSharedContext->mSharedContext);
        MOZ_ASSERT(!other->mSharedContext || !other->mSharedContext->mSharedContext);
        MOZ_ASSERT(!this->mSharedContext ||
                   !other->mSharedContext ||
                   this->mSharedContext == other->mSharedContext);

        const GLContext* thisShared = this->mSharedContext ? this->mSharedContext
                                                           : this;
        const GLContext* otherShared = other->mSharedContext ? other->mSharedContext
                                                             : other;

        return thisShared == otherShared;
    }

    bool InitOffscreen(const gfx::IntSize& size, const SurfaceCaps& caps) {
        if (!CreateScreenBuffer(size, caps))
            return false;

        MakeCurrent();
        fBindFramebuffer(LOCAL_GL_FRAMEBUFFER, 0);
        fScissor(0, 0, size.width, size.height);
        fViewport(0, 0, size.width, size.height);

        mCaps = mScreen->Caps();
        if (mCaps.any)
            DetermineCaps();

        UpdateGLFormats(mCaps);
        UpdatePixelFormat();

        return true;
    }

protected:
    
    bool CreateScreenBuffer(const gfx::IntSize& size, const SurfaceCaps& caps) {
        if (!IsOffscreenSizeAllowed(size))
            return false;

        SurfaceCaps tryCaps = caps;
        if (tryCaps.antialias) {
            
            if (CreateScreenBufferImpl(size, tryCaps))
                return true;

            NS_WARNING("CreateScreenBuffer failed to initialize an AA context! Falling back to no AA...");
            tryCaps.antialias = false;
        }
        MOZ_ASSERT(!tryCaps.antialias);

        if (CreateScreenBufferImpl(size, tryCaps))
            return true;

        NS_WARNING("CreateScreenBuffer failed to initialize non-AA context!");
        return false;
    }

    bool CreateScreenBufferImpl(const gfx::IntSize& size,
                                const SurfaceCaps& caps);

public:
    bool ResizeScreenBuffer(const gfx::IntSize& size);

protected:
    SurfaceCaps mCaps;
    nsAutoPtr<GLFormats> mGLFormats;
    nsAutoPtr<PixelBufferFormat> mPixelFormat;

public:
    void DetermineCaps();
    const SurfaceCaps& Caps() const {
        return mCaps;
    }

    
    GLFormats ChooseGLFormats(const SurfaceCaps& caps) const;
    void UpdateGLFormats(const SurfaceCaps& caps) {
        mGLFormats = new GLFormats(ChooseGLFormats(caps));
    }

    const GLFormats& GetGLFormats() const {
        MOZ_ASSERT(mGLFormats);
        return *mGLFormats;
    }

    PixelBufferFormat QueryPixelFormat();
    void UpdatePixelFormat();

    const PixelBufferFormat& GetPixelFormat() const {
        MOZ_ASSERT(mPixelFormat);
        return *mPixelFormat;
    }

    bool IsFramebufferComplete(GLuint fb, GLenum* status = nullptr);

    
    void AttachBuffersToFB(GLuint colorTex, GLuint colorRB,
                           GLuint depthRB, GLuint stencilRB,
                           GLuint fb, GLenum target = LOCAL_GL_TEXTURE_2D);

    
    bool AssembleOffscreenFBs(const GLuint colorMSRB,
                              const GLuint depthRB,
                              const GLuint stencilRB,
                              const GLuint texture,
                              GLuint* drawFB,
                              GLuint* readFB);

protected:
    friend class GLScreenBuffer;
    GLScreenBuffer* mScreen;

    void DestroyScreenBuffer();

    SharedSurface* mLockedSurface;

public:
    void LockSurface(SharedSurface* surf) {
        MOZ_ASSERT(!mLockedSurface);
        mLockedSurface = surf;
    }

    void UnlockSurface(SharedSurface* surf) {
        MOZ_ASSERT(mLockedSurface == surf);
        mLockedSurface = nullptr;
    }

    SharedSurface* GetLockedSurface() const {
        return mLockedSurface;
    }

    bool IsOffscreen() const {
        return mScreen;
    }

    GLScreenBuffer* Screen() const {
        return mScreen;
    }

    bool PublishFrame();
    SharedSurface* RequestFrame();

    



    void ClearSafely();

    bool WorkAroundDriverBugs() const { return mWorkAroundDriverBugs; }

protected:
    nsRefPtr<TextureGarbageBin> mTexGarbageBin;

public:
    TextureGarbageBin* TexGarbageBin() {
        MOZ_ASSERT(mTexGarbageBin);
        return mTexGarbageBin;
    }

    void EmptyTexGarbageBin();

    bool IsOffscreenSizeAllowed(const gfx::IntSize& aSize) const;

protected:
    bool InitWithPrefix(const char *prefix, bool trygl);

    void InitExtensions();

    nsTArray<nsIntRect> mViewportStack;
    nsTArray<nsIntRect> mScissorStack;

    GLint mMaxTextureSize;
    GLint mMaxCubeMapTextureSize;
    GLint mMaxTextureImageSize;
    GLint mMaxRenderbufferSize;
    GLsizei mMaxSamples;
    bool mNeedsTextureSizeChecks;
    bool mWorkAroundDriverBugs;

    bool IsTextureSizeSafeToPassToDriver(GLenum target, GLsizei width, GLsizei height) const {
        if (mNeedsTextureSizeChecks) {
            
            
            
            
            
            
            GLsizei maxSize = target == LOCAL_GL_TEXTURE_CUBE_MAP ||
                                (target >= LOCAL_GL_TEXTURE_CUBE_MAP_POSITIVE_X &&
                                target <= LOCAL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Z)
                              ? mMaxCubeMapTextureSize
                              : mMaxTextureSize;
            return width <= maxSize && height <= maxSize;
        }
        return true;
    }


    

protected:
    GLint FixYValue(GLint y, GLint height)
    {
        MOZ_ASSERT( !(mIsOffscreen && mFlipped) );
        return mFlipped ? ViewportRect().height - (height + y) : y;
    }

public:
    void fScissor(GLint x, GLint y, GLsizei width, GLsizei height) {
        ScissorRect().SetRect(x, y, width, height);

        
        
        y = FixYValue(y, height);
        raw_fScissor(x, y, width, height);
    }

    nsIntRect& ScissorRect() {
        return mScissorStack[mScissorStack.Length()-1];
    }

    void PushScissorRect() {
        nsIntRect copy(ScissorRect());
        mScissorStack.AppendElement(copy);
    }

    void PushScissorRect(const nsIntRect& aRect) {
        mScissorStack.AppendElement(aRect);
        fScissor(aRect.x, aRect.y, aRect.width, aRect.height);
    }

    void PopScissorRect() {
        if (mScissorStack.Length() < 2) {
            NS_WARNING("PopScissorRect with Length < 2!");
            return;
        }

        nsIntRect thisRect = ScissorRect();
        mScissorStack.TruncateLength(mScissorStack.Length() - 1);
        if (!thisRect.IsEqualInterior(ScissorRect())) {
            fScissor(ScissorRect().x, ScissorRect().y,
                     ScissorRect().width, ScissorRect().height);
        }
    }

    

private:
    
    void raw_fViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
        BEFORE_GL_CALL;
        
        
        
        
        NS_ASSERTION(!mFlipped || (x == 0 && y == 0), "TODO: Need to flip the viewport rect");
        mSymbols.fViewport(x, y, width, height);
        AFTER_GL_CALL;
    }

public:
    void fViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
        ViewportRect().SetRect(x, y, width, height);
        raw_fViewport(x, y, width, height);
    }

    nsIntRect& ViewportRect() {
        return mViewportStack[mViewportStack.Length()-1];
    }

    void PushViewportRect() {
        nsIntRect copy(ViewportRect());
        mViewportStack.AppendElement(copy);
    }

    void PushViewportRect(const nsIntRect& aRect) {
        mViewportStack.AppendElement(aRect);
        raw_fViewport(aRect.x, aRect.y, aRect.width, aRect.height);
    }

    void PopViewportRect() {
        if (mViewportStack.Length() < 2) {
            NS_WARNING("PopViewportRect with Length < 2!");
            return;
        }

        nsIntRect thisRect = ViewportRect();
        mViewportStack.TruncateLength(mViewportStack.Length() - 1);
        if (!thisRect.IsEqualInterior(ViewportRect())) {
            raw_fViewport(ViewportRect().x, ViewportRect().y,
                          ViewportRect().width, ViewportRect().height);
        }
    }


#undef ASSERT_SYMBOL_PRESENT

#ifdef MOZ_ENABLE_GL_TRACKING
    void CreatedProgram(GLContext *aOrigin, GLuint aName);
    void CreatedShader(GLContext *aOrigin, GLuint aName);
    void CreatedBuffers(GLContext *aOrigin, GLsizei aCount, GLuint *aNames);
    void CreatedQueries(GLContext *aOrigin, GLsizei aCount, GLuint *aNames);
    void CreatedTextures(GLContext *aOrigin, GLsizei aCount, GLuint *aNames);
    void CreatedFramebuffers(GLContext *aOrigin, GLsizei aCount, GLuint *aNames);
    void CreatedRenderbuffers(GLContext *aOrigin, GLsizei aCount, GLuint *aNames);
    void DeletedProgram(GLContext *aOrigin, GLuint aName);
    void DeletedShader(GLContext *aOrigin, GLuint aName);
    void DeletedBuffers(GLContext *aOrigin, GLsizei aCount, GLuint *aNames);
    void DeletedQueries(GLContext *aOrigin, GLsizei aCount, const GLuint *aNames);
    void DeletedTextures(GLContext *aOrigin, GLsizei aCount, GLuint *aNames);
    void DeletedFramebuffers(GLContext *aOrigin, GLsizei aCount, GLuint *aNames);
    void DeletedRenderbuffers(GLContext *aOrigin, GLsizei aCount, GLuint *aNames);

    void SharedContextDestroyed(GLContext *aChild);
    void ReportOutstandingNames();

    struct NamedResource {
        NamedResource()
            : origin(nullptr), name(0), originDeleted(false)
        { }

        NamedResource(GLContext *aOrigin, GLuint aName)
            : origin(aOrigin), name(aName), originDeleted(false)
        { }

        GLContext *origin;
        GLuint name;
        bool originDeleted;

        
        bool operator<(const NamedResource& aOther) const {
            if (intptr_t(origin) < intptr_t(aOther.origin))
                return true;
            if (name < aOther.name)
                return true;
            return false;
        }
        bool operator==(const NamedResource& aOther) const {
            return origin == aOther.origin &&
                name == aOther.name &&
                originDeleted == aOther.originDeleted;
        }
    };

    nsTArray<NamedResource> mTrackedPrograms;
    nsTArray<NamedResource> mTrackedShaders;
    nsTArray<NamedResource> mTrackedTextures;
    nsTArray<NamedResource> mTrackedFramebuffers;
    nsTArray<NamedResource> mTrackedRenderbuffers;
    nsTArray<NamedResource> mTrackedBuffers;
    nsTArray<NamedResource> mTrackedQueries;
#endif
};

bool DoesStringMatch(const char* aString, const char *aWantedString);


} 
} 

#endif 
