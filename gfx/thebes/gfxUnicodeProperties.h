




































#ifndef GFX_UNICODEPROPERTIES_H
#define GFX_UNICODEPROPERTIES_H

#include "prtypes.h"
#include "gfxTypes.h"

class THEBES_API gfxUnicodeProperties
{
public:
    static PRUint32 GetMirroredChar(PRUint32 aCh);

    static PRUint8 GetCombiningClass(PRUint32 aCh);

    static PRUint8 GetGeneralCategory(PRUint32 aCh);

    static PRUint8 GetEastAsianWidth(PRUint32 aCh);

    static PRInt32 GetScriptCode(PRUint32 aCh);

    enum HSType {
        HST_NONE = 0x00,
        HST_L    = 0x01,
        HST_V    = 0x02,
        HST_T    = 0x04,
        HST_LV   = 0x03,
        HST_LVT  = 0x07
    };

    static HSType GetHangulSyllableType(PRUint32 aCh);

    enum ShapingType {
        SHAPING_DEFAULT   = 0x0001,
        SHAPING_ARABIC    = 0x0002,
        SHAPING_HEBREW    = 0x0004,
        SHAPING_HANGUL    = 0x0008,
        SHAPING_MONGOLIAN = 0x0010,
        SHAPING_INDIC     = 0x0020
    };

    static PRInt32 ScriptShapingType(PRInt32 aScriptCode);
};

#endif 
