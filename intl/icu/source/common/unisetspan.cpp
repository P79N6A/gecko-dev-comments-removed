















#include "unicode/utypes.h"
#include "unicode/uniset.h"
#include "unicode/ustring.h"
#include "unicode/utf8.h"
#include "unicode/utf16.h"
#include "cmemory.h"
#include "uvector.h"
#include "unisetspan.h"

U_NAMESPACE_BEGIN




























class OffsetList {  
public:
    OffsetList() : list(staticList), capacity(0), length(0), start(0) {}

    ~OffsetList() {
        if(list!=staticList) {
            uprv_free(list);
        }
    }

    
    void setMaxLength(int32_t maxLength) {
        if(maxLength<=(int32_t)sizeof(staticList)) {
            capacity=(int32_t)sizeof(staticList);
        } else {
            UBool *l=(UBool *)uprv_malloc(maxLength);
            if(l!=NULL) {
                list=l;
                capacity=maxLength;
            }
        }
        uprv_memset(list, 0, capacity);
    }

    void clear() {
        uprv_memset(list, 0, capacity);
        start=length=0;
    }

    UBool isEmpty() const {
        return (UBool)(length==0);
    }

    
    
    
    
    
    void shift(int32_t delta) {
        int32_t i=start+delta;
        if(i>=capacity) {
            i-=capacity;
        }
        if(list[i]) {
            list[i]=FALSE;
            --length;
        }
        start=i;
    }

    
    
    void addOffset(int32_t offset) {
        int32_t i=start+offset;
        if(i>=capacity) {
            i-=capacity;
        }
        list[i]=TRUE;
        ++length;
    }

    
    UBool containsOffset(int32_t offset) const {
        int32_t i=start+offset;
        if(i>=capacity) {
            i-=capacity;
        }
        return list[i];
    }

    
    
    
    int32_t popMinimum() {
        
        int32_t i=start, result;
        while(++i<capacity) {
            if(list[i]) {
                list[i]=FALSE;
                --length;
                result=i-start;
                start=i;
                return result;
            }
        }
        

        
        
        result=capacity-start;
        i=0;
        while(!list[i]) {
            ++i;
        }
        list[i]=FALSE;
        --length;
        start=i;
        return result+=i;
    }

private:
    UBool *list;
    int32_t capacity;
    int32_t length;
    int32_t start;

    UBool staticList[16];
};


static int32_t
getUTF8Length(const UChar *s, int32_t length) {
    UErrorCode errorCode=U_ZERO_ERROR;
    int32_t length8=0;
    u_strToUTF8(NULL, 0, &length8, s, length, &errorCode);
    if(U_SUCCESS(errorCode) || errorCode==U_BUFFER_OVERFLOW_ERROR) {
        return length8;
    } else {
        
        
        return 0;
    }
}


static int32_t
appendUTF8(const UChar *s, int32_t length, uint8_t *t, int32_t capacity) {
    UErrorCode errorCode=U_ZERO_ERROR;
    int32_t length8=0;
    u_strToUTF8((char *)t, capacity, &length8, s, length, &errorCode);
    if(U_SUCCESS(errorCode)) {
        return length8;
    } else {
        
        
        return 0;
    }
}

static inline uint8_t
makeSpanLengthByte(int32_t spanLength) {
    
    return spanLength<0xfe ? (uint8_t)spanLength : (uint8_t)0xfe;
}



