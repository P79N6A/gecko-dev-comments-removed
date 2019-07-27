







#include "SkBitmap.h"
#include "SkCanvas.h"
#include "SkColor.h"
#include "SkColorPriv.h"
#include "SkFDot6.h"
#include "SkFontHost_FreeType_common.h"
#include "SkPath.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_BITMAP_H
#include FT_IMAGE_H
#include FT_OUTLINE_H

#include FT_SYNTHESIS_H




#ifndef FT_LOAD_COLOR
#    define FT_LOAD_COLOR ( 1L << 20 )
#    define FT_PIXEL_MODE_BGRA 7
#endif



static FT_Pixel_Mode compute_pixel_mode(SkMask::Format format) {
    switch (format) {
        case SkMask::kBW_Format:
            return FT_PIXEL_MODE_MONO;
        case SkMask::kA8_Format:
        default:
            return FT_PIXEL_MODE_GRAY;
    }
}



static uint16_t packTriple(U8CPU r, U8CPU g, U8CPU b) {
#ifdef SK_SHOW_TEXT_BLIT_COVERAGE
    r = SkTMax(r, (U8CPU)0x40);
    g = SkTMax(g, (U8CPU)0x40);
    b = SkTMax(b, (U8CPU)0x40);
#endif
    return SkPack888ToRGB16(r, g, b);
}

static uint16_t grayToRGB16(U8CPU gray) {
#ifdef SK_SHOW_TEXT_BLIT_COVERAGE
    gray = SkTMax(gray, (U8CPU)0x40);
#endif
    return SkPack888ToRGB16(gray, gray, gray);
}

static int bittst(const uint8_t data[], int bitOffset) {
    SkASSERT(bitOffset >= 0);
    int lowBit = data[bitOffset >> 3] >> (~bitOffset & 7);
    return lowBit & 1;
}









template<bool APPLY_PREBLEND>
static void copyFT2LCD16(const FT_Bitmap& bitmap, const SkMask& mask, int lcdIsBGR,
                         const uint8_t* tableR, const uint8_t* tableG, const uint8_t* tableB)
{
    SkASSERT(SkMask::kLCD16_Format == mask.fFormat);
    if (FT_PIXEL_MODE_LCD != bitmap.pixel_mode) {
        SkASSERT(mask.fBounds.width() == bitmap.width);
    }
    if (FT_PIXEL_MODE_LCD_V != bitmap.pixel_mode) {
        SkASSERT(mask.fBounds.height() == bitmap.rows);
    }

    const uint8_t* src = bitmap.buffer;
    uint16_t* dst = reinterpret_cast<uint16_t*>(mask.fImage);
    const size_t dstRB = mask.fRowBytes;

    const int width = mask.fBounds.width();
    const int height = mask.fBounds.height();

    switch (bitmap.pixel_mode) {
        case FT_PIXEL_MODE_MONO:
            for (int y = height; y --> 0;) {
                for (int x = 0; x < width; ++x) {
                    dst[x] = -bittst(src, x);
                }
                dst = (uint16_t*)((char*)dst + dstRB);
                src += bitmap.pitch;
            }
            break;
        case FT_PIXEL_MODE_GRAY:
            for (int y = height; y --> 0;) {
                for (int x = 0; x < width; ++x) {
                    dst[x] = grayToRGB16(src[x]);
                }
                dst = (uint16_t*)((char*)dst + dstRB);
                src += bitmap.pitch;
            }
            break;
        case FT_PIXEL_MODE_LCD:
            SkASSERT(3 * mask.fBounds.width() == bitmap.width);
            for (int y = height; y --> 0;) {
                const uint8_t* triple = src;
                if (lcdIsBGR) {
                    for (int x = 0; x < width; x++) {
                        dst[x] = packTriple(sk_apply_lut_if<APPLY_PREBLEND>(triple[2], tableR),
                                            sk_apply_lut_if<APPLY_PREBLEND>(triple[1], tableG),
                                            sk_apply_lut_if<APPLY_PREBLEND>(triple[0], tableB));
                        triple += 3;
                    }
                } else {
                    for (int x = 0; x < width; x++) {
                        dst[x] = packTriple(sk_apply_lut_if<APPLY_PREBLEND>(triple[0], tableR),
                                            sk_apply_lut_if<APPLY_PREBLEND>(triple[1], tableG),
                                            sk_apply_lut_if<APPLY_PREBLEND>(triple[2], tableB));
                        triple += 3;
                    }
                }
                src += bitmap.pitch;
                dst = (uint16_t*)((char*)dst + dstRB);
            }
            break;
        case FT_PIXEL_MODE_LCD_V:
            SkASSERT(3 * mask.fBounds.height() == bitmap.rows);
            for (int y = height; y --> 0;) {
                const uint8_t* srcR = src;
                const uint8_t* srcG = srcR + bitmap.pitch;
                const uint8_t* srcB = srcG + bitmap.pitch;
                if (lcdIsBGR) {
                    SkTSwap(srcR, srcB);
                }
                for (int x = 0; x < width; x++) {
                    dst[x] = packTriple(sk_apply_lut_if<APPLY_PREBLEND>(*srcR++, tableR),
                                        sk_apply_lut_if<APPLY_PREBLEND>(*srcG++, tableG),
                                        sk_apply_lut_if<APPLY_PREBLEND>(*srcB++, tableB));
                }
                src += 3 * bitmap.pitch;
                dst = (uint16_t*)((char*)dst + dstRB);
            }
            break;
        default:
            SkDEBUGF(("FT_Pixel_Mode %d", bitmap.pixel_mode));
            SkDEBUGFAIL("unsupported FT_Pixel_Mode for LCD16");
            break;
    }
}

















