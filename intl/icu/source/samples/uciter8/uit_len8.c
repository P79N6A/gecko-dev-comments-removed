






















#include <string.h>
#include "unicode/utypes.h"
#include "unicode/uiter.h"



























#define L8_NEXT(s, i, length, c) { \
    (c)=(uint8_t)(s)[(i)++]; \
    if((c)>=0x80) { \
        if(U8_IS_LEAD(c)) { \
            (c)=utf8_nextCharSafeBody((const uint8_t *)s, &(i), (int32_t)(length), c, -2); \
        } else { \
            (c)=U_SENTINEL; \
        } \
    } \
}

#define L8_PREV(s, start, i, c) { \
    (c)=(uint8_t)(s)[--(i)]; \
    if((c)>=0x80) { \
        if((c)<=0xbf) { \
            (c)=utf8_prevCharSafeBody((const uint8_t *)s, start, &(i), c, -2); \
        } else { \
            (c)=U_SENTINEL; \
        } \
    } \
}


































static int32_t U_CALLCONV
lenient8IteratorGetIndex(UCharIterator *iter, UCharIteratorOrigin origin) {
    switch(origin) {
    case UITER_ZERO:
    case UITER_START:
        return 0;
    case UITER_CURRENT:
        if(iter->index<0) {
            
            const uint8_t *s;
            UChar32 c;
            int32_t i, limit, index;

            s=(const uint8_t *)iter->context;
            i=index=0;
            limit=iter->start; 
            while(i<limit) {
                L8_NEXT(s, i, limit, c);
                if(c<=0xffff) {
                    ++index;
                } else {
                    index+=2;
                }
            }

            iter->start=i; 
            if(i==iter->limit) {
                iter->length=index; 
            }
            if(iter->reservedField!=0) {
                --index; 
            }
            iter->index=index;
        }
        return iter->index;
    case UITER_LIMIT:
    case UITER_LENGTH:
        if(iter->length<0) {
            const uint8_t *s;
            UChar32 c;
            int32_t i, limit, length;

            s=(const uint8_t *)iter->context;
            if(iter->index<0) {
                



                i=length=0;
                limit=iter->start;

                
                while(i<limit) {
                    L8_NEXT(s, i, limit, c);
                    if(c<=0xffff) {
                        ++length;
                    } else {
                        length+=2;
                    }
                }

                
                iter->start=i; 
                iter->index= iter->reservedField!=0 ? length-1 : length;
            } else {
                i=iter->start;
                length=iter->index;
                if(iter->reservedField!=0) {
                    ++length;
                }
            }

            
            limit=iter->limit;
            while(i<limit) {
                L8_NEXT(s, i, limit, c);
                if(c<=0xffff) {
                    ++length;
                } else {
                    length+=2;
                }
            }
            iter->length=length;
        }
        return iter->length;
    default:
        
        
        return -1;
    }
}

