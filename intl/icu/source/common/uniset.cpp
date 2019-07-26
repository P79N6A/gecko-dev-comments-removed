









#include "unicode/utypes.h"
#include "unicode/parsepos.h"
#include "unicode/symtable.h"
#include "unicode/uniset.h"
#include "unicode/utf8.h"
#include "unicode/utf16.h"
#include "ruleiter.h"
#include "cmemory.h"
#include "cstring.h"
#include "patternprops.h"
#include "uelement.h"
#include "util.h"
#include "uvector.h"
#include "charstr.h"
#include "ustrfmt.h"
#include "uassert.h"
#include "bmpset.h"
#include "unisetspan.h"



#define SET_OPEN        ((UChar)0x005B) /*[*/
#define SET_CLOSE       ((UChar)0x005D) /*]*/
#define HYPHEN          ((UChar)0x002D) /*-*/
#define COMPLEMENT      ((UChar)0x005E) /*^*/
#define COLON           ((UChar)0x003A) /*:*/
#define BACKSLASH       ((UChar)0x005C) /*\*/
#define INTERSECTION    ((UChar)0x0026) /*&*/
#define UPPER_U         ((UChar)0x0055) /*U*/
#define LOWER_U         ((UChar)0x0075) /*u*/
#define OPEN_BRACE      ((UChar)123)    /*{*/
#define CLOSE_BRACE     ((UChar)125)    /*}*/
#define UPPER_P         ((UChar)0x0050) /*P*/
#define LOWER_P         ((UChar)0x0070) /*p*/
#define UPPER_N         ((UChar)78)     /*N*/
#define EQUALS          ((UChar)0x003D) /*=*/


#define UNICODESET_HIGH 0x0110000


#define UNICODESET_LOW 0x000000


#define START_EXTRA 16


#define GROW_EXTRA START_EXTRA

U_NAMESPACE_BEGIN

SymbolTable::~SymbolTable() {}

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(UnicodeSet)







static inline UChar32 pinCodePoint(UChar32& c) {
    if (c < UNICODESET_LOW) {
        c = UNICODESET_LOW;
    } else if (c > (UNICODESET_HIGH-1)) {
        c = (UNICODESET_HIGH-1);
    }
    return c;
}

















#ifdef DEBUG_MEM
#include <stdio.h>
static int32_t _dbgCount = 0;

static inline void _dbgct(UnicodeSet* set) {
    UnicodeString str;
    set->toPattern(str, TRUE);
    char buf[40];
    str.extract(0, 39, buf, "");
    printf("DEBUG UnicodeSet: ct 0x%08X; %d %s\n", set, ++_dbgCount, buf);
}

static inline void _dbgdt(UnicodeSet* set) {
    UnicodeString str;
    set->toPattern(str, TRUE);
    char buf[40];
    str.extract(0, 39, buf, "");
    printf("DEBUG UnicodeSet: dt 0x%08X; %d %s\n", set, --_dbgCount, buf);
}

#else

#define _dbgct(set)
#define _dbgdt(set)

#endif





static void U_CALLCONV cloneUnicodeString(UElement *dst, UElement *src) {
    dst->pointer = new UnicodeString(*(UnicodeString*)src->pointer);
}

static int8_t U_CALLCONV compareUnicodeString(UElement t1, UElement t2) {
    const UnicodeString &a = *(const UnicodeString*)t1.pointer;
    const UnicodeString &b = *(const UnicodeString*)t2.pointer;
    return a.compare(b);
}








UnicodeSet::UnicodeSet() :
    len(1), capacity(1 + START_EXTRA), list(0), bmpSet(0), buffer(0),
    bufferCapacity(0), patLen(0), pat(NULL), strings(NULL), stringSpan(NULL),
    fFlags(0)
{
    UErrorCode status = U_ZERO_ERROR;
    allocateStrings(status);
    if (U_FAILURE(status)) {
        return;
    }
    list = (UChar32*) uprv_malloc(sizeof(UChar32) * capacity);
    if(list!=NULL){
        list[0] = UNICODESET_HIGH;
    } else { 
        setToBogus();
        return;
    }
    _dbgct(this);
}








UnicodeSet::UnicodeSet(UChar32 start, UChar32 end) :
    len(1), capacity(1 + START_EXTRA), list(0), bmpSet(0), buffer(0),
    bufferCapacity(0), patLen(0), pat(NULL), strings(NULL), stringSpan(NULL),
    fFlags(0)
{
    UErrorCode status = U_ZERO_ERROR;
    allocateStrings(status);
    if (U_FAILURE(status)) {
        return;
    }
    list = (UChar32*) uprv_malloc(sizeof(UChar32) * capacity);
    if(list!=NULL){
        list[0] = UNICODESET_HIGH;
        complement(start, end);
    } else { 
        setToBogus();
        return;
    }
    _dbgct(this);
}




UnicodeSet::UnicodeSet(const UnicodeSet& o) :
    UnicodeFilter(o),
    len(0), capacity(o.isFrozen() ? o.len : o.len + GROW_EXTRA), list(0),
    bmpSet(0),
    buffer(0), bufferCapacity(0),
    patLen(0), pat(NULL), strings(NULL), stringSpan(NULL),
    fFlags(0)
{
    UErrorCode status = U_ZERO_ERROR;
    allocateStrings(status);
    if (U_FAILURE(status)) {
        return;
    }
    list = (UChar32*) uprv_malloc(sizeof(UChar32) * capacity);
    if(list!=NULL){
        *this = o;
    } else { 
        setToBogus();
        return;
    }
    _dbgct(this);
}


