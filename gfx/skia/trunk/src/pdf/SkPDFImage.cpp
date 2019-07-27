






#include "SkPDFImage.h"

#include "SkBitmap.h"
#include "SkColor.h"
#include "SkColorPriv.h"
#include "SkData.h"
#include "SkFlate.h"
#include "SkPDFCatalog.h"
#include "SkRect.h"
#include "SkStream.h"
#include "SkString.h"
#include "SkUnPreMultiply.h"

static const int kNoColorTransform = 0;

static bool skip_compression(SkPDFCatalog* catalog) {
    return SkToBool(catalog->getDocumentFlags() &
                    SkPDFDocument::kFavorSpeedOverSize_Flags);
}

static size_t get_uncompressed_size(const SkBitmap& bitmap,
                                    const SkIRect& srcRect) {
    switch (bitmap.colorType()) {
        case kIndex_8_SkColorType:
            return srcRect.width() * srcRect.height();
        case kARGB_4444_SkColorType:
            return ((srcRect.width() * 3 + 1) / 2) * srcRect.height();
        case kRGB_565_SkColorType:
            return srcRect.width() * 3 * srcRect.height();
        case kRGBA_8888_SkColorType:
        case kBGRA_8888_SkColorType:
            return srcRect.width() * 3 * srcRect.height();
        case kAlpha_8_SkColorType:
            return 1;
        default:
            SkASSERT(false);
            return 0;
    }
}

static SkStream* extract_index8_image(const SkBitmap& bitmap,
                                      const SkIRect& srcRect) {
    const int rowBytes = srcRect.width();
    SkStream* stream = SkNEW_ARGS(SkMemoryStream,
                                  (get_uncompressed_size(bitmap, srcRect)));
    uint8_t* dst = (uint8_t*)stream->getMemoryBase();

    for (int y = srcRect.fTop; y < srcRect.fBottom; y++) {
        memcpy(dst, bitmap.getAddr8(srcRect.fLeft, y), rowBytes);
        dst += rowBytes;
    }
    return stream;
}

static SkStream* extract_argb4444_data(const SkBitmap& bitmap,
                                       const SkIRect& srcRect,
                                       bool extractAlpha,
                                       bool* isOpaque,
                                       bool* isTransparent) {
    SkStream* stream;
    uint8_t* dst = NULL;
    if (extractAlpha) {
        const int alphaRowBytes = (srcRect.width() + 1) / 2;
        stream = SkNEW_ARGS(SkMemoryStream,
                            (alphaRowBytes * srcRect.height()));
    } else {
        stream = SkNEW_ARGS(SkMemoryStream,
                            (get_uncompressed_size(bitmap, srcRect)));
    }
    dst = (uint8_t*)stream->getMemoryBase();

    for (int y = srcRect.fTop; y < srcRect.fBottom; y++) {
        uint16_t* src = bitmap.getAddr16(0, y);
        int x;
        for (x = srcRect.fLeft; x + 1 < srcRect.fRight; x += 2) {
            if (extractAlpha) {
                dst[0] = (SkGetPackedA4444(src[x]) << 4) |
                    SkGetPackedA4444(src[x + 1]);
                *isOpaque &= dst[0] == SK_AlphaOPAQUE;
                *isTransparent &= dst[0] == SK_AlphaTRANSPARENT;
                dst++;
            } else {
                dst[0] = (SkGetPackedR4444(src[x]) << 4) |
                    SkGetPackedG4444(src[x]);
                dst[1] = (SkGetPackedB4444(src[x]) << 4) |
                    SkGetPackedR4444(src[x + 1]);
                dst[2] = (SkGetPackedG4444(src[x + 1]) << 4) |
                    SkGetPackedB4444(src[x + 1]);
                dst += 3;
            }
        }
        if (srcRect.width() & 1) {
            if (extractAlpha) {
                dst[0] = (SkGetPackedA4444(src[x]) << 4);
                *isOpaque &= dst[0] == (SK_AlphaOPAQUE & 0xF0);
                *isTransparent &= dst[0] == (SK_AlphaTRANSPARENT & 0xF0);
                dst++;

            } else {
                dst[0] = (SkGetPackedR4444(src[x]) << 4) |
                    SkGetPackedG4444(src[x]);
                dst[1] = (SkGetPackedB4444(src[x]) << 4);
                dst += 2;
            }
        }
    }
    return stream;
}