static void copyFTBitmap(const FT_Bitmap& srcFTBitmap, SkMask& dstMask) {
    SkASSERT(dstMask.fBounds.width() == srcFTBitmap.width);
    SkASSERT(dstMask.fBounds.height() == srcFTBitmap.rows);

    const uint8_t* src = reinterpret_cast<const uint8_t*>(srcFTBitmap.buffer);
    const FT_Pixel_Mode srcFormat = static_cast<FT_Pixel_Mode>(srcFTBitmap.pixel_mode);
    
    const int srcPitch = srcFTBitmap.pitch;
    const size_t srcRowBytes = SkTAbs(srcPitch);

    uint8_t* dst = dstMask.fImage;
    const SkMask::Format dstFormat = static_cast<SkMask::Format>(dstMask.fFormat);
    const size_t dstRowBytes = dstMask.fRowBytes;

    const size_t width = srcFTBitmap.width;
    const size_t height = srcFTBitmap.rows;

    if (SkMask::kLCD16_Format == dstFormat) {
        copyFT2LCD16<false>(srcFTBitmap, dstMask, false, NULL, NULL, NULL);
        return;
    }

    if ((FT_PIXEL_MODE_MONO == srcFormat && SkMask::kBW_Format == dstFormat) ||
        (FT_PIXEL_MODE_GRAY == srcFormat && SkMask::kA8_Format == dstFormat))
    {
        size_t commonRowBytes = SkTMin(srcRowBytes, dstRowBytes);
        for (size_t y = height; y --> 0;) {
            memcpy(dst, src, commonRowBytes);
            src += srcPitch;
            dst += dstRowBytes;
        }
    } else if (FT_PIXEL_MODE_MONO == srcFormat && SkMask::kA8_Format == dstFormat) {
        for (size_t y = height; y --> 0;) {
            uint8_t byte = 0;
            int bits = 0;
            const uint8_t* src_row = src;
            uint8_t* dst_row = dst;
            for (size_t x = width; x --> 0;) {
                if (0 == bits) {
                    byte = *src_row++;
                    bits = 8;
                }
                *dst_row++ = byte & 0x80 ? 0xff : 0x00;
                bits--;
                byte <<= 1;
            }
            src += srcPitch;
            dst += dstRowBytes;
        }
    } else if (FT_PIXEL_MODE_BGRA == srcFormat && SkMask::kARGB32_Format == dstFormat) {
        
        for (size_t y = height; y --> 0;) {
            const uint8_t* src_row = src;
            SkPMColor* dst_row = reinterpret_cast<SkPMColor*>(dst);
            for (size_t x = 0; x < width; ++x) {
                uint8_t b = *src_row++;
                uint8_t g = *src_row++;
                uint8_t r = *src_row++;
                uint8_t a = *src_row++;
                *dst_row++ = SkPackARGB32(a, r, g, b);
#ifdef SK_SHOW_TEXT_BLIT_COVERAGE
                *(dst_row-1) = SkFourByteInterp256(*(dst_row-1), SK_ColorWHITE, 0x40);
#endif
            }
            src += srcPitch;
            dst += dstRowBytes;
        }
    } else {
        SkDEBUGF(("FT_Pixel_Mode %d, SkMask::Format %d\n", srcFormat, dstFormat));
        SkDEBUGFAIL("unsupported combination of FT_Pixel_Mode and SkMask::Format");
    }
}