UnicodeSet::UnicodeSet(const UnicodeSet& o, UBool ) :
    UnicodeFilter(o),
    len(0), capacity(o.len + GROW_EXTRA), list(0),
    bmpSet(0),
    buffer(0), bufferCapacity(0),
    patLen(0), pat(NULL), strings(NULL), stringSpan(NULL),
    fFlags(0)
{
    UErrorCode status = U_ZERO_ERROR;
    allocateStrings(status);
    if (U_FAILURE(status)) {
        return;
    }
    list = (UChar32*) uprv_malloc(sizeof(UChar32) * capacity);
    if(list!=NULL){
        
        len = o.len;
        uprv_memcpy(list, o.list, len*sizeof(UChar32));
        if (strings != NULL && o.strings != NULL) {
            strings->assign(*o.strings, cloneUnicodeString, status);
        } else { 
            setToBogus();
            return;
        }
        if (o.pat) {
            setPattern(UnicodeString(o.pat, o.patLen));
        }
    } else { 
        setToBogus();
        return;
    }
    _dbgct(this);
}




UnicodeSet::~UnicodeSet() {
    _dbgdt(this); 
    uprv_free(list);
    delete bmpSet;
    if (buffer) {
        uprv_free(buffer);
    }
    delete strings;
    delete stringSpan;
    releasePattern();
}




UnicodeSet& UnicodeSet::operator=(const UnicodeSet& o) {
    if (this == &o) {
        return *this;
    }
    if (isFrozen()) {
        return *this;
    }
    if (o.isBogus()) {
        setToBogus();
        return *this;
    }
    UErrorCode ec = U_ZERO_ERROR;
    ensureCapacity(o.len, ec);
    if (U_FAILURE(ec)) {
        return *this; 
    }
    len = o.len;
    uprv_memcpy(list, o.list, len*sizeof(UChar32));
    if (o.bmpSet == NULL) {
        bmpSet = NULL;
    } else {
        bmpSet = new BMPSet(*o.bmpSet, list, len);
        if (bmpSet == NULL) { 
            setToBogus();
            return *this;
        }
    }
    if (strings != NULL && o.strings != NULL) {
        strings->assign(*o.strings, cloneUnicodeString, ec);
    } else { 
        setToBogus();
        return *this;
    }
    if (o.stringSpan == NULL) {
        stringSpan = NULL;
    } else {
        stringSpan = new UnicodeSetStringSpan(*o.stringSpan, *strings);
        if (stringSpan == NULL) { 
            setToBogus();
            return *this;
        }
    }
    releasePattern();
    if (o.pat) {
        setPattern(UnicodeString(o.pat, o.patLen));
    }
    return *this;
}






UnicodeFunctor* UnicodeSet::clone() const {
    return new UnicodeSet(*this);
}

UnicodeFunctor *UnicodeSet::cloneAsThawed() const {
    return new UnicodeSet(*this, TRUE);
}











UBool UnicodeSet::operator==(const UnicodeSet& o) const {
    if (len != o.len) return FALSE;
    for (int32_t i = 0; i < len; ++i) {
        if (list[i] != o.list[i]) return FALSE;
    }
    if (*strings != *o.strings) return FALSE;
    return TRUE;
}







int32_t UnicodeSet::hashCode(void) const {
    int32_t result = len;
    for (int32_t i = 0; i < len; ++i) {
        result *= 1000003;
        result += list[i];
    }
    return result;
}












int32_t UnicodeSet::size(void) const {
    int32_t n = 0;
    int32_t count = getRangeCount();
    for (int32_t i = 0; i < count; ++i) {
        n += getRangeEnd(i) - getRangeStart(i) + 1;
    }
    return n + strings->size();
}






UBool UnicodeSet::isEmpty(void) const {
    return len == 1 && strings->size() == 0;
}






UBool UnicodeSet::contains(UChar32 c) const {
    
    
    
    
    
    
    
    if (bmpSet != NULL) {
        return bmpSet->contains(c);
    }
    if (stringSpan != NULL) {
        return stringSpan->contains(c);
    }
    if (c >= UNICODESET_HIGH) { 
        return FALSE;
    }
    int32_t i = findCodePoint(c);
    return (UBool)(i & 1); 
}










int32_t UnicodeSet::findCodePoint(UChar32 c) const {
    









    
    
    if (c < list[0])
        return 0;
    
    
    int32_t lo = 0;
    int32_t hi = len - 1;
    if (lo >= hi || c >= list[hi-1])
        return hi;
    
    
    for (;;) {
        int32_t i = (lo + hi) >> 1;
        if (i == lo) {
            break; 
        } else if (c < list[i]) {
            hi = i;
        } else {
            lo = i;
        }
    }
    return hi;
}








UBool UnicodeSet::contains(UChar32 start, UChar32 end) const {
    
    
    
    
    int32_t i = findCodePoint(start);
    return ((i & 1) != 0 && end < list[i]);
}







UBool UnicodeSet::contains(const UnicodeString& s) const {
    if (s.length() == 0) return FALSE;
    int32_t cp = getSingleCP(s);
    if (cp < 0) {
        return strings->contains((void*) &s);
    } else {
        return contains((UChar32) cp);
    }
}







UBool UnicodeSet::containsAll(const UnicodeSet& c) const {
    
    
    
    int32_t n = c.getRangeCount();
    for (int i=0; i<n; ++i) {
        if (!contains(c.getRangeStart(i), c.getRangeEnd(i))) {
            return FALSE;
        }
    }
    if (!strings->containsAll(*c.strings)) return FALSE;
    return TRUE;
}







UBool UnicodeSet::containsAll(const UnicodeString& s) const {
    return (UBool)(span(s.getBuffer(), s.length(), USET_SPAN_CONTAINED) ==
                   s.length());
}








UBool UnicodeSet::containsNone(UChar32 start, UChar32 end) const {
    
    
    
    
    int32_t i = findCodePoint(start);
    return ((i & 1) == 0 && end < list[i]);
}







UBool UnicodeSet::containsNone(const UnicodeSet& c) const {
    
    
    
    int32_t n = c.getRangeCount();
    for (int32_t i=0; i<n; ++i) {
        if (!containsNone(c.getRangeStart(i), c.getRangeEnd(i))) {
            return FALSE;
        }
    }
    if (!strings->containsNone(*c.strings)) return FALSE;
    return TRUE;
}







UBool UnicodeSet::containsNone(const UnicodeString& s) const {
    return (UBool)(span(s.getBuffer(), s.length(), USET_SPAN_NOT_CONTAINED) ==
                   s.length());
}






