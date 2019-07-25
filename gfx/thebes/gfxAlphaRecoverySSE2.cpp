



































#include "mozilla/SSE.h"
#include "gfxAlphaRecovery.h"
#include <emmintrin.h>




#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_AMD64))
__declspec(align(16)) static PRUint32 greenMaski[] =
    { 0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00 };
__declspec(align(16)) static PRUint32 alphaMaski[] =
    { 0xff000000, 0xff000000, 0xff000000, 0xff000000 };
#elif defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
static PRUint32 greenMaski[] __attribute__ ((aligned (16))) =
    { 0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00 };
static PRUint32 alphaMaski[] __attribute__ ((aligned (16))) =
    { 0xff000000, 0xff000000, 0xff000000, 0xff000000 };
#elif defined(__SUNPRO_CC) && (defined(__i386) || defined(__x86_64__))
#pragma align 16 (greenMaski, alphaMaski)
static PRUint32 greenMaski[] = { 0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00 };
static PRUint32 alphaMaski[] = { 0xff000000, 0xff000000, 0xff000000, 0xff000000 };
#endif

bool
gfxAlphaRecovery::RecoverAlphaSSE2(gfxImageSurface* blackSurf,
                                   const gfxImageSurface* whiteSurf)
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

    if ((NS_PTR_TO_UINT32(blackData) & 0xf) != (NS_PTR_TO_UINT32(whiteData) & 0xf) ||
        (blackSurf->Stride() - whiteSurf->Stride()) & 0xf) {
        
        return PR_FALSE;
    }

    __m128i greenMask = _mm_load_si128((__m128i*)greenMaski);
    __m128i alphaMask = _mm_load_si128((__m128i*)alphaMaski);

    for (PRInt32 i = 0; i < size.height; ++i) {
        PRInt32 j = 0;
        
        while (NS_PTR_TO_UINT32(blackData) & 0xf && j < size.width) {
            *((PRUint32*)blackData) =
                RecoverPixel(*reinterpret_cast<PRUint32*>(blackData),
                             *reinterpret_cast<PRUint32*>(whiteData));
            blackData += 4;
            whiteData += 4;
            j++;
        }
        
        
        
        for (; j < size.width - 8; j += 8) {
            __m128i black1 = _mm_load_si128((__m128i*)blackData);
            __m128i white1 = _mm_load_si128((__m128i*)whiteData);
            __m128i black2 = _mm_load_si128((__m128i*)(blackData + 16));
            __m128i white2 = _mm_load_si128((__m128i*)(whiteData + 16));

            
            
            white1 = _mm_subs_epu8(white1, black1);
            white2 = _mm_subs_epu8(white2, black2);
            white1 = _mm_subs_epu8(greenMask, white1);
            white2 = _mm_subs_epu8(greenMask, white2);
            
            
            
            
            black1 = _mm_andnot_si128(alphaMask, black1);
            black2 = _mm_andnot_si128(alphaMask, black2);
            white1 = _mm_slli_si128(white1, 2);
            white2 = _mm_slli_si128(white2, 2);
            white1 = _mm_and_si128(alphaMask, white1);
            white2 = _mm_and_si128(alphaMask, white2);
            black1 = _mm_or_si128(white1, black1);
            black2 = _mm_or_si128(white2, black2);

            _mm_store_si128((__m128i*)blackData, black1);
            _mm_store_si128((__m128i*)(blackData + 16), black2);
            blackData += 32;
            whiteData += 32;
        }
        for (; j < size.width - 4; j += 4) {
            __m128i black = _mm_load_si128((__m128i*)blackData);
            __m128i white = _mm_load_si128((__m128i*)whiteData);

            white = _mm_subs_epu8(white, black);
            white = _mm_subs_epu8(greenMask, white);
            black = _mm_andnot_si128(alphaMask, black);
            white = _mm_slli_si128(white, 2);
            white = _mm_and_si128(alphaMask, white);
            black = _mm_or_si128(white, black);
            _mm_store_si128((__m128i*)blackData, black);
            blackData += 16;
            whiteData += 16;
        }
        
        while (j < size.width) {
            *((PRUint32*)blackData) =
                RecoverPixel(*reinterpret_cast<PRUint32*>(blackData),
                             *reinterpret_cast<PRUint32*>(whiteData));
            blackData += 4;
            whiteData += 4;
            j++;
        }
        blackData += blackSurf->Stride() - j * 4;
        whiteData += whiteSurf->Stride() - j * 4;
    }

    blackSurf->MarkDirty();

    return PR_TRUE;
}

static PRInt32
ByteAlignment(PRInt32 aAlignToLog2, PRInt32 aX, PRInt32 aY=0, PRInt32 aStride=1)
{
    return (aX + aStride * aY) & ((1 << aAlignToLog2) - 1);
}

 nsIntRect
gfxAlphaRecovery::AlignRectForSubimageRecovery(const nsIntRect& aRect,
                                               gfxImageSurface* aSurface)
{
    NS_ASSERTION(gfxASurface::ImageFormatARGB32 == aSurface->Format(),
                 "Thebes grew support for non-ARGB32 COLOR_ALPHA?");
    static const PRInt32 kByteAlignLog2 = GoodAlignmentLog2();
    static const PRInt32 bpp = 4;
    static const PRInt32 pixPerAlign = (1 << kByteAlignLog2) / bpp;
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    gfxIntSize surfaceSize = aSurface->GetSize();
    const PRInt32 stride = bpp * surfaceSize.width;
    if (stride != aSurface->Stride()) {
        NS_WARNING("Unexpected stride, falling back on slow alpha recovery");
        return aRect;
    }

    const PRInt32 x = aRect.x, y = aRect.y, w = aRect.width, h = aRect.height;
    const PRInt32 r = x + w;
    const PRInt32 sw = surfaceSize.width;
    const PRInt32 strideAlign = ByteAlignment(kByteAlignLog2, stride);

    
    
    
    
    
    
    
    
    
    PRInt32 dx, dy, dr;
    for (dy = 0; (dy < pixPerAlign) && (y - dy >= 0); ++dy) {
        for (dx = 0; (dx < pixPerAlign) && (x - dx >= 0); ++dx) {
            if (0 != ByteAlignment(kByteAlignLog2,
                                   bpp * (x - dx), y - dy, stride)) {
                continue;
            }
            for (dr = 0; (dr < pixPerAlign) && (r + dr <= sw); ++dr) {
                if (strideAlign == ByteAlignment(kByteAlignLog2,
                                                 bpp * (w + dr + dx))) {
                    goto FOUND_SOLUTION;
                }
            }
        }
    }

    
    return aRect;

FOUND_SOLUTION:
    nsIntRect solution = nsIntRect(x - dx, y - dy, w + dr + dx, h + dy);
    NS_ABORT_IF_FALSE(nsIntRect(0, 0, sw, surfaceSize.height).Contains(solution),
                      "'Solution' extends outside surface bounds!");
    return solution;
}
