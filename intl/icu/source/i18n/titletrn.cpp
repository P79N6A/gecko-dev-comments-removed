









#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/uchar.h"
#include "unicode/uniset.h"
#include "unicode/ustring.h"
#include "unicode/utf16.h"
#include "titletrn.h"
#include "umutex.h"
#include "ucase.h"
#include "cpputils.h"

U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(TitlecaseTransliterator)

TitlecaseTransliterator::TitlecaseTransliterator() :
    CaseMapTransliterator(UNICODE_STRING("Any-Title", 9), NULL)
{
    
    setMaximumContextLength(2);
}




TitlecaseTransliterator::~TitlecaseTransliterator() {
}




TitlecaseTransliterator::TitlecaseTransliterator(const TitlecaseTransliterator& o) :
    CaseMapTransliterator(o)
{
}













Transliterator* TitlecaseTransliterator::clone(void) const {
    return new TitlecaseTransliterator(*this);
}




void TitlecaseTransliterator::handleTransliterate(
                                  Replaceable& text, UTransPosition& offsets,
                                  UBool isIncremental) const
{
    
    
    
    
    
    

    if (offsets.start >= offsets.limit) {
        return;
    }

    
    int32_t type;

    
    
    UBool doTitle = TRUE;
    
    
    
    
    
    UChar32 c;
    int32_t start;
    for (start = offsets.start - 1; start >= offsets.contextStart; start -= U16_LENGTH(c)) {
        c = text.char32At(start);
        type=ucase_getTypeOrIgnorable(fCsp, c);
        if(type>0) { 
            doTitle=FALSE;
            break;
        } else if(type==0) { 
            break;
        }
        
    }
    
    
    
    
    UCaseContext csc;
    uprv_memset(&csc, 0, sizeof(csc));
    csc.p = &text;
    csc.start = offsets.contextStart;
    csc.limit = offsets.contextLimit;

    UnicodeString tmp;
    const UChar *s;
    int32_t textPos, delta, result, locCache=0;

    for(textPos=offsets.start; textPos<offsets.limit;) {
        csc.cpStart=textPos;
        c=text.char32At(textPos);
        csc.cpLimit=textPos+=U16_LENGTH(c);

        type=ucase_getTypeOrIgnorable(fCsp, c);
        if(type>=0) { 
            if(doTitle) {
                result=ucase_toFullTitle(fCsp, c, utrans_rep_caseContextIterator, &csc, &s, "", &locCache);
            } else {
                result=ucase_toFullLower(fCsp, c, utrans_rep_caseContextIterator, &csc, &s, "", &locCache);
            }
            doTitle = (UBool)(type==0); 

            if(csc.b1 && isIncremental) {
                
                
                offsets.start=csc.cpStart;
                return;
            }

            if(result>=0) {
                
                
                if(result<=UCASE_MAX_STRING_LENGTH) {
                    
                    tmp.setTo(FALSE, s, result);
                    delta=result-U16_LENGTH(c);
                } else {
                    
                    tmp.setTo(result);
                    delta=tmp.length()-U16_LENGTH(c);
                }
                text.handleReplaceBetween(csc.cpStart, textPos, tmp);
                if(delta!=0) {
                    textPos+=delta;
                    csc.limit=offsets.contextLimit+=delta;
                    offsets.limit+=delta;
                }
            }
        }
    }
    offsets.start=textPos;
}

U_NAMESPACE_END

#endif 