UBool UnicodeSet::matchesIndexValue(uint8_t v) const {
    







    int32_t i;
    int32_t rangeCount=getRangeCount();
    for (i=0; i<rangeCount; ++i) {
        UChar32 low = getRangeStart(i);
        UChar32 high = getRangeEnd(i);
        if ((low & ~0xFF) == (high & ~0xFF)) {
            if ((low & 0xFF) <= v && v <= (high & 0xFF)) {
                return TRUE;
            }
        } else if ((low & 0xFF) <= v || v <= (high & 0xFF)) {
            return TRUE;
        }
    }
    if (strings->size() != 0) {
        for (i=0; i<strings->size(); ++i) {
            const UnicodeString& s = *(const UnicodeString*)strings->elementAt(i);
            
            
            
            
            
            UChar32 c = s.char32At(0);
            if ((c & 0xFF) == v) {
                return TRUE;
            }
        }
    }
    return FALSE;
}





UMatchDegree UnicodeSet::matches(const Replaceable& text,
                                 int32_t& offset,
                                 int32_t limit,
                                 UBool incremental) {
    if (offset == limit) {
        
        
        
        if (contains(U_ETHER)) {
            return incremental ? U_PARTIAL_MATCH : U_MATCH;
        } else {
            return U_MISMATCH;
        }
    } else {
        if (strings->size() != 0) { 

            
            

            
            
            

            int32_t i;
            UBool forward = offset < limit;

            
            
            
            UChar firstChar = text.charAt(offset);

            
            
            int32_t highWaterLength = 0;

            for (i=0; i<strings->size(); ++i) {
                const UnicodeString& trial = *(const UnicodeString*)strings->elementAt(i);

                
                
                
                

                UChar c = trial.charAt(forward ? 0 : trial.length() - 1);

                
                
                if (forward && c > firstChar) break;
                if (c != firstChar) continue;

                int32_t matchLen = matchRest(text, offset, limit, trial);

                if (incremental) {
                    int32_t maxLen = forward ? limit-offset : offset-limit;
                    if (matchLen == maxLen) {
                        
                        return U_PARTIAL_MATCH;
                    }
                }

                if (matchLen == trial.length()) {
                    
                    if (matchLen > highWaterLength) {
                        highWaterLength = matchLen;
                    }
                    
                    
                    if (forward && matchLen < highWaterLength) {
                        break;
                    }
                    continue;
                }
            }

            
            
            if (highWaterLength != 0) {
                offset += forward ? highWaterLength : -highWaterLength;
                return U_MATCH;
            }
        }
        return UnicodeFilter::matches(text, offset, limit, incremental);
    }
}






















int32_t UnicodeSet::matchRest(const Replaceable& text,
                              int32_t start, int32_t limit,
                              const UnicodeString& s) {
    int32_t i;
    int32_t maxLen;
    int32_t slen = s.length();
    if (start < limit) {
        maxLen = limit - start;
        if (maxLen > slen) maxLen = slen;
        for (i = 1; i < maxLen; ++i) {
            if (text.charAt(start + i) != s.charAt(i)) return 0;
        }
    } else {
        maxLen = start - limit;
        if (maxLen > slen) maxLen = slen;
        --slen; 
        for (i = 1; i < maxLen; ++i) {
            if (text.charAt(start - i) != s.charAt(slen - i)) return 0;
        }
    }
    return maxLen;
}




void UnicodeSet::addMatchSetTo(UnicodeSet& toUnionTo) const {
    toUnionTo.addAll(*this);
}








int32_t UnicodeSet::indexOf(UChar32 c) const {
    if (c < MIN_VALUE || c > MAX_VALUE) {
        return -1;
    }
    int32_t i = 0;
    int32_t n = 0;
    for (;;) {
        UChar32 start = list[i++];
        if (c < start) {
            return -1;
        }
        UChar32 limit = list[i++];
        if (c < limit) {
            return n + c - start;
        }
        n += limit - start;
    }
}









UChar32 UnicodeSet::charAt(int32_t index) const {
    if (index >= 0) {
        
        
        
        int32_t len2 = len & ~1;
        for (int32_t i=0; i < len2;) {
            UChar32 start = list[i++];
            int32_t count = list[i++] - start;
            if (index < count) {
                return (UChar32)(start + index);
            }
            index -= count;
        }
    }
    return (UChar32)-1;
}









UnicodeSet& UnicodeSet::set(UChar32 start, UChar32 end) {
    clear();
    complement(start, end);
    return *this;
}












UnicodeSet& UnicodeSet::add(UChar32 start, UChar32 end) {
    if (pinCodePoint(start) < pinCodePoint(end)) {
        UChar32 range[3] = { start, end+1, UNICODESET_HIGH };
        add(range, 2, 0);
    } else if (start == end) {
        add(start);
    }
    return *this;
}



#ifdef DEBUG_US_ADD
#include <stdio.h>
void dump(UChar32 c) {
    if (c <= 0xFF) {
        printf("%c", (char)c);
    } else {
        printf("U+%04X", c);
    }
}
void dump(const UChar32* list, int32_t len) {
    printf("[");
    for (int32_t i=0; i<len; ++i) {
        if (i != 0) printf(", ");
        dump(list[i]);
    }
    printf("]");
}
#endif






