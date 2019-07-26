




#ifndef GFX_ASURFACE_H
#define GFX_ASURFACE_H

#ifdef MOZ_DUMP_PAINTING
 #define MOZ_DUMP_IMAGES
#endif

#include "gfxTypes.h"
#include "gfxRect.h"
#include "nsAutoPtr.h"
#include "nsAutoRef.h"
#include "nsThreadUtils.h"

typedef struct _cairo_surface cairo_surface_t;
typedef struct _cairo_user_data_key cairo_user_data_key_t;

typedef void (*thebes_destroy_func_t) (void *data);

class gfxImageSurface;
struct nsIntPoint;
struct nsIntRect;





class THEBES_API gfxASurface {
public:
#ifdef MOZILLA_INTERNAL_API
    nsrefcnt AddRef(void);
    nsrefcnt Release(void);

    
    virtual nsrefcnt AddRefExternal(void)
    {
      return AddRef();
    }
    virtual nsrefcnt ReleaseExternal(void)
    {
      return Release();
    }
#else
    virtual nsrefcnt AddRef(void);
    virtual nsrefcnt Release(void);
#endif

public:
    



    typedef enum {
        ImageFormatARGB32, 
        ImageFormatRGB24,  
        ImageFormatA8,     
        ImageFormatA1,     
        ImageFormatRGB16_565,  
        ImageFormatUnknown
    } gfxImageFormat;

    typedef enum {
        SurfaceTypeImage,
        SurfaceTypePDF,
        SurfaceTypePS,
        SurfaceTypeXlib,
        SurfaceTypeXcb,
        SurfaceTypeGlitz,           
        SurfaceTypeQuartz,
        SurfaceTypeWin32,
        SurfaceTypeBeOS,
        SurfaceTypeDirectFB,        
        SurfaceTypeSVG,
        SurfaceTypeOS2,
        SurfaceTypeWin32Printing,
        SurfaceTypeQuartzImage,
        SurfaceTypeScript,
        SurfaceTypeQPainter,
        SurfaceTypeRecording,
        SurfaceTypeVG,
        SurfaceTypeGL,
        SurfaceTypeDRM,
        SurfaceTypeTee,
        SurfaceTypeXML,
        SurfaceTypeSkia,
        SurfaceTypeSubsurface,
        SurfaceTypeD2D,
        SurfaceTypeMax
    } gfxSurfaceType;

    typedef enum {
        CONTENT_COLOR       = 0x1000,
        CONTENT_ALPHA       = 0x2000,
        CONTENT_COLOR_ALPHA = 0x3000,
        CONTENT_SENTINEL    = 0xffff
    } gfxContentType;

    


    static already_AddRefed<gfxASurface> Wrap(cairo_surface_t *csurf);

    
    cairo_surface_t *CairoSurface() {
        NS_ASSERTION(mSurface != nullptr, "gfxASurface::CairoSurface called with mSurface == nullptr!");
        return mSurface;
    }

    gfxSurfaceType GetType() const;

    gfxContentType GetContentType() const;

    void SetDeviceOffset(const gfxPoint& offset);
    gfxPoint GetDeviceOffset() const;

    virtual bool GetRotateForLandscape() { return false; }

    void Flush() const;
    void MarkDirty();
    void MarkDirty(const gfxRect& r);

    
    virtual nsresult BeginPrinting(const nsAString& aTitle, const nsAString& aPrintToFileName);
    virtual nsresult EndPrinting();
    virtual nsresult AbortPrinting();
    virtual nsresult BeginPage();
    virtual nsresult EndPage();

    void SetData(const cairo_user_data_key_t *key,
                 void *user_data,
                 thebes_destroy_func_t destroy);
    void *GetData(const cairo_user_data_key_t *key);

    virtual void Finish();

    




    virtual already_AddRefed<gfxASurface> CreateSimilarSurface(gfxContentType aType,
                                                               const gfxIntSize& aSize);

    




    virtual already_AddRefed<gfxImageSurface> GetAsImageSurface()
    {
      return nullptr;
    }

    




    virtual already_AddRefed<gfxImageSurface> GetAsReadableARGB32ImageSurface();

    



    already_AddRefed<gfxImageSurface> CopyToARGB32ImageSurface();

    int CairoStatus();

    



    static bool CheckSurfaceSize(const gfxIntSize& sz, int32_t limit = 0);

    


    static int32_t FormatStrideForWidth(gfxImageFormat format, int32_t width);

    



    virtual int32_t GetDefaultContextFlags() const { return 0; }

    static gfxContentType ContentFromFormat(gfxImageFormat format);

