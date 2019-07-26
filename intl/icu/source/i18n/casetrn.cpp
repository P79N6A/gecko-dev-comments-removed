

















#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/uchar.h"
#include "unicode/ustring.h"
#include "unicode/utf.h"
#include "unicode/utf16.h"
#include "tolowtrn.h"
#include "ucase.h"
#include "cpputils.h"


U_CFUNC UChar32 U_CALLCONV
utrans_rep_caseContextIterator(void *context, int8_t dir)
{
    U_NAMESPACE_USE

    UCaseContext *csc=(UCaseContext *)context;
    Replaceable *rep=(Replaceable *)csc->p;
    UChar32 c;

    if(dir<0) {
        
        csc->index=csc->cpStart;
        csc->dir=dir;
    } else if(dir>0) {
        
        csc->index=csc->cpLimit;
        csc->dir=dir;
    } else {
        
        dir=csc->dir;
    }

    
    
    if(dir<0) {
        if(csc->start<csc->index) {
            c=rep->char32At(csc->index-1);
            if(c<0) {
                csc->start=csc->index;
            } else {
                csc->index-=U16_LENGTH(c);
                return c;
            }
        }
    } else {
        
        if(csc->index<csc->limit) {
            c=rep->char32At(csc->index);
            if(c<0) {
                csc->limit=csc->index;
                csc->b1=TRUE;
            } else {
                csc->index+=U16_LENGTH(c);
                return c;
            }
        } else {
            csc->b1=TRUE;
        }
    }
    return U_SENTINEL;
}

U_NAMESPACE_BEGIN

UOBJECT_DEFINE_ABSTRACT_RTTI_IMPLEMENTATION(CaseMapTransliterator)




CaseMapTransliterator::CaseMapTransliterator(const UnicodeString &id, UCaseMapFull *map) : 
    Transliterator(id, 0),
    fCsp(ucase_getSingleton()),
    fMap(map)
{
    
    
}




CaseMapTransliterator::~CaseMapTransliterator() {
}




CaseMapTransliterator::CaseMapTransliterator(const CaseMapTransliterator& o) :
    Transliterator(o),
    fCsp(o.fCsp), fMap(o.fMap)
{
}





















void CaseMapTransliterator::handleTransliterate(Replaceable& text,
                                 UTransPosition& offsets, 
                                 UBool isIncremental) const
{
    if (offsets.start >= offsets.limit) {
        return;
    }

    UCaseContext csc;
    uprv_memset(&csc, 0, sizeof(csc));
    csc.p = &text;
    csc.start = offsets.contextStart;
    csc.limit = offsets.contextLimit;

    UnicodeString tmp;
    const UChar *s;
    UChar32 c;
    int32_t textPos, delta, result, locCache=0;

    for(textPos=offsets.start; textPos<offsets.limit;) {
        csc.cpStart=textPos;
        c=text.char32At(textPos);
        csc.cpLimit=textPos+=U16_LENGTH(c);

        result=fMap(fCsp, c, utrans_rep_caseContextIterator, &csc, &s, "", &locCache);

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
    offsets.start=textPos;
}

U_NAMESPACE_END

#endif 
