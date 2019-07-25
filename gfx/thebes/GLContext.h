







































#ifndef GLCONTEXT_H_
#define GLCONTEXT_H_

#include <stdio.h>
#if defined(XP_UNIX)
#include <stdint.h>
#endif
#include <string.h>
#include <ctype.h>

#ifdef WIN32
#include <windows.h>
#endif

#include "GLDefs.h"
#include "gfxASurface.h"
#include "gfxImageSurface.h"
#include "gfxContext.h"
#include "gfxRect.h"
#include "nsISupportsImpl.h"
#include "prlink.h"

#include "nsDataHashtable.h"
#include "nsHashKeys.h"
#include "nsRegion.h"
#include "nsAutoPtr.h"
#include "nsThreadUtils.h"

#if defined(MOZ_PLATFORM_MAEMO) || defined(ANDROID) || defined(MOZ_EGL_XRENDER_COMPOSITE)
#define USE_GLES2 1
#endif

typedef char realGLboolean;

#include "GLContextSymbols.h"

namespace mozilla {
  namespace layers {
    class LayerManagerOGL;
    class ColorTextureLayerProgram;
  };

namespace gl {
class GLContext;

class LibrarySymbolLoader
{
public:
    bool OpenLibrary(const char *library);

    typedef PRFuncPtr (GLAPIENTRY * PlatformLookupFunction) (const char *);

    enum {
        MAX_SYMBOL_NAMES = 5,
        MAX_SYMBOL_LENGTH = 128
    };

    typedef struct {
        PRFuncPtr *symPointer;
        const char *symNames[MAX_SYMBOL_NAMES];
    } SymLoadStruct;

    bool LoadSymbols(SymLoadStruct *firstStruct,
                       bool tryplatform = false,
                       const char *prefix = nsnull);

    


    static PRFuncPtr LookupSymbol(PRLibrary *lib,
                                  const char *symname,
                                  PlatformLookupFunction lookupFunction = nsnull);
    static bool LoadSymbols(PRLibrary *lib,
                              SymLoadStruct *firstStruct,
                              PlatformLookupFunction lookupFunction = nsnull,
                              const char *prefix = nsnull);
protected:
    LibrarySymbolLoader() {
        mLibrary = nsnull;
        mLookupFunc = nsnull;
    }

    PRLibrary *mLibrary;
    PlatformLookupFunction mLookupFunc;
};

enum ShaderProgramType {
    RGBALayerProgramType,
    BGRALayerProgramType,
    RGBXLayerProgramType,
    BGRXLayerProgramType,
    RGBARectLayerProgramType,
    ColorLayerProgramType,
    YCbCrLayerProgramType,
    ComponentAlphaPass1ProgramType,
    ComponentAlphaPass2ProgramType,
    Copy2DProgramType,
    Copy2DRectProgramType,
    NumProgramTypes
};


















class TextureImage
{
    NS_INLINE_DECL_REFCOUNTING(TextureImage)
public:
    enum TextureState
    {
      Created, 
      Allocated,  
      Valid  
    };

    typedef gfxASurface::gfxContentType ContentType;

    virtual ~TextureImage() {}

    




















    virtual gfxASurface* BeginUpdate(nsIntRegion& aRegion) = 0;
    






    virtual void GetUpdateRegion(nsIntRegion& aForRegion) {
    };
    






    virtual void EndUpdate() = 0;

    



    virtual void BeginTileIteration() {
    };

    virtual bool NextTile() {
        return false;
    };

    virtual nsIntRect GetTileRect() {
        return nsIntRect(nsIntPoint(0,0), mSize);
    };

    virtual GLuint GetTextureID() = 0;

    virtual PRUint32 GetTileCount() {
        return 1;
    };

    







    virtual void Resize(const nsIntSize& aSize) {
        mSize = aSize;
        nsIntRegion r(nsIntRect(0, 0, aSize.width, aSize.height));
        BeginUpdate(r);
        EndUpdate();
    }

    




    virtual bool DirectUpdate(gfxASurface *aSurf, const nsIntRegion& aRegion, const nsIntPoint& aFrom = nsIntPoint(0,0)) = 0;

    virtual void BindTexture(GLenum aTextureUnit) = 0;
    virtual void ReleaseTexture() {};

    void BindTextureAndApplyFilter(GLenum aTextureUnit) {
        BindTexture(aTextureUnit);
        ApplyFilter();
    }

    class ScopedBindTexture
    {
    public:
        ScopedBindTexture(TextureImage *aTexture, GLenum aTextureUnit) :
          mTexture(aTexture)
        {
            if (mTexture) {
                mTexture->BindTexture(aTextureUnit);
            }
        }

        ~ScopedBindTexture()
        {
            if (mTexture) {
                mTexture->ReleaseTexture();
            }       
        }

    protected:
        TextureImage *mTexture;
    };

    class ScopedBindTextureAndApplyFilter
        : public ScopedBindTexture
    {
    public:
        ScopedBindTextureAndApplyFilter(TextureImage *aTexture, GLenum aTextureUnit) :
          ScopedBindTexture(aTexture, aTextureUnit)
        {
            if (mTexture) {
                mTexture->ApplyFilter();
            }
        }
    };

    




    virtual ShaderProgramType GetShaderProgramType()
    {
         return mShaderType;
    }

    

    



    virtual already_AddRefed<gfxASurface> GetBackingSurface()
    { return NULL; }

    const nsIntSize& GetSize() const { return mSize; }
    ContentType GetContentType() const { return mContentType; }
    virtual bool InUpdate() const = 0;
    GLenum GetWrapMode() const { return mWrapMode; }

    void SetFilter(gfxPattern::GraphicsFilter aFilter) { mFilter = aFilter; }

    