UnicodeSetStringSpan::UnicodeSetStringSpan(const UnicodeSet &set,
                                           const UVector &setStrings,
                                           uint32_t which)
        : spanSet(0, 0x10ffff), pSpanNotSet(NULL), strings(setStrings),
          utf8Lengths(NULL), spanLengths(NULL), utf8(NULL),
          utf8Length(0),
          maxLength16(0), maxLength8(0),
          all((UBool)(which==ALL)) {
    spanSet.retainAll(set);
    if(which&NOT_CONTAINED) {
        
        
        pSpanNotSet=&spanSet;
    }

    
    
    
    
    
    
    
    int32_t stringsLength=strings.size();

    int32_t i, spanLength;
    UBool someRelevant=FALSE;
    for(i=0; i<stringsLength; ++i) {
        const UnicodeString &string=*(const UnicodeString *)strings.elementAt(i);
        const UChar *s16=string.getBuffer();
        int32_t length16=string.length();
        UBool thisRelevant;
        spanLength=spanSet.span(s16, length16, USET_SPAN_CONTAINED);
        if(spanLength<length16) {  
            someRelevant=thisRelevant=TRUE;
        } else {
            thisRelevant=FALSE;
        }
        if((which&UTF16) && length16>maxLength16) {
            maxLength16=length16;
        }
        if((which&UTF8) && (thisRelevant || (which&CONTAINED))) {
            int32_t length8=getUTF8Length(s16, length16);
            utf8Length+=length8;
            if(length8>maxLength8) {
                maxLength8=length8;
            }
        }
    }
    if(!someRelevant) {
        maxLength16=maxLength8=0;
        return;
    }

    
    
    if(all) {
        spanSet.freeze();
    }

    uint8_t *spanBackLengths;
    uint8_t *spanUTF8Lengths;
    uint8_t *spanBackUTF8Lengths;

    
    int32_t allocSize;
    if(all) {
        
        allocSize=stringsLength*(4+1+1+1+1)+utf8Length;
    } else {
        allocSize=stringsLength;  
        if(which&UTF8) {
            
            allocSize+=stringsLength*4+utf8Length;
        }
    }
    if(allocSize<=(int32_t)sizeof(staticLengths)) {
        utf8Lengths=staticLengths;
    } else {
        utf8Lengths=(int32_t *)uprv_malloc(allocSize);
        if(utf8Lengths==NULL) {
            maxLength16=maxLength8=0;  
            return;  
        }
    }

    if(all) {
        
        spanLengths=(uint8_t *)(utf8Lengths+stringsLength);
        spanBackLengths=spanLengths+stringsLength;
        spanUTF8Lengths=spanBackLengths+stringsLength;
        spanBackUTF8Lengths=spanUTF8Lengths+stringsLength;
        utf8=spanBackUTF8Lengths+stringsLength;
    } else {
        
        if(which&UTF8) {
            spanLengths=(uint8_t *)(utf8Lengths+stringsLength);
            utf8=spanLengths+stringsLength;
        } else {
            spanLengths=(uint8_t *)utf8Lengths;
        }
        spanBackLengths=spanUTF8Lengths=spanBackUTF8Lengths=spanLengths;
    }

    
    int32_t utf8Count=0;  

    for(i=0; i<stringsLength; ++i) {
        const UnicodeString &string=*(const UnicodeString *)strings.elementAt(i);
        const UChar *s16=string.getBuffer();
        int32_t length16=string.length();
        spanLength=spanSet.span(s16, length16, USET_SPAN_CONTAINED);
        if(spanLength<length16) {  
            if(which&UTF16) {
                if(which&CONTAINED) {
                    if(which&FWD) {
                        spanLengths[i]=makeSpanLengthByte(spanLength);
                    }
                    if(which&BACK) {
                        spanLength=length16-spanSet.spanBack(s16, length16, USET_SPAN_CONTAINED);
                        spanBackLengths[i]=makeSpanLengthByte(spanLength);
                    }
                } else  {
                    spanLengths[i]=spanBackLengths[i]=0;  
                }
            }
            if(which&UTF8) {
                uint8_t *s8=utf8+utf8Count;
                int32_t length8=appendUTF8(s16, length16, s8, utf8Length-utf8Count);
                utf8Count+=utf8Lengths[i]=length8;
                if(length8==0) {  
                    spanUTF8Lengths[i]=spanBackUTF8Lengths[i]=(uint8_t)ALL_CP_CONTAINED;
                } else {  
                    if(which&CONTAINED) {
                        if(which&FWD) {
                            spanLength=spanSet.spanUTF8((const char *)s8, length8, USET_SPAN_CONTAINED);
                            spanUTF8Lengths[i]=makeSpanLengthByte(spanLength);
                        }
                        if(which&BACK) {
                            spanLength=length8-spanSet.spanBackUTF8((const char *)s8, length8, USET_SPAN_CONTAINED);
                            spanBackUTF8Lengths[i]=makeSpanLengthByte(spanLength);
                        }
                    } else  {
                        spanUTF8Lengths[i]=spanBackUTF8Lengths[i]=0;  
                    }
                }
            }
            if(which&NOT_CONTAINED) {
                
                
                UChar32 c;
                if(which&FWD) {
                    int32_t len=0;
                    U16_NEXT(s16, len, length16, c);
                    addToSpanNotSet(c);
                }
                if(which&BACK) {
                    int32_t len=length16;
                    U16_PREV(s16, 0, len, c);
                    addToSpanNotSet(c);
                }
            }
        } else {  
            if(which&UTF8) {
                if(which&CONTAINED) {  
                    uint8_t *s8=utf8+utf8Count;
                    int32_t length8=appendUTF8(s16, length16, s8, utf8Length-utf8Count);
                    utf8Count+=utf8Lengths[i]=length8;
                } else {
                    utf8Lengths[i]=0;
                }
            }
            if(all) {
                spanLengths[i]=spanBackLengths[i]=
                    spanUTF8Lengths[i]=spanBackUTF8Lengths[i]=
                        (uint8_t)ALL_CP_CONTAINED;
            } else {
                
                spanLengths[i]=(uint8_t)ALL_CP_CONTAINED;
            }
        }
    }

    
    if(all) {
        pSpanNotSet->freeze();
    }
}