UnicodeSet& UnicodeSet::add(UChar32 c) {
    
    
    
    int32_t i = findCodePoint(pinCodePoint(c));

    
    if ((i & 1) != 0  || isFrozen() || isBogus()) return *this;

    
    

    
    

    
    
    

    

#ifdef DEBUG_US_ADD
    printf("Add of ");
    dump(c);
    printf(" found at %d", i);
    printf(": ");
    dump(list, len);
    printf(" => ");
#endif

    if (c == list[i]-1) {
        
        list[i] = c;
        
        if (c == (UNICODESET_HIGH - 1)) {
            UErrorCode status = U_ZERO_ERROR;
            ensureCapacity(len+1, status);
            if (U_FAILURE(status)) {
                return *this; 
            }
            list[len++] = UNICODESET_HIGH;
        }
        if (i > 0 && c == list[i-1]) {
            

            
            
            

            
            
            
            UChar32* dst = list + i - 1;
            UChar32* src = dst + 2;
            UChar32* srclimit = list + len;
            while (src < srclimit) *(dst++) = *(src++);

            len -= 2;
        }
    }

    else if (i > 0 && c == list[i-1]) {
        
        list[i-1]++;
        
    }

    else {
        
        


        
        
        

        
        
        

        UErrorCode status = U_ZERO_ERROR;
        ensureCapacity(len+2, status);
        if (U_FAILURE(status)) {
            return *this; 
        }

        
        
        
        UChar32* src = list + len;
        UChar32* dst = src + 2;
        UChar32* srclimit = list + i;
        while (src > srclimit) *(--dst) = *(--src);

        list[i] = c;
        list[i+1] = c+1;
        len += 2;
    }

#ifdef DEBUG_US_ADD
    dump(list, len);
    printf("\n");

    for (i=1; i<len; ++i) {
        if (list[i] <= list[i-1]) {
            
            printf("ERROR: list has been corrupted\n");
            exit(1);
        }
    }
#endif

    releasePattern();
    return *this;
}










UnicodeSet& UnicodeSet::add(const UnicodeString& s) {
    if (s.length() == 0 || isFrozen() || isBogus()) return *this;
    int32_t cp = getSingleCP(s);
    if (cp < 0) {
        if (!strings->contains((void*) &s)) {
            _add(s);
            releasePattern();
        }
    } else {
        add((UChar32)cp);
    }
    return *this;
}






void UnicodeSet::_add(const UnicodeString& s) {
    if (isFrozen() || isBogus()) {
        return;
    }
    UnicodeString* t = new UnicodeString(s);
    if (t == NULL) { 
        setToBogus();
        return;
    }
    UErrorCode ec = U_ZERO_ERROR;
    strings->sortedInsert(t, compareUnicodeString, ec);
    if (U_FAILURE(ec)) {
        setToBogus();
        delete t;
    }
}






int32_t UnicodeSet::getSingleCP(const UnicodeString& s) {
    
    
    
    if (s.length() > 2) return -1;
    if (s.length() == 1) return s.charAt(0);

    
    UChar32 cp = s.char32At(0);
    if (cp > 0xFFFF) { 
        return cp;
    }
    return -1;
}







UnicodeSet& UnicodeSet::addAll(const UnicodeString& s) {
    UChar32 cp;
    for (int32_t i = 0; i < s.length(); i += U16_LENGTH(cp)) {
        cp = s.char32At(i);
        add(cp);
    }
    return *this;
}







UnicodeSet& UnicodeSet::retainAll(const UnicodeString& s) {
    UnicodeSet set;
    set.addAll(s);
    retainAll(set);
    return *this;
}







UnicodeSet& UnicodeSet::complementAll(const UnicodeString& s) {
    UnicodeSet set;
    set.addAll(s);
    complementAll(set);
    return *this;
}







UnicodeSet& UnicodeSet::removeAll(const UnicodeString& s) {
    UnicodeSet set;
    set.addAll(s);
    removeAll(set);
    return *this;
}

UnicodeSet& UnicodeSet::removeAllStrings() {
    strings->removeAllElements();
    return *this;
}








UnicodeSet* U_EXPORT2 UnicodeSet::createFrom(const UnicodeString& s) {
    UnicodeSet *set = new UnicodeSet();
    if (set != NULL) { 
        set->add(s);
    }
    return set;
}







UnicodeSet* U_EXPORT2 UnicodeSet::createFromAll(const UnicodeString& s) {
    UnicodeSet *set = new UnicodeSet();
    if (set != NULL) { 
        set->addAll(s);
    }
    return set;
}











UnicodeSet& UnicodeSet::retain(UChar32 start, UChar32 end) {
    if (pinCodePoint(start) <= pinCodePoint(end)) {
        UChar32 range[3] = { start, end+1, UNICODESET_HIGH };
        retain(range, 2, 0);
    } else {
        clear();
    }
    return *this;
}

UnicodeSet& UnicodeSet::retain(UChar32 c) {
    return retain(c, c);
}












UnicodeSet& UnicodeSet::remove(UChar32 start, UChar32 end) {
    if (pinCodePoint(start) <= pinCodePoint(end)) {
        UChar32 range[3] = { start, end+1, UNICODESET_HIGH };
        retain(range, 2, 2);
    }
    return *this;
}






UnicodeSet& UnicodeSet::remove(UChar32 c) {
    return remove(c, c);
}








UnicodeSet& UnicodeSet::remove(const UnicodeString& s) {
    if (s.length() == 0 || isFrozen() || isBogus()) return *this;
    int32_t cp = getSingleCP(s);
    if (cp < 0) {
        strings->removeElement((void*) &s);
        releasePattern();
    } else {
        remove((UChar32)cp, (UChar32)cp);
    }
    return *this;
}












UnicodeSet& UnicodeSet::complement(UChar32 start, UChar32 end) {
    if (isFrozen() || isBogus()) {
        return *this;
    }
    if (pinCodePoint(start) <= pinCodePoint(end)) {
        UChar32 range[3] = { start, end+1, UNICODESET_HIGH };
        exclusiveOr(range, 2, 0);
    }
    releasePattern();
    return *this;
}

UnicodeSet& UnicodeSet::complement(UChar32 c) {
    return complement(c, c);
}





UnicodeSet& UnicodeSet::complement(void) {
    if (isFrozen() || isBogus()) {
        return *this;
    }
    UErrorCode status = U_ZERO_ERROR;
    if (list[0] == UNICODESET_LOW) {
        ensureBufferCapacity(len-1, status);
        if (U_FAILURE(status)) {
            return *this;
        }
        uprv_memcpy(buffer, list + 1, (len-1)*sizeof(UChar32));
        --len;
    } else {
        ensureBufferCapacity(len+1, status);
        if (U_FAILURE(status)) {
            return *this;
        }
        uprv_memcpy(buffer + 1, list, len*sizeof(UChar32));
        buffer[0] = UNICODESET_LOW;
        ++len;
    }
    swapBuffers();
    releasePattern();
    return *this;
}









