




































#ifndef GFX_QUARTZPDFSURFACE_H
#define GFX_QUARTZPDFSURFACE_H

#include "gfxASurface.h"

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

    gfxSize GetSize() {
        gfxSize size(mRect.size.width, mRect.size.height);
        return size;
    }

    CGContextRef GetCGContext() { return mCGContext; }

protected:
    CGContextRef mCGContext;
    CGRect mRect;
};
#endif 
