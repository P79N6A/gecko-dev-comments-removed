




































#include "gfxAlphaRecovery.h"

#include "gfxImageSurface.h"





















#define SET_ALPHA(v, a) (((v) & ~(0xFF << 24)) | ((a) << 24))
#define GREEN_OF(v) (((v) >> 8) & 0xFF)

 PRBool
gfxAlphaRecovery::RecoverAlpha(gfxImageSurface* blackSurf,
                               const gfxImageSurface* whiteSurf,
                               Analysis* analysis)
{
    gfxIntSize size = blackSurf->GetSize();

    if (size != whiteSurf->GetSize() ||
        (blackSurf->Format() != gfxASurface::ImageFormatARGB32 &&
         blackSurf->Format() != gfxASurface::ImageFormatRGB24) ||
        (whiteSurf->Format() != gfxASurface::ImageFormatARGB32 &&
         whiteSurf->Format() != gfxASurface::ImageFormatRGB24))
        return PR_FALSE;

    if (size.width == 0 || size.height == 0) {
        if (analysis) {
            analysis->uniformAlpha = PR_TRUE;
            analysis->uniformColor = PR_TRUE;
            
            analysis->alpha = 1.0;
            analysis->r = analysis->g = analysis->b = 0.0;
        }
        return PR_TRUE;
    }
  
    unsigned char* blackData = blackSurf->Data();
    unsigned char* whiteData = whiteSurf->Data();
    if (!blackData || !whiteData)
        return PR_FALSE;

    blackSurf->Flush();
    whiteSurf->Flush();

    PRUint32 black = *reinterpret_cast<PRUint32*>(blackData);
    PRUint32 white = *reinterpret_cast<PRUint32*>(whiteData);
    unsigned char first_alpha =
        255 - (GREEN_OF(white) - GREEN_OF(black));
    
    PRUint32 first = SET_ALPHA(black, first_alpha);

    PRUint32 deltas = 0;
    for (PRInt32 i = 0; i < size.height; ++i) {
        PRUint32* blackPixel = reinterpret_cast<PRUint32*>(blackData);
        const PRUint32* whitePixel = reinterpret_cast<PRUint32*>(whiteData);
        for (PRInt32 j = 0; j < size.width; ++j) {
            black = blackPixel[j];
            white = whitePixel[j];
            unsigned char pixel_alpha =
                255 - (GREEN_OF(white) - GREEN_OF(black));
        
            black = SET_ALPHA(black, pixel_alpha);
            blackPixel[j] = black;
            deltas |= (first ^ black);
        }
        blackData += blackSurf->Stride();
        whiteData += whiteSurf->Stride();
    }

    blackSurf->MarkDirty();
    
    if (analysis) {
        analysis->uniformAlpha = (deltas >> 24) == 0;
        analysis->uniformColor = PR_FALSE;
        if (analysis->uniformAlpha) {
            analysis->alpha = first_alpha/255.0;
            



            analysis->uniformColor = (deltas & ~(0xFF << 24)) == 0;
            if (analysis->uniformColor) {
                if (first_alpha == 0) {
                    
                    analysis->r = analysis->g = analysis->b = 0.0;
                } else {
                    double d_first_alpha = first_alpha;
                    analysis->r = (first & 0xFF)/d_first_alpha;
                    analysis->g = ((first >> 8) & 0xFF)/d_first_alpha;
                    analysis->b = ((first >> 16) & 0xFF)/d_first_alpha;
                }
            }
        }
    }

    return PR_TRUE;
}
