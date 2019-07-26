












#include "unicode/utypes.h"
#include "unicode/uscript.h"

#include "scrptrun.h"

#define ARRAY_SIZE(array) (sizeof array  / sizeof array[0])

const char ScriptRun::fgClassID=0;

UChar32 ScriptRun::pairedChars[] = {
    0x0028, 0x0029, 
    0x003c, 0x003e,
    0x005b, 0x005d,
    0x007b, 0x007d,
    0x00ab, 0x00bb, 
    0x2018, 0x2019, 
    0x201c, 0x201d,
    0x2039, 0x203a,
    0x3008, 0x3009, 
    0x300a, 0x300b,
    0x300c, 0x300d,
    0x300e, 0x300f,
    0x3010, 0x3011,
    0x3014, 0x3015,
    0x3016, 0x3017,
    0x3018, 0x3019,
    0x301a, 0x301b
};

const int32_t ScriptRun::pairedCharCount = ARRAY_SIZE(pairedChars);
const int32_t ScriptRun::pairedCharPower = 1 << highBit(pairedCharCount);
const int32_t ScriptRun::pairedCharExtra = pairedCharCount - pairedCharPower;

int8_t ScriptRun::highBit(int32_t value)
{
    if (value <= 0) {
        return -32;
    }

    int8_t bit = 0;

    if (value >= 1 << 16) {
        value >>= 16;
        bit += 16;
    }

    if (value >= 1 << 8) {
        value >>= 8;
        bit += 8;
    }

    if (value >= 1 << 4) {
        value >>= 4;
        bit += 4;
    }

    if (value >= 1 << 2) {
        value >>= 2;
        bit += 2;
    }

    if (value >= 1 << 1) {
        value >>= 1;
        bit += 1;
    }

    return bit;
}

int32_t ScriptRun::getPairIndex(UChar32 ch)
{
    int32_t probe = pairedCharPower;
    int32_t index = 0;

    if (ch >= pairedChars[pairedCharExtra]) {
        index = pairedCharExtra;
    }

    while (probe > (1 << 0)) {
        probe >>= 1;

        if (ch >= pairedChars[index + probe]) {
            index += probe;
        }
    }

    if (pairedChars[index] != ch) {
        index = -1;
    }

    return index;
}

UBool ScriptRun::sameScript(int32_t scriptOne, int32_t scriptTwo)
{
    return scriptOne <= USCRIPT_INHERITED || scriptTwo <= USCRIPT_INHERITED || scriptOne == scriptTwo;
}

UBool ScriptRun::next()
{
    int32_t startSP  = parenSP;  
    UErrorCode error = U_ZERO_ERROR;

    
    if (scriptEnd >= charLimit) {
        return false;
    }
    
    scriptCode = USCRIPT_COMMON;

    for (scriptStart = scriptEnd; scriptEnd < charLimit; scriptEnd += 1) {
        UChar   high = charArray[scriptEnd];
        UChar32 ch   = high;

        
        
        if (high >= 0xD800 && high <= 0xDBFF && scriptEnd < charLimit - 1)
        {
            UChar low = charArray[scriptEnd + 1];

            
            
            if (low >= 0xDC00 && low <= 0xDFFF) {
                ch = (high - 0xD800) * 0x0400 + low - 0xDC00 + 0x10000;
                scriptEnd += 1;
            }
        }

        UScriptCode sc = uscript_getScript(ch, &error);
        int32_t pairIndex = getPairIndex(ch);

        
        
        
        
        
        
        if (pairIndex >= 0) {
            if ((pairIndex & 1) == 0) {
                parenStack[++parenSP].pairIndex = pairIndex;
                parenStack[parenSP].scriptCode  = scriptCode;
            } else if (parenSP >= 0) {
                int32_t pi = pairIndex & ~1;

                while (parenSP >= 0 && parenStack[parenSP].pairIndex != pi) {
                    parenSP -= 1;
                }

                if (parenSP < startSP) {
                    startSP = parenSP;
                }

                if (parenSP >= 0) {
                    sc = parenStack[parenSP].scriptCode;
                }
            }
        }

        if (sameScript(scriptCode, sc)) {
            if (scriptCode <= USCRIPT_INHERITED && sc > USCRIPT_INHERITED) {
                scriptCode = sc;

                
                
                while (startSP < parenSP) {
                    parenStack[++startSP].scriptCode = scriptCode;
                }
            }

            
            
            if (pairIndex >= 0 && (pairIndex & 1) != 0 && parenSP >= 0) {
                parenSP -= 1;
                startSP -= 1;
            }
        } else {
            
            
            if (ch >= 0x10000) {
                scriptEnd -= 1;
            }

            break;
        }
    }

    return true;
}

