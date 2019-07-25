














































































#ifndef GFX_SCRIPTITEMIZER_H
#define GFX_SCRIPTITEMIZER_H

#include "prtypes.h"
#include "harfbuzz/hb.h"

#define PAREN_STACK_DEPTH 32

class gfxScriptItemizer
{
public:
    gfxScriptItemizer(const PRUnichar *src, PRUint32 length);

    void SetText(const PRUnichar *src, PRUint32 length);

    bool Next(PRUint32& aRunStart, PRUint32& aRunLimit,
                PRInt32& aRunScript);

protected:
    void reset() {
        scriptStart = 0;
        scriptLimit = 0;
        scriptCode  = PRInt32(HB_SCRIPT_INVALID_CODE);
        parenSP     = -1;
        pushCount   =  0;
        fixupCount  =  0;
    }

    void push(PRInt32 pairIndex, PRInt32 scriptCode);
    void pop();
    void fixup(PRInt32 scriptCode);

    struct ParenStackEntry {
        PRInt32 pairIndex;
        PRInt32 scriptCode;
    };

    const PRUnichar *textPtr;
    PRUint32 textLength;

    PRUint32 scriptStart;
    PRUint32 scriptLimit;
    PRInt32  scriptCode;

    struct ParenStackEntry parenStack[PAREN_STACK_DEPTH];
    PRUint32 parenSP;
    PRUint32 pushCount;
    PRUint32 fixupCount;
};

#endif 