UnicodeSetStringSpan::UnicodeSetStringSpan(const UnicodeSetStringSpan &otherStringSpan,
                                           const UVector &newParentSetStrings)
        : spanSet(otherStringSpan.spanSet), pSpanNotSet(NULL), strings(newParentSetStrings),
          utf8Lengths(NULL), spanLengths(NULL), utf8(NULL),
          utf8Length(otherStringSpan.utf8Length),
          maxLength16(otherStringSpan.maxLength16), maxLength8(otherStringSpan.maxLength8),
          all(TRUE) {
    if(otherStringSpan.pSpanNotSet==&otherStringSpan.spanSet) {
        pSpanNotSet=&spanSet;
    } else {
        pSpanNotSet=(UnicodeSet *)otherStringSpan.pSpanNotSet->clone();
    }

    
    
    int32_t stringsLength=strings.size();
    int32_t allocSize=stringsLength*(4+1+1+1+1)+utf8Length;
    if(allocSize<=(int32_t)sizeof(staticLengths)) {
        utf8Lengths=staticLengths;
    } else {
        utf8Lengths=(int32_t *)uprv_malloc(allocSize);
        if(utf8Lengths==NULL) {
            maxLength16=maxLength8=0;  
            return;  
        }
    }

    spanLengths=(uint8_t *)(utf8Lengths+stringsLength);
    utf8=spanLengths+stringsLength*4;
    uprv_memcpy(utf8Lengths, otherStringSpan.utf8Lengths, allocSize);
}

UnicodeSetStringSpan::~UnicodeSetStringSpan() {
    if(pSpanNotSet!=NULL && pSpanNotSet!=&spanSet) {
        delete pSpanNotSet;
    }
    if(utf8Lengths!=NULL && utf8Lengths!=staticLengths) {
        uprv_free(utf8Lengths);
    }
}

void UnicodeSetStringSpan::addToSpanNotSet(UChar32 c) {
    if(pSpanNotSet==NULL || pSpanNotSet==&spanSet) {
        if(spanSet.contains(c)) {
            return;  
        }
        UnicodeSet *newSet=(UnicodeSet *)spanSet.cloneAsThawed();
        if(newSet==NULL) {
            return;  
        } else {
            pSpanNotSet=newSet;
        }
    }
    pSpanNotSet->add(c);
}


static inline UBool
matches16(const UChar *s, const UChar *t, int32_t length) {
    do {
        if(*s++!=*t++) {
            return FALSE;
        }
    } while(--length>0);
    return TRUE;
}

static inline UBool
matches8(const uint8_t *s, const uint8_t *t, int32_t length) {
    do {
        if(*s++!=*t++) {
            return FALSE;
        }
    } while(--length>0);
    return TRUE;
}




static inline UBool
matches16CPB(const UChar *s, int32_t start, int32_t limit, const UChar *t, int32_t length) {
    s+=start;
    limit-=start;
    return matches16(s, t, length) &&
           !(0<start && U16_IS_LEAD(s[-1]) && U16_IS_TRAIL(s[0])) &&
           !(length<limit && U16_IS_LEAD(s[length-1]) && U16_IS_TRAIL(s[length]));
}



static inline int32_t
spanOne(const UnicodeSet &set, const UChar *s, int32_t length) {
    UChar c=*s, c2;
    if(c>=0xd800 && c<=0xdbff && length>=2 && U16_IS_TRAIL(c2=s[1])) {
        return set.contains(U16_GET_SUPPLEMENTARY(c, c2)) ? 2 : -2;
    }
    return set.contains(c) ? 1 : -1;
}

