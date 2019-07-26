















#include "unicode/utypes.h"
#include "unicode/uniset.h"
#include "unicode/utf8.h"
#include "unicode/utf16.h"
#include "cmemory.h"
#include "bmpset.h"
#include "uassert.h"

U_NAMESPACE_BEGIN

BMPSet::BMPSet(const int32_t *parentList, int32_t parentListLength) :
        list(parentList), listLength(parentListLength) {
    uprv_memset(asciiBytes, 0, sizeof(asciiBytes));
    uprv_memset(table7FF, 0, sizeof(table7FF));
    uprv_memset(bmpBlockBits, 0, sizeof(bmpBlockBits));

    






    list4kStarts[0]=findCodePoint(0x800, 0, listLength-1);
    int32_t i;
    for(i=1; i<=0x10; ++i) {
        list4kStarts[i]=findCodePoint(i<<12, list4kStarts[i-1], listLength-1);
    }
    list4kStarts[0x11]=listLength-1;

    initBits();
    overrideIllegal();
}

BMPSet::BMPSet(const BMPSet &otherBMPSet, const int32_t *newParentList, int32_t newParentListLength) :
        list(newParentList), listLength(newParentListLength) {
    uprv_memcpy(asciiBytes, otherBMPSet.asciiBytes, sizeof(asciiBytes));
    uprv_memcpy(table7FF, otherBMPSet.table7FF, sizeof(table7FF));
    uprv_memcpy(bmpBlockBits, otherBMPSet.bmpBlockBits, sizeof(bmpBlockBits));
    uprv_memcpy(list4kStarts, otherBMPSet.list4kStarts, sizeof(list4kStarts));
}

BMPSet::~BMPSet() {
}





static void set32x64Bits(uint32_t table[64], int32_t start, int32_t limit) {
    U_ASSERT(start<limit);
    U_ASSERT(limit<=0x800);

    int32_t lead=start>>6;  
    int32_t trail=start&0x3f;  

    
    uint32_t bits=(uint32_t)1<<lead;
    if((start+1)==limit) {  
        table[trail]|=bits;
        return;
    }

    int32_t limitLead=limit>>6;
    int32_t limitTrail=limit&0x3f;

    if(lead==limitLead) {
        
        while(trail<limitTrail) {
            table[trail++]|=bits;
        }
    } else {
        
        
        
        if(trail>0) {
            do {
                table[trail++]|=bits;
            } while(trail<64);
            ++lead;
        }
        if(lead<limitLead) {
            bits=~((1<<lead)-1);
            if(limitLead<0x20) {
                bits&=(1<<limitLead)-1;
            }
            for(trail=0; trail<64; ++trail) {
                table[trail]|=bits;
            }
        }
        
        
        
        bits=(uint32_t)1<<((limitLead == 0x20) ? (limitLead - 1) : limitLead);
        for(trail=0; trail<limitTrail; ++trail) {
            table[trail]|=bits;
        }
    }
}

void BMPSet::initBits() {
    UChar32 start, limit;
    int32_t listIndex=0;

    
    do {
        start=list[listIndex++];
        if(listIndex<listLength) {
            limit=list[listIndex++];
        } else {
            limit=0x110000;
        }
        if(start>=0x80) {
            break;
        }
        do {
            asciiBytes[start++]=1;
        } while(start<limit && start<0x80);
    } while(limit<=0x80);

    
    while(start<0x800) {
        set32x64Bits(table7FF, start, limit<=0x800 ? limit : 0x800);
        if(limit>0x800) {
            start=0x800;
            break;
        }

        start=list[listIndex++];
        if(listIndex<listLength) {
            limit=list[listIndex++];
        } else {
            limit=0x110000;
        }
    }

    
    int32_t minStart=0x800;
    while(start<0x10000) {
        if(limit>0x10000) {
            limit=0x10000;
        }

        if(start<minStart) {
            start=minStart;
        }
        if(start<limit) {  
            if(start&0x3f) {
                
                start>>=6;
                bmpBlockBits[start&0x3f]|=0x10001<<(start>>6);
                start=(start+1)<<6;  
                minStart=start;      
            }
            if(start<limit) {
                if(start<(limit&~0x3f)) {
                    
                    set32x64Bits(bmpBlockBits, start>>6, limit>>6);
                }

                if(limit&0x3f) {
                    
                    limit>>=6;
                    bmpBlockBits[limit&0x3f]|=0x10001<<(limit>>6);
                    limit=(limit+1)<<6;  
                    minStart=limit;      
                }
            }
        }

        if(limit==0x10000) {
            break;
        }

        start=list[listIndex++];
        if(listIndex<listLength) {
            limit=list[listIndex++];
        } else {
            limit=0x110000;
        }
    }
}









