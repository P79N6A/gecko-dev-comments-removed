






#include "unicode/utypes.h"

#if !UCONFIG_NO_CONVERSION

#include "inputext.h"

#include "cmemory.h"
#include "cstring.h"

#include <string.h>

U_NAMESPACE_BEGIN

#define BUFFER_SIZE 8192

#define ARRAY_SIZE(array) (sizeof array / sizeof array[0])

#define NEW_ARRAY(type,count) (type *) uprv_malloc((count) * sizeof(type))
#define DELETE_ARRAY(array) uprv_free((void *) (array))

InputText::InputText(UErrorCode &status)
    : fInputBytes(NEW_ARRAY(uint8_t, BUFFER_SIZE)), 
                                                 
      fByteStats(NEW_ARRAY(int16_t, 256)),       
                                                 
      fDeclaredEncoding(0),
      fRawInput(0),
      fRawLength(0)
{
    if (fInputBytes == NULL || fByteStats == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
    }
}

InputText::~InputText()
{
    DELETE_ARRAY(fDeclaredEncoding);
    DELETE_ARRAY(fByteStats);
    DELETE_ARRAY(fInputBytes);
}

void InputText::setText(const char *in, int32_t len)
{
    fInputLen  = 0;
    fC1Bytes   = FALSE;
    fRawInput  = (const uint8_t *) in;
    fRawLength = len == -1? (int32_t)uprv_strlen(in) : len;
}

void InputText::setDeclaredEncoding(const char* encoding, int32_t len)
{
    if(encoding) {
        if (len == -1) {
            len = (int32_t)uprv_strlen(encoding);
        }

        len += 1;     
        uprv_free(fDeclaredEncoding);
        fDeclaredEncoding = NEW_ARRAY(char, len);
        uprv_strncpy(fDeclaredEncoding, encoding, len);
    }
}

UBool InputText::isSet() const 
{
    return fRawInput != NULL;
}







void InputText::MungeInput(UBool fStripTags) {
    int     srci = 0;
    int     dsti = 0;
    uint8_t b;
    bool    inMarkup = FALSE;
    int32_t openTags = 0;
    int32_t badTags  = 0;

    
    
    
    
    
    
    
    if (fStripTags) {
        for (srci = 0; srci < fRawLength && dsti < BUFFER_SIZE; srci += 1) {
            b = fRawInput[srci];

            if (b == (uint8_t)0x3C) { 
                if (inMarkup) {
                    badTags += 1;
                }

                inMarkup = TRUE;
                openTags += 1;
            }

            if (! inMarkup) {
                fInputBytes[dsti++] = b;
            }

            if (b == (uint8_t)0x3E) { 
                inMarkup = FALSE;
            }
        }

        fInputLen = dsti;
    }

    
    
    
    
    
    if (openTags<5 || openTags/5 < badTags || 
        (fInputLen < 100 && fRawLength>600))
    {
        int32_t limit = fRawLength;

        if (limit > BUFFER_SIZE) {
            limit = BUFFER_SIZE;
        }

        for (srci=0; srci<limit; srci++) {
            fInputBytes[srci] = fRawInput[srci];
        }

        fInputLen = srci;
    }

    
    
    
    

    uprv_memset(fByteStats, 0, (sizeof fByteStats[0]) * 256);

    for (srci = 0; srci < fInputLen; srci += 1) {
        fByteStats[fInputBytes[srci]] += 1;
    }

    for (int32_t i = 0x80; i <= 0x9F; i += 1) {
        if (fByteStats[i] != 0) {
            fC1Bytes = TRUE;
            break;
        }
    }
}

U_NAMESPACE_END
#endif

