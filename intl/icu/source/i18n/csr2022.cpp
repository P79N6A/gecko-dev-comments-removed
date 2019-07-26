






#include "unicode/utypes.h"

#if !UCONFIG_NO_CONVERSION

#include "cstring.h"

#include "csr2022.h"
#include "csmatch.h"

U_NAMESPACE_BEGIN

#define ARRAY_SIZE(array) (sizeof array / sizeof array[0])













int32_t CharsetRecog_2022::match_2022(const uint8_t *text, int32_t textLen, const uint8_t escapeSequences[][5], int32_t escapeSequences_length) const
{
    int32_t i, j;
    int32_t escN;
    int32_t hits   = 0;
    int32_t misses = 0;
    int32_t shifts = 0;
    int32_t quality;

    i = 0;
    while(i < textLen) {
        if(text[i] == 0x1B) {
            escN = 0;
            while(escN < escapeSequences_length) {
                const uint8_t *seq = escapeSequences[escN];
                int32_t seq_length = (int32_t)uprv_strlen((const char *) seq);

                if (textLen-i >= seq_length) {
                    j = 1;
                    while(j < seq_length) {
                        if(seq[j] != text[i+j]) {
                            goto checkEscapes;
                        }

                        j += 1;
                    }

                    hits += 1;
                    i += seq_length-1;
                    goto scanInput;
                }
                
checkEscapes:
                escN += 1;
            }

            misses += 1;
        }

        if( text[i]== 0x0e || text[i] == 0x0f){
            shifts += 1;
        }

scanInput:
        i += 1;
    }

    if (hits == 0) {
        return 0;
    }

    
    
    
    
    
    
    quality = (100*hits - 100*misses) / (hits + misses);

    
    
    
    if (hits+shifts < 5) {
        quality -= (5-(hits+shifts))*10;
    }

    if (quality < 0) {
        quality = 0;
    }

    return quality;
}


static const uint8_t escapeSequences_2022JP[][5] = {
    {0x1b, 0x24, 0x28, 0x43, 0x00},   
    {0x1b, 0x24, 0x28, 0x44, 0x00},   
    {0x1b, 0x24, 0x40, 0x00, 0x00},   
    {0x1b, 0x24, 0x41, 0x00, 0x00},   
    {0x1b, 0x24, 0x42, 0x00, 0x00},   
    {0x1b, 0x26, 0x40, 0x00, 0x00},   
    {0x1b, 0x28, 0x42, 0x00, 0x00},   
    {0x1b, 0x28, 0x48, 0x00, 0x00},   
    {0x1b, 0x28, 0x49, 0x00, 0x00},   
    {0x1b, 0x28, 0x4a, 0x00, 0x00},   
    {0x1b, 0x2e, 0x41, 0x00, 0x00},   
    {0x1b, 0x2e, 0x46, 0x00, 0x00}    
};

static const uint8_t escapeSequences_2022KR[][5] = {
    {0x1b, 0x24, 0x29, 0x43, 0x00}   
};

static const uint8_t escapeSequences_2022CN[][5] = {
    {0x1b, 0x24, 0x29, 0x41, 0x00},   
    {0x1b, 0x24, 0x29, 0x47, 0x00},   
    {0x1b, 0x24, 0x2A, 0x48, 0x00},   
    {0x1b, 0x24, 0x29, 0x45, 0x00},   
    {0x1b, 0x24, 0x2B, 0x49, 0x00},   
    {0x1b, 0x24, 0x2B, 0x4A, 0x00},   
    {0x1b, 0x24, 0x2B, 0x4B, 0x00},   
    {0x1b, 0x24, 0x2B, 0x4C, 0x00},   
    {0x1b, 0x24, 0x2B, 0x4D, 0x00},   
    {0x1b, 0x4e, 0x00, 0x00, 0x00},   
    {0x1b, 0x4f, 0x00, 0x00, 0x00},   
};

CharsetRecog_2022JP::~CharsetRecog_2022JP() {}

const char *CharsetRecog_2022JP::getName() const {
    return "ISO-2022-JP";
}

UBool CharsetRecog_2022JP::match(InputText *textIn, CharsetMatch *results) const {
    int32_t confidence = match_2022(textIn->fInputBytes, 
                                    textIn->fInputLen, 
                                    escapeSequences_2022JP, 
                                    ARRAY_SIZE(escapeSequences_2022JP));
    results->set(textIn, this, confidence);
    return (confidence > 0);
}

CharsetRecog_2022KR::~CharsetRecog_2022KR() {}

const char *CharsetRecog_2022KR::getName() const {
    return "ISO-2022-KR";
}

UBool CharsetRecog_2022KR::match(InputText *textIn, CharsetMatch *results) const {
    int32_t confidence = match_2022(textIn->fInputBytes, 
                                    textIn->fInputLen, 
                                    escapeSequences_2022KR, 
                                    ARRAY_SIZE(escapeSequences_2022KR));
    results->set(textIn, this, confidence);
    return (confidence > 0);
}

CharsetRecog_2022CN::~CharsetRecog_2022CN() {}

const char *CharsetRecog_2022CN::getName() const {
    return "ISO-2022-CN";
}

UBool CharsetRecog_2022CN::match(InputText *textIn, CharsetMatch *results) const {
    int32_t confidence = match_2022(textIn->fInputBytes,
                                    textIn->fInputLen,
                                    escapeSequences_2022CN,
                                    ARRAY_SIZE(escapeSequences_2022CN));
    results->set(textIn, this, confidence);
    return (confidence > 0);
}

CharsetRecog_2022::~CharsetRecog_2022() {
    
}

U_NAMESPACE_END
#endif