UnicodeSet& UnicodeSet::complement(const UnicodeString& s) {
    if (s.length() == 0 || isFrozen() || isBogus()) return *this;
    int32_t cp = getSingleCP(s);
    if (cp < 0) {
        if (strings->contains((void*) &s)) {
            strings->removeElement((void*) &s);
        } else {
            _add(s);
        }
        releasePattern();
    } else {
        complement((UChar32)cp, (UChar32)cp);
    }
    return *this;
}











UnicodeSet& UnicodeSet::addAll(const UnicodeSet& c) {
    if ( c.len>0 && c.list!=NULL ) {
        add(c.list, c.len, 0);
    }

    
    if ( c.strings!=NULL ) {
        for (int32_t i=0; i<c.strings->size(); ++i) {
            const UnicodeString* s = (const UnicodeString*)c.strings->elementAt(i);
            if (!strings->contains((void*) s)) {
                _add(*s);
            }
        }
    }
    return *this;
}










UnicodeSet& UnicodeSet::retainAll(const UnicodeSet& c) {
    if (isFrozen() || isBogus()) {
        return *this;
    }
    retain(c.list, c.len, 0);
    strings->retainAll(*c.strings);
    return *this;
}










UnicodeSet& UnicodeSet::removeAll(const UnicodeSet& c) {
    if (isFrozen() || isBogus()) {
        return *this;
    }
    retain(c.list, c.len, 2);
    strings->removeAll(*c.strings);
    return *this;
}









UnicodeSet& UnicodeSet::complementAll(const UnicodeSet& c) {
    if (isFrozen() || isBogus()) {
        return *this;
    }
    exclusiveOr(c.list, c.len, 0);

    for (int32_t i=0; i<c.strings->size(); ++i) {
        void* e = c.strings->elementAt(i);
        if (!strings->removeElement(e)) {
            _add(*(const UnicodeString*)e);
        }
    }
    return *this;
}





UnicodeSet& UnicodeSet::clear(void) {
    if (isFrozen()) {
        return *this;
    }
    if (list != NULL) {
        list[0] = UNICODESET_HIGH;
    }
    len = 1;
    releasePattern();
    if (strings != NULL) {
        strings->removeAllElements();
    }
    if (list != NULL && strings != NULL) {
        
        fFlags = 0;
    }
    return *this;
}







int32_t UnicodeSet::getRangeCount() const {
    return len/2;
}







UChar32 UnicodeSet::getRangeStart(int32_t index) const {
    return list[index*2];
}







UChar32 UnicodeSet::getRangeEnd(int32_t index) const {
    return list[index*2 + 1] - 1;
}

int32_t UnicodeSet::getStringCount() const {
    return strings->size();
}

const UnicodeString* UnicodeSet::getString(int32_t index) const {
    return (const UnicodeString*) strings->elementAt(index);
}





UnicodeSet& UnicodeSet::compact() {
    if (isFrozen() || isBogus()) {
        return *this;
    }
    
    if (buffer != NULL) {
        uprv_free(buffer);
        buffer = NULL;
    }
    if (len < capacity) {
        
        
        int32_t newCapacity = len + (len == 0);
        UChar32* temp = (UChar32*) uprv_realloc(list, sizeof(UChar32) * newCapacity);
        if (temp) {
            list = temp;
            capacity = newCapacity;
        }
        
        
    }
    return *this;
}

