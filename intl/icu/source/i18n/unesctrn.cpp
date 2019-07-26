









#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/uchar.h"
#include "unicode/utf16.h"
#include "unesctrn.h"
#include "util.h"

#include "cmemory.h"

U_NAMESPACE_BEGIN




static const UChar END = 0xFFFF;


static const UChar SPEC_Unicode[] = {
    2, 0, 16, 4, 6, 85, 43,
    END
};


static const UChar SPEC_Java[] = {
    2, 0, 16, 4, 4, 92, 117,
    END
};


static const UChar SPEC_C[] = {
    2, 0, 16, 4, 4, 92, 117,
    2, 0, 16, 8, 8, 92, 85,
    END
};


static const UChar SPEC_XML[] = {
    3, 1, 16, 1, 6, 38, 35, 120, 59,
    END
};


static const UChar SPEC_XML10[] = {
    2, 1, 10, 1, 7, 38, 35, 59,
    END
};


static const UChar SPEC_Perl[] = {
    3, 1, 16, 1, 6, 92, 120, 123, 125,
    END
};


static const UChar SPEC_Any[] = {
    2, 0, 16, 4, 6, 85, 43,                      
    2, 0, 16, 4, 4, 92, 117,                     
    2, 0, 16, 8, 8, 92, 85,                      
    3, 1, 16, 1, 6, 38, 35, 120, 59,   
    2, 1, 10, 1, 7, 38, 35, 59,             
    3, 1, 16, 1, 6, 92, 120, 123, 125, 
    END
};

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(UnescapeTransliterator)

static UChar* copySpec(const UChar* spec) {
    int32_t len = 0;
    while (spec[len] != END) {
        ++len;
    }
    ++len;
    UChar *result = (UChar *)uprv_malloc(len*sizeof(UChar));
    
    if (result != NULL) {
    	uprv_memcpy(result, spec, len*sizeof(result[0]));
    }
    return result;
}




static Transliterator* _createUnicode(const UnicodeString& ID, Transliterator::Token ) {
    return new UnescapeTransliterator(ID, SPEC_Unicode);
}
static Transliterator* _createJava(const UnicodeString& ID, Transliterator::Token ) {
    return new UnescapeTransliterator(ID, SPEC_Java);
}
static Transliterator* _createC(const UnicodeString& ID, Transliterator::Token ) {
    return new UnescapeTransliterator(ID, SPEC_C);
}
static Transliterator* _createXML(const UnicodeString& ID, Transliterator::Token ) {
    return new UnescapeTransliterator(ID, SPEC_XML);
}
static Transliterator* _createXML10(const UnicodeString& ID, Transliterator::Token ) {
    return new UnescapeTransliterator(ID, SPEC_XML10);
}
static Transliterator* _createPerl(const UnicodeString& ID, Transliterator::Token ) {
    return new UnescapeTransliterator(ID, SPEC_Perl);
}
static Transliterator* _createAny(const UnicodeString& ID, Transliterator::Token ) {
    return new UnescapeTransliterator(ID, SPEC_Any);
}





void UnescapeTransliterator::registerIDs() {
    Token t = integerToken(0);

    Transliterator::_registerFactory(UNICODE_STRING_SIMPLE("Hex-Any/Unicode"), _createUnicode, t);

    Transliterator::_registerFactory(UNICODE_STRING_SIMPLE("Hex-Any/Java"), _createJava, t);

    Transliterator::_registerFactory(UNICODE_STRING_SIMPLE("Hex-Any/C"), _createC, t);

    Transliterator::_registerFactory(UNICODE_STRING_SIMPLE("Hex-Any/XML"), _createXML, t);

    Transliterator::_registerFactory(UNICODE_STRING_SIMPLE("Hex-Any/XML10"), _createXML10, t);

    Transliterator::_registerFactory(UNICODE_STRING_SIMPLE("Hex-Any/Perl"), _createPerl, t);

    Transliterator::_registerFactory(UNICODE_STRING_SIMPLE("Hex-Any"), _createAny, t);
}




UnescapeTransliterator::UnescapeTransliterator(const UnicodeString& newID,
                                               const UChar *newSpec) :
    Transliterator(newID, NULL)
{
    this->spec = copySpec(newSpec);
}




UnescapeTransliterator::UnescapeTransliterator(const UnescapeTransliterator& o) :
    Transliterator(o) {
    this->spec = copySpec(o.spec);
}

UnescapeTransliterator::~UnescapeTransliterator() {
    uprv_free(spec);
}




Transliterator* UnescapeTransliterator::clone() const {
    return new UnescapeTransliterator(*this);
}




void UnescapeTransliterator::handleTransliterate(Replaceable& text, UTransPosition& pos,
                                                 UBool isIncremental) const {
    int32_t start = pos.start;
    int32_t limit = pos.limit;
    int32_t i, j, ipat;

    while (start < limit) {
        
        
        
        for (j=0, ipat=0; spec[ipat] != END; ++j) {

            
            int32_t prefixLen = spec[ipat++];
            int32_t suffixLen = spec[ipat++];
            int8_t  radix     = (int8_t) spec[ipat++];
            int32_t minDigits = spec[ipat++];
            int32_t maxDigits = spec[ipat++];

            
            
            int32_t s = start;
            UBool match = TRUE;

            for (i=0; i<prefixLen; ++i) {
                if (s >= limit) {
                    if (i > 0) {
                        
                        
                        
                        
                        if (isIncremental) {
                            goto exit;
                        }
                        match = FALSE;
                        break;
                    }
                }
                UChar c = text.charAt(s++);
                if (c != spec[ipat + i]) {
                    match = FALSE;
                    break;
                }
            }

            if (match) {
                UChar32 u = 0;
                int32_t digitCount = 0;
                for (;;) {
                    if (s >= limit) {
                        
                        if (s > start && isIncremental) {
                            goto exit;
                        }
                        break;
                    }
                    UChar32 ch = text.char32At(s);
                    int32_t digit = u_digit(ch, radix);
                    if (digit < 0) {
                        break;
                    }
                    s += U16_LENGTH(ch);
                    u = (u * radix) + digit;
                    if (++digitCount == maxDigits) {
                        break;
                    }
                }

                match = (digitCount >= minDigits);

                if (match) {
                    for (i=0; i<suffixLen; ++i) {
                        if (s >= limit) {
                            
                            if (s > start && isIncremental) {
                                goto exit;
                            }
                            match = FALSE;
                            break;
                        }
                        UChar c = text.charAt(s++);
                        if (c != spec[ipat + prefixLen + i]) {
                            match = FALSE;
                            break;
                        }
                    }

                    if (match) {
                        
                        UnicodeString str(u);
                        text.handleReplaceBetween(start, s, str);
                        limit -= s - start - str.length();
                        
                        
                        
                        
                        break;
                    }
                }
            }

            ipat += prefixLen + suffixLen;
        }

        if (start < limit) {
            start += U16_LENGTH(text.char32At(start));
        }
    }

  exit:
    pos.contextLimit += limit - pos.limit;
    pos.limit = limit;
    pos.start = start;
}

U_NAMESPACE_END

#endif 