static inline int convert_8_to_1(unsigned byte) {
    SkASSERT(byte <= 0xFF);
    
    return (byte >> 6) != 0;
}

static uint8_t pack_8_to_1(const uint8_t alpha[8]) {
    unsigned bits = 0;
    for (int i = 0; i < 8; ++i) {
        bits <<= 1;
        bits |= convert_8_to_1(alpha[i]);
    }
    return SkToU8(bits);
}

static void packA8ToA1(const SkMask& mask, const uint8_t* src, size_t srcRB) {
    const int height = mask.fBounds.height();
    const int width = mask.fBounds.width();
    const int octs = width >> 3;
    const int leftOverBits = width & 7;

    uint8_t* dst = mask.fImage;
    const int dstPad = mask.fRowBytes - SkAlign8(width)/8;
    SkASSERT(dstPad >= 0);

    const int srcPad = srcRB - width;
    SkASSERT(srcPad >= 0);

    for (int y = 0; y < height; ++y) {
        for (int i = 0; i < octs; ++i) {
            *dst++ = pack_8_to_1(src);
            src += 8;
        }
        if (leftOverBits > 0) {
            unsigned bits = 0;
            int shift = 7;
            for (int i = 0; i < leftOverBits; ++i, --shift) {
                bits |= convert_8_to_1(*src++) << shift;
            }
            *dst++ = bits;
        }
        src += srcPad;
        dst += dstPad;
    }
}

inline SkMask::Format SkMaskFormat_for_SkColorType(SkColorType colorType) {
    switch (colorType) {
        case kAlpha_8_SkColorType:
            return SkMask::kA8_Format;
        case kN32_SkColorType:
            return SkMask::kARGB32_Format;
        default:
            SkDEBUGFAIL("unsupported SkBitmap::Config");
            return SkMask::kA8_Format;
    }
}

inline SkColorType SkColorType_for_FTPixelMode(FT_Pixel_Mode pixel_mode) {
    switch (pixel_mode) {
        case FT_PIXEL_MODE_MONO:
        case FT_PIXEL_MODE_GRAY:
            return kAlpha_8_SkColorType;
        case FT_PIXEL_MODE_BGRA:
            return kN32_SkColorType;
        default:
            SkDEBUGFAIL("unsupported FT_PIXEL_MODE");
            return kAlpha_8_SkColorType;
    }
}

inline SkColorType SkColorType_for_SkMaskFormat(SkMask::Format format) {
    switch (format) {
        case SkMask::kBW_Format:
        case SkMask::kA8_Format:
        case SkMask::kLCD16_Format:
            return kAlpha_8_SkColorType;
        case SkMask::kARGB32_Format:
            return kN32_SkColorType;
        default:
            SkDEBUGFAIL("unsupported destination SkBitmap::Config");
            return kAlpha_8_SkColorType;
    }
}

