
















































#ifndef GFX_SCRIPTITEMIZER_H
#define GFX_SCRIPTITEMIZER_H

#include "mozilla/StandardInteger.h"
#include "prtypes.h"
#include "harfbuzz/hb.h"
#include "nsUnicodeScriptCodes.h"

#define PAREN_STACK_DEPTH 32

class gfxScriptItemizer
{
public:
    gfxScriptItemizer(const PRUnichar *src, uint32_t length);

    void SetText(const PRUnichar *src, uint32_t length);

    bool Next(uint32_t& aRunStart, uint32_t& aRunLimit,
              int32_t& aRunScript);

protected:
    void reset() {
        scriptStart = 0;
        scriptLimit = 0;
        scriptCode  = MOZ_SCRIPT_INVALID;
        parenSP     = -1;
        pushCount   =  0;
        fixupCount  =  0;
    }

    void push(uint32_t endPairChar, int32_t scriptCode);
    void pop();
    void fixup(int32_t scriptCode);

    struct ParenStackEntry {
        uint32_t endPairChar;
        int32_t  scriptCode;
    };

    const PRUnichar *textPtr;
    uint32_t textLength;

    uint32_t scriptStart;
    uint32_t scriptLimit;
    int32_t  scriptCode;

    struct ParenStackEntry parenStack[PAREN_STACK_DEPTH];
    uint32_t parenSP;
    uint32_t pushCount;
    uint32_t fixupCount;
};

#endif 
