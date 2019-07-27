




#ifndef GFX_ASURFACE_H
#define GFX_ASURFACE_H

#include "mozilla/MemoryReporting.h"
#include "mozilla/UniquePtr.h"

#include "gfxTypes.h"
#include "nscore.h"
#include "nsSize.h"
#include "mozilla/gfx/Rect.h"

#ifdef MOZILLA_INTERNAL_API
#include "nsStringFwd.h"
#else
#include "nsStringAPI.h"
#endif

class gfxImageSurface;
struct gfxRect;
struct gfxPoint;

template <typename T>
struct already_AddRefed;





class gfxASurface {
public:
#ifdef MOZILLA_INTERNAL_API
    nsrefcnt AddRef(void);
    nsrefcnt Release(void);

    
    virtual nsrefcnt AddRefExternal(void);
    virtual nsrefcnt ReleaseExternal(void);
#else
    virtual nsrefcnt AddRef(void);
    virtual nsrefcnt Release(void);
#endif

public:

    


    static already_AddRefed<gfxASurface> Wrap(cairo_surface_t *csurf, const gfxIntSize& aSize = gfxIntSize(-1, -1));

    
    cairo_surface_t *CairoSurface() {
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
                                                               const nsIntSize& aSize);

    




    virtual already_AddRefed<gfxImageSurface> GetAsImageSurface();

    




    virtual already_AddRefed<gfxImageSurface> GetAsReadableARGB32ImageSurface();

    



    already_AddRefed<gfxImageSurface> CopyToARGB32ImageSurface();

    int CairoStatus();

    



    static bool CheckSurfaceSize(const nsIntSize& sz, int32_t limit = 0);

    


    static int32_t FormatStrideForWidth(gfxImageFormat format, int32_t width);

    static gfxContentType ContentFromFormat(gfxImageFormat format);

    void SetSubpixelAntialiasingEnabled(bool aEnabled);
    bool GetSubpixelAntialiasingEnabled();

    



    static void RecordMemoryUsedForSurfaceType(gfxSurfaceType aType,
                                               int32_t aBytes);

    





    void RecordMemoryUsed(int32_t aBytes);
    void RecordMemoryFreed();

    virtual int32_t KnownMemoryUsed() { return mBytesRecorded; }

    virtual size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;
    virtual size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;
    
    
    
    
    
    
    
    virtual bool SizeOfIsMeasured() const { return false; }

    



    virtual gfxMemoryLocation GetMemoryLocation() const;

    static int32_t BytePerPixelFromFormat(gfxImageFormat format);

    virtual const nsIntSize GetSize() const;

    void SetOpaqueRect(const gfxRect& aRect);

    const gfxRect& GetOpaqueRect() {
        if (!!mOpaqueRect)
            return *mOpaqueRect;
        return GetEmptyOpaqueRect();
    }

    


    void SetAllowUseAsSource(bool aAllow) { mAllowUseAsSource = aAllow; }
    bool GetAllowUseAsSource() { return mAllowUseAsSource; }

    static uint8_t BytesPerPixel(gfxImageFormat aImageFormat);

protected:
    gfxASurface();

    static gfxASurface* GetSurfaceWrapper(cairo_surface_t *csurf);
    static void SetSurfaceWrapper(cairo_surface_t *csurf, gfxASurface *asurf);

    
    
    
    void Init(cairo_surface_t *surface, bool existingSurface = false);

    
    
    static const gfxRect& GetEmptyOpaqueRect();

    virtual ~gfxASurface();

    cairo_surface_t *mSurface;
    mozilla::UniquePtr<gfxRect> mOpaqueRect;

private:
    static void SurfaceDestroyFunc(void *data);

    int32_t mFloatingRefs;
    int32_t mBytesRecorded;

protected:
    bool mSurfaceValid;
    bool mAllowUseAsSource;
};




class gfxUnknownSurface : public gfxASurface {
public:
    gfxUnknownSurface(cairo_surface_t *surf, const gfxIntSize& aSize)
        : mSize(aSize)
    {
        Init(surf, true);
    }

    virtual ~gfxUnknownSurface() { }
    virtual const nsIntSize GetSize() const override { return mSize; }

private:
    nsIntSize mSize;
};

#endif 