void SkScalerContext_FreeType_Base::generateGlyphImage(FT_Face face, const SkGlyph& glyph) {
    const bool doBGR = SkToBool(fRec.fFlags & SkScalerContext::kLCD_BGROrder_Flag);
    const bool doVert = SkToBool(fRec.fFlags & SkScalerContext::kLCD_Vertical_Flag);

    switch ( face->glyph->format ) {
        case FT_GLYPH_FORMAT_OUTLINE: {
            FT_Outline* outline = &face->glyph->outline;
            FT_BBox     bbox;
            FT_Bitmap   target;

            int dx = 0, dy = 0;
            if (fRec.fFlags & SkScalerContext::kSubpixelPositioning_Flag) {
                dx = SkFixedToFDot6(glyph.getSubXFixed());
                dy = SkFixedToFDot6(glyph.getSubYFixed());
                
                dy = -dy;
            }
            FT_Outline_Get_CBox(outline, &bbox);
            







            FT_Outline_Translate(outline, dx - ((bbox.xMin + dx) & ~63),
                                          dy - ((bbox.yMin + dy) & ~63));

            if (SkMask::kLCD16_Format == glyph.fMaskFormat) {
                FT_Render_Glyph(face->glyph, doVert ? FT_RENDER_MODE_LCD_V : FT_RENDER_MODE_LCD);
                SkMask mask;
                glyph.toMask(&mask);
                if (fPreBlend.isApplicable()) {
                    copyFT2LCD16<true>(face->glyph->bitmap, mask, doBGR,
                                       fPreBlend.fR, fPreBlend.fG, fPreBlend.fB);
                } else {
                    copyFT2LCD16<false>(face->glyph->bitmap, mask, doBGR,
                                        fPreBlend.fR, fPreBlend.fG, fPreBlend.fB);
                }
            } else {
                target.width = glyph.fWidth;
                target.rows = glyph.fHeight;
                target.pitch = glyph.rowBytes();
                target.buffer = reinterpret_cast<uint8_t*>(glyph.fImage);
                target.pixel_mode = compute_pixel_mode( (SkMask::Format)fRec.fMaskFormat);
                target.num_grays = 256;

                memset(glyph.fImage, 0, glyph.rowBytes() * glyph.fHeight);
                FT_Outline_Get_Bitmap(face->glyph->library, outline, &target);
            }
        } break;

        case FT_GLYPH_FORMAT_BITMAP: {
            FT_Pixel_Mode pixel_mode = static_cast<FT_Pixel_Mode>(face->glyph->bitmap.pixel_mode);
            SkMask::Format maskFormat = static_cast<SkMask::Format>(glyph.fMaskFormat);

            
            SkASSERT(FT_PIXEL_MODE_MONO == pixel_mode ||
                     FT_PIXEL_MODE_GRAY == pixel_mode ||
                     FT_PIXEL_MODE_BGRA == pixel_mode);

            
            SkASSERT(SkMask::kBW_Format == maskFormat ||
                     SkMask::kA8_Format == maskFormat ||
                     SkMask::kARGB32_Format == maskFormat ||
                     SkMask::kLCD16_Format == maskFormat);

            if (fRec.fFlags & SkScalerContext::kEmbolden_Flag &&
                !(face->style_flags & FT_STYLE_FLAG_BOLD))
            {
                FT_GlyphSlot_Own_Bitmap(face->glyph);
                FT_Bitmap_Embolden(face->glyph->library, &face->glyph->bitmap,
                                   kBitmapEmboldenStrength, 0);
            }

            
            if (glyph.fWidth == face->glyph->bitmap.width &&
                glyph.fHeight == face->glyph->bitmap.rows &&
                glyph.fTop == -face->glyph->bitmap_top &&
                glyph.fLeft == face->glyph->bitmap_left)
            {
                SkMask dstMask;
                glyph.toMask(&dstMask);
                copyFTBitmap(face->glyph->bitmap, dstMask);
                break;
            }

            

            
            SkBitmap unscaledBitmap;
            unscaledBitmap.allocPixels(SkImageInfo::Make(face->glyph->bitmap.width,
                                                         face->glyph->bitmap.rows,
                                                         SkColorType_for_FTPixelMode(pixel_mode),
                                                         kPremul_SkAlphaType));

            SkMask unscaledBitmapAlias;
            unscaledBitmapAlias.fImage = reinterpret_cast<uint8_t*>(unscaledBitmap.getPixels());
            unscaledBitmapAlias.fBounds.set(0, 0, unscaledBitmap.width(), unscaledBitmap.height());
            unscaledBitmapAlias.fRowBytes = unscaledBitmap.rowBytes();
            unscaledBitmapAlias.fFormat = SkMaskFormat_for_SkColorType(unscaledBitmap.colorType());
            copyFTBitmap(face->glyph->bitmap, unscaledBitmapAlias);

            
            
            
            
            int bitmapRowBytes = 0;
            if (SkMask::kBW_Format != maskFormat && SkMask::kLCD16_Format != maskFormat) {
                bitmapRowBytes = glyph.rowBytes();
            }
            SkBitmap dstBitmap;
            dstBitmap.setInfo(SkImageInfo::Make(glyph.fWidth, glyph.fHeight,
                                                SkColorType_for_SkMaskFormat(maskFormat),
                                                kPremul_SkAlphaType),
                              bitmapRowBytes);
            if (SkMask::kBW_Format == maskFormat || SkMask::kLCD16_Format == maskFormat) {
                dstBitmap.allocPixels();
            } else {
                dstBitmap.setPixels(glyph.fImage);
            }

            
            SkCanvas canvas(dstBitmap);
            canvas.clear(SK_ColorTRANSPARENT);
            canvas.scale(SkIntToScalar(glyph.fWidth) / SkIntToScalar(face->glyph->bitmap.width),
                         SkIntToScalar(glyph.fHeight) / SkIntToScalar(face->glyph->bitmap.rows));
            SkPaint paint;
            paint.setFilterLevel(SkPaint::kLow_FilterLevel);
            canvas.drawBitmap(unscaledBitmap, 0, 0, &paint);

            
            if (SkMask::kBW_Format == maskFormat) {
                
                SkMask dstMask;
                glyph.toMask(&dstMask);
                packA8ToA1(dstMask, dstBitmap.getAddr8(0, 0), dstBitmap.rowBytes());
            } else if (SkMask::kLCD16_Format == maskFormat) {
                
                uint8_t* src = dstBitmap.getAddr8(0, 0);
                uint16_t* dst = reinterpret_cast<uint16_t*>(glyph.fImage);
                for (int y = dstBitmap.height(); y --> 0;) {
                    for (int x = 0; x < dstBitmap.width(); ++x) {
                        dst[x] = grayToRGB16(src[x]);
                    }
                    dst = (uint16_t*)((char*)dst + glyph.rowBytes());
                    src += dstBitmap.rowBytes();
                }
            }

        } break;

        default:
            SkDEBUGFAIL("unknown glyph format");
            memset(glyph.fImage, 0, glyph.rowBytes() * glyph.fHeight);
            return;
    }



#if defined(SK_GAMMA_APPLY_TO_A8)
    if (SkMask::kA8_Format == glyph.fMaskFormat && fPreBlend.isApplicable()) {
        uint8_t* SK_RESTRICT dst = (uint8_t*)glyph.fImage;
        unsigned rowBytes = glyph.rowBytes();

        for (int y = glyph.fHeight - 1; y >= 0; --y) {
            for (int x = glyph.fWidth - 1; x >= 0; --x) {
                dst[x] = fPreBlend.fG[dst[x]];
            }
            dst += rowBytes;
        }
    }
#endif
}