    virtual void ApplyFilter() = 0;

protected:
    friend class GLContext;

    





    TextureImage(const nsIntSize& aSize,
                 GLenum aWrapMode, ContentType aContentType,
                 bool aIsRGB = false)
        : mSize(aSize)
        , mWrapMode(aWrapMode)
        , mContentType(aContentType)
    {}

    nsIntSize mSize;
    GLenum mWrapMode;
    ContentType mContentType;
    ShaderProgramType mShaderType;
    gfxPattern::GraphicsFilter mFilter;
};










class BasicTextureImage
    : public TextureImage
{
public:
    typedef gfxASurface::gfxImageFormat ImageFormat;
    virtual ~BasicTextureImage();

    BasicTextureImage(GLuint aTexture,
                      const nsIntSize& aSize,
                      GLenum aWrapMode,
                      ContentType aContentType,
                      GLContext* aContext)
        : TextureImage(aSize, aWrapMode, aContentType)
        , mTexture(aTexture)
        , mTextureState(Created)
        , mGLContext(aContext)
        , mUpdateOffset(0, 0)
    {}

    virtual void BindTexture(GLenum aTextureUnit);

    virtual gfxASurface* BeginUpdate(nsIntRegion& aRegion);
    virtual void GetUpdateRegion(nsIntRegion& aForRegion);
    virtual void EndUpdate();
    virtual bool DirectUpdate(gfxASurface* aSurf, const nsIntRegion& aRegion, const nsIntPoint& aFrom = nsIntPoint(0,0));
    virtual GLuint GetTextureID() { return mTexture; };
    
    virtual already_AddRefed<gfxASurface>
      GetSurfaceForUpdate(const gfxIntSize& aSize, ImageFormat aFmt);

    
    
    
    virtual bool FinishedSurfaceUpdate();

    
    virtual void FinishedSurfaceUpload();

    virtual bool InUpdate() const { return !!mUpdateSurface; }

    virtual void Resize(const nsIntSize& aSize);

    virtual void ApplyFilter();
protected:

    GLuint mTexture;
    TextureState mTextureState;
    GLContext* mGLContext;
    nsRefPtr<gfxASurface> mUpdateSurface;
    nsIntRegion mUpdateRegion;

    
    nsIntPoint mUpdateOffset;
};






class TiledTextureImage
    : public TextureImage
{
public:
    TiledTextureImage(GLContext* aGL, nsIntSize aSize,
        TextureImage::ContentType, bool aUseNearestFilter = false);
    ~TiledTextureImage();
    void DumpDiv();
    virtual gfxASurface* BeginUpdate(nsIntRegion& aRegion);
    virtual void GetUpdateRegion(nsIntRegion& aForRegion);
    virtual void EndUpdate();
    virtual void Resize(const nsIntSize& aSize);
    virtual PRUint32 GetTileCount();
    virtual void BeginTileIteration();
    virtual bool NextTile();
    virtual nsIntRect GetTileRect();
    virtual GLuint GetTextureID() {
        return mImages[mCurrentImage]->GetTextureID();
    };
    virtual bool DirectUpdate(gfxASurface* aSurf, const nsIntRegion& aRegion, const nsIntPoint& aFrom = nsIntPoint(0,0));
    virtual bool InUpdate() const { return mInUpdate; };
    virtual void BindTexture(GLenum);
    virtual void ApplyFilter();
protected:
    unsigned int mCurrentImage;
    nsTArray< nsRefPtr<TextureImage> > mImages;
    bool mInUpdate;
    nsIntSize mSize;
    unsigned int mTileSize;
    unsigned int mRows, mColumns;
    GLContext* mGL;
    bool mUseNearestFilter;
    
    nsRefPtr<gfxASurface> mUpdateSurface;
    
    nsIntRegion mUpdateRegion;
    TextureState mTextureState;
};

struct THEBES_API ContextFormat
{
    static const ContextFormat BasicRGBA32Format;

    enum StandardContextFormat {
        Empty,
        BasicRGBA32,
        StrictBasicRGBA32,
        BasicRGB24,
        StrictBasicRGB24,
        BasicRGB16_565,
        StrictBasicRGB16_565
    };

    ContextFormat() {
        memset(this, 0, sizeof(ContextFormat));
    }

    ContextFormat(const StandardContextFormat cf) {
        memset(this, 0, sizeof(ContextFormat));
        switch (cf) {
        case BasicRGBA32:
            red = green = blue = alpha = 8;
            minRed = minGreen = minBlue = minAlpha = 1;
            break;

        case StrictBasicRGBA32:
            red = green = blue = alpha = 8;
            minRed = minGreen = minBlue = minAlpha = 8;
            break;

        case BasicRGB24:
            red = green = blue = 8;
            minRed = minGreen = minBlue = 1;
            break;

        case StrictBasicRGB24:
            red = green = blue = 8;
            minRed = minGreen = minBlue = 8;
            break;

        case StrictBasicRGB16_565:
            red = minRed = 5;
            green = minGreen = 6;
            blue = minBlue = 5;
            break;

        default:
            break;
        }
    }

    int depth, minDepth;
    int stencil, minStencil;
    int red, minRed;
    int green, minGreen;
    int blue, minBlue;
    int alpha, minAlpha;
    int samples;

