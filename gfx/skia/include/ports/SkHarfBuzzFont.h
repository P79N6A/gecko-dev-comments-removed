






#ifndef SkHarfBuzzFont_DEFINED
#define SkHarfBuzzFont_DEFINED

extern "C" {
#include "harfbuzz-shaper.h"

}

#include "SkTypes.h"

class SkPaint;
class SkTypeface;

class SkHarfBuzzFont {
public:
    

    virtual SkTypeface* getTypeface() const = 0;
    




    virtual void setupPaint(SkPaint*) const = 0;
    
    


    static HB_Error GetFontTableFunc(void* skharfbuzzfont, const HB_Tag tag,
                                     HB_Byte* buffer, HB_UInt* len);

    static const HB_FontClass& GetFontClass();
};

#endif