static inline int32_t
spanOneBack(const UnicodeSet &set, const UChar *s, int32_t length) {
    UChar c=s[length-1], c2;
    if(c>=0xdc00 && c<=0xdfff && length>=2 && U16_IS_LEAD(c2=s[length-2])) {
        return set.contains(U16_GET_SUPPLEMENTARY(c2, c)) ? 2 : -2;
    }
    return set.contains(c) ? 1 : -1;
}

static inline int32_t
spanOneUTF8(const UnicodeSet &set, const uint8_t *s, int32_t length) {
    UChar32 c=*s;
    if((int8_t)c>=0) {
        return set.contains(c) ? 1 : -1;
    }
    
    int32_t i=0;
    U8_NEXT(s, i, length, c);
    return set.contains(c) ? i : -i;
}

static inline int32_t
spanOneBackUTF8(const UnicodeSet &set, const uint8_t *s, int32_t length) {
    UChar32 c=s[length-1];
    if((int8_t)c>=0) {
        return set.contains(c) ? 1 : -1;
    }
    int32_t i=length-1;
    c=utf8_prevCharSafeBody(s, 0, &i, c, -1);
    length-=i;
    return set.contains(c) ? length : -length;
}













































































































int32_t UnicodeSetStringSpan::span(const UChar *s, int32_t length, USetSpanCondition spanCondition) const {
    if(spanCondition==USET_SPAN_NOT_CONTAINED) {
        return spanNot(s, length);
    }
    int32_t spanLength=spanSet.span(s, length, USET_SPAN_CONTAINED);
    if(spanLength==length) {
        return length;
    }

    
    OffsetList offsets;
    if(spanCondition==USET_SPAN_CONTAINED) {
        
        offsets.setMaxLength(maxLength16);
    }
    int32_t pos=spanLength, rest=length-pos;
    int32_t i, stringsLength=strings.size();
    for(;;) {
        if(spanCondition==USET_SPAN_CONTAINED) {
            for(i=0; i<stringsLength; ++i) {
                int32_t overlap=spanLengths[i];
                if(overlap==ALL_CP_CONTAINED) {
                    continue;  
                }
                const UnicodeString &string=*(const UnicodeString *)strings.elementAt(i);
                const UChar *s16=string.getBuffer();
                int32_t length16=string.length();

                
                if(overlap>=LONG_SPAN) {
                    overlap=length16;
                    
                    U16_BACK_1(s16, 0, overlap);  
                }
                if(overlap>spanLength) {
                    overlap=spanLength;
                }
                int32_t inc=length16-overlap;  
                for(;;) {
                    if(inc>rest) {
                        break;
                    }
                    
                    if(!offsets.containsOffset(inc) && matches16CPB(s, pos-overlap, length, s16, length16)) {
                        if(inc==rest) {
                            return length;  
                        }
                        offsets.addOffset(inc);
                    }
                    if(overlap==0) {
                        break;
                    }
                    --overlap;
                    ++inc;
                }
            }
        } else  {
            int32_t maxInc=0, maxOverlap=0;
            for(i=0; i<stringsLength; ++i) {
                int32_t overlap=spanLengths[i];
                
                

                const UnicodeString &string=*(const UnicodeString *)strings.elementAt(i);
                const UChar *s16=string.getBuffer();
                int32_t length16=string.length();

                
                if(overlap>=LONG_SPAN) {
                    overlap=length16;
                    
                    
                }
                if(overlap>spanLength) {
                    overlap=spanLength;
                }
                int32_t inc=length16-overlap;  
                for(;;) {
                    if(inc>rest || overlap<maxOverlap) {
                        break;
                    }
                    
                    if( (overlap>maxOverlap ||  inc>maxInc) &&
                        matches16CPB(s, pos-overlap, length, s16, length16)
                    ) {
                        maxInc=inc;  
                        maxOverlap=overlap;
                        break;
                    }
                    --overlap;
                    ++inc;
                }
            }

            if(maxInc!=0 || maxOverlap!=0) {
                
                
                pos+=maxInc;
                rest-=maxInc;
                if(rest==0) {
                    return length;  
                }
                spanLength=0;  
                continue;
            }
        }
        

        if(spanLength!=0 || pos==0) {
            
            
            
            
            
            if(offsets.isEmpty()) {
                return pos;  
            }
            
        } else {
            
            if(offsets.isEmpty()) {
                
                
                spanLength=spanSet.span(s+pos, rest, USET_SPAN_CONTAINED);
                if( spanLength==rest || 
                    spanLength==0       
                ) {
                    return pos+spanLength;
                }
                pos+=spanLength;
                rest-=spanLength;
                continue;  
            } else {
                
                
                
                spanLength=spanOne(spanSet, s+pos, rest);
                if(spanLength>0) {
                    if(spanLength==rest) {
                        return length;  
                    }
                    
                    
                    
                    pos+=spanLength;
                    rest-=spanLength;
                    offsets.shift(spanLength);
                    spanLength=0;
                    continue;  
                }
                
            }
        }
        int32_t minOffset=offsets.popMinimum();
        pos+=minOffset;
        rest-=minOffset;
        spanLength=0;  
    }
}

