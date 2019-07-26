









#include "unicode/utypes.h"

#if  !UCONFIG_NO_TRANSLITERATION && !UCONFIG_NO_BREAK_ITERATION

#include "unicode/unifilt.h"
#include "unicode/uchar.h"
#include "unicode/uniset.h"
#include "unicode/brkiter.h"
#include "brktrans.h"
#include "unicode/uchar.h"
#include "cmemory.h"
#include "uprops.h"
#include "uinvchar.h"
#include "util.h"
#include "uvectr32.h"

U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(BreakTransliterator)

static const UChar SPACE       = 32;  






BreakTransliterator::BreakTransliterator(UnicodeFilter* adoptedFilter) :
    Transliterator(UNICODE_STRING("Any-BreakInternal", 17), adoptedFilter),
    fInsertion(SPACE) {
        bi = NULL;
        UErrorCode status = U_ZERO_ERROR;
        boundaries = new UVector32(status);
    }





BreakTransliterator::~BreakTransliterator() {
    delete bi;
    bi = NULL;
    delete boundaries;
    boundaries = NULL;
}




BreakTransliterator::BreakTransliterator(const BreakTransliterator& o) :
    Transliterator(o) {
        bi = NULL;
        if (o.bi != NULL) {
            bi = o.bi->clone();
        }
        fInsertion = o.fInsertion;
        UErrorCode status = U_ZERO_ERROR;
        boundaries = new UVector32(status);
    }





Transliterator* BreakTransliterator::clone(void) const {
    return new BreakTransliterator(*this);
}




void BreakTransliterator::handleTransliterate(Replaceable& text, UTransPosition& offsets,
                                                    UBool isIncremental ) const {

        UErrorCode status = U_ZERO_ERROR;
        boundaries->removeAllElements();
        BreakTransliterator *nonConstThis = (BreakTransliterator *)this;
        nonConstThis->getBreakIterator(); 
        UnicodeString sText = replaceableAsString(text);
        bi->setText(sText);
        bi->preceding(offsets.start);

        
        

        int32_t boundary;
        for(boundary = bi->next(); boundary != UBRK_DONE && boundary < offsets.limit; boundary = bi->next()) {
            if (boundary == 0) continue;
            

            UChar32 cp = sText.char32At(boundary-1);
            int type = u_charType(cp);
            
            if ((U_MASK(type) & (U_GC_L_MASK | U_GC_M_MASK)) == 0) continue;

            cp = sText.char32At(boundary);
            type = u_charType(cp);
            
            if ((U_MASK(type) & (U_GC_L_MASK | U_GC_M_MASK)) == 0) continue;

            boundaries->addElement(boundary, status);
            
        }

        int delta = 0;
        int lastBoundary = 0;

        if (boundaries->size() != 0) { 
            delta = boundaries->size() * fInsertion.length();
            lastBoundary = boundaries->lastElementi();

            

            while (boundaries->size() > 0) {
                boundary = boundaries->popi();
                text.handleReplaceBetween(boundary, boundary, fInsertion);
            }
        }

        
        offsets.contextLimit += delta;
        offsets.limit += delta;
        offsets.start = isIncremental ? lastBoundary + delta : offsets.limit;

        
        
}




const UnicodeString &BreakTransliterator::getInsertion() const {
    return fInsertion;
}




void BreakTransliterator::setInsertion(const UnicodeString &insertion) {
    this->fInsertion = insertion;
}






BreakIterator *BreakTransliterator::getBreakIterator() {
    UErrorCode status = U_ZERO_ERROR;
    if (bi == NULL) {
        
        
        bi = BreakIterator::createWordInstance(Locale::getEnglish(), status);
    }
    return bi;
}








UnicodeString BreakTransliterator::replaceableAsString(Replaceable &r) {
    UnicodeString s;
    UnicodeString *rs = dynamic_cast<UnicodeString *>(&r);
    if (rs != NULL) {
        s = *rs;
    } else {
        r.extractBetween(0, r.length(), s);
    }
    return s;
}

U_NAMESPACE_END

#endif 
