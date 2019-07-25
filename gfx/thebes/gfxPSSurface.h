




































#ifndef GFX_PSSURFACE_H
#define GFX_PSSURFACE_H

#include "gfxASurface.h"


#include "nsCOMPtr.h"
#include "nsIOutputStream.h"
#include "gfxContext.h"

class THEBES_API gfxPSSurface : public gfxASurface {
public:
    enum PageOrientation {
        PORTRAIT,
        LANDSCAPE
    };

    gfxPSSurface(nsIOutputStream *aStream, const gfxSize& aSizeInPoints, PageOrientation aOrientation);
    virtual ~gfxPSSurface();

    virtual nsresult BeginPrinting(const nsAString& aTitle, const nsAString& aPrintToFileName);
    virtual nsresult EndPrinting();
    virtual nsresult AbortPrinting();
    virtual nsresult BeginPage();
    virtual nsresult EndPage();
    virtual void Finish();

    void SetDPI(double x, double y);
    void GetDPI(double *xDPI, double *yDPI);

    virtual bool GetRotateForLandscape() { return (mOrientation == LANDSCAPE); }

    
    virtual const gfxIntSize GetSize() const
    {
        return mSize;
    }

    virtual PRInt32 GetDefaultContextFlags() const
    {
        return gfxContext::FLAG_SIMPLIFY_OPERATORS |
               gfxContext::FLAG_DISABLE_SNAPPING;
    }

private:
    nsCOMPtr<nsIOutputStream> mStream;
    double mXDPI;
    double mYDPI;
    gfxIntSize mSize;
    PageOrientation mOrientation;
};

#endif
