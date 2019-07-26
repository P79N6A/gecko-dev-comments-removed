









#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/unifilt.h"
#include "unicode/uchar.h"
#include "unicode/utf16.h"
#include "uni2name.h"
#include "cstring.h"
#include "cmemory.h"
#include "uprops.h"

U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(UnicodeNameTransliterator)

static const UChar OPEN_DELIM[] = {92,78,123,0}; 
static const UChar CLOSE_DELIM  = 125; 
#define OPEN_DELIM_LEN 3




UnicodeNameTransliterator::UnicodeNameTransliterator(UnicodeFilter* adoptedFilter) :
    Transliterator(UNICODE_STRING("Any-Name", 8), adoptedFilter) {
}




UnicodeNameTransliterator::~UnicodeNameTransliterator() {}




UnicodeNameTransliterator::UnicodeNameTransliterator(const UnicodeNameTransliterator& o) :
    Transliterator(o) {}













Transliterator* UnicodeNameTransliterator::clone(void) const {
    return new UnicodeNameTransliterator(*this);
}






void UnicodeNameTransliterator::handleTransliterate(Replaceable& text, UTransPosition& offsets,
                                                    UBool ) const {
    
    
    

    int32_t maxLen = uprv_getMaxCharNameLength();
    if (maxLen == 0) {
        offsets.start = offsets.limit;
        return;
    }

    
    char* buf = (char*) uprv_malloc(maxLen);
    if (buf == NULL) {
        offsets.start = offsets.limit;
        return;
    }
    
    int32_t cursor = offsets.start;
    int32_t limit = offsets.limit;

    UnicodeString str(FALSE, OPEN_DELIM, OPEN_DELIM_LEN);
    UErrorCode status;
    int32_t len;

    while (cursor < limit) {
        UChar32 c = text.char32At(cursor);
        int32_t clen = U16_LENGTH(c);
        status = U_ZERO_ERROR;
        if ((len = u_charName(c, U_EXTENDED_CHAR_NAME, buf, maxLen, &status)) >0 && !U_FAILURE(status)) {
            str.truncate(OPEN_DELIM_LEN);
            str.append(UnicodeString(buf, len, US_INV)).append(CLOSE_DELIM);
            text.handleReplaceBetween(cursor, cursor+clen, str);
            len += OPEN_DELIM_LEN + 1; 
            cursor += len; 
            limit += len-clen; 
        } else {
            cursor += clen;
        }
    }

    offsets.contextLimit += limit - offsets.limit;
    offsets.limit = limit;
    offsets.start = cursor;

    uprv_free(buf);
}

U_NAMESPACE_END

#endif 
