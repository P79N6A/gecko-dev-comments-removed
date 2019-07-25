




































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

    static PRInt32 ScriptShapingLevel(PRInt32 aScriptCode);
};

#endif 