static int move_proc(const FT_Vector* pt, void* ctx) {
    SkPath* path = (SkPath*)ctx;
    path->close();  
    path->moveTo(SkFDot6ToScalar(pt->x), -SkFDot6ToScalar(pt->y));
    return 0;
}

static int line_proc(const FT_Vector* pt, void* ctx) {
    SkPath* path = (SkPath*)ctx;
    path->lineTo(SkFDot6ToScalar(pt->x), -SkFDot6ToScalar(pt->y));
    return 0;
}

static int quad_proc(const FT_Vector* pt0, const FT_Vector* pt1,
                     void* ctx) {
    SkPath* path = (SkPath*)ctx;
    path->quadTo(SkFDot6ToScalar(pt0->x), -SkFDot6ToScalar(pt0->y),
                 SkFDot6ToScalar(pt1->x), -SkFDot6ToScalar(pt1->y));
    return 0;
}

static int cubic_proc(const FT_Vector* pt0, const FT_Vector* pt1,
                      const FT_Vector* pt2, void* ctx) {
    SkPath* path = (SkPath*)ctx;
    path->cubicTo(SkFDot6ToScalar(pt0->x), -SkFDot6ToScalar(pt0->y),
                  SkFDot6ToScalar(pt1->x), -SkFDot6ToScalar(pt1->y),
                  SkFDot6ToScalar(pt2->x), -SkFDot6ToScalar(pt2->y));
    return 0;
}

void SkScalerContext_FreeType_Base::generateGlyphPath(FT_Face face,
                                                      SkPath* path)
{
    FT_Outline_Funcs    funcs;

    funcs.move_to   = move_proc;
    funcs.line_to   = line_proc;
    funcs.conic_to  = quad_proc;
    funcs.cubic_to  = cubic_proc;
    funcs.shift     = 0;
    funcs.delta     = 0;

    FT_Error err = FT_Outline_Decompose(&face->glyph->outline, &funcs, path);

    if (err != 0) {
        path->reset();
        return;
    }

    path->close();
}