int32_t UnicodeSetStringSpan::spanBack(const UChar *s, int32_t length, USetSpanCondition spanCondition) const {
    if(spanCondition==USET_SPAN_NOT_CONTAINED) {
        return spanNotBack(s, length);
    }
    int32_t pos=spanSet.spanBack(s, length, USET_SPAN_CONTAINED);
    if(pos==0) {
        return 0;
    }
    int32_t spanLength=length-pos;

    
    OffsetList offsets;
    if(spanCondition==USET_SPAN_CONTAINED) {
        
        offsets.setMaxLength(maxLength16);
    }
    int32_t i, stringsLength=strings.size();
    uint8_t *spanBackLengths=spanLengths;
    if(all) {
        spanBackLengths+=stringsLength;
    }
    for(;;) {
        if(spanCondition==USET_SPAN_CONTAINED) {
            for(i=0; i<stringsLength; ++i) {
                int32_t overlap=spanBackLengths[i];
                if(overlap==ALL_CP_CONTAINED) {
                    continue;  
                }
                const UnicodeString &string=*(const UnicodeString *)strings.elementAt(i);
                const UChar *s16=string.getBuffer();
                int32_t length16=string.length();

                
                if(overlap>=LONG_SPAN) {
                    overlap=length16;
                    
                    int32_t len1=0;
                    U16_FWD_1(s16, len1, overlap);
                    overlap-=len1;  
                }
                if(overlap>spanLength) {
                    overlap=spanLength;
                }
                int32_t dec=length16-overlap;  
                for(;;) {
                    if(dec>pos) {
                        break;
                    }
                    
                    if(!offsets.containsOffset(dec) && matches16CPB(s, pos-dec, length, s16, length16)) {
                        if(dec==pos) {
                            return 0;  
                        }
                        offsets.addOffset(dec);
                    }
                    if(overlap==0) {
                        break;
                    }
                    --overlap;
                    ++dec;
                }
            }
        } else  {
            int32_t maxDec=0, maxOverlap=0;
            for(i=0; i<stringsLength; ++i) {
                int32_t overlap=spanBackLengths[i];
                
                

                const UnicodeString &string=*(const UnicodeString *)strings.elementAt(i);
                const UChar *s16=string.getBuffer();
                int32_t length16=string.length();

                
                if(overlap>=LONG_SPAN) {
                    overlap=length16;
                    
                    
                }
                if(overlap>spanLength) {
                    overlap=spanLength;
                }
                int32_t dec=length16-overlap;  
                for(;;) {
                    if(dec>pos || overlap<maxOverlap) {
                        break;
                    }
                    
                    if( (overlap>maxOverlap ||  dec>maxDec) &&
                        matches16CPB(s, pos-dec, length, s16, length16)
                    ) {
                        maxDec=dec;  
                        maxOverlap=overlap;
                        break;
                    }
                    --overlap;
                    ++dec;
                }
            }

            if(maxDec!=0 || maxOverlap!=0) {
                
                
                pos-=maxDec;
                if(pos==0) {
                    return 0;  
                }
                spanLength=0;  
                continue;
            }
        }
        

        if(spanLength!=0 || pos==length) {
            
            
            
            
            
            if(offsets.isEmpty()) {
                return pos;  
            }
            
        } else {
            
            if(offsets.isEmpty()) {
                
                
                int32_t oldPos=pos;
                pos=spanSet.spanBack(s, oldPos, USET_SPAN_CONTAINED);
                spanLength=oldPos-pos;
                if( pos==0 ||           
                    spanLength==0       
                ) {
                    return pos;
                }
                continue;  
            } else {
                
                
                
                spanLength=spanOneBack(spanSet, s, pos);
                if(spanLength>0) {
                    if(spanLength==pos) {
                        return 0;  
                    }
                    
                    
                    
                    pos-=spanLength;
                    offsets.shift(spanLength);
                    spanLength=0;
                    continue;  
                }
                
            }
        }
        pos-=offsets.popMinimum();
        spanLength=0;  
    }
}