    int colorBits() const { return red + green + blue; }
};

class GLContext
    : public LibrarySymbolLoader
{
    THEBES_INLINE_DECL_THREADSAFE_REFCOUNTING(GLContext)
public:
    GLContext(const ContextFormat& aFormat,
              bool aIsOffscreen = false,
              GLContext *aSharedContext = nsnull)
      : mInitialized(false),
        mIsOffscreen(aIsOffscreen),
#ifdef USE_GLES2
        mIsGLES2(true),
#else
        mIsGLES2(false),
#endif
        mIsGlobalSharedContext(false),
        mVendor(-1),
        mDebugMode(0),
        mCreationFormat(aFormat),
        mSharedContext(aSharedContext),
        mOffscreenTexture(0),
        mFlipped(false),
        mBlitProgram(0),
        mBlitFramebuffer(0),
        mOffscreenDrawFBO(0),
        mOffscreenReadFBO(0),
        mOffscreenFBOsDirty(false),
        mOffscreenColorRB(0),
        mOffscreenDepthRB(0),
        mOffscreenStencilRB(0)
#ifdef DEBUG
        , mGLError(LOCAL_GL_NO_ERROR)
#endif
    {
        mUserData.Init();
    }

    virtual ~GLContext() {
        NS_ASSERTION(IsDestroyed(), "GLContext implementation must call MarkDestroyed in destructor!");
#ifdef DEBUG
        if (mSharedContext) {
            GLContext *tip = mSharedContext;
            while (tip->mSharedContext)
                tip = tip->mSharedContext;
            tip->SharedContextDestroyed(this);
            tip->ReportOutstandingNames();
        }
#endif
    }

    enum GLContextType {
        ContextTypeUnknown,
        ContextTypeWGL,
        ContextTypeCGL,
        ContextTypeGLX,
        ContextTypeEGL,
        ContextTypeOSMesa
    };

    virtual GLContextType GetContextType() { return ContextTypeUnknown; }

    virtual bool MakeCurrentImpl(bool aForce = false) = 0;

    bool MakeCurrent(bool aForce = false) {
#ifdef DEBUG
        sCurrentGLContext = this;
#endif
        return MakeCurrentImpl(aForce);
    }

    virtual bool SetupLookupFunction() = 0;

    virtual void WindowDestroyed() {}

    virtual void ReleaseSurface() {}

    void *GetUserData(void *aKey) {
        void *result = nsnull;
        mUserData.Get(aKey, &result);
        return result;
    }

    void SetUserData(void *aKey, void *aValue) {
        mUserData.Put(aKey, aValue);
    }

    
    
    void THEBES_API MarkDestroyed();

    bool IsDestroyed() {
        
        return mSymbols.fUseProgram == nsnull;
    }

    enum NativeDataType {
      NativeGLContext,
      NativeImageSurface,
      NativeThebesSurface,
      NativeDataTypeMax
    };

    virtual void *GetNativeData(NativeDataType aType) { return NULL; }
    GLContext *GetSharedContext() { return mSharedContext; }

    bool IsGlobalSharedContext() { return mIsGlobalSharedContext; }
    void SetIsGlobalSharedContext(bool aIsOne) { mIsGlobalSharedContext = aIsOne; }

    const ContextFormat& CreationFormat() { return mCreationFormat; }
    const ContextFormat& ActualFormat() { return mActualFormat; }

    


    virtual void *GetD3DShareHandle() { return nsnull; }

    


    virtual bool IsDoubleBuffered() { return false; }

    




    bool IsGLES2() const {
        return mIsGLES2;
    }
    
    


    bool HasES2Compatibility() {
        return mIsGLES2 || IsExtensionSupported(ARB_ES2_compatibility);
    }

    enum {
        VendorIntel,
        VendorNVIDIA,
        VendorATI,
        VendorQualcomm,
        VendorOther
    };

    int Vendor() const {
        return mVendor;
    }

    




    virtual bool SwapBuffers() { return false; }

    


    virtual bool BindTexImage() { return false; }
    


    virtual bool ReleaseTexImage() { return false; }

    


    void ApplyFilterToBoundTexture(gfxPattern::GraphicsFilter aFilter);


    



    














    virtual bool BindOffscreenNeedsTexture(GLContext *aOffscreen) {
        return aOffscreen->mOffscreenTexture == 0;
    }

    virtual bool BindTex2DOffscreen(GLContext *aOffscreen) {
        if (aOffscreen->GetContextType() != GetContextType()) {
          return false;
        }

        if (!aOffscreen->mOffscreenDrawFBO && !aOffscreen->mOffscreenReadFBO) {
            return false;
        }

        if (!aOffscreen->mSharedContext ||
            aOffscreen->mSharedContext != mSharedContext)
        {
            return false;
        }

        fBindTexture(LOCAL_GL_TEXTURE_2D, aOffscreen->mOffscreenTexture);

        return true;
    }

    virtual void UnbindTex2DOffscreen(GLContext *aOffscreen) { }

    bool IsOffscreen() {
        return mIsOffscreen;
    }

    







    virtual bool ResizeOffscreen(const gfxIntSize& aNewSize) {
        if (mOffscreenDrawFBO || mOffscreenReadFBO)
            return ResizeOffscreenFBO(aNewSize, mOffscreenReadFBO != 0);
        return false;
    }

    




    gfxIntSize OffscreenSize() {
        return mOffscreenSize;
    }

    





    gfxIntSize OffscreenActualSize() {
        return mOffscreenActualSize;
    }

    






    GLuint GetOffscreenFBO() {
        
        return 0;
    }

    GLuint GetOffscreenTexture() {
        return mOffscreenTexture;
    }

    virtual bool SupportsFramebufferMultisample() {
        return IsExtensionSupported(EXT_framebuffer_multisample) || IsExtensionSupported(ANGLE_framebuffer_multisample);
    }

    virtual bool SupportsOffscreenSplit() {
        return IsExtensionSupported(EXT_framebuffer_blit) || IsExtensionSupported(ANGLE_framebuffer_blit);
    }

    GLuint GetBoundDrawFBO() {
        GLint ret = 0;
        if (SupportsOffscreenSplit())
            fGetIntegerv(LOCAL_GL_DRAW_FRAMEBUFFER_BINDING_EXT, &ret);
        else
            fGetIntegerv(LOCAL_GL_FRAMEBUFFER_BINDING, &ret);
        return ret;
    }

    GLuint GetBoundReadFBO() {
        GLint ret = 0;
        if (SupportsOffscreenSplit())
            fGetIntegerv(LOCAL_GL_READ_FRAMEBUFFER_BINDING_EXT, &ret);
        else
            fGetIntegerv(LOCAL_GL_FRAMEBUFFER_BINDING, &ret);
        return ret;
    }

    void BindDrawFBO(GLuint name) {
        if (SupportsOffscreenSplit())
            fBindFramebuffer(LOCAL_GL_DRAW_FRAMEBUFFER_EXT, name);
        else
            fBindFramebuffer(LOCAL_GL_FRAMEBUFFER, name);
    }

    void BindReadFBO(GLuint name) {
        if (SupportsOffscreenSplit())
            fBindFramebuffer(LOCAL_GL_READ_FRAMEBUFFER_EXT, name);
        else
            fBindFramebuffer(LOCAL_GL_FRAMEBUFFER, name);
    }

    GLuint SwapBoundDrawFBO(GLuint name) {
        GLuint prev = GetBoundDrawFBO();
        BindDrawFBO(name);
        return prev;
    }

    GLuint SwapBoundReadFBO(GLuint name) {
        GLuint prev = GetBoundReadFBO();
        BindReadFBO(name);
        return prev;
    }

    void BindOffscreenDrawBuffer() {
        BindDrawFBO(mOffscreenDrawFBO);
    }

    void BindOffscreenReadBuffer() {
        BindReadFBO(mOffscreenReadFBO);
    }

    void BindOffscreenBuffers() {
        BindOffscreenDrawBuffer();
        BindOffscreenReadBuffer();
    }

private:
    GLuint mPrevDrawFBOBinding;
    GLuint mPrevReadFBOBinding;
    bool mOffscreenFBOsDirty;

    void BeforeGLDrawCall() {
        
        mPrevDrawFBOBinding = GetBoundDrawFBO();
        if (mPrevDrawFBOBinding == 0) {
            BindDrawFBO(mOffscreenDrawFBO);
        } else if (mPrevDrawFBOBinding != mOffscreenDrawFBO)
            return;

        
        if (mOffscreenDrawFBO == mOffscreenReadFBO)
            return;

        
        if (mOffscreenFBOsDirty)
            return;

        mOffscreenFBOsDirty = true;
    }

    void AfterGLDrawCall() {
        if (mPrevDrawFBOBinding == 0) {
            BindDrawFBO(0);
        }
    }

    void BeforeGLReadCall() {
        
        mPrevReadFBOBinding = GetBoundReadFBO();
        if (mPrevReadFBOBinding == 0) {
            BindReadFBO(mOffscreenReadFBO);
        } else if (mPrevReadFBOBinding != mOffscreenReadFBO)
            return;

        
        if (mOffscreenDrawFBO == mOffscreenReadFBO)
            return;

        
        if (!mOffscreenFBOsDirty)
            return;

        const bool scissor = fIsEnabled(LOCAL_GL_SCISSOR_TEST);
        if (scissor)
            fDisable(LOCAL_GL_SCISSOR_TEST);

        
        GLuint prevDraw = SwapBoundDrawFBO(mOffscreenReadFBO);
        BindReadFBO(mOffscreenDrawFBO); 

        GLint width = mOffscreenActualSize.width;
        GLint height = mOffscreenActualSize.height;
        raw_fBlitFramebuffer(0, 0, width, height,
                             0, 0, width, height,
                             LOCAL_GL_COLOR_BUFFER_BIT,
                             LOCAL_GL_NEAREST);

        BindDrawFBO(prevDraw);
        BindReadFBO(mOffscreenReadFBO);

        if (scissor)
            fEnable(LOCAL_GL_SCISSOR_TEST);

        mOffscreenFBOsDirty = false;
    }

    void AfterGLReadCall() {
        if (mPrevReadFBOBinding == 0) {
            BindReadFBO(0);
        }
    }

public:
    
    void fClear(GLbitfield mask) {
        BeforeGLDrawCall();
        raw_fClear(mask);
        AfterGLDrawCall();
    }

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

    
    void fReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels) {
        BeforeGLReadCall();
        raw_fReadPixels(x, y, width, height, format, type, pixels);
        AfterGLReadCall();
    }

