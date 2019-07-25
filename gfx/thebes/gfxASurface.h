




































#ifndef GFX_ASURFACE_H
#define GFX_ASURFACE_H

#include "gfxTypes.h"
#include "gfxRect.h"
#include "nsAutoPtr.h"

typedef struct _cairo_surface cairo_surface_t;
typedef struct _cairo_user_data_key cairo_user_data_key_t;

typedef void (*thebes_destroy_func_t) (void *data);

class gfxImageSurface;
struct nsIntPoint;
struct nsIntRect;





class THEBES_API gfxASurface {
public:
    nsrefcnt AddRef(void);
    nsrefcnt Release(void);

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
        CONTENT_COLOR_ALPHA = 0x3000
    } gfxContentType;

    


    static already_AddRefed<gfxASurface> Wrap(cairo_surface_t *csurf);

    
    cairo_surface_t *CairoSurface() {
        NS_ASSERTION(mSurface != nsnull, "gfxASurface::CairoSurface called with mSurface == nsnull!");
        return mSurface;
    }

    gfxSurfaceType GetType() const;

    gfxContentType GetContentType() const;

    void SetDeviceOffset(const gfxPoint& offset);
    gfxPoint GetDeviceOffset() const;

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
      return nsnull;
    }

    int CairoStatus();

    



    static PRBool CheckSurfaceSize(const gfxIntSize& sz, PRInt32 limit = 0);

    


    static PRInt32 FormatStrideForWidth(gfxImageFormat format, PRInt32 width);

    



    virtual PRInt32 GetDefaultContextFlags() const { return 0; }

    static gfxContentType ContentFromFormat(gfxImageFormat format);
    static gfxImageFormat FormatFromContent(gfxContentType format);

    void SetSubpixelAntialiasingEnabled(PRBool aEnabled);
    PRBool GetSubpixelAntialiasingEnabled();

    



    static void RecordMemoryUsedForSurfaceType(gfxASurface::gfxSurfaceType aType,
                                               PRInt32 aBytes);

    





    void RecordMemoryUsed(PRInt32 aBytes);
    void RecordMemoryFreed();

    PRInt32 KnownMemoryUsed() { return mBytesRecorded; }

    static PRInt32 BytePerPixelFromFormat(gfxImageFormat format);

    virtual const gfxIntSize GetSize() const { return gfxIntSize(-1, -1); }

    void DumpAsDataURL();

    void SetOpaqueRect(const gfxRect& aRect) {
        if (aRect.IsEmpty()) {
            mOpaqueRect = nsnull;
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

    


    void SetAllowUseAsSource(PRBool aAllow) { mAllowUseAsSource = aAllow; }
    PRBool GetAllowUseAsSource() { return mAllowUseAsSource; }

protected:
    gfxASurface() : mSurface(nsnull), mFloatingRefs(0), mBytesRecorded(0),
                    mSurfaceValid(PR_FALSE), mAllowUseAsSource(PR_TRUE)
    {
        MOZ_COUNT_CTOR(gfxASurface);
    }

    static gfxASurface* GetSurfaceWrapper(cairo_surface_t *csurf);
    static void SetSurfaceWrapper(cairo_surface_t *csurf, gfxASurface *asurf);

    




    void FastMovePixels(const nsIntRect& aSourceRect,
                        const nsIntPoint& aDestTopLeft);

    
    
    
    void Init(cairo_surface_t *surface, PRBool existingSurface = PR_FALSE);

    virtual ~gfxASurface()
    {
        RecordMemoryFreed();

        MOZ_COUNT_DTOR(gfxASurface);
    }

    cairo_surface_t *mSurface;
    nsAutoPtr<gfxRect> mOpaqueRect;

private:
    static void SurfaceDestroyFunc(void *data);

    PRInt32 mFloatingRefs;
    PRInt32 mBytesRecorded;

protected:
    PRPackedBool mSurfaceValid;
    PRPackedBool mAllowUseAsSource;
};




class THEBES_API gfxUnknownSurface : public gfxASurface {
public:
    gfxUnknownSurface(cairo_surface_t *surf) {
        Init(surf, PR_TRUE);
    }

    virtual ~gfxUnknownSurface() { }
};

#endif 