static SkStream* extract_rgb565_image(const SkBitmap& bitmap,
                                      const SkIRect& srcRect) {
    SkStream* stream = SkNEW_ARGS(SkMemoryStream,
                                  (get_uncompressed_size(bitmap,
                                                     srcRect)));
    uint8_t* dst = (uint8_t*)stream->getMemoryBase();
    for (int y = srcRect.fTop; y < srcRect.fBottom; y++) {
        uint16_t* src = bitmap.getAddr16(0, y);
        for (int x = srcRect.fLeft; x < srcRect.fRight; x++) {
            dst[0] = SkGetPackedR16(src[x]);
            dst[1] = SkGetPackedG16(src[x]);
            dst[2] = SkGetPackedB16(src[x]);
            dst += 3;
        }
    }
    return stream;
}

static SkStream* extract_argb8888_data(const SkBitmap& bitmap,
                                       const SkIRect& srcRect,
                                       bool extractAlpha,
                                       bool* isOpaque,
                                       bool* isTransparent) {
    SkStream* stream;
    if (extractAlpha) {
        stream = SkNEW_ARGS(SkMemoryStream,
                            (srcRect.width() * srcRect.height()));
    } else {
        stream = SkNEW_ARGS(SkMemoryStream,
                            (get_uncompressed_size(bitmap, srcRect)));
    }
    uint8_t* dst = (uint8_t*)stream->getMemoryBase();

    for (int y = srcRect.fTop; y < srcRect.fBottom; y++) {
        uint32_t* src = bitmap.getAddr32(0, y);
        for (int x = srcRect.fLeft; x < srcRect.fRight; x++) {
            if (extractAlpha) {
                dst[0] = SkGetPackedA32(src[x]);
                *isOpaque &= dst[0] == SK_AlphaOPAQUE;
                *isTransparent &= dst[0] == SK_AlphaTRANSPARENT;
                dst++;
            } else {
                dst[0] = SkGetPackedR32(src[x]);
                dst[1] = SkGetPackedG32(src[x]);
                dst[2] = SkGetPackedB32(src[x]);
                dst += 3;
            }
        }
    }
    return stream;
}

static SkStream* extract_a8_alpha(const SkBitmap& bitmap,
                                  const SkIRect& srcRect,
                                  bool* isOpaque,
                                  bool* isTransparent) {
    const int alphaRowBytes = srcRect.width();
    SkStream* stream = SkNEW_ARGS(SkMemoryStream,
                                  (alphaRowBytes * srcRect.height()));
    uint8_t* alphaDst = (uint8_t*)stream->getMemoryBase();

    for (int y = srcRect.fTop; y < srcRect.fBottom; y++) {
        uint8_t* src = bitmap.getAddr8(0, y);
        for (int x = srcRect.fLeft; x < srcRect.fRight; x++) {
            alphaDst[0] = src[x];
            *isOpaque &= alphaDst[0] == SK_AlphaOPAQUE;
            *isTransparent &= alphaDst[0] == SK_AlphaTRANSPARENT;
            alphaDst++;
        }
    }
    return stream;
}

static SkStream* create_black_image() {
    SkStream* stream = SkNEW_ARGS(SkMemoryStream, (1));
    ((uint8_t*)stream->getMemoryBase())[0] = 0;
    return stream;
}














static SkStream* extract_image_data(const SkBitmap& bitmap,
                                    const SkIRect& srcRect,
                                    bool extractAlpha, bool* isTransparent) {
    SkColorType colorType = bitmap.colorType();
    if (extractAlpha && (kIndex_8_SkColorType == colorType ||
                         kRGB_565_SkColorType == colorType)) {
        if (isTransparent != NULL) {
            *isTransparent = false;
        }
        return NULL;
    }
    bool isOpaque = true;
    bool transparent = extractAlpha;
    SkStream* stream = NULL;

    bitmap.lockPixels();
    switch (colorType) {
        case kIndex_8_SkColorType:
            if (!extractAlpha) {
                stream = extract_index8_image(bitmap, srcRect);
            }
            break;
        case kARGB_4444_SkColorType:
            stream = extract_argb4444_data(bitmap, srcRect, extractAlpha,
                                           &isOpaque, &transparent);
            break;
        case kRGB_565_SkColorType:
            if (!extractAlpha) {
                stream = extract_rgb565_image(bitmap, srcRect);
            }
            break;
        case kN32_SkColorType:
            stream = extract_argb8888_data(bitmap, srcRect, extractAlpha,
                                           &isOpaque, &transparent);
            break;
        case kAlpha_8_SkColorType:
            if (!extractAlpha) {
                stream = create_black_image();
            } else {
                stream = extract_a8_alpha(bitmap, srcRect,
                                          &isOpaque, &transparent);
            }
            break;
        default:
            SkASSERT(false);
    }
    bitmap.unlockPixels();

    if (isTransparent != NULL) {
        *isTransparent = transparent;
    }
    if (extractAlpha && (transparent || isOpaque)) {
        SkSafeUnref(stream);
        return NULL;
    }
    return stream;
}