int32_t UnicodeSetStringSpan::spanUTF8(const uint8_t *s, int32_t length, USetSpanCondition spanCondition) const {
    if(spanCondition==USET_SPAN_NOT_CONTAINED) {
        return spanNotUTF8(s, length);
    }
    int32_t spanLength=spanSet.spanUTF8((const char *)s, length, USET_SPAN_CONTAINED);
    if(spanLength==length) {
        return length;
    }

    
    OffsetList offsets;
    if(spanCondition==USET_SPAN_CONTAINED) {
        
        offsets.setMaxLength(maxLength8);
    }
    int32_t pos=spanLength, rest=length-pos;
    int32_t i, stringsLength=strings.size();
    uint8_t *spanUTF8Lengths=spanLengths;
    if(all) {
        spanUTF8Lengths+=2*stringsLength;
    }
    for(;;) {
        const uint8_t *s8=utf8;
        int32_t length8;
        if(spanCondition==USET_SPAN_CONTAINED) {
            for(i=0; i<stringsLength; ++i) {
                length8=utf8Lengths[i];
                if(length8==0) {
                    continue;  
                }
                int32_t overlap=spanUTF8Lengths[i];
                if(overlap==ALL_CP_CONTAINED) {
                    s8+=length8;
                    continue;  
                }

                
                if(overlap>=LONG_SPAN) {
                    overlap=length8;
                    
                    U8_BACK_1(s8, 0, overlap);  
                }
                if(overlap>spanLength) {
                    overlap=spanLength;
                }
                int32_t inc=length8-overlap;  
                for(;;) {
                    if(inc>rest) {
                        break;
                    }
                    
                    
                    
                    if( !U8_IS_TRAIL(s[pos-overlap]) &&
                        !offsets.containsOffset(inc) &&
                        matches8(s+pos-overlap, s8, length8)
                        
                    ) {
                        if(inc==rest) {
                            return length;  
                        }
                        offsets.addOffset(inc);
                    }
                    if(overlap==0) {
                        break;
                    }
                    --overlap;
                    ++inc;
                }
                s8+=length8;
            }
        } else  {
            int32_t maxInc=0, maxOverlap=0;
            for(i=0; i<stringsLength; ++i) {
                length8=utf8Lengths[i];
                if(length8==0) {
                    continue;  
                }
                int32_t overlap=spanUTF8Lengths[i];
                
                

                
                if(overlap>=LONG_SPAN) {
                    overlap=length8;
                    
                    
                }
                if(overlap>spanLength) {
                    overlap=spanLength;
                }
                int32_t inc=length8-overlap;  
                for(;;) {
                    if(inc>rest || overlap<maxOverlap) {
                        break;
                    }
                    
                    
                    
                    if( !U8_IS_TRAIL(s[pos-overlap]) &&
                        (overlap>maxOverlap ||  inc>maxInc) &&
                        matches8(s+pos-overlap, s8, length8)
                        
                    ) {
                        maxInc=inc;  
                        maxOverlap=overlap;
                        break;
                    }
                    --overlap;
                    ++inc;
                }
                s8+=length8;
            }

            if(maxInc!=0 || maxOverlap!=0) {
                
                
                pos+=maxInc;
                rest-=maxInc;
                if(rest==0) {
                    return length;  
                }
                spanLength=0;  
                continue;
            }
        }
        

        if(spanLength!=0 || pos==0) {
            
            
            
            
            
            if(offsets.isEmpty()) {
                return pos;  
            }
            
        } else {
            
            if(offsets.isEmpty()) {
                
                
                spanLength=spanSet.spanUTF8((const char *)s+pos, rest, USET_SPAN_CONTAINED);
                if( spanLength==rest || 
                    spanLength==0       
                ) {
                    return pos+spanLength;
                }
                pos+=spanLength;
                rest-=spanLength;
                continue;  
            } else {
                
                
                
                spanLength=spanOneUTF8(spanSet, s+pos, rest);
                if(spanLength>0) {
                    if(spanLength==rest) {
                        return length;  
                    }
                    
                    
                    
                    pos+=spanLength;
                    rest-=spanLength;
                    offsets.shift(spanLength);
                    spanLength=0;
                    continue;  
                }
                
            }
        }
        int32_t minOffset=offsets.popMinimum();
        pos+=minOffset;
        rest-=minOffset;
        spanLength=0;  
    }
}

