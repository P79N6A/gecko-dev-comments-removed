






#ifndef SkFont_DEFINED
#define SkFont_DEFINED

#include "SkRefCnt.h"
#include "SkScalar.h"

class SkPaint;
class SkTypeface;

enum SkTextEncoding {
    kUTF8_SkTextEncoding,
    kUTF16_SkTextEncoding,
    kUTF32_SkTextEncoding,
    kGlyphID_SkTextEncoding,
};


















































class SkFont : public SkRefCnt {
public:
    enum Flags {
        




        kEnableAutoHints_Flag       = 1 << 0,

        




        kEnableByteCodeHints_Flag   = 1 << 1,

        








        kUseNonlinearMetrics_Flag   = 1 << 2,

        kVertical_Flag              = 1 << 3,
        kEmbeddedBitmaps_Flag       = 1 << 4,
        kGenA8FromLCD_Flag          = 1 << 5,
        kEmbolden_Flag              = 1 << 6,
        kDevKern_Flag               = 1 << 7,   
    };

    enum MaskType {
        kBW_MaskType,
        kA8_MaskType,
        kLCD_MaskType,
    };

    static SkFont* Create(SkTypeface*, SkScalar size, MaskType, uint32_t flags);
    static SkFont* Create(SkTypeface*, SkScalar size, SkScalar scaleX, SkScalar skewX,
                          MaskType, uint32_t flags);

    



    SkFont* cloneWithSize(SkScalar size) const;

    SkTypeface* getTypeface() const { return fTypeface; }
    SkScalar    getSize() const { return fSize; }
    SkScalar    getScaleX() const { return fScaleX; }
    SkScalar    getSkewX() const { return fSkewX; }
    uint32_t    getFlags() const { return fFlags; }
    MaskType    getMaskType() const { return (MaskType)fMaskType; }

    bool isVertical() const { return SkToBool(fFlags & kVertical_Flag); }
    bool isEmbolden() const { return SkToBool(fFlags & kEmbolden_Flag); }
    bool isEnableAutoHints() const { return SkToBool(fFlags & kEnableAutoHints_Flag); }
    bool isEnableByteCodeHints() const { return SkToBool(fFlags & kEnableByteCodeHints_Flag); }
    bool isUseNonLinearMetrics() const { return SkToBool(fFlags & kUseNonlinearMetrics_Flag); }

    int textToGlyphs(const void* text, size_t byteLength, SkTextEncoding,
                     uint16_t glyphs[], int maxGlyphCount) const;

    SkScalar measureText(const void* text, size_t byteLength, SkTextEncoding) const;

    static SkFont* Testing_CreateFromPaint(const SkPaint&);

private:
    enum {
        kAllFlags = 0xFF,
    };

    SkFont(SkTypeface*, SkScalar size, SkScalar scaleX, SkScalar skewX, MaskType, uint32_t flags);
    virtual ~SkFont();

    SkTypeface* fTypeface;
    SkScalar    fSize;
    SkScalar    fScaleX;
    SkScalar    fSkewX;
    uint16_t    fFlags;
    uint8_t     fMaskType;

};

#endif
