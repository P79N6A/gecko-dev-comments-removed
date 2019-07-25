




































#include "gfxQuartzPDFSurface.h"

#include "cairo-quartz.h"

gfxQuartzPDFSurface::gfxQuartzPDFSurface(const char *filename, gfxSize aSizeInPoints)
{
    mRect = CGRectMake(0.0, 0.0, aSizeInPoints.width, aSizeInPoints.height);

    CFStringRef file = CFStringCreateWithCString(kCFAllocatorDefault, filename, kCFStringEncodingUTF8);
    CFURLRef fileURL = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, file, kCFURLPOSIXPathStyle, false);
    mCGContext = CGPDFContextCreateWithURL(fileURL, &mRect, NULL);

    CFRelease(file);
    CFRelease(fileURL);

    Init(cairo_quartz_surface_create_for_cg_context(mCGContext, aSizeInPoints.width, aSizeInPoints.height));
}

gfxQuartzPDFSurface::~gfxQuartzPDFSurface()
{
    CGContextRelease(mCGContext);
}


nsresult
gfxQuartzPDFSurface::BeginPrinting(const nsAString& aTitle, const nsAString& aPrintToFileName)
{
    return NS_OK;
}

nsresult
gfxQuartzPDFSurface::EndPrinting()
{
    return NS_OK;
}

nsresult
gfxQuartzPDFSurface::AbortPrinting()
{
    return NS_OK;
}

nsresult
gfxQuartzPDFSurface::BeginPage()
{
    CGContextBeginPage(mCGContext, &mRect);

    return NS_OK;
}

nsresult
gfxQuartzPDFSurface::EndPage()
{
    CGContextEndPage(mCGContext);
    return NS_OK;
}