    void fCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border) {
        BeforeGLReadCall();
        raw_fCopyTexImage2D(target, level, internalformat,
                            x, y, width, height, border);
        AfterGLReadCall();
    }

    void fCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) {
        BeforeGLReadCall();
        raw_fCopyTexSubImage2D(target, level, xoffset, yoffset,
                               x, y, width, height);
        AfterGLReadCall();
    }

    void ForceDirtyFBOs() {
        GLuint draw = SwapBoundReadFBO(mOffscreenDrawFBO);

        BeforeGLDrawCall();
        
        AfterGLDrawCall();

        BindDrawFBO(draw);
    }

    void BlitDirtyFBOs() {
        GLuint read = SwapBoundReadFBO(mOffscreenReadFBO);

        BeforeGLReadCall();
        
        AfterGLReadCall();

        BindReadFBO(read);
    }

    
    void fFinish() {
        BeforeGLReadCall();
        raw_fFinish();
        AfterGLReadCall();
    }

    
    void fBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) {
        BeforeGLDrawCall();
        BeforeGLReadCall();
        raw_fBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
        AfterGLReadCall();
        AfterGLDrawCall();
    }

#if defined(MOZ_X11) && defined(MOZ_EGL_XRENDER_COMPOSITE)
    virtual gfxASurface* GetOffscreenPixmapSurface()
    {
      return 0;
    };
    
    virtual bool WaitNative() { return false; }
