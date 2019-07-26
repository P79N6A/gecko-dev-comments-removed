









#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/unifilt.h"
#include "unicode/uchar.h"
#include "unicode/uniset.h"
#include "unicode/utf16.h"
#include "cmemory.h"
#include "name2uni.h"
#include "patternprops.h"
#include "uprops.h"
#include "uinvchar.h"
#include "util.h"

U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(NameUnicodeTransliterator)

static const UChar OPEN[] = {92,78,126,123,126,0}; 
static const UChar OPEN_DELIM  = 92;  
static const UChar CLOSE_DELIM = 125; 
static const UChar SPACE       = 32;  

U_CDECL_BEGIN



static void U_CALLCONV
_set_add(USet *set, UChar32 c) {
    uset_add(set, c);
}












U_CDECL_END





NameUnicodeTransliterator::NameUnicodeTransliterator(UnicodeFilter* adoptedFilter) :
    Transliterator(UNICODE_STRING("Name-Any", 8), adoptedFilter) {

    UnicodeSet *legalPtr = &legal;
    
    USetAdder sa = {
        (USet *)legalPtr, 
        _set_add,
        NULL, 
        NULL, 
        NULL, 
        NULL
    };
    uprv_getCharNameCharacters(&sa);
}




NameUnicodeTransliterator::~NameUnicodeTransliterator() {}




NameUnicodeTransliterator::NameUnicodeTransliterator(const NameUnicodeTransliterator& o) :
    Transliterator(o), legal(o.legal) {}














Transliterator* NameUnicodeTransliterator::clone(void) const {
    return new NameUnicodeTransliterator(*this);
}




void NameUnicodeTransliterator::handleTransliterate(Replaceable& text, UTransPosition& offsets,
                                                    UBool isIncremental) const {
    
    
    

    int32_t maxLen = uprv_getMaxCharNameLength();
    if (maxLen == 0) {
        offsets.start = offsets.limit;
        return;
    }

    
    ++maxLen; 
    char* cbuf = (char*) uprv_malloc(maxLen);
    if (cbuf == NULL) {
        offsets.start = offsets.limit;
        return;
    }

    UnicodeString openPat(TRUE, OPEN, -1);
    UnicodeString str, name;

    int32_t cursor = offsets.start;
    int32_t limit = offsets.limit;

    
    
    
    int32_t mode = 0;
    int32_t openPos = -1; 

    UChar32 c;
    while (cursor < limit) {
        c = text.char32At(cursor);

        switch (mode) {
        case 0: 
            if (c == OPEN_DELIM) { 
                openPos = cursor;
                int32_t i =
                    ICU_Utility::parsePattern(openPat, text, cursor, limit);
                if (i >= 0 && i < limit) {
                    mode = 1;
                    name.truncate(0);
                    cursor = i;
                    continue; 
                }
            }
            break;

        case 1: 
            
            
            
            

            
            
            if (PatternProps::isWhiteSpace(c)) {
                
                if (name.length() > 0 &&
                    name.charAt(name.length()-1) != SPACE) {
                    name.append(SPACE);
                    
                    
                    if (name.length() > maxLen) {
                        mode = 0;
                    }
                }
                break;
            }

            if (c == CLOSE_DELIM) {
                int32_t len = name.length();

                
                if (len > 0 &&
                    name.charAt(len-1) == SPACE) {
                    --len;
                }

                if (uprv_isInvariantUString(name.getBuffer(), len)) {
                    name.extract(0, len, cbuf, maxLen, US_INV);

                    UErrorCode status = U_ZERO_ERROR;
                    c = u_charFromName(U_EXTENDED_CHAR_NAME, cbuf, &status);
                    if (U_SUCCESS(status)) {
                        

                        
                        cursor++; 

                        str.truncate(0);
                        str.append(c);
                        text.handleReplaceBetween(openPos, cursor, str);

                        
                        
                        
                        int32_t delta = cursor - openPos - str.length();
                        cursor -= delta;
                        limit -= delta;
                        
                    }
                }
                
                
                mode = 0;
                openPos = -1; 
                continue; 
            }
            
            
            
            
            if (legal.contains(c)) {
                name.append(c);
                
                
                if (name.length() >= maxLen) {
                    mode = 0;
                }
            }
            
            
            else {
                --cursor; 
                mode = 0;
            }

            break;
        }

        cursor += U16_LENGTH(c);
    }
        
    offsets.contextLimit += limit - offsets.limit;
    offsets.limit = limit;
    
    
    offsets.start = (isIncremental && openPos >= 0) ? openPos : cursor;

    uprv_free(cbuf);
}

U_NAMESPACE_END

#endif 
