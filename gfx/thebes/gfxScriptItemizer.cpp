














































































#include "gfxScriptItemizer.h"
#include "gfxUnicodeProperties.h"
#include "gfxFontUtils.h" 

#include "harfbuzz/hb.h"

#include "nsCharTraits.h"

#define ARRAY_SIZE(array) (sizeof array / sizeof array[0])

#define MOD(sp) ((sp) % PAREN_STACK_DEPTH)
#define LIMIT_INC(sp) (((sp) < PAREN_STACK_DEPTH)? (sp) + 1 : PAREN_STACK_DEPTH)
#define INC(sp,count) (MOD((sp) + (count)))
#define INC1(sp) (INC(sp, 1))
#define DEC(sp,count) (MOD((sp) + PAREN_STACK_DEPTH - (count)))
#define DEC1(sp) (DEC(sp, 1))
#define STACK_IS_EMPTY() (pushCount <= 0)
#define STACK_IS_NOT_EMPTY() (! STACK_IS_EMPTY())
#define TOP() (parenStack[parenSP])
#define SYNC_FIXUP() (fixupCount = 0)


static const PRUint16 pairedChars[] = {
    0x0028, 0x0029, 
    0x003c, 0x003e,
    0x005b, 0x005d,
    0x007b, 0x007d,
    0x00ab, 0x00bb, 
    0x2018, 0x2019, 
    0x201c, 0x201d,
    0x2039, 0x203a,
    0x207d, 0x207e, 
    0x208d, 0x208e,
    0x275b, 0x275c, 
    0x275d, 0x275e,
    0x2768, 0x2769,
    0x276a, 0x276b,
    0x276c, 0x276d,
    0x276e, 0x276f,
    0x2770, 0x2771,
    0x2772, 0x2773,
    0x2774, 0x2775,
    
    0x2e22, 0x2e23, 
    0x2e24, 0x2e25,
    0x2e26, 0x2e27,
    0x2e28, 0x2e29,
    0x3008, 0x3009, 
    0x300a, 0x300b,
    0x300c, 0x300d,
    0x300e, 0x300f,
    0x3010, 0x3011,
    0x3014, 0x3015,
    0x3016, 0x3017,
    0x3018, 0x3019,
    0x301a, 0x301b,
    0xfe59, 0xfe5a, 
    0xfe5b, 0xfe5c,
    0xfe5d, 0xfe5e,
    0xfe64, 0xfe65,
    0xff08, 0xff09, 
    0xff1c, 0xff1e,
    0xff3b, 0xff3d,
    0xff5b, 0xff5d,
    0xff5f, 0xff60,
    0xff62, 0xff63
};

void
gfxScriptItemizer::push(PRInt32 pairIndex, PRInt32 scriptCode)
{
    pushCount  = LIMIT_INC(pushCount);
    fixupCount = LIMIT_INC(fixupCount);

    parenSP = INC1(parenSP);
    parenStack[parenSP].pairIndex  = pairIndex;
    parenStack[parenSP].scriptCode = scriptCode;
}

void
gfxScriptItemizer::pop()
{
    if (STACK_IS_EMPTY()) {
        return;
    }

    if (fixupCount > 0) {
        fixupCount -= 1;
    }

    pushCount -= 1;
    parenSP = DEC1(parenSP);
  
    


    if (STACK_IS_EMPTY()) {
        parenSP = -1;
    }
}

void
gfxScriptItemizer::fixup(PRInt32 scriptCode)
{
    PRInt32 fixupSP = DEC(parenSP, fixupCount);

    while (fixupCount-- > 0) {
        fixupSP = INC1(fixupSP);
        parenStack[fixupSP].scriptCode = scriptCode;
    }
}

static PRInt32
getPairIndex(PRUint32 ch)
{
    PRInt32 pairedCharCount = ARRAY_SIZE(pairedChars);
    PRInt32 pairedCharPower = mozilla::FindHighestBit(pairedCharCount);
    PRInt32 pairedCharExtra = pairedCharCount - pairedCharPower;

    PRInt32 probe = pairedCharPower;
    PRInt32 pairIndex = 0;

    if (ch >= pairedChars[pairedCharExtra]) {
        pairIndex = pairedCharExtra;
    }

    while (probe > 1) {
        probe >>= 1;

        if (ch >= pairedChars[pairIndex + probe]) {
            pairIndex += probe;
        }
    }

    if (pairedChars[pairIndex] != ch) {
        pairIndex = -1;
    }

    return pairIndex;
}

static bool
sameScript(PRInt32 scriptOne, PRInt32 scriptTwo)
{
    return scriptOne <= HB_SCRIPT_INHERITED ||
           scriptTwo <= HB_SCRIPT_INHERITED ||
           scriptOne == scriptTwo;
}

gfxScriptItemizer::gfxScriptItemizer(const PRUnichar *src, PRUint32 length)
    : textPtr(src), textLength(length)
{
    reset();
}

void
gfxScriptItemizer::SetText(const PRUnichar *src, PRUint32 length)
{
    textPtr  = src;
    textLength = length;

    reset();
}

bool
gfxScriptItemizer::Next(PRUint32& aRunStart, PRUint32& aRunLimit,
                        PRInt32& aRunScript)
{
    
    if (scriptLimit >= textLength) {
        return PR_FALSE;
    }

    SYNC_FIXUP();
    scriptCode = HB_SCRIPT_COMMON;

    for (scriptStart = scriptLimit; scriptLimit < textLength; scriptLimit += 1) {
        PRUint32 ch;
        PRInt32 sc;
        PRInt32 pairIndex;
        PRUint32 startOfChar = scriptLimit;

        ch = textPtr[scriptLimit];

        





        if (ch == 0x20) {
            while (STACK_IS_NOT_EMPTY()) {
                pop();
            }
            sc = HB_SCRIPT_COMMON;
            pairIndex = -1;
        } else {
            
            if (NS_IS_HIGH_SURROGATE(ch) && scriptLimit < textLength - 1) {
                PRUint32 low = textPtr[scriptLimit + 1];
                if (NS_IS_LOW_SURROGATE(low)) {
                    ch = SURROGATE_TO_UCS4(ch, low);
                    scriptLimit += 1;
                }
            }

            sc = gfxUnicodeProperties::GetScriptCode(ch);

            pairIndex = getPairIndex(ch);

            







            if (pairIndex >= 0) {
                if ((pairIndex & 1) == 0) {
                    push(pairIndex, scriptCode);
                } else {
                    PRInt32 pi = pairIndex & ~1;

                    while (STACK_IS_NOT_EMPTY() && TOP().pairIndex != pi) {
                        pop();
                    }

                    if (STACK_IS_NOT_EMPTY()) {
                        sc = TOP().scriptCode;
                    }
                }
            }
        }

        if (sameScript(scriptCode, sc)) {
            if (scriptCode <= HB_SCRIPT_INHERITED && sc > HB_SCRIPT_INHERITED) {
                scriptCode = sc;

                fixup(scriptCode);
            }

            



            if (pairIndex >= 0 && (pairIndex & 1) != 0) {
                pop();
            }
        } else {
            



            scriptLimit = startOfChar;

            break;
        }
    }

    aRunStart = scriptStart;
    aRunLimit = scriptLimit;
    aRunScript = scriptCode;

    return PR_TRUE;
}
