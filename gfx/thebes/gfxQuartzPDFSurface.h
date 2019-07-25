




































#ifndef GFX_QUARTZPDFSURFACE_H
#define GFX_QUARTZPDFSURFACE_H

#include "gfxASurface.h"
#include "gfxContext.h"

#include <Carbon/Carbon.h>

class THEBES_API gfxQuartzPDFSurface : public gfxASurface {
public:
    gfxQuartzPDFSurface(const char *filename, gfxSize aSizeInPoints);
    virtual ~gfxQuartzPDFSurface();

    nsresult BeginPrinting(const nsAString& aTitle, const nsAString& aPrintToFileName);
    nsresult EndPrinting();
    nsresult AbortPrinting();
    nsresult BeginPage();
    nsresult EndPage();

    virtual const gfxIntSize GetSize() const {
        gfxIntSize size(mRect.size.width, mRect.size.height);
        return size;
    }

    CGContextRef GetCGContext() { return mCGContext; }

    virtual PRInt32 GetDefaultContextFlags() const { return gfxContext::FLAG_DISABLE_SNAPPING; }

protected:
    CGContextRef mCGContext;
    CGRect mRect;
};
#endif 