static SkPDFArray* make_indexed_color_space(SkColorTable* table) {
    SkPDFArray* result = new SkPDFArray();
    result->reserve(4);
    result->appendName("Indexed");
    result->appendName("DeviceRGB");
    result->appendInt(table->count() - 1);

    
    
    SkString index;
    for (int i = 0; i < table->count(); i++) {
        char buf[3];
        SkColor color = SkUnPreMultiply::PMColorToColor((*table)[i]);
        buf[0] = SkGetPackedR32(color);
        buf[1] = SkGetPackedG32(color);
        buf[2] = SkGetPackedB32(color);
        index.append(buf, 3);
    }
    result->append(new SkPDFString(index))->unref();
    return result;
}





static uint32_t remove_alpha_argb8888(uint32_t pmColor) {
    SkColor color = SkUnPreMultiply::PMColorToColor(pmColor);
    return SkPackARGB32NoCheck(SK_AlphaOPAQUE,
                               SkColorGetR(color),
                               SkColorGetG(color),
                               SkColorGetB(color));
}

static uint16_t remove_alpha_argb4444(uint16_t pmColor) {
    return SkPixel32ToPixel4444(
            remove_alpha_argb8888(SkPixel4444ToPixel32(pmColor)));
}

static uint32_t get_argb8888_neighbor_avg_color(const SkBitmap& bitmap,
                                                int xOrig, int yOrig) {
    uint8_t count = 0;
    uint16_t r = 0;
    uint16_t g = 0;
    uint16_t b = 0;

    for (int y = yOrig - 1; y <= yOrig + 1; y++) {
        if (y < 0 || y >= bitmap.height()) {
            continue;
        }
        uint32_t* src = bitmap.getAddr32(0, y);
        for (int x = xOrig - 1; x <= xOrig + 1; x++) {
            if (x < 0 || x >= bitmap.width()) {
                continue;
            }
            if (SkGetPackedA32(src[x]) != SK_AlphaTRANSPARENT) {
                uint32_t color = remove_alpha_argb8888(src[x]);
                r += SkGetPackedR32(color);
                g += SkGetPackedG32(color);
                b += SkGetPackedB32(color);
                count++;
            }
        }
    }

    if (count == 0) {
        return SkPackARGB32NoCheck(SK_AlphaOPAQUE, 0, 0, 0);
    } else {
        return SkPackARGB32NoCheck(SK_AlphaOPAQUE,
                                   r / count, g / count, b / count);
    }
}

static uint16_t get_argb4444_neighbor_avg_color(const SkBitmap& bitmap,
                                                int xOrig, int yOrig) {
    uint8_t count = 0;
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;

    for (int y = yOrig - 1; y <= yOrig + 1; y++) {
        if (y < 0 || y >= bitmap.height()) {
            continue;
        }
        uint16_t* src = bitmap.getAddr16(0, y);
        for (int x = xOrig - 1; x <= xOrig + 1; x++) {
            if (x < 0 || x >= bitmap.width()) {
                continue;
            }
            if ((SkGetPackedA4444(src[x]) & 0x0F) != SK_AlphaTRANSPARENT) {
                uint16_t color = remove_alpha_argb4444(src[x]);
                r += SkGetPackedR4444(color);
                g += SkGetPackedG4444(color);
                b += SkGetPackedB4444(color);
                count++;
            }
        }
    }

    if (count == 0) {
        return SkPackARGB4444(SK_AlphaOPAQUE & 0x0F, 0, 0, 0);
    } else {
        return SkPackARGB4444(SK_AlphaOPAQUE & 0x0F,
                                   r / count, g / count, b / count);
    }
}

