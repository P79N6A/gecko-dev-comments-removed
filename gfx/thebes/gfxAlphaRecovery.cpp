




































#include "gfxAlphaRecovery.h"

#include "gfxImageSurface.h"





















static inline PRUint32
RecoverPixel(PRUint32 black, PRUint32 white)
{
    const PRUint32 GREEN_MASK = 0x0000FF00;
    const PRUint32 ALPHA_MASK = 0xFF000000;

    







    PRUint32 diff = (white & GREEN_MASK) - (black & GREEN_MASK);
    

    PRUint32 limit = diff & ALPHA_MASK;
    
    PRUint32 alpha = (ALPHA_MASK - (diff << 16)) | limit;

    return alpha | (black & ~ALPHA_MASK);
}

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

    blackSurf->Flush();
    whiteSurf->Flush();

    unsigned char* blackData = blackSurf->Data();
    unsigned char* whiteData = whiteSurf->Data();

    
    PRUint32 first;
    if (size.width == 0 || size.height == 0) {
        first = 0;
    } else {
        if (!blackData || !whiteData)
            return PR_FALSE;

        first = RecoverPixel(*reinterpret_cast<PRUint32*>(blackData),
                             *reinterpret_cast<PRUint32*>(whiteData));
    }

    PRUint32 deltas = 0;
    for (PRInt32 i = 0; i < size.height; ++i) {
        PRUint32* blackPixel = reinterpret_cast<PRUint32*>(blackData);
        const PRUint32* whitePixel = reinterpret_cast<PRUint32*>(whiteData);
        for (PRInt32 j = 0; j < size.width; ++j) {
            PRUint32 recovered = RecoverPixel(blackPixel[j], whitePixel[j]);
            blackPixel[j] = recovered;
            deltas |= (first ^ recovered);
        }
        blackData += blackSurf->Stride();
        whiteData += whiteSurf->Stride();
    }

    blackSurf->MarkDirty();
    
    if (analysis) {
        analysis->uniformAlpha = (deltas >> 24) == 0;
        analysis->uniformColor = PR_FALSE;
        if (analysis->uniformAlpha) {
            double d_first_alpha = first >> 24;
            analysis->alpha = d_first_alpha/255.0;
            



            analysis->uniformColor = deltas == 0;
            if (analysis->uniformColor) {
                if (d_first_alpha == 0.0) {
                    
                    analysis->r = analysis->g = analysis->b = 0.0;
                } else {
                    analysis->r = (first & 0xFF)/d_first_alpha;
                    analysis->g = ((first >> 8) & 0xFF)/d_first_alpha;
                    analysis->b = ((first >> 16) & 0xFF)/d_first_alpha;
                }
            }
        }
    }

    return PR_TRUE;
}