static int32_t U_CALLCONV
lenient8IteratorMove(UCharIterator *iter, int32_t delta, UCharIteratorOrigin origin) {
    const uint8_t *s;
    UChar32 c;
    int32_t pos; 
    int32_t i; 
    UBool havePos;

    
    switch(origin) {
    case UITER_ZERO:
    case UITER_START:
        pos=delta;
        havePos=TRUE;
        
        break;
    case UITER_CURRENT:
        if(iter->index>=0) {
            pos=iter->index+delta;
            havePos=TRUE;
        } else {
            
            pos=0;
            havePos=FALSE;
        }
        break;
    case UITER_LIMIT:
    case UITER_LENGTH:
        if(iter->length>=0) {
            pos=iter->length+delta;
            havePos=TRUE;
        } else {
            
            iter->index=-1;
            iter->start=iter->limit;
            iter->reservedField=0;
            if(delta>=0) {
                return UITER_UNKNOWN_INDEX;
            } else {
                
                pos=0;
                havePos=FALSE;
            }
        }
        break;
    default:
        return -1;  
    }

    if(havePos) {
        
        if(pos<=0) {
            iter->index=iter->start=iter->reservedField=0;
            return 0;
        } else if(iter->length>=0 && pos>=iter->length) {
            iter->index=iter->length;
            iter->start=iter->limit;
            iter->reservedField=0;
            return iter->index;
        }

        
        if(iter->index<0 || pos<iter->index/2) {
            
            iter->index=iter->start=iter->reservedField=0;
        } else if(iter->length>=0 && (iter->length-pos)<(pos-iter->index)) {
            




            iter->index=iter->length;
            iter->start=iter->limit;
            iter->reservedField=0;
        }

        delta=pos-iter->index;
        if(delta==0) {
            return iter->index; 
        }
    } else {
        
        if(delta==0) {
            return UITER_UNKNOWN_INDEX; 
        } else if(-delta>=iter->start) {
            
            iter->index=iter->start=iter->reservedField=0;
            return 0;
        } else if(delta>=(iter->limit-iter->start)) {
            
            iter->index=iter->length; 
            iter->start=iter->limit;
            iter->reservedField=0;
            return iter->index>=0 ? iter->index : UITER_UNKNOWN_INDEX;
        }
    }

    

    
    s=(const uint8_t *)iter->context;
    pos=iter->index; 
    i=iter->start;
    if(delta>0) {
        
        int32_t limit=iter->limit;
        if(iter->reservedField!=0) {
            iter->reservedField=0;
            ++pos;
            --delta;
        }
        while(delta>0 && i<limit) {
            L8_NEXT(s, i, limit, c);
            if(c<0xffff) {
                ++pos;
                --delta;
            } else if(delta>=2) {
                pos+=2;
                delta-=2;
            } else  {
                
                iter->reservedField=c;
                ++pos;
                break; 
            }
        }
        if(i==limit) {
            if(iter->length<0 && iter->index>=0) {
                iter->length= iter->reservedField==0 ? pos : pos+1;
            } else if(iter->index<0 && iter->length>=0) {
                iter->index= iter->reservedField==0 ? iter->length : iter->length-1;
            }
        }
    } else  {
        
        if(iter->reservedField!=0) {
            iter->reservedField=0;
            i-=4; 
            --pos;
            ++delta;
        }
        while(delta<0 && i>0) {
            L8_PREV(s, 0, i, c);
            if(c<0xffff) {
                --pos;
                ++delta;
            } else if(delta<=-2) {
                pos-=2;
                delta+=2;
            } else  {
                
                i+=4; 
                iter->reservedField=c;
                --pos;
                break; 
            }
        }
    }

    iter->start=i;
    if(iter->index>=0) {
        return iter->index=pos;
    } else {
        
        if(i<=1) {
            return iter->index=i; 
        } else {
            
            return UITER_UNKNOWN_INDEX;
        }
    }
}

static UBool U_CALLCONV
lenient8IteratorHasNext(UCharIterator *iter) {
    return iter->reservedField!=0 || iter->start<iter->limit;
}

static UBool U_CALLCONV
lenient8IteratorHasPrevious(UCharIterator *iter) {
    return iter->start>0;
}

static UChar32 U_CALLCONV
lenient8IteratorCurrent(UCharIterator *iter) {
    if(iter->reservedField!=0) {
        return U16_TRAIL(iter->reservedField);
    } else if(iter->start<iter->limit) {
        const uint8_t *s=(const uint8_t *)iter->context;
        UChar32 c;
        int32_t i=iter->start;

        L8_NEXT(s, i, iter->limit, c);
        if(c<0) {
            return 0xfffd;
        } else if(c<=0xffff) {
            return c;
        } else {
            return U16_LEAD(c);
        }
    } else {
        return U_SENTINEL;
    }
}