static SkBitmap unpremultiply_bitmap(const SkBitmap& bitmap,
                                     const SkIRect& srcRect) {
    SkBitmap outBitmap;
    outBitmap.allocPixels(bitmap.info().makeWH(srcRect.width(), srcRect.height()));
    int dstRow = 0;

    outBitmap.lockPixels();
    bitmap.lockPixels();
    switch (bitmap.colorType()) {
        case kARGB_4444_SkColorType: {
            for (int y = srcRect.fTop; y < srcRect.fBottom; y++) {
                uint16_t* dst = outBitmap.getAddr16(0, dstRow);
                uint16_t* src = bitmap.getAddr16(0, y);
                for (int x = srcRect.fLeft; x < srcRect.fRight; x++) {
                    uint8_t a = SkGetPackedA4444(src[x]);
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    if (a == (SK_AlphaTRANSPARENT & 0x0F)) {
                        *dst = get_argb4444_neighbor_avg_color(bitmap, x, y);
                    } else {
                        *dst = remove_alpha_argb4444(src[x]);
                    }
                    dst++;
                }
                dstRow++;
            }
            break;
        }
        case kN32_SkColorType: {
            for (int y = srcRect.fTop; y < srcRect.fBottom; y++) {
                uint32_t* dst = outBitmap.getAddr32(0, dstRow);
                uint32_t* src = bitmap.getAddr32(0, y);
                for (int x = srcRect.fLeft; x < srcRect.fRight; x++) {
                    uint8_t a = SkGetPackedA32(src[x]);
                    if (a == SK_AlphaTRANSPARENT) {
                        *dst = get_argb8888_neighbor_avg_color(bitmap, x, y);
                    } else {
                        *dst = remove_alpha_argb8888(src[x]);
                    }
                    dst++;
                }
                dstRow++;
            }
            break;
        }
        default:
            SkASSERT(false);
    }
    bitmap.unlockPixels();
    outBitmap.unlockPixels();

    outBitmap.setImmutable();

    return outBitmap;
}


SkPDFImage* SkPDFImage::CreateImage(const SkBitmap& bitmap,
                                    const SkIRect& srcRect,
                                    SkPicture::EncodeBitmap encoder) {
    if (bitmap.colorType() == kUnknown_SkColorType) {
        return NULL;
    }

    bool isTransparent = false;
    SkAutoTUnref<SkStream> alphaData;
    if (!bitmap.isOpaque()) {
        
        
        
        
        alphaData.reset(
                extract_image_data(bitmap, srcRect, true, &isTransparent));
    }
    if (isTransparent) {
        return NULL;
    }

    SkPDFImage* image;
    SkColorType colorType = bitmap.colorType();
    if (alphaData.get() != NULL && (kN32_SkColorType == colorType ||
                                    kARGB_4444_SkColorType == colorType)) {
        SkBitmap unpremulBitmap = unpremultiply_bitmap(bitmap, srcRect);
        image = SkNEW_ARGS(SkPDFImage, (NULL, unpremulBitmap, false,
                           SkIRect::MakeWH(srcRect.width(), srcRect.height()),
                           encoder));
    } else {
        image = SkNEW_ARGS(SkPDFImage, (NULL, bitmap, false, srcRect, encoder));
    }
    if (alphaData.get() != NULL) {
        SkAutoTUnref<SkPDFImage> mask(
                SkNEW_ARGS(SkPDFImage, (alphaData.get(), bitmap,
                                        true, srcRect, NULL)));
        image->addSMask(mask);
    }

    return image;
}

SkPDFImage::~SkPDFImage() {
    fResources.unrefAll();
}

SkPDFImage* SkPDFImage::addSMask(SkPDFImage* mask) {
    fResources.push(mask);
    mask->ref();
    insert("SMask", new SkPDFObjRef(mask))->unref();
    return mask;
}

void SkPDFImage::getResources(const SkTSet<SkPDFObject*>& knownResourceObjects,
                              SkTSet<SkPDFObject*>* newResourceObjects) {
    GetResourcesHelper(&fResources, knownResourceObjects, newResourceObjects);
}