    void SetSubpixelAntialiasingEnabled(bool aEnabled);
    bool GetSubpixelAntialiasingEnabled();

    



    static void RecordMemoryUsedForSurfaceType(gfxASurface::gfxSurfaceType aType,
                                               int32_t aBytes);

    





    void RecordMemoryUsed(int32_t aBytes);
    void RecordMemoryFreed();

    virtual int32_t KnownMemoryUsed() { return mBytesRecorded; }

    virtual size_t SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) const;
    virtual size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const;
    
    
    
    
    
    
    
    virtual bool SizeOfIsMeasured() const { return false; }

    




    enum MemoryLocation {
      MEMORY_IN_PROCESS_HEAP,
      MEMORY_IN_PROCESS_NONHEAP,
      MEMORY_OUT_OF_PROCESS
    };

    



    virtual MemoryLocation GetMemoryLocation() const;

    static int32_t BytePerPixelFromFormat(gfxImageFormat format);

    virtual const gfxIntSize GetSize() const { return gfxIntSize(-1, -1); }

#ifdef MOZ_DUMP_IMAGES
    



    


    void WriteAsPNG(const char* aFile);

    


    void DumpAsDataURL(FILE* aOutput = stdout);

    


    void PrintAsDataURL();

    


    void CopyAsDataURL();
    
    void WriteAsPNG_internal(FILE* aFile, bool aBinary);
#endif

    void SetOpaqueRect(const gfxRect& aRect) {
        if (aRect.IsEmpty()) {
            mOpaqueRect = nullptr;
        } else if (mOpaqueRect) {
            *mOpaqueRect = aRect;
        } else {
            mOpaqueRect = new gfxRect(aRect);
        }
    }
    const gfxRect& GetOpaqueRect() {
        if (mOpaqueRect)
            return *mOpaqueRect;
        static const gfxRect empty(0, 0, 0, 0);
        return empty;
    }

    









    virtual void MovePixels(const nsIntRect& aSourceRect,
                            const nsIntPoint& aDestTopLeft);

    


    void SetAllowUseAsSource(bool aAllow) { mAllowUseAsSource = aAllow; }
    bool GetAllowUseAsSource() { return mAllowUseAsSource; }

    static uint8_t BytesPerPixel(gfxImageFormat aImageFormat);

protected:
    gfxASurface() : mSurface(nullptr), mFloatingRefs(0), mBytesRecorded(0),
                    mSurfaceValid(false), mAllowUseAsSource(true)
    {
        MOZ_COUNT_CTOR(gfxASurface);
    }

    static gfxASurface* GetSurfaceWrapper(cairo_surface_t *csurf);
    static void SetSurfaceWrapper(cairo_surface_t *csurf, gfxASurface *asurf);

    




    void FastMovePixels(const nsIntRect& aSourceRect,
                        const nsIntPoint& aDestTopLeft);

    
    
    
    void Init(cairo_surface_t *surface, bool existingSurface = false);

    virtual ~gfxASurface()
    {
        RecordMemoryFreed();

        MOZ_COUNT_DTOR(gfxASurface);
    }

    cairo_surface_t *mSurface;
    nsAutoPtr<gfxRect> mOpaqueRect;

private:
    static void SurfaceDestroyFunc(void *data);

    int32_t mFloatingRefs;
    int32_t mBytesRecorded;

protected:
    bool mSurfaceValid;
    bool mAllowUseAsSource;
};




class THEBES_API gfxUnknownSurface : public gfxASurface {
public:
    gfxUnknownSurface(cairo_surface_t *surf) {
        Init(surf, true);
    }

    virtual ~gfxUnknownSurface() { }
};

#ifndef XPCOM_GLUE_AVOID_NSPR












class nsMainThreadSurfaceRef;

template <>
class nsAutoRefTraits<nsMainThreadSurfaceRef> {
public:
  typedef gfxASurface* RawRef;

  


  class SurfaceReleaser : public nsRunnable {
  public:
    SurfaceReleaser(RawRef aRef) : mRef(aRef) {}
    NS_IMETHOD Run() {
      mRef->Release();
      return NS_OK;
    }
    RawRef mRef;
  };

  static RawRef Void() { return nullptr; }
  static void Release(RawRef aRawRef)
  {
    if (NS_IsMainThread()) {
      aRawRef->Release();
      return;
    }
    nsCOMPtr<nsIRunnable> runnable = new SurfaceReleaser(aRawRef);
    NS_DispatchToMainThread(runnable);
  }
  static void AddRef(RawRef aRawRef)
  {
    NS_ASSERTION(NS_IsMainThread(),
                 "Can only add a reference on the main thread");
    aRawRef->AddRef();
  }
};

#endif
#endif 
