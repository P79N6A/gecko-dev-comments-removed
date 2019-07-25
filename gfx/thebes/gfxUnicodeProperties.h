




































#ifndef GFX_UNICODEPROPERTIES_H
#define GFX_UNICODEPROPERTIES_H

#include "prtypes.h"

class gfxUnicodeProperties
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

    static PRInt32 ScriptShapingLevel(PRInt32 aScriptCode);
};

#endif 