SkPDFImage::SkPDFImage(SkStream* stream,
                       const SkBitmap& bitmap,
                       bool isAlpha,
                       const SkIRect& srcRect,
                       SkPicture::EncodeBitmap encoder)
    : fIsAlpha(isAlpha),
      fSrcRect(srcRect),
      fEncoder(encoder) {

    if (bitmap.isImmutable()) {
        fBitmap = bitmap;
    } else {
        bitmap.deepCopyTo(&fBitmap);
        fBitmap.setImmutable();
    }

    if (stream != NULL) {
        this->setData(stream);
        fStreamValid = true;
    } else {
        fStreamValid = false;
    }

    SkColorType colorType = fBitmap.colorType();

    insertName("Type", "XObject");
    insertName("Subtype", "Image");

    bool alphaOnly = (kAlpha_8_SkColorType == colorType);

    if (!isAlpha && alphaOnly) {
        
        
        SkAutoTUnref<SkPDFInt> one(new SkPDFInt(1));
        insert("Width", one.get());
        insert("Height", one.get());
    } else {
        insertInt("Width", fSrcRect.width());
        insertInt("Height", fSrcRect.height());
    }

    if (isAlpha || alphaOnly) {
        insertName("ColorSpace", "DeviceGray");
    } else if (kIndex_8_SkColorType == colorType) {
        SkAutoLockPixels alp(fBitmap);
        insert("ColorSpace",
               make_indexed_color_space(fBitmap.getColorTable()))->unref();
    } else {
        insertName("ColorSpace", "DeviceRGB");
    }

    int bitsPerComp = 8;
    if (kARGB_4444_SkColorType == colorType) {
        bitsPerComp = 4;
    }
    insertInt("BitsPerComponent", bitsPerComp);

    if (kRGB_565_SkColorType == colorType) {
        SkASSERT(!isAlpha);
        SkAutoTUnref<SkPDFInt> zeroVal(new SkPDFInt(0));
        SkAutoTUnref<SkPDFScalar> scale5Val(
                new SkPDFScalar(8.2258f));  
        SkAutoTUnref<SkPDFScalar> scale6Val(
                new SkPDFScalar(4.0476f));  
        SkAutoTUnref<SkPDFArray> decodeValue(new SkPDFArray());
        decodeValue->reserve(6);
        decodeValue->append(zeroVal.get());
        decodeValue->append(scale5Val.get());
        decodeValue->append(zeroVal.get());
        decodeValue->append(scale6Val.get());
        decodeValue->append(zeroVal.get());
        decodeValue->append(scale5Val.get());
        insert("Decode", decodeValue.get());
    }
}

SkPDFImage::SkPDFImage(SkPDFImage& pdfImage)
    : SkPDFStream(pdfImage),
      fBitmap(pdfImage.fBitmap),
      fIsAlpha(pdfImage.fIsAlpha),
      fSrcRect(pdfImage.fSrcRect),
      fEncoder(pdfImage.fEncoder),
      fStreamValid(pdfImage.fStreamValid) {
    
    
    
}

bool SkPDFImage::populate(SkPDFCatalog* catalog) {
    if (getState() == kUnused_State) {
        
        SkDynamicMemoryWStream dctCompressedWStream;
        if (!skip_compression(catalog) && fEncoder &&
                get_uncompressed_size(fBitmap, fSrcRect) > 1) {
            SkBitmap subset;
            
            if (!fBitmap.extractSubset(&subset, fSrcRect)) {
                return false;
            }
            size_t pixelRefOffset = 0;
            SkAutoTUnref<SkData> data(fEncoder(&pixelRefOffset, subset));
            if (data.get() && data->size() < get_uncompressed_size(fBitmap,
                                                                   fSrcRect)) {
                this->setData(data.get());

                insertName("Filter", "DCTDecode");
                insertInt("ColorTransform", kNoColorTransform);
                insertInt("Length", this->dataSize());
                setState(kCompressed_State);
                return true;
            }
        }
        
        if (!fStreamValid) {
            SkAutoTUnref<SkStream> stream(
                    extract_image_data(fBitmap, fSrcRect, fIsAlpha, NULL));
            this->setData(stream);
            fStreamValid = true;
        }
        return INHERITED::populate(catalog);
    } else if (getState() == kNoCompression_State &&
            !skip_compression(catalog) &&
            (SkFlate::HaveFlate() || fEncoder)) {
        
        
        if (!getSubstitute()) {
            SkPDFStream* substitute = SkNEW_ARGS(SkPDFImage, (*this));
            setSubstitute(substitute);
            catalog->setSubstitute(this, substitute);
        }
        return false;
    }
    return true;
}
