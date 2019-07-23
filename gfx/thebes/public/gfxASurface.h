




































#ifndef GFX_ASURFACE_H
#define GFX_ASURFACE_H

#include "gfxTypes.h"
#include "gfxRect.h"
#include "nsStringFwd.h"

typedef struct _cairo_surface cairo_surface_t;
typedef struct _cairo_user_data_key cairo_user_data_key_t;

typedef void (*thebes_destroy_func_t) (void *data);





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
        SurfaceTypeOS2
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

    void Flush();
    void MarkDirty();
    void MarkDirty(const gfxRect& r);

    
    virtual nsresult BeginPrinting(const nsAString& aTitle, const nsAString& aPrintToFileName) { return NS_ERROR_NOT_IMPLEMENTED; }
    virtual nsresult EndPrinting() { return NS_ERROR_NOT_IMPLEMENTED; }
    virtual nsresult AbortPrinting() { return NS_ERROR_NOT_IMPLEMENTED; }
    virtual nsresult BeginPage() { return NS_ERROR_NOT_IMPLEMENTED; }
    virtual nsresult EndPage() { return NS_ERROR_NOT_IMPLEMENTED; }

    void SetData(const cairo_user_data_key_t *key,
                 void *user_data,
                 thebes_destroy_func_t destroy);
    void *GetData(const cairo_user_data_key_t *key);

    virtual void Finish();

    int CairoStatus();

    



    static PRBool CheckSurfaceSize(const gfxIntSize& sz, PRInt32 limit = 0);

protected:
    gfxASurface() : mSurface(nsnull), mFloatingRefs(0), mSurfaceValid(PR_FALSE) { }

    static gfxASurface* GetSurfaceWrapper(cairo_surface_t *csurf);
    static void SetSurfaceWrapper(cairo_surface_t *csurf, gfxASurface *asurf);

    void Init(cairo_surface_t *surface, PRBool existingSurface = PR_FALSE);

    virtual ~gfxASurface() {
    }
private:
    static void SurfaceDestroyFunc(void *data);

    cairo_surface_t *mSurface;
    PRInt32 mFloatingRefs;

protected:
    PRPackedBool mSurfaceValid;
};




class THEBES_API gfxUnknownSurface : public gfxASurface {
public:
    gfxUnknownSurface(cairo_surface_t *surf) {
        Init(surf, PR_TRUE);
    }

    virtual ~gfxUnknownSurface() { }
};

#endif 