#endif

    virtual bool TextureImageSupportsGetBackingSurface() {
        return false;
    }

    virtual bool RenewSurface() { return false; }

    












    virtual already_AddRefed<TextureImage>
    CreateTextureImage(const nsIntSize& aSize,
                       TextureImage::ContentType aContentType,
                       GLenum aWrapMode,
                       bool aUseNearestFilter=false);

    






    virtual already_AddRefed<TextureImage>
    TileGenFunc(const nsIntSize& aSize,
                TextureImage::ContentType aContentType,
                bool aUseNearestFilter = false)
    {
        return nsnull;
    };

    










    already_AddRefed<gfxImageSurface> ReadTextureImage(GLuint aTexture,
                                                       const gfxIntSize& aSize,
                                                       GLenum aTextureFormat);

    



    void THEBES_API ReadPixelsIntoImageSurface(GLint aX, GLint aY,
                                    GLsizei aWidth, GLsizei aHeight,
                                    gfxImageSurface *aDest);

    
























    void BlitTextureImage(TextureImage *aSrc, const nsIntRect& aSrcRect,
                          TextureImage *aDst, const nsIntRect& aDstRect);

    


























    ShaderProgramType UploadSurfaceToTexture(gfxASurface *aSurface, 
                                             const nsIntRegion& aDstRegion,
                                             GLuint& aTexture,
                                             bool aOverwrite = false,
                                             const nsIntPoint& aSrcPoint = nsIntPoint(0, 0),
                                             bool aPixelBuffer = false);

    
    void TexImage2D(GLenum target, GLint level, GLint internalformat, 
                    GLsizei width, GLsizei height, GLsizei stride,
                    GLint pixelsize, GLint border, GLenum format, 
                    GLenum type, const GLvoid *pixels);


    void TexSubImage2D(GLenum target, GLint level, 
                       GLint xoffset, GLint yoffset, 
                       GLsizei width, GLsizei height, GLsizei stride,
                       GLint pixelsize, GLenum format, 
                       GLenum type, const GLvoid* pixels);

    

    struct RectTriangles {
        RectTriangles() { }

        void addRect(GLfloat x0, GLfloat y0, GLfloat x1, GLfloat y1,
                     GLfloat tx0, GLfloat ty0, GLfloat tx1, GLfloat ty1);

        




        float* vertexPointer() {
            return &vertexCoords[0].x;
        };

        float* texCoordPointer() {
            return &texCoords[0].u;
        };

        unsigned int elements() {
            return vertexCoords.Length();
        };

        typedef struct { GLfloat x,y; } vert_coord;
        typedef struct { GLfloat u,v; } tex_coord;
    private:
        
        nsAutoTArray<vert_coord, 6> vertexCoords;
        nsAutoTArray<tex_coord, 6>  texCoords;
    };

    












    static void DecomposeIntoNoRepeatTriangles(const nsIntRect& aTexCoordRect,
                                               const nsIntSize& aTexSize,
                                               RectTriangles& aRects);

    






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
        OES_depth_texture,
        OES_packed_depth_stencil,
        IMG_read_format,
        EXT_read_format_bgra,
        APPLE_client_storage,
        ARB_texture_non_power_of_two,
        ARB_pixel_buffer_object,
        ARB_ES2_compatibility,
        OES_texture_float,
        ARB_texture_float,
        EXT_unpack_subimage,
        OES_standard_derivatives,
        EXT_framebuffer_blit,
        ANGLE_framebuffer_blit,
        EXT_framebuffer_multisample,
        ANGLE_framebuffer_multisample,
        OES_rgb8_rgba8,
        Extensions_Max
    };

    bool IsExtensionSupported(GLExtensions aKnownExtension) {
        return mAvailableExtensions[aKnownExtension];
    }

    
    bool IsExtensionSupported(const char *extension);

    
    static bool ListHasExtension(const GLubyte *extensions,
                                   const char *extension);

    GLint GetMaxTextureSize() { return mMaxTextureSize; }
    void SetFlipped(bool aFlipped) { mFlipped = aFlipped; }

    
    
    
    
    template<size_t setlen>
    struct ExtensionBitset {
        ExtensionBitset() {
            for (size_t i = 0; i < setlen; ++i)
                values[i] = false;
        }

        bool& operator[](size_t index) {
            NS_ASSERTION(index < setlen, "out of range");
            return values[index];
        }

        bool values[setlen];
    };

protected:
    bool mInitialized;
    bool mIsOffscreen;
    bool mIsGLES2;
    bool mIsGlobalSharedContext;

    PRInt32 mVendor;

    enum {
        DebugEnabled = 1 << 0,
        DebugTrace = 1 << 1,
        DebugAbortOnError = 1 << 2
    };

    PRUint32 mDebugMode;

    ContextFormat mCreationFormat;
    nsRefPtr<GLContext> mSharedContext;

    GLContextSymbols mSymbols;

#ifdef DEBUG
    
    
    
    static THEBES_API GLContext* sCurrentGLContext;
