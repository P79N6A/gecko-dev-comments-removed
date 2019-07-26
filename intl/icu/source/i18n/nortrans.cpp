









#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/normalizer2.h"
#include "unicode/utf16.h"
#include "cstring.h"
#include "nortrans.h"

U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(NormalizationTransliterator)

static inline Transliterator::Token cstrToken(const char *s) {
    return Transliterator::pointerToken((void *)s);
}




void NormalizationTransliterator::registerIDs() {
    
    Transliterator::_registerFactory(UNICODE_STRING_SIMPLE("Any-NFC"),
                                     _create, cstrToken("nfc\0\0"));
    Transliterator::_registerFactory(UNICODE_STRING_SIMPLE("Any-NFKC"),
                                     _create, cstrToken("nfkc\0\0"));
    Transliterator::_registerFactory(UNICODE_STRING_SIMPLE("Any-NFD"),
                                     _create, cstrToken("nfc\0\1"));
    Transliterator::_registerFactory(UNICODE_STRING_SIMPLE("Any-NFKD"),
                                     _create, cstrToken("nfkc\0\1"));
    Transliterator::_registerFactory(UNICODE_STRING_SIMPLE("Any-FCD"),
                                     _create, cstrToken("nfc\0\2"));
    Transliterator::_registerFactory(UNICODE_STRING_SIMPLE("Any-FCC"),
                                     _create, cstrToken("nfc\0\3"));
    Transliterator::_registerSpecialInverse(UNICODE_STRING_SIMPLE("NFC"),
                                            UNICODE_STRING_SIMPLE("NFD"), TRUE);
    Transliterator::_registerSpecialInverse(UNICODE_STRING_SIMPLE("NFKC"),
                                            UNICODE_STRING_SIMPLE("NFKD"), TRUE);
    Transliterator::_registerSpecialInverse(UNICODE_STRING_SIMPLE("FCC"),
                                            UNICODE_STRING_SIMPLE("NFD"), FALSE);
    Transliterator::_registerSpecialInverse(UNICODE_STRING_SIMPLE("FCD"),
                                            UNICODE_STRING_SIMPLE("FCD"), FALSE);
}




Transliterator* NormalizationTransliterator::_create(const UnicodeString& ID,
                                                     Token context) {
    const char *name = (const char *)context.pointer;
    UNormalization2Mode mode = (UNormalization2Mode)uprv_strchr(name, 0)[1];
    UErrorCode errorCode = U_ZERO_ERROR;
    const Normalizer2 *norm2 = Normalizer2::getInstance(NULL, name, mode, errorCode);
    if(U_SUCCESS(errorCode)) {
        return new NormalizationTransliterator(ID, *norm2);
    } else {
        return NULL;
    }
}




NormalizationTransliterator::NormalizationTransliterator(const UnicodeString& id,
                                                         const Normalizer2 &norm2) :
    Transliterator(id, 0), fNorm2(norm2) {}




NormalizationTransliterator::~NormalizationTransliterator() {
}




NormalizationTransliterator::NormalizationTransliterator(const NormalizationTransliterator& o) :
    Transliterator(o), fNorm2(o.fNorm2) {}




Transliterator* NormalizationTransliterator::clone(void) const {
    return new NormalizationTransliterator(*this);
}




void NormalizationTransliterator::handleTransliterate(Replaceable& text, UTransPosition& offsets,
                                                      UBool isIncremental) const {
    
    int32_t start = offsets.start;
    int32_t limit = offsets.limit;
    if(start >= limit) {
        return;
    }

    
























    UErrorCode errorCode = U_ZERO_ERROR;
    UnicodeString segment;
    UnicodeString normalized;
    UChar32 c = text.char32At(start);
    do {
        int32_t prev = start;
        
        
        segment.remove();
        do {
            segment.append(c);
            start += U16_LENGTH(c);
        } while(start < limit && !fNorm2.hasBoundaryBefore(c = text.char32At(start)));
        if(start == limit && isIncremental && !fNorm2.hasBoundaryAfter(c)) {
            
            
            
            start=prev;
            break;
        }
        fNorm2.normalize(segment, normalized, errorCode);
        if(U_FAILURE(errorCode)) {
            break;
        }
        if(segment != normalized) {
            
            text.handleReplaceBetween(prev, start, normalized);

            
            int32_t delta = normalized.length() - (start - prev);
            start += delta;
            limit += delta;
        }
    } while(start < limit);

    offsets.start = start;
    offsets.contextLimit += limit - offsets.limit;
    offsets.limit = limit;
}

U_NAMESPACE_END

#endif 