void BMPSet::overrideIllegal() {
    uint32_t bits, mask;
    int32_t i;

    if(containsSlow(0xfffd, list4kStarts[0xf], list4kStarts[0x10])) {
        
        for(i=0x80; i<0xc0; ++i) {
            asciiBytes[i]=1;
        }

        bits=3;                 
        for(i=0; i<64; ++i) {
            table7FF[i]|=bits;
        }

        bits=1;                 
        for(i=0; i<32; ++i) {   
            bmpBlockBits[i]|=bits;
        }

        mask=~(0x10001<<0xd);   
        bits=1<<0xd;
        for(i=32; i<64; ++i) {  
            bmpBlockBits[i]=(bmpBlockBits[i]&mask)|bits;
        }
    } else {
        
        mask=~(0x10001<<0xd);   
        for(i=32; i<64; ++i) {  
            bmpBlockBits[i]&=mask;
        }
    }
}

int32_t BMPSet::findCodePoint(UChar32 c, int32_t lo, int32_t hi) const {
    









    
    
    if (c < list[lo])
        return lo;
    
    
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

UBool
BMPSet::contains(UChar32 c) const {
    if((uint32_t)c<=0x7f) {
        return (UBool)asciiBytes[c];
    } else if((uint32_t)c<=0x7ff) {
        return (UBool)((table7FF[c&0x3f]&((uint32_t)1<<(c>>6)))!=0);
    } else if((uint32_t)c<0xd800 || (c>=0xe000 && c<=0xffff)) {
        int lead=c>>12;
        uint32_t twoBits=(bmpBlockBits[(c>>6)&0x3f]>>lead)&0x10001;
        if(twoBits<=1) {
            
            
            return (UBool)twoBits;
        } else {
            
            return containsSlow(c, list4kStarts[lead], list4kStarts[lead+1]);
        }
    } else if((uint32_t)c<=0x10ffff) {
        
        return containsSlow(c, list4kStarts[0xd], list4kStarts[0x11]);
    } else {
        
        
        return FALSE;
    }
}





const UChar *
BMPSet::span(const UChar *s, const UChar *limit, USetSpanCondition spanCondition) const {
    UChar c, c2;

    if(spanCondition) {
        
        do {
            c=*s;
            if(c<=0x7f) {
                if(!asciiBytes[c]) {
                    break;
                }
            } else if(c<=0x7ff) {
                if((table7FF[c&0x3f]&((uint32_t)1<<(c>>6)))==0) {
                    break;
                }
            } else if(c<0xd800 || c>=0xe000) {
                int lead=c>>12;
                uint32_t twoBits=(bmpBlockBits[(c>>6)&0x3f]>>lead)&0x10001;
                if(twoBits<=1) {
                    
                    
                    if(twoBits==0) {
                        break;
                    }
                } else {
                    
                    if(!containsSlow(c, list4kStarts[lead], list4kStarts[lead+1])) {
                        break;
                    }
                }
            } else if(c>=0xdc00 || (s+1)==limit || (c2=s[1])<0xdc00 || c2>=0xe000) {
                
                if(!containsSlow(c, list4kStarts[0xd], list4kStarts[0xe])) {
                    break;
                }
            } else {
                
                if(!containsSlow(U16_GET_SUPPLEMENTARY(c, c2), list4kStarts[0x10], list4kStarts[0x11])) {
                    break;
                }
                ++s;
            }
        } while(++s<limit);
    } else {
        
        do {
            c=*s;
            if(c<=0x7f) {
                if(asciiBytes[c]) {
                    break;
                }
            } else if(c<=0x7ff) {
                if((table7FF[c&0x3f]&((uint32_t)1<<(c>>6)))!=0) {
                    break;
                }
            } else if(c<0xd800 || c>=0xe000) {
                int lead=c>>12;
                uint32_t twoBits=(bmpBlockBits[(c>>6)&0x3f]>>lead)&0x10001;
                if(twoBits<=1) {
                    
                    
                    if(twoBits!=0) {
                        break;
                    }
                } else {
                    
                    if(containsSlow(c, list4kStarts[lead], list4kStarts[lead+1])) {
                        break;
                    }
                }
            } else if(c>=0xdc00 || (s+1)==limit || (c2=s[1])<0xdc00 || c2>=0xe000) {
                
                if(containsSlow(c, list4kStarts[0xd], list4kStarts[0xe])) {
                    break;
                }
            } else {
                
                if(containsSlow(U16_GET_SUPPLEMENTARY(c, c2), list4kStarts[0x10], list4kStarts[0x11])) {
                    break;
                }
                ++s;
            }
        } while(++s<limit);
    }
    return s;
}


const UChar *
BMPSet::spanBack(const UChar *s, const UChar *limit, USetSpanCondition spanCondition) const {
    UChar c, c2;

    if(spanCondition) {
        
        for(;;) {
            c=*(--limit);
            if(c<=0x7f) {
                if(!asciiBytes[c]) {
                    break;
                }
            } else if(c<=0x7ff) {
                if((table7FF[c&0x3f]&((uint32_t)1<<(c>>6)))==0) {
                    break;
                }
            } else if(c<0xd800 || c>=0xe000) {
                int lead=c>>12;
                uint32_t twoBits=(bmpBlockBits[(c>>6)&0x3f]>>lead)&0x10001;
                if(twoBits<=1) {
                    
                    
                    if(twoBits==0) {
                        break;
                    }
                } else {
                    
                    if(!containsSlow(c, list4kStarts[lead], list4kStarts[lead+1])) {
                        break;
                    }
                }
            } else if(c<0xdc00 || s==limit || (c2=*(limit-1))<0xd800 || c2>=0xdc00) {
                
                if(!containsSlow(c, list4kStarts[0xd], list4kStarts[0xe])) {
                    break;
                }
            } else {
                
                if(!containsSlow(U16_GET_SUPPLEMENTARY(c2, c), list4kStarts[0x10], list4kStarts[0x11])) {
                    break;
                }
                --limit;
            }
            if(s==limit) {
                return s;
            }
        }
    } else {
        
        for(;;) {
            c=*(--limit);
            if(c<=0x7f) {
                if(asciiBytes[c]) {
                    break;
                }
            } else if(c<=0x7ff) {
                if((table7FF[c&0x3f]&((uint32_t)1<<(c>>6)))!=0) {
                    break;
                }
            } else if(c<0xd800 || c>=0xe000) {
                int lead=c>>12;
                uint32_t twoBits=(bmpBlockBits[(c>>6)&0x3f]>>lead)&0x10001;
                if(twoBits<=1) {
                    
                    
                    if(twoBits!=0) {
                        break;
                    }
                } else {
                    
                    if(containsSlow(c, list4kStarts[lead], list4kStarts[lead+1])) {
                        break;
                    }
                }
            } else if(c<0xdc00 || s==limit || (c2=*(limit-1))<0xd800 || c2>=0xdc00) {
                
                if(containsSlow(c, list4kStarts[0xd], list4kStarts[0xe])) {
                    break;
                }
            } else {
                
                if(containsSlow(U16_GET_SUPPLEMENTARY(c2, c), list4kStarts[0x10], list4kStarts[0x11])) {
                    break;
                }
                --limit;
            }
            if(s==limit) {
                return s;
            }
        }
    }
    return limit+1;
}





const uint8_t *
BMPSet::spanUTF8(const uint8_t *s, int32_t length, USetSpanCondition spanCondition) const {
    const uint8_t *limit=s+length;
    uint8_t b=*s;
    if((int8_t)b>=0) {
        
        if(spanCondition) {
            do {
                if(!asciiBytes[b] || ++s==limit) {
                    return s;
                }
                b=*s;
            } while((int8_t)b>=0);
        } else {
            do {
                if(asciiBytes[b] || ++s==limit) {
                    return s;
                }
                b=*s;
            } while((int8_t)b>=0);
        }
        length=(int32_t)(limit-s);
    }

    if(spanCondition!=USET_SPAN_NOT_CONTAINED) {
        spanCondition=USET_SPAN_CONTAINED;  
    }

    const uint8_t *limit0=limit;

    









    b=*(limit-1);
    if((int8_t)b<0) {
        
        if(b<0xc0) {
            
            if(length>=2 && (b=*(limit-2))>=0xe0) {
                limit-=2;
                if(asciiBytes[0x80]!=spanCondition) {
                    limit0=limit;
                }
            } else if(b<0xc0 && b>=0x80 && length>=3 && (b=*(limit-3))>=0xf0) {
                
                limit-=3;
                if(asciiBytes[0x80]!=spanCondition) {
                    limit0=limit;
                }
            }
        } else {
            
            --limit;
            if(asciiBytes[0x80]!=spanCondition) {
                limit0=limit;
            }
        }
    }

    uint8_t t1, t2, t3;

    while(s<limit) {
        b=*s;
        if(b<0xc0) {
            
            if(spanCondition) {
                do {
                    if(!asciiBytes[b]) {
                        return s;
                    } else if(++s==limit) {
                        return limit0;
                    }
                    b=*s;
                } while(b<0xc0);
            } else {
                do {
                    if(asciiBytes[b]) {
                        return s;
                    } else if(++s==limit) {
                        return limit0;
                    }
                    b=*s;
                } while(b<0xc0);
            }
        }
        ++s;  
        if(b>=0xe0) {
            if(b<0xf0) {
                if( 
                    (t1=(uint8_t)(s[0]-0x80)) <= 0x3f &&
                    (t2=(uint8_t)(s[1]-0x80)) <= 0x3f
                ) {
                    b&=0xf;
                    uint32_t twoBits=(bmpBlockBits[t1]>>b)&0x10001;
                    if(twoBits<=1) {
                        
                        
                        if(twoBits!=(uint32_t)spanCondition) {
                            return s-1;
                        }
                    } else {
                        
                        UChar32 c=(b<<12)|(t1<<6)|t2;
                        if(containsSlow(c, list4kStarts[b], list4kStarts[b+1]) != spanCondition) {
                            return s-1;
                        }
                    }
                    s+=2;
                    continue;
                }
            } else if( 
                (t1=(uint8_t)(s[0]-0x80)) <= 0x3f &&
                (t2=(uint8_t)(s[1]-0x80)) <= 0x3f &&
                (t3=(uint8_t)(s[2]-0x80)) <= 0x3f
            ) {
                
                UChar32 c=((UChar32)(b-0xf0)<<18)|((UChar32)t1<<12)|(t2<<6)|t3;
                if( (   (0x10000<=c && c<=0x10ffff) ?
                            containsSlow(c, list4kStarts[0x10], list4kStarts[0x11]) :
                            asciiBytes[0x80]
                    ) != spanCondition
                ) {
                    return s-1;
                }
                s+=3;
                continue;
            }
        } else  {
            if( 
                (t1=(uint8_t)(*s-0x80)) <= 0x3f
            ) {
                if((USetSpanCondition)((table7FF[t1]&((uint32_t)1<<(b&0x1f)))!=0) != spanCondition) {
                    return s-1;
                }
                ++s;
                continue;
            }
        }

        
        
        
        if(asciiBytes[0x80]!=spanCondition) {
            return s-1;
        }
    }

    return limit0;
}








int32_t
BMPSet::spanBackUTF8(const uint8_t *s, int32_t length, USetSpanCondition spanCondition) const {
    if(spanCondition!=USET_SPAN_NOT_CONTAINED) {
        spanCondition=USET_SPAN_CONTAINED;  
    }

    uint8_t b;

    do {
        b=s[--length];
        if((int8_t)b>=0) {
            
            if(spanCondition) {
                do {
                    if(!asciiBytes[b]) {
                        return length+1;
                    } else if(length==0) {
                        return 0;
                    }
                    b=s[--length];
                } while((int8_t)b>=0);
            } else {
                do {
                    if(asciiBytes[b]) {
                        return length+1;
                    } else if(length==0) {
                        return 0;
                    }
                    b=s[--length];
                } while((int8_t)b>=0);
            }
        }

        int32_t prev=length;
        UChar32 c;
        if(b<0xc0) {
            
            c=utf8_prevCharSafeBody(s, 0, &length, b, -1);
            if(c<0) {
                c=0xfffd;
            }
        } else {
            
            c=0xfffd;
        }
        
        if(c<=0x7ff) {
            if((USetSpanCondition)((table7FF[c&0x3f]&((uint32_t)1<<(c>>6)))!=0) != spanCondition) {
                return prev+1;
            }
        } else if(c<=0xffff) {
            int lead=c>>12;
            uint32_t twoBits=(bmpBlockBits[(c>>6)&0x3f]>>lead)&0x10001;
            if(twoBits<=1) {
                
                
                if(twoBits!=(uint32_t)spanCondition) {
                    return prev+1;
                }
            } else {
                
                if(containsSlow(c, list4kStarts[lead], list4kStarts[lead+1]) != spanCondition) {
                    return prev+1;
                }
            }
        } else {
            if(containsSlow(c, list4kStarts[0x10], list4kStarts[0x11]) != spanCondition) {
                return prev+1;
            }
        }
    } while(length>0);
    return 0;
}

U_NAMESPACE_END