#endif

    void UpdateActualFormat();
    ContextFormat mActualFormat;

    gfxIntSize mOffscreenSize;
    gfxIntSize mOffscreenActualSize;
    GLuint mOffscreenTexture;
    bool mFlipped;

    
    GLuint mBlitProgram, mBlitFramebuffer;
    void UseBlitProgram();
    void SetBlitFramebufferForDestTexture(GLuint aTexture);

    
    
    bool ResizeOffscreenFBO(const gfxIntSize& aSize, const bool aUseReadFBO, const bool aDisableAA);
    bool ResizeOffscreenFBO(const gfxIntSize& aSize, const bool aUseReadFBO) {
        if (ResizeOffscreenFBO(aSize, aUseReadFBO, false))
            return true;

        if (!mCreationFormat.samples)
            return false;
        printf("ResizeOffscreenFBO failed with AA, retrying without...\n");
        return ResizeOffscreenFBO(aSize, aUseReadFBO, true);
    }
    void DeleteOffscreenFBO();
    GLuint mOffscreenDrawFBO;
    GLuint mOffscreenReadFBO;
    GLuint mOffscreenColorRB;
    GLuint mOffscreenDepthRB;
    GLuint mOffscreenStencilRB;

    ExtensionBitset<Extensions_Max> mAvailableExtensions;

    
    
    
    void ClearSafely();

    nsDataHashtable<nsVoidPtrHashKey, void*> mUserData;

    void SetIsGLES2(bool aIsGLES2) {
        NS_ASSERTION(!mInitialized, "SetIsGLES2 can only be called before initialization!");
        mIsGLES2 = aIsGLES2;
    }

    bool InitWithPrefix(const char *prefix, bool trygl);

    void InitExtensions();

    virtual already_AddRefed<TextureImage>
    CreateBasicTextureImage(GLuint aTexture,
                            const nsIntSize& aSize,
                            GLenum aWrapMode,
                            TextureImage::ContentType aContentType,
                            GLContext* aContext)
    {
        nsRefPtr<BasicTextureImage> teximage(
            new BasicTextureImage(aTexture, aSize, aWrapMode, aContentType, aContext));
        return teximage.forget();
    }

    bool IsOffscreenSizeAllowed(const gfxIntSize& aSize) const {
        PRInt32 biggerDimension = NS_MAX(aSize.width, aSize.height);
        PRInt32 maxAllowed = NS_MIN(mMaxRenderbufferSize, mMaxTextureSize);
        return biggerDimension <= maxAllowed;
    }

protected:
    nsTArray<nsIntRect> mViewportStack;
    nsTArray<nsIntRect> mScissorStack;

    GLint mMaxTextureSize;
    GLint mMaxRenderbufferSize;

public:
 
    


    GLenum GetAndClearError() {
        
        GLenum error = fGetError();
        
        if (error) {
            
            while(fGetError()) {}
        }
        
        return error;
    }

#ifdef DEBUG

#ifndef MOZ_FUNCTION_NAME
# ifdef __GNUC__
#  define MOZ_FUNCTION_NAME __PRETTY_FUNCTION__
# elif defined(_MSC_VER)
#  define MOZ_FUNCTION_NAME __FUNCTION__
# else
#  define MOZ_FUNCTION_NAME __func__  // defined in C99, supported in various C++ compilers. Just raw function name.
# endif
#endif

protected:
    GLenum mGLError;

public:

    void BeforeGLCall(const char* glFunction) {
        if (mDebugMode) {
            
            
            
            if (!NS_IsMainThread()) {
                NS_ERROR("OpenGL call from non-main thread. While this is fine in itself, "
                         "the OpenGL debug mode, which is currently enabled, doesn't support this. "
                         "It needs to be patched by making GLContext::sCurrentGLContext be thread-local.\n");
                NS_ABORT();
            }
            if (mDebugMode & DebugTrace)
                printf_stderr("[gl:%p] > %s\n", this, glFunction);
            if (this != sCurrentGLContext) {
                printf_stderr("Fatal: %s called on non-current context %p. "
                              "The current context for this thread is %p.\n",
                               glFunction, this, sCurrentGLContext);
                NS_ABORT();
            }
        }
    }

    void AfterGLCall(const char* glFunction) {
        if (mDebugMode) {
            
            
            
            mSymbols.fFinish();
            mGLError = mSymbols.fGetError();
            if (mDebugMode & DebugTrace)
                printf_stderr("[gl:%p] < %s [0x%04x]\n", this, glFunction, mGLError);
            if (mGLError != LOCAL_GL_NO_ERROR) {
                printf_stderr("GL ERROR: %s generated GL error %s(0x%04x)\n", 
                              glFunction,
                              GLErrorToString(mGLError),
                              mGLError);
                if (mDebugMode & DebugAbortOnError)
                    NS_ABORT();
            }
        }
    }

    const char* GLErrorToString(GLenum aError)
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

#define BEFORE_GL_CALL do {                     \
    BeforeGLCall(MOZ_FUNCTION_NAME);            \
} while (0)
    
#define AFTER_GL_CALL do {                      \
    AfterGLCall(MOZ_FUNCTION_NAME);             \
} while (0)

#else

#define BEFORE_GL_CALL do { } while (0)
#define AFTER_GL_CALL do { } while (0)

#endif

    

    GLenum fGetError() {
#ifdef DEBUG
        
        if (mDebugMode) {
            GLenum err = mGLError;
            mGLError = LOCAL_GL_NO_ERROR;
            return err;
        }
#endif

        return mSymbols.fGetError();
    }


    

protected:

    GLint FixYValue(GLint y, GLint height)
    {
        return mFlipped ? ViewportRect().height - (height + y) : y;
    }

    
    void raw_fScissor(GLint x, GLint y, GLsizei width, GLsizei height) {
        BEFORE_GL_CALL;
        
        
        mSymbols.fScissor(x, 
                          FixYValue(y, height),
                          width, 
                          height);
        AFTER_GL_CALL;
    }

public:

    
    void fScissor(GLint x, GLint y, GLsizei width, GLsizei height) {
        ScissorRect().SetRect(x, y, width, height);
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
        raw_fScissor(aRect.x, aRect.y, aRect.width, aRect.height);
    }

    void PopScissorRect() {
        if (mScissorStack.Length() < 2) {
            NS_WARNING("PopScissorRect with Length < 2!");
            return;
        }

        nsIntRect thisRect = ScissorRect();
        mScissorStack.TruncateLength(mScissorStack.Length() - 1);
        if (!thisRect.IsEqualInterior(ScissorRect())) {
            raw_fScissor(ScissorRect().x, ScissorRect().y,
                              ScissorRect().width, ScissorRect().height);
        }
    }

    