int32_t UnicodeSet::serialize(uint16_t *dest, int32_t destCapacity, UErrorCode& ec) const {
    int32_t bmpLength, length, destLength;

    if (U_FAILURE(ec)) {
        return 0;
    }

    if (destCapacity<0 || (destCapacity>0 && dest==NULL)) {
        ec=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    
    length=this->len-1; 
    
    if (length==0) {
        
        if (destCapacity>0) {
            *dest=0;
        } else {
            ec=U_BUFFER_OVERFLOW_ERROR;
        }
        return 1;
    }
    

    if (this->list[length-1]<=0xffff) {
        
        bmpLength=length;
    } else if (this->list[0]>=0x10000) {
        
        bmpLength=0;
        length*=2;
    } else {
        
        for (bmpLength=0; bmpLength<length && this->list[bmpLength]<=0xffff; ++bmpLength) {}
        length=bmpLength+2*(length-bmpLength);
    }

    
    if (length>0x7fff) {
        
        ec=U_INDEX_OUTOFBOUNDS_ERROR;
        return 0;
    }

    





    destLength=length+((length>bmpLength)?2:1);
    if (destLength<=destCapacity) {
        const UChar32 *p;
        int32_t i;

        *dest=(uint16_t)length;
        if (length>bmpLength) {
            *dest|=0x8000;
            *++dest=(uint16_t)bmpLength;
        }
        ++dest;

        
        p=this->list;
        for (i=0; i<bmpLength; ++i) {
            *dest++=(uint16_t)*p++;
        }

        
        for (; i<length; i+=2) {
            *dest++=(uint16_t)(*p>>16);
            *dest++=(uint16_t)*p++;
        }
    } else {
        ec=U_BUFFER_OVERFLOW_ERROR;
    }
    return destLength;
}








UBool UnicodeSet::allocateStrings(UErrorCode &status) {
    if (U_FAILURE(status)) {
        return FALSE;
    }
    strings = new UVector(uprv_deleteUObject,
                          uhash_compareUnicodeString, 1, status);
    if (strings == NULL) { 
        status = U_MEMORY_ALLOCATION_ERROR;
        return FALSE;
    }
    if (U_FAILURE(status)) {
        delete strings;
        strings = NULL;
        return FALSE;
    } 
    return TRUE;
}

void UnicodeSet::ensureCapacity(int32_t newLen, UErrorCode& ec) {
    if (newLen <= capacity)
        return;
    UChar32* temp = (UChar32*) uprv_realloc(list, sizeof(UChar32) * (newLen + GROW_EXTRA));
    if (temp == NULL) {
        ec = U_MEMORY_ALLOCATION_ERROR;
        setToBogus();
        return;
    }
    list = temp;
    capacity = newLen + GROW_EXTRA;
    
}

void UnicodeSet::ensureBufferCapacity(int32_t newLen, UErrorCode& ec) {
    if (buffer != NULL && newLen <= bufferCapacity)
        return;
    UChar32* temp = (UChar32*) uprv_realloc(buffer, sizeof(UChar32) * (newLen + GROW_EXTRA));
    if (temp == NULL) {
        ec = U_MEMORY_ALLOCATION_ERROR;
        setToBogus();
        return;
    }
    buffer = temp;
    bufferCapacity = newLen + GROW_EXTRA;
    
}




void UnicodeSet::swapBuffers(void) {
    
    UChar32* temp = list;
    list = buffer;
    buffer = temp;

    int32_t c = capacity;
    capacity = bufferCapacity;
    bufferCapacity = c;
}

void UnicodeSet::setToBogus() {
    clear(); 
    fFlags = kIsBogus;
}





static inline UChar32 max(UChar32 a, UChar32 b) {
    return (a > b) ? a : b;
}




void UnicodeSet::exclusiveOr(const UChar32* other, int32_t otherLen, int8_t polarity) {
    if (isFrozen() || isBogus()) {
        return;
    }
    UErrorCode status = U_ZERO_ERROR;
    ensureBufferCapacity(len + otherLen, status);
    if (U_FAILURE(status)) {
        return;
    }

    int32_t i = 0, j = 0, k = 0;
    UChar32 a = list[i++];
    UChar32 b;
    if (polarity == 1 || polarity == 2) {
        b = UNICODESET_LOW;
        if (other[j] == UNICODESET_LOW) { 
            ++j;
            b = other[j];
        }
    } else {
        b = other[j++];
    }
    
    
    for (;;) {
        if (a < b) {
            buffer[k++] = a;
            a = list[i++];
        } else if (b < a) {
            buffer[k++] = b;
            b = other[j++];
        } else if (a != UNICODESET_HIGH) { 
            
            a = list[i++];
            b = other[j++];
        } else { 
            buffer[k++] = UNICODESET_HIGH;
            len = k;
            break;
        }
    }
    swapBuffers();
    releasePattern();
}






void UnicodeSet::add(const UChar32* other, int32_t otherLen, int8_t polarity) {
    if (isFrozen() || isBogus() || other==NULL) {
        return;
    }
    UErrorCode status = U_ZERO_ERROR;
    ensureBufferCapacity(len + otherLen, status);
    if (U_FAILURE(status)) {
        return;
    }

    int32_t i = 0, j = 0, k = 0;
    UChar32 a = list[i++];
    UChar32 b = other[j++];
    
    
    for (;;) {
        switch (polarity) {
          case 0: 
            if (a < b) { 
                
                if (k > 0 && a <= buffer[k-1]) {
                    
                    a = max(list[i], buffer[--k]);
                } else {
                    
                    buffer[k++] = a;
                    a = list[i];
                }
                i++; 
                polarity ^= 1;
            } else if (b < a) { 
                if (k > 0 && b <= buffer[k-1]) {
                    b = max(other[j], buffer[--k]);
                } else {
                    buffer[k++] = b;
                    b = other[j];
                }
                j++;
                polarity ^= 2;
            } else { 
                if (a == UNICODESET_HIGH) goto loop_end;
                
                
                if (k > 0 && a <= buffer[k-1]) {
                    a = max(list[i], buffer[--k]);
                } else {
                    
                    buffer[k++] = a;
                    a = list[i];
                }
                i++;
                polarity ^= 1;
                b = other[j++];
                polarity ^= 2;
            }
            break;
          case 3: 
            if (b <= a) { 
                if (a == UNICODESET_HIGH) goto loop_end;
                buffer[k++] = a;
            } else { 
                if (b == UNICODESET_HIGH) goto loop_end;
                buffer[k++] = b;
            }
            a = list[i++];
            polarity ^= 1;   
            b = other[j++];
            polarity ^= 2;
            break;
          case 1: 
            if (a < b) { 
                buffer[k++] = a; a = list[i++]; polarity ^= 1;
            } else if (b < a) { 
                b = other[j++];
                polarity ^= 2;
            } else { 
                if (a == UNICODESET_HIGH) goto loop_end;
                a = list[i++];
                polarity ^= 1;
                b = other[j++];
                polarity ^= 2;
            }
            break;
          case 2: 
            if (b < a) { 
                buffer[k++] = b;
                b = other[j++];
                polarity ^= 2;
            } else  if (a < b) { 
                a = list[i++];
                polarity ^= 1;
            } else { 
                if (a == UNICODESET_HIGH) goto loop_end;
                a = list[i++];
                polarity ^= 1;
                b = other[j++];
                polarity ^= 2;
            }
            break;
        }
    }
 loop_end:
    buffer[k++] = UNICODESET_HIGH;    
    len = k;
    swapBuffers();
    releasePattern();
}






void UnicodeSet::retain(const UChar32* other, int32_t otherLen, int8_t polarity) {
    if (isFrozen() || isBogus()) {
        return;
    }
    UErrorCode status = U_ZERO_ERROR;
    ensureBufferCapacity(len + otherLen, status);
    if (U_FAILURE(status)) {
        return;
    }

    int32_t i = 0, j = 0, k = 0;
    UChar32 a = list[i++];
    UChar32 b = other[j++];
    
    
    for (;;) {
        switch (polarity) {
          case 0: 
            if (a < b) { 
                a = list[i++];
                polarity ^= 1;
            } else if (b < a) { 
                b = other[j++];
                polarity ^= 2;
            } else { 
                if (a == UNICODESET_HIGH) goto loop_end;
                buffer[k++] = a;
                a = list[i++];
                polarity ^= 1;
                b = other[j++];
                polarity ^= 2;
            }
            break;
          case 3: 
            if (a < b) { 
                buffer[k++] = a;
                a = list[i++];
                polarity ^= 1;
            } else if (b < a) { 
                buffer[k++] = b;
                b = other[j++];
                polarity ^= 2;
            } else { 
                if (a == UNICODESET_HIGH) goto loop_end;
                buffer[k++] = a;
                a = list[i++];
                polarity ^= 1;
                b = other[j++];
                polarity ^= 2;
            }
            break;
          case 1: 
            if (a < b) { 
                a = list[i++];
                polarity ^= 1;
            } else if (b < a) { 
                buffer[k++] = b;
                b = other[j++];
                polarity ^= 2;
            } else { 
                if (a == UNICODESET_HIGH) goto loop_end;
                a = list[i++];
                polarity ^= 1;
                b = other[j++];
                polarity ^= 2;
            }
            break;
          case 2: 
            if (b < a) { 
                b = other[j++];
                polarity ^= 2;
            } else  if (a < b) { 
                buffer[k++] = a;
                a = list[i++];
                polarity ^= 1;
            } else { 
                if (a == UNICODESET_HIGH) goto loop_end;
                a = list[i++];
                polarity ^= 1;
                b = other[j++];
                polarity ^= 2;
            }
            break;
        }
    }
 loop_end:
    buffer[k++] = UNICODESET_HIGH;    
    len = k;
    swapBuffers();
    releasePattern();
}





void UnicodeSet::_appendToPat(UnicodeString& buf, const UnicodeString& s, UBool
escapeUnprintable) {
    UChar32 cp;
    for (int32_t i = 0; i < s.length(); i += U16_LENGTH(cp)) {
        _appendToPat(buf, cp = s.char32At(i), escapeUnprintable);
    }
}





void UnicodeSet::_appendToPat(UnicodeString& buf, UChar32 c, UBool
escapeUnprintable) {
    if (escapeUnprintable && ICU_Utility::isUnprintable(c)) {
        
        
        if (ICU_Utility::escapeUnprintable(buf, c)) {
            return;
        }
    }
    
    switch (c) {
    case SET_OPEN:
    case SET_CLOSE:
    case HYPHEN:
    case COMPLEMENT:
    case INTERSECTION:
    case BACKSLASH:
    case OPEN_BRACE:
    case CLOSE_BRACE:
    case COLON:
    case SymbolTable::SYMBOL_REF:
        buf.append(BACKSLASH);
        break;
    default:
        
        if (PatternProps::isWhiteSpace(c)) {
            buf.append(BACKSLASH);
        }
        break;
    }
    buf.append(c);
}






UnicodeString& UnicodeSet::_toPattern(UnicodeString& result,
                                      UBool escapeUnprintable) const
{
    if (pat != NULL) {
        int32_t i;
        int32_t backslashCount = 0;
        for (i=0; i<patLen; ) {
            UChar32 c;
            U16_NEXT(pat, i, patLen, c);
            if (escapeUnprintable && ICU_Utility::isUnprintable(c)) {
                
                
                
                
                if ((backslashCount % 2) == 1) {
                    result.truncate(result.length() - 1);
                }
                ICU_Utility::escapeUnprintable(result, c);
                backslashCount = 0;
            } else {
                result.append(c);
                if (c == BACKSLASH) {
                    ++backslashCount;
                } else {
                    backslashCount = 0;
                }
            }
        }
        return result;
    }

    return _generatePattern(result, escapeUnprintable);
}






UnicodeString& UnicodeSet::toPattern(UnicodeString& result,
                                     UBool escapeUnprintable) const
{
    result.truncate(0);
    return _toPattern(result, escapeUnprintable);
}






UnicodeString& UnicodeSet::_generatePattern(UnicodeString& result,
                                            UBool escapeUnprintable) const
{
    result.append(SET_OPEN);











    int32_t count = getRangeCount();

    
    
    
    if (count > 1 &&
        getRangeStart(0) == MIN_VALUE &&
        getRangeEnd(count-1) == MAX_VALUE) {

        
        result.append(COMPLEMENT);

        for (int32_t i = 1; i < count; ++i) {
            UChar32 start = getRangeEnd(i-1)+1;
            UChar32 end = getRangeStart(i)-1;
            _appendToPat(result, start, escapeUnprintable);
            if (start != end) {
                if ((start+1) != end) {
                    result.append(HYPHEN);
                }
                _appendToPat(result, end, escapeUnprintable);
            }
        }
    }

    
    else {
        for (int32_t i = 0; i < count; ++i) {
            UChar32 start = getRangeStart(i);
            UChar32 end = getRangeEnd(i);
            _appendToPat(result, start, escapeUnprintable);
            if (start != end) {
                if ((start+1) != end) {
                    result.append(HYPHEN);
                }
                _appendToPat(result, end, escapeUnprintable);
            }
        }
    }

    for (int32_t i = 0; i<strings->size(); ++i) {
        result.append(OPEN_BRACE);
        _appendToPat(result,
                     *(const UnicodeString*) strings->elementAt(i),
                     escapeUnprintable);
        result.append(CLOSE_BRACE);
    }
    return result.append(SET_CLOSE);
}




void UnicodeSet::releasePattern() {
    if (pat) {
        uprv_free(pat);
        pat = NULL;
        patLen = 0;
    }
}




void UnicodeSet::setPattern(const UnicodeString& newPat) {
    releasePattern();
    int32_t newPatLen = newPat.length();
    pat = (UChar *)uprv_malloc((newPatLen + 1) * sizeof(UChar));
    if (pat) {
        patLen = newPatLen;
        newPat.extractBetween(0, patLen, pat);
        pat[patLen] = 0;
    }
    
    
}

UnicodeFunctor *UnicodeSet::freeze() {
    if(!isFrozen() && !isBogus()) {
        
        
        

        
        if (buffer != NULL) {
            uprv_free(buffer);
            buffer = NULL;
        }
        if (capacity > (len + GROW_EXTRA)) {
            
            
            capacity = len + (len == 0);
            list = (UChar32*) uprv_realloc(list, sizeof(UChar32) * capacity);
            if (list == NULL) { 
                setToBogus();
                return this;
            }
        }

        
        if (!strings->isEmpty()) {
            stringSpan = new UnicodeSetStringSpan(*this, *strings, UnicodeSetStringSpan::ALL);
            if (stringSpan != NULL && !stringSpan->needsStringSpanUTF16()) {
                
                
                
                
                
                delete stringSpan;
                stringSpan = NULL;
            }
        }
        if (stringSpan == NULL) {
            
            bmpSet=new BMPSet(list, len);
            if (bmpSet == NULL) { 
                setToBogus();
            }
        }
    }
    return this;
}

int32_t UnicodeSet::span(const UChar *s, int32_t length, USetSpanCondition spanCondition) const {
    if(length>0 && bmpSet!=NULL) {
        return (int32_t)(bmpSet->span(s, s+length, spanCondition)-s);
    }
    if(length<0) {
        length=u_strlen(s);
    }
    if(length==0) {
        return 0;
    }
    if(stringSpan!=NULL) {
        return stringSpan->span(s, length, spanCondition);
    } else if(!strings->isEmpty()) {
        uint32_t which= spanCondition==USET_SPAN_NOT_CONTAINED ?
                            UnicodeSetStringSpan::FWD_UTF16_NOT_CONTAINED :
                            UnicodeSetStringSpan::FWD_UTF16_CONTAINED;
        UnicodeSetStringSpan strSpan(*this, *strings, which);
        if(strSpan.needsStringSpanUTF16()) {
            return strSpan.span(s, length, spanCondition);
        }
    }

    if(spanCondition!=USET_SPAN_NOT_CONTAINED) {
        spanCondition=USET_SPAN_CONTAINED;  
    }

    UChar32 c;
    int32_t start=0, prev=0;
    do {
        U16_NEXT(s, start, length, c);
        if(spanCondition!=contains(c)) {
            break;
        }
    } while((prev=start)<length);
    return prev;
}

int32_t UnicodeSet::spanBack(const UChar *s, int32_t length, USetSpanCondition spanCondition) const {
    if(length>0 && bmpSet!=NULL) {
        return (int32_t)(bmpSet->spanBack(s, s+length, spanCondition)-s);
    }
    if(length<0) {
        length=u_strlen(s);
    }
    if(length==0) {
        return 0;
    }
    if(stringSpan!=NULL) {
        return stringSpan->spanBack(s, length, spanCondition);
    } else if(!strings->isEmpty()) {
        uint32_t which= spanCondition==USET_SPAN_NOT_CONTAINED ?
                            UnicodeSetStringSpan::BACK_UTF16_NOT_CONTAINED :
                            UnicodeSetStringSpan::BACK_UTF16_CONTAINED;
        UnicodeSetStringSpan strSpan(*this, *strings, which);
        if(strSpan.needsStringSpanUTF16()) {
            return strSpan.spanBack(s, length, spanCondition);
        }
    }

    if(spanCondition!=USET_SPAN_NOT_CONTAINED) {
        spanCondition=USET_SPAN_CONTAINED;  
    }

    UChar32 c;
    int32_t prev=length;
    do {
        U16_PREV(s, 0, length, c);
        if(spanCondition!=contains(c)) {
            break;
        }
    } while((prev=length)>0);
    return prev;
}

int32_t UnicodeSet::spanUTF8(const char *s, int32_t length, USetSpanCondition spanCondition) const {
    if(length>0 && bmpSet!=NULL) {
        const uint8_t *s0=(const uint8_t *)s;
        return (int32_t)(bmpSet->spanUTF8(s0, length, spanCondition)-s0);
    }
    if(length<0) {
        length=(int32_t)uprv_strlen(s);
    }
    if(length==0) {
        return 0;
    }
    if(stringSpan!=NULL) {
        return stringSpan->spanUTF8((const uint8_t *)s, length, spanCondition);
    } else if(!strings->isEmpty()) {
        uint32_t which= spanCondition==USET_SPAN_NOT_CONTAINED ?
                            UnicodeSetStringSpan::FWD_UTF8_NOT_CONTAINED :
                            UnicodeSetStringSpan::FWD_UTF8_CONTAINED;
        UnicodeSetStringSpan strSpan(*this, *strings, which);
        if(strSpan.needsStringSpanUTF8()) {
            return strSpan.spanUTF8((const uint8_t *)s, length, spanCondition);
        }
    }

    if(spanCondition!=USET_SPAN_NOT_CONTAINED) {
        spanCondition=USET_SPAN_CONTAINED;  
    }

    UChar32 c;
    int32_t start=0, prev=0;
    do {
        U8_NEXT(s, start, length, c);
        if(c<0) {
            c=0xfffd;
        }
        if(spanCondition!=contains(c)) {
            break;
        }
    } while((prev=start)<length);
    return prev;
}

int32_t UnicodeSet::spanBackUTF8(const char *s, int32_t length, USetSpanCondition spanCondition) const {
    if(length>0 && bmpSet!=NULL) {
        const uint8_t *s0=(const uint8_t *)s;
        return bmpSet->spanBackUTF8(s0, length, spanCondition);
    }
    if(length<0) {
        length=(int32_t)uprv_strlen(s);
    }
    if(length==0) {
        return 0;
    }
    if(stringSpan!=NULL) {
        return stringSpan->spanBackUTF8((const uint8_t *)s, length, spanCondition);
    } else if(!strings->isEmpty()) {
        uint32_t which= spanCondition==USET_SPAN_NOT_CONTAINED ?
                            UnicodeSetStringSpan::BACK_UTF8_NOT_CONTAINED :
                            UnicodeSetStringSpan::BACK_UTF8_CONTAINED;
        UnicodeSetStringSpan strSpan(*this, *strings, which);
        if(strSpan.needsStringSpanUTF8()) {
            return strSpan.spanBackUTF8((const uint8_t *)s, length, spanCondition);
        }
    }

    if(spanCondition!=USET_SPAN_NOT_CONTAINED) {
        spanCondition=USET_SPAN_CONTAINED;  
    }

    UChar32 c;
    int32_t prev=length;
    do {
        U8_PREV(s, 0, length, c);
        if(c<0) {
            c=0xfffd;
        }
        if(spanCondition!=contains(c)) {
            break;
        }
    } while((prev=length)>0);
    return prev;
}

U_NAMESPACE_END
