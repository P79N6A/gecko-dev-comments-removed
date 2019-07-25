




































#ifndef GFX_PDFSURFACE_H
#define GFX_PDFSURFACE_H

#include "gfxASurface.h"
#include "gfxContext.h"


#include "nsCOMPtr.h"
#include "nsIOutputStream.h"

class THEBES_API gfxPDFSurface : public gfxASurface {
public:
    gfxPDFSurface(nsIOutputStream *aStream, const gfxSize& aSizeInPoints);
    virtual ~gfxPDFSurface();

    virtual nsresult BeginPrinting(const nsAString& aTitle, const nsAString& aPrintToFileName);
    virtual nsresult EndPrinting();
    virtual nsresult AbortPrinting();
    virtual nsresult BeginPage();
    virtual nsresult EndPage();
    virtual void Finish();

    void SetDPI(double x, double y);
    void GetDPI(double *xDPI, double *yDPI);

    
    virtual const gfxIntSize GetSize() const
    {
        return gfxIntSize(mSize.width, mSize.height);
    }

    virtual PRInt32 GetDefaultContextFlags() const
    {
        return gfxContext::FLAG_SIMPLIFY_OPERATORS |
               gfxContext::FLAG_DISABLE_SNAPPING |
               gfxContext::FLAG_DISABLE_COPY_BACKGROUND;
    }

private:
    nsCOMPtr<nsIOutputStream> mStream;
    double mXDPI;
    double mYDPI;
    gfxSize mSize;
};

#endif 
