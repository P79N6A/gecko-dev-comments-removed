




































#ifndef GFX_PSSURFACE_H
#define GFX_PSSURFACE_H

#include "gfxASurface.h"


#include "nsCOMPtr.h"
#include "nsIOutputStream.h"

class THEBES_API gfxPSSurface : public gfxASurface {
public:
    gfxPSSurface(nsIOutputStream *aStream, const gfxSize& aSizeInPonits);
    virtual ~gfxPSSurface();

    virtual nsresult BeginPrinting(const nsAString& aTitle, const nsAString& aPrintToFileName);
    virtual nsresult EndPrinting();
    virtual nsresult AbortPrinting();
    virtual nsresult BeginPage();
    virtual nsresult EndPage();
    virtual void Finish();

    void SetDPI(double x, double y);
    void GetDPI(double *xDPI, double *yDPI);

    
    const gfxSize& GetSize() const { return mSize; }

private:
    nsCOMPtr<nsIOutputStream> mStream;
    double mXDPI;
    double mYDPI;
    gfxSize mSize;
};

#endif 