protected:

    
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

    void fBufferData(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage) {
        BEFORE_GL_CALL;
        mSymbols.fBufferData(target, size, data, usage);
        AFTER_GL_CALL;
    }

    void fBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data) {
        BEFORE_GL_CALL;
        mSymbols.fBufferSubData(target, offset, size, data);
        AFTER_GL_CALL;
    }

    void raw_fClear(GLbitfield mask) {
        BEFORE_GL_CALL;
        mSymbols.fClear(mask);
        AFTER_GL_CALL;
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

    void fCullFace(GLenum mode) {
        BEFORE_GL_CALL;
        mSymbols.fCullFace(mode);
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

    void raw_fFinish() {
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

    void fGetIntegerv(GLenum pname, GLint *params) {
        BEFORE_GL_CALL;
        mSymbols.fGetIntegerv(pname, params);
        AFTER_GL_CALL;
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

    realGLboolean fIsEnabled (GLenum capability) {
        BEFORE_GL_CALL;
        realGLboolean retval = mSymbols.fIsEnabled(capability);
        AFTER_GL_CALL;
        return retval;
    }

    realGLboolean fIsProgram (GLuint program) {
        BEFORE_GL_CALL;
        realGLboolean retval = mSymbols.fIsProgram(program);
        AFTER_GL_CALL;
        return retval;
    }

    realGLboolean fIsShader (GLuint shader) {
        BEFORE_GL_CALL;
        realGLboolean retval = mSymbols.fIsShader(shader);
        AFTER_GL_CALL;
        return retval;
    }

    realGLboolean fIsTexture (GLuint texture) {
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

    void fPixelStorei(GLenum pname, GLint param) {
        BEFORE_GL_CALL;
        mSymbols.fPixelStorei(pname, param);
        AFTER_GL_CALL;
    }

    void fPolygonOffset(GLfloat factor, GLfloat bias) {
        BEFORE_GL_CALL;
        mSymbols.fPolygonOffset(factor, bias);
        AFTER_GL_CALL;
    }

    void fReadBuffer(GLenum mode) {
        BEFORE_GL_CALL;
        mSymbols.fReadBuffer(mode);
        AFTER_GL_CALL;
    }

    void raw_fReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels) {
        BEFORE_GL_CALL;
        mSymbols.fReadPixels(x, y, width, height, format, type, pixels);
        AFTER_GL_CALL;
    }

    void fSampleCoverage(GLclampf value, realGLboolean invert) {
        BEFORE_GL_CALL;
        mSymbols.fSampleCoverage(value, invert);
        AFTER_GL_CALL;
    }

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

    void fTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels) {
        BEFORE_GL_CALL;
        mSymbols.fTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
        AFTER_GL_CALL;
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

    void raw_fCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border) {
        BEFORE_GL_CALL;
        mSymbols.fCopyTexImage2D(target, level, internalformat, 
                                 x, FixYValue(y, height),
                                 width, height, border);
        AFTER_GL_CALL;
    }

    void raw_fCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) {
        BEFORE_GL_CALL;
        mSymbols.fCopyTexSubImage2D(target, level, xoffset, yoffset, 
                                    x, FixYValue(y, height),
                                    width, height);
        AFTER_GL_CALL;
    }

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

    void fBindFramebuffer(GLenum target, GLuint framebuffer) {
        BEFORE_GL_CALL;
        mSymbols.fBindFramebuffer(target, framebuffer);
        AFTER_GL_CALL;
    }

    void fBindRenderbuffer(GLenum target, GLuint renderbuffer) {
        BEFORE_GL_CALL;
        mSymbols.fBindRenderbuffer(target, renderbuffer);
        AFTER_GL_CALL;
    }

    GLenum fCheckFramebufferStatus (GLenum target) {
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

    void raw_fBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) {
        BEFORE_GL_CALL;
        mSymbols.fBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
        AFTER_GL_CALL;
    }

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

    void fRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height) {
        BEFORE_GL_CALL;
        mSymbols.fRenderbufferStorageMultisample(target, samples, internalFormat, width, height);
        AFTER_GL_CALL;
    }

    void fDepthRange(GLclampf a, GLclampf b) {
        BEFORE_GL_CALL;
        if (mIsGLES2) {
            mSymbols.fDepthRangef(a, b);
        } else {
            mSymbols.fDepthRange(a, b);
        }
        AFTER_GL_CALL;
    }

    void fClearDepth(GLclampf v) {
        BEFORE_GL_CALL;
        if (mIsGLES2) {
            mSymbols.fClearDepthf(v);
        } else {
            mSymbols.fClearDepth(v);
        }
        AFTER_GL_CALL;
    }

    void* fMapBuffer(GLenum target, GLenum access) {
        BEFORE_GL_CALL;
        void *ret = mSymbols.fMapBuffer(target, access);
        AFTER_GL_CALL;
        return ret;
    }

    realGLboolean fUnmapBuffer(GLenum target) {
        BEFORE_GL_CALL;
        realGLboolean ret = mSymbols.fUnmapBuffer(target);
        AFTER_GL_CALL;
        return ret;
    }


#ifdef DEBUG
     GLContext *TrackingContext() {
         GLContext *tip = this;
         while (tip->mSharedContext)
             tip = tip->mSharedContext;
         return tip;
     }

#define TRACKING_CONTEXT(a) do { TrackingContext()->a; } while (0)
#else
#define TRACKING_CONTEXT(a) do {} while (0)
#endif

     GLuint GLAPIENTRY fCreateProgram() {
         BEFORE_GL_CALL;
         GLuint ret = mSymbols.fCreateProgram();
         AFTER_GL_CALL;
         TRACKING_CONTEXT(CreatedProgram(this, ret));
         return ret;
     }

     GLuint GLAPIENTRY fCreateShader(GLenum t) {
         BEFORE_GL_CALL;
         GLuint ret = mSymbols.fCreateShader(t);
         AFTER_GL_CALL;
         TRACKING_CONTEXT(CreatedShader(this, ret));
         return ret;
     }

     void GLAPIENTRY fGenBuffers(GLsizei n, GLuint* names) {
         BEFORE_GL_CALL;
         mSymbols.fGenBuffers(n, names);
         AFTER_GL_CALL;
         TRACKING_CONTEXT(CreatedBuffers(this, n, names));
     }

     void GLAPIENTRY fGenTextures(GLsizei n, GLuint* names) {
         BEFORE_GL_CALL;
         mSymbols.fGenTextures(n, names);
         AFTER_GL_CALL;
         TRACKING_CONTEXT(CreatedTextures(this, n, names));
     }

     void GLAPIENTRY fGenFramebuffers(GLsizei n, GLuint* names) {
         BEFORE_GL_CALL;
         mSymbols.fGenFramebuffers(n, names);
         AFTER_GL_CALL;
         TRACKING_CONTEXT(CreatedFramebuffers(this, n, names));
     }

     void GLAPIENTRY fGenRenderbuffers(GLsizei n, GLuint* names) {
         BEFORE_GL_CALL;
         mSymbols.fGenRenderbuffers(n, names);
         AFTER_GL_CALL;
         TRACKING_CONTEXT(CreatedRenderbuffers(this, n, names));
     }

     void GLAPIENTRY fDeleteProgram(GLuint program) {
         BEFORE_GL_CALL;
         mSymbols.fDeleteProgram(program);
         AFTER_GL_CALL;
         TRACKING_CONTEXT(DeletedProgram(this, program));
     }

     void GLAPIENTRY fDeleteShader(GLuint shader) {
         BEFORE_GL_CALL;
         mSymbols.fDeleteShader(shader);
         AFTER_GL_CALL;
         TRACKING_CONTEXT(DeletedShader(this, shader));
     }

     void GLAPIENTRY fDeleteBuffers(GLsizei n, GLuint *names) {
         BEFORE_GL_CALL;
         mSymbols.fDeleteBuffers(n, names);
         AFTER_GL_CALL;
         TRACKING_CONTEXT(DeletedBuffers(this, n, names));
     }

     void GLAPIENTRY fDeleteTextures(GLsizei n, GLuint *names) {
         BEFORE_GL_CALL;
         mSymbols.fDeleteTextures(n, names);
         AFTER_GL_CALL;
         TRACKING_CONTEXT(DeletedTextures(this, n, names));
     }

     void GLAPIENTRY fDeleteFramebuffers(GLsizei n, GLuint *names) {
         BEFORE_GL_CALL;
         if (n == 1 && *names == 0) {
            
         } else {
            mSymbols.fDeleteFramebuffers(n, names);
         }
         AFTER_GL_CALL;
         TRACKING_CONTEXT(DeletedFramebuffers(this, n, names));
     }

     void GLAPIENTRY fDeleteRenderbuffers(GLsizei n, GLuint *names) {
         BEFORE_GL_CALL;
         mSymbols.fDeleteRenderbuffers(n, names);
         AFTER_GL_CALL;
         TRACKING_CONTEXT(DeletedRenderbuffers(this, n, names));
     }
#ifdef DEBUG
    void THEBES_API CreatedProgram(GLContext *aOrigin, GLuint aName);
    void THEBES_API CreatedShader(GLContext *aOrigin, GLuint aName);
    void THEBES_API CreatedBuffers(GLContext *aOrigin, GLsizei aCount, GLuint *aNames);
    void THEBES_API CreatedTextures(GLContext *aOrigin, GLsizei aCount, GLuint *aNames);
    void THEBES_API CreatedFramebuffers(GLContext *aOrigin, GLsizei aCount, GLuint *aNames);
    void THEBES_API CreatedRenderbuffers(GLContext *aOrigin, GLsizei aCount, GLuint *aNames);
    void THEBES_API DeletedProgram(GLContext *aOrigin, GLuint aName);
    void THEBES_API DeletedShader(GLContext *aOrigin, GLuint aName);
    void THEBES_API DeletedBuffers(GLContext *aOrigin, GLsizei aCount, GLuint *aNames);
    void THEBES_API DeletedTextures(GLContext *aOrigin, GLsizei aCount, GLuint *aNames);
    void THEBES_API DeletedFramebuffers(GLContext *aOrigin, GLsizei aCount, GLuint *aNames);
    void THEBES_API DeletedRenderbuffers(GLContext *aOrigin, GLsizei aCount, GLuint *aNames);

    void SharedContextDestroyed(GLContext *aChild);
    void ReportOutstandingNames();

    struct NamedResource {
        NamedResource()
            : origin(nsnull), name(0), originDeleted(false)
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
#endif

};

inline bool
DoesVendorStringMatch(const char* aVendorString, const char *aWantedVendor)
{
    if (!aVendorString || !aWantedVendor)
        return false;

    const char *occurrence = strstr(aVendorString, aWantedVendor);

    
    if (!occurrence)
        return false;

    
    if (occurrence != aVendorString && isalpha(*(occurrence-1)))
        return false;

    
    const char *afterOccurrence = occurrence + strlen(aWantedVendor);
    if (isalpha(*afterOccurrence))
        return false;

    return true;
}

} 
} 

#endif 