static UChar32 U_CALLCONV
lenient8IteratorNext(UCharIterator *iter) {
    int32_t index;

    if(iter->reservedField!=0) {
        UChar trail=U16_TRAIL(iter->reservedField);
        iter->reservedField=0;
        if((index=iter->index)>=0) {
            iter->index=index+1;
        }
        return trail;
    } else if(iter->start<iter->limit) {
        const uint8_t *s=(const uint8_t *)iter->context;
        UChar32 c;

        L8_NEXT(s, iter->start, iter->limit, c);
        if((index=iter->index)>=0) {
            iter->index=++index;
            if(iter->length<0 && iter->start==iter->limit) {
                iter->length= c<=0xffff ? index : index+1;
            }
        } else if(iter->start==iter->limit && iter->length>=0) {
            iter->index= c<=0xffff ? iter->length : iter->length-1;
        }
        if(c<0) {
            return 0xfffd;
        } else if(c<=0xffff) {
            return c;
        } else {
            iter->reservedField=c;
            return U16_LEAD(c);
        }
    } else {
        return U_SENTINEL;
    }
}

static UChar32 U_CALLCONV
lenient8IteratorPrevious(UCharIterator *iter) {
    int32_t index;

    if(iter->reservedField!=0) {
        UChar lead=U16_LEAD(iter->reservedField);
        iter->reservedField=0;
        iter->start-=4; 
        if((index=iter->index)>0) {
            iter->index=index-1;
        }
        return lead;
    } else if(iter->start>0) {
        const uint8_t *s=(const uint8_t *)iter->context;
        UChar32 c;

        L8_PREV(s, 0, iter->start, c);
        if((index=iter->index)>0) {
            iter->index=index-1;
        } else if(iter->start<=1) {
            iter->index= c<=0xffff ? iter->start : iter->start+1;
        }
        if(c<0) {
            return 0xfffd;
        } else if(c<=0xffff) {
            return c;
        } else {
            iter->start+=4; 
            iter->reservedField=c;
            return U16_TRAIL(c);
        }
    } else {
        return U_SENTINEL;
    }
}

static uint32_t U_CALLCONV
lenient8IteratorGetState(const UCharIterator *iter) {
    uint32_t state=(uint32_t)(iter->start<<1);
    if(iter->reservedField!=0) {
        state|=1;
    }
    return state;
}

static void U_CALLCONV
lenient8IteratorSetState(UCharIterator *iter, uint32_t state, UErrorCode *pErrorCode) {
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        
    } else if(iter==NULL) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
    } else if(state==lenient8IteratorGetState(iter)) {
        
    } else {
        int32_t index=(int32_t)(state>>1); 
        state&=1; 

        if((state==0 ? index<0 : index<4) || iter->limit<index) {
            *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
        } else {
            iter->start=index; 
            if(index<=1) {
                iter->index=index;
            } else {
                iter->index=-1; 
            }
            if(state==0) {
                iter->reservedField=0;
            } else {
                
                UChar32 c;
                L8_PREV((const uint8_t *)iter->context, 0, index, c);
                if(c<=0xffff) {
                    *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
                } else {
                    iter->reservedField=c;
                }
            }
        }
    }
}

static const UCharIterator lenient8Iterator={
    0, 0, 0, 0, 0, 0,
    lenient8IteratorGetIndex,
    lenient8IteratorMove,
    lenient8IteratorHasNext,
    lenient8IteratorHasPrevious,
    lenient8IteratorCurrent,
    lenient8IteratorNext,
    lenient8IteratorPrevious,
    NULL,
    lenient8IteratorGetState,
    lenient8IteratorSetState
};

U_CAPI void U_EXPORT2
uiter_setLenient8(UCharIterator *iter, const char *s, int32_t length) {
    if(iter!=0) {
        if(s!=0 && length>=-1) {
            *iter=lenient8Iterator;
            iter->context=s;
            if(length>=0) {
                iter->limit=length;
            } else {
                iter->limit=strlen(s);
            }
            iter->length= iter->limit<=1 ? iter->limit : -1;
        } else {
            
            uiter_setString(iter, NULL, 0);
        }
    }
}
