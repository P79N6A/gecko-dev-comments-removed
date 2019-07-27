










#include "sfwdchit.h"
#include "unicode/ustring.h"
#include "unicode/unistr.h"
#include "uhash.h"
#include "cmemory.h"




const int32_t SimpleFwdCharIterator::kInvalidHashCode = 0;
const int32_t SimpleFwdCharIterator::kEmptyHashCode = 1;

#if 0 
SimpleFwdCharIterator::SimpleFwdCharIterator(const UnicodeString& s) {

    fHashCode = kInvalidHashCode;
    fLen = s.length();
    fStart = new UChar[fLen];
    if(fStart == NULL) {
        fBogus = TRUE;
    } else {
        fEnd = fStart+fLen;
        fCurrent = fStart;
        fBogus = FALSE;
        s.extract(0, fLen, fStart);          
    }
    
}
#endif

SimpleFwdCharIterator::SimpleFwdCharIterator(UChar *s, int32_t len, UBool adopt) {

    fHashCode = kInvalidHashCode;

    fLen = len==-1 ? u_strlen(s) : len;

    if(adopt == FALSE) {
        fStart = new UChar[fLen];
        if(fStart == NULL) {
            fBogus = TRUE;
        } else {
            uprv_memcpy(fStart, s, fLen);
            fEnd = fStart+fLen;
            fCurrent = fStart;
            fBogus = FALSE;
        }
    } else { 
        fCurrent = fStart = s;
        fEnd = fStart + fLen;
        fBogus = FALSE;
    }

}

SimpleFwdCharIterator::~SimpleFwdCharIterator() {
    delete[] fStart;
}

#if 0 
UBool SimpleFwdCharIterator::operator==(const ForwardCharacterIterator& that) const {
    if(this == &that) {
        return TRUE;
    }













    return FALSE;
}
#endif

int32_t SimpleFwdCharIterator::hashCode(void) const {
    if (fHashCode == kInvalidHashCode)
    {
        UHashTok key;
        key.pointer = fStart;
        ((SimpleFwdCharIterator *)this)->fHashCode = uhash_hashUChars(key);
    }
    return fHashCode;
}
        
UClassID SimpleFwdCharIterator::getDynamicClassID(void) const {
    return NULL;
}

UChar SimpleFwdCharIterator::nextPostInc(void) {
    if(fCurrent == fEnd) {
        return ForwardCharacterIterator::DONE;
    } else {
        return *(fCurrent)++;
    }
}
        
UChar32 SimpleFwdCharIterator::next32PostInc(void) {
    return ForwardCharacterIterator::DONE;
}
        
UBool SimpleFwdCharIterator::hasNext() {
    return fCurrent < fEnd;
}
