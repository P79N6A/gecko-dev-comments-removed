




































#include "gfxImageSurface.h"
#include "gfxUtils.h"

static PRUint8 sUnpremultiplyTable[256*256];
static PRUint8 sPremultiplyTable[256*256];
static PRBool sTablesInitialized = PR_FALSE;

static const PRUint8 PremultiplyValue(PRUint8 a, PRUint8 v) {
    return sPremultiplyTable[a*256+v];
}

static const PRUint8 UnpremultiplyValue(PRUint8 a, PRUint8 v) {
    return sUnpremultiplyTable[a*256+v];
}

static void
CalculateTables()
{
    
    
    
    
    

    

    
    for (PRUint32 c = 0; c <= 255; c++) {
        sUnpremultiplyTable[c] = c;
    }

    for (int a = 1; a <= 255; a++) {
        for (int c = 0; c <= 255; c++) {
            sUnpremultiplyTable[a*256+c] = (PRUint8)((c * 255) / a);
        }
    }

    

    for (int a = 0; a <= 255; a++) {
        for (int c = 0; c <= 255; c++) {
            sPremultiplyTable[a*256+c] = (a * c + 254) / 255;
        }
    }

    sTablesInitialized = PR_TRUE;
}

void
gfxUtils::PremultiplyImageSurface(gfxImageSurface *aSourceSurface,
                                  gfxImageSurface *aDestSurface)
{
    if (!aDestSurface)
        aDestSurface = aSourceSurface;

    NS_ASSERTION(aSourceSurface->Format() == aDestSurface->Format() &&
                 aSourceSurface->Width() == aDestSurface->Width() &&
                 aSourceSurface->Height() == aDestSurface->Height() &&
                 aSourceSurface->Stride() == aDestSurface->Stride(),
                 "Source and destination surfaces don't have identical characteristics");

    NS_ASSERTION(aSourceSurface->Stride() == aSourceSurface->Width() * 4,
                 "Source surface stride isn't tightly packed");

    
    if (aSourceSurface->Format() != gfxASurface::ImageFormatARGB32) {
        if (aDestSurface != aSourceSurface) {
            memcpy(aDestSurface->Data(), aSourceSurface->Data(),
                   aSourceSurface->Stride() * aSourceSurface->Height());
        }
        return;
    }

    if (!sTablesInitialized)
        CalculateTables();

    PRUint8 *src = aSourceSurface->Data();
    PRUint8 *dst = aDestSurface->Data();

    PRUint32 dim = aSourceSurface->Width() * aSourceSurface->Height();
    for (PRUint32 i = 0; i < dim; ++i) {
#ifdef IS_LITTLE_ENDIAN
        PRUint8 b = *src++;
        PRUint8 g = *src++;
        PRUint8 r = *src++;
        PRUint8 a = *src++;

        *dst++ = PremultiplyValue(a, b);
        *dst++ = PremultiplyValue(a, g);
        *dst++ = PremultiplyValue(a, r);
        *dst++ = a;
#else
        PRUint8 a = *src++;
        PRUint8 r = *src++;
        PRUint8 g = *src++;
        PRUint8 b = *src++;

        *dst++ = a;
        *dst++ = PremultiplyValue(a, r);
        *dst++ = PremultiplyValue(a, g);
        *dst++ = PremultiplyValue(a, b);
#endif
    }
}

void
gfxUtils::UnpremultiplyImageSurface(gfxImageSurface *aSourceSurface,
                                    gfxImageSurface *aDestSurface)
{
    if (!aDestSurface)
        aDestSurface = aSourceSurface;

    NS_ASSERTION(aSourceSurface->Format() == aDestSurface->Format() &&
                 aSourceSurface->Width() == aDestSurface->Width() &&
                 aSourceSurface->Height() == aDestSurface->Height() &&
                 aSourceSurface->Stride() == aDestSurface->Stride(),
                 "Source and destination surfaces don't have identical characteristics");

    NS_ASSERTION(aSourceSurface->Stride() == aSourceSurface->Width() * 4,
                 "Source surface stride isn't tightly packed");

    
    if (aSourceSurface->Format() != gfxASurface::ImageFormatARGB32) {
        if (aDestSurface != aSourceSurface) {
            memcpy(aDestSurface->Data(), aSourceSurface->Data(),
                   aSourceSurface->Stride() * aSourceSurface->Height());
        }
        return;
    }

    if (!sTablesInitialized)
        CalculateTables();

    PRUint8 *src = aSourceSurface->Data();
    PRUint8 *dst = aDestSurface->Data();

    PRUint32 dim = aSourceSurface->Width() * aSourceSurface->Height();
    for (PRUint32 i = 0; i < dim; ++i) {
#ifdef IS_LITTLE_ENDIAN
        PRUint8 b = *src++;
        PRUint8 g = *src++;
        PRUint8 r = *src++;
        PRUint8 a = *src++;

        *dst++ = UnpremultiplyValue(a, b);
        *dst++ = UnpremultiplyValue(a, g);
        *dst++ = UnpremultiplyValue(a, r);
        *dst++ = a;
#else
        PRUint8 a = *src++;
        PRUint8 r = *src++;
        PRUint8 g = *src++;
        PRUint8 b = *src++;

        *dst++ = a;
        *dst++ = UnpremultiplyValue(a, r);
        *dst++ = UnpremultiplyValue(a, g);
        *dst++ = UnpremultiplyValue(a, b);
#endif
    }
}