int32_t UnicodeSetStringSpan::spanBackUTF8(const uint8_t *s, int32_t length, USetSpanCondition spanCondition) const {
    if(spanCondition==USET_SPAN_NOT_CONTAINED) {
        return spanNotBackUTF8(s, length);
    }
    int32_t pos=spanSet.spanBackUTF8((const char *)s, length, USET_SPAN_CONTAINED);
    if(pos==0) {
        return 0;
    }
    int32_t spanLength=length-pos;

    
    OffsetList offsets;
    if(spanCondition==USET_SPAN_CONTAINED) {
        
        offsets.setMaxLength(maxLength8);
    }
    int32_t i, stringsLength=strings.size();
    uint8_t *spanBackUTF8Lengths=spanLengths;
    if(all) {
        spanBackUTF8Lengths+=3*stringsLength;
    }
    for(;;) {
        const uint8_t *s8=utf8;
        int32_t length8;
        if(spanCondition==USET_SPAN_CONTAINED) {
            for(i=0; i<stringsLength; ++i) {
                length8=utf8Lengths[i];
                if(length8==0) {
                    continue;  
                }
                int32_t overlap=spanBackUTF8Lengths[i];
                if(overlap==ALL_CP_CONTAINED) {
                    s8+=length8;
                    continue;  
                }

                
                if(overlap>=LONG_SPAN) {
                    overlap=length8;
                    
                    int32_t len1=0;
                    U8_FWD_1(s8, len1, overlap);
                    overlap-=len1;  
                }
                if(overlap>spanLength) {
                    overlap=spanLength;
                }
                int32_t dec=length8-overlap;  
                for(;;) {
                    if(dec>pos) {
                        break;
                    }
                    
                    
                    
                    if( !U8_IS_TRAIL(s[pos-dec]) &&
                        !offsets.containsOffset(dec) &&
                        matches8(s+pos-dec, s8, length8)
                    ) {
                        if(dec==pos) {
                            return 0;  
                        }
                        offsets.addOffset(dec);
                    }
                    if(overlap==0) {
                        break;
                    }
                    --overlap;
                    ++dec;
                }
                s8+=length8;
            }
        } else  {
            int32_t maxDec=0, maxOverlap=0;
            for(i=0; i<stringsLength; ++i) {
                length8=utf8Lengths[i];
                if(length8==0) {
                    continue;  
                }
                int32_t overlap=spanBackUTF8Lengths[i];
                
                

                
                if(overlap>=LONG_SPAN) {
                    overlap=length8;
                    
                    
                }
                if(overlap>spanLength) {
                    overlap=spanLength;
                }
                int32_t dec=length8-overlap;  
                for(;;) {
                    if(dec>pos || overlap<maxOverlap) {
                        break;
                    }
                    
                    
                    
                    if( !U8_IS_TRAIL(s[pos-dec]) &&
                        (overlap>maxOverlap ||  dec>maxDec) &&
                        matches8(s+pos-dec, s8, length8)
                    ) {
                        maxDec=dec;  
                        maxOverlap=overlap;
                        break;
                    }
                    --overlap;
                    ++dec;
                }
                s8+=length8;
            }

            if(maxDec!=0 || maxOverlap!=0) {
                
                
                pos-=maxDec;
                if(pos==0) {
                    return 0;  
                }
                spanLength=0;  
                continue;
            }
        }
        

        if(spanLength!=0 || pos==length) {
            
            
            
            
            
            if(offsets.isEmpty()) {
                return pos;  
            }
            
        } else {
            
            if(offsets.isEmpty()) {
                
                
                int32_t oldPos=pos;
                pos=spanSet.spanBackUTF8((const char *)s, oldPos, USET_SPAN_CONTAINED);
                spanLength=oldPos-pos;
                if( pos==0 ||           
                    spanLength==0       
                ) {
                    return pos;
                }
                continue;  
            } else {
                
                
                
                spanLength=spanOneBackUTF8(spanSet, s, pos);
                if(spanLength>0) {
                    if(spanLength==pos) {
                        return 0;  
                    }
                    
                    
                    
                    pos-=spanLength;
                    offsets.shift(spanLength);
                    spanLength=0;
                    continue;  
                }
                
            }
        }
        pos-=offsets.popMinimum();
        spanLength=0;  
    }
}






























