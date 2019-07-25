




































#ifndef GFX_WINDOWSSURFACE_H
#define GFX_WINDOWSSURFACE_H

#include "gfxASurface.h"
#include "gfxImageSurface.h"

#include <windows.h>

class gfxContext;

class THEBES_API gfxWindowsSurface : public gfxASurface {
public:
    enum {
        FLAG_TAKE_DC = (1 << 0),
        FLAG_FOR_PRINTING = (1 << 1),
        FLAG_IS_TRANSPARENT = (1 << 2)
    };

    gfxWindowsSurface(HWND wnd, PRUint32 flags = 0);
    gfxWindowsSurface(HDC dc, PRUint32 flags = 0);

    
    gfxWindowsSurface(const gfxIntSize& size,
                      gfxImageFormat imageFormat = ImageFormatRGB24);

    
    gfxWindowsSurface(HDC dc,
                      const gfxIntSize& size,
                      gfxImageFormat imageFormat = ImageFormatRGB24);

    gfxWindowsSurface(cairo_surface_t *csurf);

    virtual already_AddRefed<gfxASurface> CreateSimilarSurface(gfxContentType aType,
                                                               const gfxIntSize& aSize);

    void InitWithDC(PRUint32 flags);

    virtual ~gfxWindowsSurface();

    HDC GetDC() { return mDC; }

    HDC GetDCWithClip(gfxContext *);

    already_AddRefed<gfxImageSurface> GetAsImageSurface();

    already_AddRefed<gfxWindowsSurface> OptimizeToDDB(HDC dc,
                                                      const gfxIntSize& size,
                                                      gfxImageFormat format);

    nsresult BeginPrinting(const nsAString& aTitle, const nsAString& aPrintToFileName);
    nsresult EndPrinting();
    nsresult AbortPrinting();
    nsresult BeginPage();
    nsresult EndPage();

    virtual PRInt32 GetDefaultContextFlags() const;

    void MovePixels(const nsIntRect& aSourceRect,
                    const nsIntPoint& aDestTopLeft)
    {
        FastMovePixels(aSourceRect, aDestTopLeft);
    }

private:
    PRPackedBool mOwnsDC;
    PRPackedBool mForPrinting;

    HDC mDC;
    HWND mWnd;
};

#ifdef WINCE


#define ETO_GLYPH_INDEX 0
#define ETO_PDY 0
#define HALFTONE COLORONCOLOR
#define GM_ADVANCED 2
#define MWT_IDENTITY 1

inline int SetGraphicsMode(HDC hdc, int iMode) {return 1;}
inline int GetGraphicsMode(HDC hdc)            {return 1;} 
inline void GdiFlush()                         {}
inline BOOL SetWorldTransform(HDC hdc, CONST XFORM *lpXform) { return FALSE; }
inline BOOL GetWorldTransform(HDC hdc, LPXFORM lpXform )     { return FALSE; }
inline BOOL ModifyWorldTransform(HDC hdc, CONST XFORM * lpxf, DWORD mode) { return 1; }

#endif

#endif
