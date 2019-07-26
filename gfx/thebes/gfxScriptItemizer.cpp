
















































#include "gfxScriptItemizer.h"
#include "nsUnicodeProperties.h"

#include "nsCharTraits.h"

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

void
gfxScriptItemizer::push(uint32_t endPairChar, int32_t scriptCode)
{
    pushCount  = LIMIT_INC(pushCount);
    fixupCount = LIMIT_INC(fixupCount);

    parenSP = INC1(parenSP);
    parenStack[parenSP].endPairChar = endPairChar;
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
gfxScriptItemizer::fixup(int32_t scriptCode)
{
    int32_t fixupSP = DEC(parenSP, fixupCount);

    while (fixupCount-- > 0) {
        fixupSP = INC1(fixupSP);
        parenStack[fixupSP].scriptCode = scriptCode;
    }
}

static inline bool
SameScript(int32_t runScript, int32_t currCharScript)
{
    return runScript <= MOZ_SCRIPT_INHERITED ||
           currCharScript <= MOZ_SCRIPT_INHERITED ||
           currCharScript == runScript;
}




static inline bool
HasMirroredChar(uint32_t aCh)
{
    return GetCharProps1(aCh).mMirrorOffsetIndex != 0;
}

gfxScriptItemizer::gfxScriptItemizer(const PRUnichar *src, uint32_t length)
    : textPtr(src), textLength(length)
{
    reset();
}

void
gfxScriptItemizer::SetText(const PRUnichar *src, uint32_t length)
{
    textPtr  = src;
    textLength = length;

    reset();
}

bool
gfxScriptItemizer::Next(uint32_t& aRunStart, uint32_t& aRunLimit,
                        int32_t& aRunScript)
{
    
    if (scriptLimit >= textLength) {
        return false;
    }

    SYNC_FIXUP();
    scriptCode = MOZ_SCRIPT_COMMON;

    for (scriptStart = scriptLimit; scriptLimit < textLength; scriptLimit += 1) {
        uint32_t ch;
        int32_t sc;
        uint32_t startOfChar = scriptLimit;

        ch = textPtr[scriptLimit];

        
        if (NS_IS_HIGH_SURROGATE(ch) && scriptLimit < textLength - 1) {
            uint32_t low = textPtr[scriptLimit + 1];
            if (NS_IS_LOW_SURROGATE(low)) {
                ch = SURROGATE_TO_UCS4(ch, low);
                scriptLimit += 1;
            }
        }

        
        
        
        
        
        
        const nsCharProps2& charProps = GetCharProps2(ch);

        
        
        uint8_t gc = HB_UNICODE_GENERAL_CATEGORY_UNASSIGNED;

        sc = charProps.mScriptCode;
        if (sc == MOZ_SCRIPT_COMMON) {
            










            gc = charProps.mCategory;
            if (gc == HB_UNICODE_GENERAL_CATEGORY_OPEN_PUNCTUATION) {
                uint32_t endPairChar = mozilla::unicode::GetMirroredChar(ch);
                if (endPairChar != ch) {
                    push(endPairChar, scriptCode);
                }
            } else if (gc == HB_UNICODE_GENERAL_CATEGORY_CLOSE_PUNCTUATION &&
                HasMirroredChar(ch))
            {
                while (STACK_IS_NOT_EMPTY() && TOP().endPairChar != ch) {
                    pop();
                }

                if (STACK_IS_NOT_EMPTY()) {
                    sc = TOP().scriptCode;
                }
            }
        }

        if (SameScript(scriptCode, sc)) {
            if (scriptCode <= MOZ_SCRIPT_INHERITED &&
                sc > MOZ_SCRIPT_INHERITED)
            {
                scriptCode = sc;
                fixup(scriptCode);
            }

            



            if (gc == HB_UNICODE_GENERAL_CATEGORY_CLOSE_PUNCTUATION &&
                HasMirroredChar(ch)) {
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

    return true;
}