int32_t UnicodeSetStringSpan::spanNot(const UChar *s, int32_t length) const {
    int32_t pos=0, rest=length;
    int32_t i, stringsLength=strings.size();
    do {
        
        
        i=pSpanNotSet->span(s+pos, rest, USET_SPAN_NOT_CONTAINED);
        if(i==rest) {
            return length;  
        }
        pos+=i;
        rest-=i;

        
        
        int32_t cpLength=spanOne(spanSet, s+pos, rest);
        if(cpLength>0) {
            return pos;  
        }

        
        for(i=0; i<stringsLength; ++i) {
            if(spanLengths[i]==ALL_CP_CONTAINED) {
                continue;  
            }
            const UnicodeString &string=*(const UnicodeString *)strings.elementAt(i);
            const UChar *s16=string.getBuffer();
            int32_t length16=string.length();
            if(length16<=rest && matches16CPB(s, pos, length, s16, length16)) {
                return pos;  
            }
        }

        
        
        
        pos-=cpLength;
        rest+=cpLength;
    } while(rest!=0);
    return length;  
}

int32_t UnicodeSetStringSpan::spanNotBack(const UChar *s, int32_t length) const {
    int32_t pos=length;
    int32_t i, stringsLength=strings.size();
    do {
        
        
        pos=pSpanNotSet->spanBack(s, pos, USET_SPAN_NOT_CONTAINED);
        if(pos==0) {
            return 0;  
        }

        
        
        int32_t cpLength=spanOneBack(spanSet, s, pos);
        if(cpLength>0) {
            return pos;  
        }

        
        for(i=0; i<stringsLength; ++i) {
            
            
            
            if(spanLengths[i]==ALL_CP_CONTAINED) {
                continue;  
            }
            const UnicodeString &string=*(const UnicodeString *)strings.elementAt(i);
            const UChar *s16=string.getBuffer();
            int32_t length16=string.length();
            if(length16<=pos && matches16CPB(s, pos-length16, length, s16, length16)) {
                return pos;  
            }
        }

        
        
        
        pos+=cpLength;
    } while(pos!=0);
    return 0;  
}

int32_t UnicodeSetStringSpan::spanNotUTF8(const uint8_t *s, int32_t length) const {
    int32_t pos=0, rest=length;
    int32_t i, stringsLength=strings.size();
    uint8_t *spanUTF8Lengths=spanLengths;
    if(all) {
        spanUTF8Lengths+=2*stringsLength;
    }
    do {
        
        
        i=pSpanNotSet->spanUTF8((const char *)s+pos, rest, USET_SPAN_NOT_CONTAINED);
        if(i==rest) {
            return length;  
        }
        pos+=i;
        rest-=i;

        
        
        int32_t cpLength=spanOneUTF8(spanSet, s+pos, rest);
        if(cpLength>0) {
            return pos;  
        }

        
        const uint8_t *s8=utf8;
        int32_t length8;
        for(i=0; i<stringsLength; ++i) {
            length8=utf8Lengths[i];
            
            if(length8!=0 && spanUTF8Lengths[i]!=ALL_CP_CONTAINED && length8<=rest && matches8(s+pos, s8, length8)) {
                return pos;  
            }
            s8+=length8;
        }

        
        
        
        pos-=cpLength;
        rest+=cpLength;
    } while(rest!=0);
    return length;  
}

int32_t UnicodeSetStringSpan::spanNotBackUTF8(const uint8_t *s, int32_t length) const {
    int32_t pos=length;
    int32_t i, stringsLength=strings.size();
    uint8_t *spanBackUTF8Lengths=spanLengths;
    if(all) {
        spanBackUTF8Lengths+=3*stringsLength;
    }
    do {
        
        
        pos=pSpanNotSet->spanBackUTF8((const char *)s, pos, USET_SPAN_NOT_CONTAINED);
        if(pos==0) {
            return 0;  
        }

        
        
        int32_t cpLength=spanOneBackUTF8(spanSet, s, pos);
        if(cpLength>0) {
            return pos;  
        }

        
        const uint8_t *s8=utf8;
        int32_t length8;
        for(i=0; i<stringsLength; ++i) {
            length8=utf8Lengths[i];
            
            if(length8!=0 && spanBackUTF8Lengths[i]!=ALL_CP_CONTAINED && length8<=pos && matches8(s+pos-length8, s8, length8)) {
                return pos;  
            }
            s8+=length8;
        }

        
        
        
        pos+=cpLength;
    } while(pos!=0);
    return 0;  
}

U_NAMESPACE_END
