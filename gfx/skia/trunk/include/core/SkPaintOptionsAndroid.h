








#ifndef SkPaintOptionsAndroid_DEFINED
#define SkPaintOptionsAndroid_DEFINED

#include "SkTypes.h"
#include "SkString.h"

class SkReadBuffer;
class SkWriteBuffer;







class SkLanguage {
public:
    SkLanguage() { }
    SkLanguage(const SkString& tag) : fTag(tag) { }
    SkLanguage(const char* tag) : fTag(tag) { }
    SkLanguage(const char* tag, size_t len) : fTag(tag, len) { }
    SkLanguage(const SkLanguage& b) : fTag(b.fTag) { }

    


    const SkString& getTag() const { return fTag; }

    


    SkLanguage getParent() const;

    bool operator==(const SkLanguage& b) const {
        return fTag == b.fTag;
    }
    bool operator!=(const SkLanguage& b) const {
        return fTag != b.fTag;
    }
    SkLanguage& operator=(const SkLanguage& b) {
        fTag = b.fTag;
        return *this;
    }

private:
    
    SkString fTag;
};

class SkPaintOptionsAndroid {
public:
    SkPaintOptionsAndroid() {
        fFontVariant = kDefault_Variant;
        fUseFontFallbacks = false;
    }

    SkPaintOptionsAndroid& operator=(const SkPaintOptionsAndroid& b) {
        fLanguage = b.fLanguage;
        fFontVariant = b.fFontVariant;
        fUseFontFallbacks = b.fUseFontFallbacks;
        return *this;
    }

    bool operator==(const SkPaintOptionsAndroid& b) const {
        return !(*this != b);
    }

    bool operator!=(const SkPaintOptionsAndroid& b) const {
        return fLanguage != b.fLanguage ||
               fFontVariant != b.fFontVariant ||
               fUseFontFallbacks != b.fUseFontFallbacks;
    }

    void flatten(SkWriteBuffer&) const;
    void unflatten(SkReadBuffer&);

    


    const SkLanguage& getLanguage() const { return fLanguage; }

    


    void setLanguage(const SkLanguage& language) { fLanguage = language; }
    void setLanguage(const char* languageTag) { fLanguage = SkLanguage(languageTag); }


    enum FontVariant {
       kDefault_Variant = 0x01,
       kCompact_Variant = 0x02,
       kElegant_Variant = 0x04,
       kLast_Variant = kElegant_Variant,
    };

    


    FontVariant getFontVariant() const { return fFontVariant; }

    


    void setFontVariant(FontVariant fontVariant) {
        SkASSERT((unsigned)fontVariant <= kLast_Variant);
        fFontVariant = fontVariant;
    }

    bool isUsingFontFallbacks() const { return fUseFontFallbacks; }

    void setUseFontFallbacks(bool useFontFallbacks) {
        fUseFontFallbacks = useFontFallbacks;
    }

private:
    SkLanguage fLanguage;
    FontVariant fFontVariant;
    bool fUseFontFallbacks;
};

#endif 
