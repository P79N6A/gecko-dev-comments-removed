






#include "unicode/utypes.h"

#if !UCONFIG_NO_CONVERSION

#include "csrutf8.h"
#include "csmatch.h"

U_NAMESPACE_BEGIN

CharsetRecog_UTF8::~CharsetRecog_UTF8()
{
    
}

const char *CharsetRecog_UTF8::getName() const
{
    return "UTF-8";
}

UBool CharsetRecog_UTF8::match(InputText* input, CharsetMatch *results) const {
    bool hasBOM = FALSE;
    int32_t numValid = 0;
    int32_t numInvalid = 0;
    const uint8_t *inputBytes = input->fRawInput;
    int32_t i;
    int32_t trailBytes = 0;
    int32_t confidence;

    if (input->fRawLength >= 3 && 
        inputBytes[0] == 0xEF && inputBytes[1] == 0xBB && inputBytes[2] == 0xBF) {
            hasBOM = TRUE;
    }

    
    for (i=0; i < input->fRawLength; i += 1) {
        int32_t b = inputBytes[i];

        if ((b & 0x80) == 0) {
            continue;   
        }

        
        if ((b & 0x0E0) == 0x0C0) {
            trailBytes = 1;
        } else if ((b & 0x0F0) == 0x0E0) {
            trailBytes = 2;
        } else if ((b & 0x0F8) == 0xF0) {
            trailBytes = 3;
        } else {
            numInvalid += 1;
            continue;
        }

        
        for (;;) {
            i += 1;

            if (i >= input->fRawLength) {
                break;
            }

            b = inputBytes[i];

            if ((b & 0xC0) != 0x080) {
                numInvalid += 1;
                break;
            }

            if (--trailBytes == 0) {
                numValid += 1;
                break;
            }
        }

    }

    
    
    confidence = 0;
    if (hasBOM && numInvalid == 0) {
        confidence = 100;
    } else if (hasBOM && numValid > numInvalid*10) {
        confidence = 80;
    } else if (numValid > 3 && numInvalid == 0) {
        confidence = 100;
    } else if (numValid > 0 && numInvalid == 0) {
        confidence = 80;
    } else if (numValid == 0 && numInvalid == 0) {
        
        
        confidence = 15;
    } else if (numValid > numInvalid*10) {
        
        confidence = 25;
    }

    results->set(input, this, confidence);
    return (confidence > 0);
}

U_NAMESPACE_END
#endif
