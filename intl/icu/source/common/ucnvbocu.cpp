


















#include "unicode/utypes.h"

#if !UCONFIG_NO_CONVERSION

#include "unicode/ucnv.h"
#include "unicode/ucnv_cb.h"
#include "unicode/utf16.h"
#include "putilimp.h"
#include "ucnv_bld.h"
#include "ucnv_cnv.h"
#include "uassert.h"























#define BOCU1_ASCII_PREV        0x40


#define BOCU1_MIN               0x21
#define BOCU1_MIDDLE            0x90
#define BOCU1_MAX_LEAD          0xfe
#define BOCU1_MAX_TRAIL         0xff
#define BOCU1_RESET             0xff


#define BOCU1_COUNT             (BOCU1_MAX_LEAD-BOCU1_MIN+1)


#define BOCU1_TRAIL_CONTROLS_COUNT  20
#define BOCU1_TRAIL_BYTE_OFFSET     (BOCU1_MIN-BOCU1_TRAIL_CONTROLS_COUNT)


#define BOCU1_TRAIL_COUNT       ((BOCU1_MAX_TRAIL-BOCU1_MIN+1)+BOCU1_TRAIL_CONTROLS_COUNT)





#define BOCU1_SINGLE            64


#define BOCU1_LEAD_2            43
#define BOCU1_LEAD_3            3
#define BOCU1_LEAD_4            1


#define BOCU1_REACH_POS_1   (BOCU1_SINGLE-1)
#define BOCU1_REACH_NEG_1   (-BOCU1_SINGLE)


#define BOCU1_REACH_POS_2   (BOCU1_REACH_POS_1+BOCU1_LEAD_2*BOCU1_TRAIL_COUNT)
#define BOCU1_REACH_NEG_2   (BOCU1_REACH_NEG_1-BOCU1_LEAD_2*BOCU1_TRAIL_COUNT)


#define BOCU1_REACH_POS_3   \
    (BOCU1_REACH_POS_2+BOCU1_LEAD_3*BOCU1_TRAIL_COUNT*BOCU1_TRAIL_COUNT)

#define BOCU1_REACH_NEG_3   (BOCU1_REACH_NEG_2-BOCU1_LEAD_3*BOCU1_TRAIL_COUNT*BOCU1_TRAIL_COUNT)


#define BOCU1_START_POS_2   (BOCU1_MIDDLE+BOCU1_REACH_POS_1+1)
#define BOCU1_START_POS_3   (BOCU1_START_POS_2+BOCU1_LEAD_2)
#define BOCU1_START_POS_4   (BOCU1_START_POS_3+BOCU1_LEAD_3)
     

#define BOCU1_START_NEG_2   (BOCU1_MIDDLE+BOCU1_REACH_NEG_1)
#define BOCU1_START_NEG_3   (BOCU1_START_NEG_2-BOCU1_LEAD_2)
#define BOCU1_START_NEG_4   (BOCU1_START_NEG_3-BOCU1_LEAD_3)
     


#define BOCU1_LENGTH_FROM_LEAD(lead) \
    ((BOCU1_START_NEG_2<=(lead) && (lead)<BOCU1_START_POS_2) ? 1 : \
     (BOCU1_START_NEG_3<=(lead) && (lead)<BOCU1_START_POS_3) ? 2 : \
     (BOCU1_START_NEG_4<=(lead) && (lead)<BOCU1_START_POS_4) ? 3 : 4)


#define BOCU1_LENGTH_FROM_PACKED(packed) \
    ((uint32_t)(packed)<0x04000000 ? (packed)>>24 : 4)





























#define BOCU1_TRAIL_TO_BYTE(t) ((t)>=BOCU1_TRAIL_CONTROLS_COUNT ? (t)+BOCU1_TRAIL_BYTE_OFFSET : bocu1TrailToByte[t])







static const int8_t
bocu1ByteToTrail[BOCU1_MIN]={

    -1,   0x00, 0x01, 0x02, 0x03, 0x04, 0x05, -1,


    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,


    0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d,


    0x0e, 0x0f, -1,   -1,   0x10, 0x11, 0x12, 0x13,


    -1
};






static const int8_t
bocu1TrailToByte[BOCU1_TRAIL_CONTROLS_COUNT]={

    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x10, 0x11,


    0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,


    0x1c, 0x1d, 0x1e, 0x1f
};














#define NEGDIVMOD(n, d, m) { \
    (m)=(n)%(d); \
    (n)/=(d); \
    if((m)<0) { \
        --(n); \
        (m)+=(d); \
    } \
}




#define DIFF_IS_SINGLE(diff) (BOCU1_REACH_NEG_1<=(diff) && (diff)<=BOCU1_REACH_POS_1)


#define PACK_SINGLE_DIFF(diff) (BOCU1_MIDDLE+(diff))


#define DIFF_IS_DOUBLE(diff) (BOCU1_REACH_NEG_2<=(diff) && (diff)<=BOCU1_REACH_POS_2)



#define BOCU1_SIMPLE_PREV(c) (((c)&~0x7f)+BOCU1_ASCII_PREV)








static inline int32_t
bocu1Prev(int32_t c) {
    
    if( c<=0x309f) {
        
        return 0x3070;
    } else if(0x4e00<=c && c<=0x9fa5) {
        
        return 0x4e00-BOCU1_REACH_NEG_2;
    } else if(0xac00<=c ) {
        
        return (0xd7a3+0xac00)/2;
    } else {
        
        return BOCU1_SIMPLE_PREV(c);
    }
}


#define BOCU1_PREV(c) ((c)<0x3040 || (c)>0xd7a3 ? BOCU1_SIMPLE_PREV(c) : bocu1Prev(c))






























static int32_t
packDiff(int32_t diff) {
    int32_t result, m;

    U_ASSERT(!DIFF_IS_SINGLE(diff)); 
    if(diff>=BOCU1_REACH_NEG_1) {
        
#if 0   
        if(diff<=BOCU1_REACH_POS_1) {
            
            return 0x01000000|(BOCU1_MIDDLE+diff);
        } else
#endif
        if(diff<=BOCU1_REACH_POS_2) {
            
            diff-=BOCU1_REACH_POS_1+1;
            result=0x02000000;

            m=diff%BOCU1_TRAIL_COUNT;
            diff/=BOCU1_TRAIL_COUNT;
            result|=BOCU1_TRAIL_TO_BYTE(m);

            result|=(BOCU1_START_POS_2+diff)<<8;
        } else if(diff<=BOCU1_REACH_POS_3) {
            
            diff-=BOCU1_REACH_POS_2+1;
            result=0x03000000;

            m=diff%BOCU1_TRAIL_COUNT;
            diff/=BOCU1_TRAIL_COUNT;
            result|=BOCU1_TRAIL_TO_BYTE(m);

            m=diff%BOCU1_TRAIL_COUNT;
            diff/=BOCU1_TRAIL_COUNT;
            result|=BOCU1_TRAIL_TO_BYTE(m)<<8;

            result|=(BOCU1_START_POS_3+diff)<<16;
        } else {
            
            diff-=BOCU1_REACH_POS_3+1;

            m=diff%BOCU1_TRAIL_COUNT;
            diff/=BOCU1_TRAIL_COUNT;
            result=BOCU1_TRAIL_TO_BYTE(m);

            m=diff%BOCU1_TRAIL_COUNT;
            diff/=BOCU1_TRAIL_COUNT;
            result|=BOCU1_TRAIL_TO_BYTE(m)<<8;

            



            result|=BOCU1_TRAIL_TO_BYTE(diff)<<16;

            result|=((uint32_t)BOCU1_START_POS_4)<<24;
        }
    } else {
        
        if(diff>=BOCU1_REACH_NEG_2) {
            
            diff-=BOCU1_REACH_NEG_1;
            result=0x02000000;

            NEGDIVMOD(diff, BOCU1_TRAIL_COUNT, m);
            result|=BOCU1_TRAIL_TO_BYTE(m);

            result|=(BOCU1_START_NEG_2+diff)<<8;
        } else if(diff>=BOCU1_REACH_NEG_3) {
            
            diff-=BOCU1_REACH_NEG_2;
            result=0x03000000;

            NEGDIVMOD(diff, BOCU1_TRAIL_COUNT, m);
            result|=BOCU1_TRAIL_TO_BYTE(m);

            NEGDIVMOD(diff, BOCU1_TRAIL_COUNT, m);
            result|=BOCU1_TRAIL_TO_BYTE(m)<<8;

            result|=(BOCU1_START_NEG_3+diff)<<16;
        } else {
            
            diff-=BOCU1_REACH_NEG_3;

            NEGDIVMOD(diff, BOCU1_TRAIL_COUNT, m);
            result=BOCU1_TRAIL_TO_BYTE(m);

            NEGDIVMOD(diff, BOCU1_TRAIL_COUNT, m);
            result|=BOCU1_TRAIL_TO_BYTE(m)<<8;

            




            m=diff+BOCU1_TRAIL_COUNT;
            result|=BOCU1_TRAIL_TO_BYTE(m)<<16;

            result|=BOCU1_MIN<<24;
        }
    }
    return result;
}


static void
_Bocu1FromUnicodeWithOffsets(UConverterFromUnicodeArgs *pArgs,
                             UErrorCode *pErrorCode) {
    UConverter *cnv;
    const UChar *source, *sourceLimit;
    uint8_t *target;
    int32_t targetCapacity;
    int32_t *offsets;

    int32_t prev, c, diff;

    int32_t sourceIndex, nextSourceIndex;

U_ALIGN_CODE(16)

    
    cnv=pArgs->converter;
    source=pArgs->source;
    sourceLimit=pArgs->sourceLimit;
    target=(uint8_t *)pArgs->target;
    targetCapacity=(int32_t)(pArgs->targetLimit-pArgs->target);
    offsets=pArgs->offsets;

    
    c=cnv->fromUChar32;
    prev=(int32_t)cnv->fromUnicodeStatus;
    if(prev==0) {
        prev=BOCU1_ASCII_PREV;
    }

    
    sourceIndex= c==0 ? 0 : -1;
    nextSourceIndex=0;

    
    if(c!=0 && targetCapacity>0) {
        goto getTrail;
    }

fastSingle:
    
    
    diff=(int32_t)(sourceLimit-source);
    if(targetCapacity>diff) {
        targetCapacity=diff;
    }
    while(targetCapacity>0 && (c=*source)<0x3000) {
        if(c<=0x20) {
            if(c!=0x20) {
                prev=BOCU1_ASCII_PREV;
            }
            *target++=(uint8_t)c;
            *offsets++=nextSourceIndex++;
            ++source;
            --targetCapacity;
        } else {
            diff=c-prev;
            if(DIFF_IS_SINGLE(diff)) {
                prev=BOCU1_SIMPLE_PREV(c);
                *target++=(uint8_t)PACK_SINGLE_DIFF(diff);
                *offsets++=nextSourceIndex++;
                ++source;
                --targetCapacity;
            } else {
                break;
            }
        }
    }
    
    targetCapacity=(int32_t)((const uint8_t *)pArgs->targetLimit-target);
    sourceIndex=nextSourceIndex; 

    
    while(source<sourceLimit) {
        if(targetCapacity>0) {
            c=*source++;
            ++nextSourceIndex;

            if(c<=0x20) {
                




                if(c!=0x20) {
                    prev=BOCU1_ASCII_PREV;
                }
                *target++=(uint8_t)c;
                *offsets++=sourceIndex;
                --targetCapacity;

                sourceIndex=nextSourceIndex;
                continue;
            }

            if(U16_IS_LEAD(c)) {
getTrail:
                if(source<sourceLimit) {
                    
                    UChar trail=*source;
                    if(U16_IS_TRAIL(trail)) {
                        ++source;
                        ++nextSourceIndex;
                        c=U16_GET_SUPPLEMENTARY(c, trail);
                    }
                } else {
                    
                    c=-c; 
                    break;
                }
            }

            








            diff=c-prev;
            prev=BOCU1_PREV(c);
            if(DIFF_IS_SINGLE(diff)) {
                *target++=(uint8_t)PACK_SINGLE_DIFF(diff);
                *offsets++=sourceIndex;
                --targetCapacity;
                sourceIndex=nextSourceIndex;
                if(c<0x3000) {
                    goto fastSingle;
                }
            } else if(DIFF_IS_DOUBLE(diff) && 2<=targetCapacity) {
                
                int32_t m;

                if(diff>=0) {
                    diff-=BOCU1_REACH_POS_1+1;
                    m=diff%BOCU1_TRAIL_COUNT;
                    diff/=BOCU1_TRAIL_COUNT;
                    diff+=BOCU1_START_POS_2;
                } else {
                    diff-=BOCU1_REACH_NEG_1;
                    NEGDIVMOD(diff, BOCU1_TRAIL_COUNT, m);
                    diff+=BOCU1_START_NEG_2;
                }
                *target++=(uint8_t)diff;
                *target++=(uint8_t)BOCU1_TRAIL_TO_BYTE(m);
                *offsets++=sourceIndex;
                *offsets++=sourceIndex;
                targetCapacity-=2;
                sourceIndex=nextSourceIndex;
            } else {
                int32_t length; 

                diff=packDiff(diff);
                length=BOCU1_LENGTH_FROM_PACKED(diff);

                
                
                if(length<=targetCapacity) {
                    switch(length) {
                        
                    case 4:
                        *target++=(uint8_t)(diff>>24);
                        *offsets++=sourceIndex;
                    case 3: 
                        *target++=(uint8_t)(diff>>16);
                        *offsets++=sourceIndex;
                    case 2: 
                        *target++=(uint8_t)(diff>>8);
                        *offsets++=sourceIndex;
                    
                        *target++=(uint8_t)diff;
                        *offsets++=sourceIndex;
                    default:
                        
                        break;
                    }
                    targetCapacity-=length;
                    sourceIndex=nextSourceIndex;
                } else {
                    uint8_t *charErrorBuffer;

                    





                    
                    length-=targetCapacity;
                    charErrorBuffer=(uint8_t *)cnv->charErrorBuffer;
                    switch(length) {
                        
                    case 3:
                        *charErrorBuffer++=(uint8_t)(diff>>16);
                    case 2: 
                        *charErrorBuffer++=(uint8_t)(diff>>8);
                    case 1: 
                        *charErrorBuffer=(uint8_t)diff;
                    default:
                        
                        break;
                    }
                    cnv->charErrorBufferLength=(int8_t)length;

                    
                    diff>>=8*length; 
                    switch(targetCapacity) {
                        
                    case 3:
                        *target++=(uint8_t)(diff>>16);
                        *offsets++=sourceIndex;
                    case 2: 
                        *target++=(uint8_t)(diff>>8);
                        *offsets++=sourceIndex;
                    case 1: 
                        *target++=(uint8_t)diff;
                        *offsets++=sourceIndex;
                    default:
                        
                        break;
                    }

                    
                    targetCapacity=0;
                    *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                    break;
                }
            }
        } else {
            
            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
            break;
        }
    }

    
    cnv->fromUChar32= c<0 ? -c : 0;
    cnv->fromUnicodeStatus=(uint32_t)prev;

    
    pArgs->source=source;
    pArgs->target=(char *)target;
    pArgs->offsets=offsets;
}








static void
_Bocu1FromUnicode(UConverterFromUnicodeArgs *pArgs,
                  UErrorCode *pErrorCode) {
    UConverter *cnv;
    const UChar *source, *sourceLimit;
    uint8_t *target;
    int32_t targetCapacity;

    int32_t prev, c, diff;

    
    cnv=pArgs->converter;
    source=pArgs->source;
    sourceLimit=pArgs->sourceLimit;
    target=(uint8_t *)pArgs->target;
    targetCapacity=(int32_t)(pArgs->targetLimit-pArgs->target);

    
    c=cnv->fromUChar32;
    prev=(int32_t)cnv->fromUnicodeStatus;
    if(prev==0) {
        prev=BOCU1_ASCII_PREV;
    }

    
    if(c!=0 && targetCapacity>0) {
        goto getTrail;
    }

fastSingle:
    
    
    diff=(int32_t)(sourceLimit-source);
    if(targetCapacity>diff) {
        targetCapacity=diff;
    }
    while(targetCapacity>0 && (c=*source)<0x3000) {
        if(c<=0x20) {
            if(c!=0x20) {
                prev=BOCU1_ASCII_PREV;
            }
            *target++=(uint8_t)c;
        } else {
            diff=c-prev;
            if(DIFF_IS_SINGLE(diff)) {
                prev=BOCU1_SIMPLE_PREV(c);
                *target++=(uint8_t)PACK_SINGLE_DIFF(diff);
            } else {
                break;
            }
        }
        ++source;
        --targetCapacity;
    }
    
    targetCapacity=(int32_t)((const uint8_t *)pArgs->targetLimit-target);

    
    while(source<sourceLimit) {
        if(targetCapacity>0) {
            c=*source++;

            if(c<=0x20) {
                




                if(c!=0x20) {
                    prev=BOCU1_ASCII_PREV;
                }
                *target++=(uint8_t)c;
                --targetCapacity;
                continue;
            }

            if(U16_IS_LEAD(c)) {
getTrail:
                if(source<sourceLimit) {
                    
                    UChar trail=*source;
                    if(U16_IS_TRAIL(trail)) {
                        ++source;
                        c=U16_GET_SUPPLEMENTARY(c, trail);
                    }
                } else {
                    
                    c=-c; 
                    break;
                }
            }

            








            diff=c-prev;
            prev=BOCU1_PREV(c);
            if(DIFF_IS_SINGLE(diff)) {
                *target++=(uint8_t)PACK_SINGLE_DIFF(diff);
                --targetCapacity;
                if(c<0x3000) {
                    goto fastSingle;
                }
            } else if(DIFF_IS_DOUBLE(diff) && 2<=targetCapacity) {
                
                int32_t m;

                if(diff>=0) {
                    diff-=BOCU1_REACH_POS_1+1;
                    m=diff%BOCU1_TRAIL_COUNT;
                    diff/=BOCU1_TRAIL_COUNT;
                    diff+=BOCU1_START_POS_2;
                } else {
                    diff-=BOCU1_REACH_NEG_1;
                    NEGDIVMOD(diff, BOCU1_TRAIL_COUNT, m);
                    diff+=BOCU1_START_NEG_2;
                }
                *target++=(uint8_t)diff;
                *target++=(uint8_t)BOCU1_TRAIL_TO_BYTE(m);
                targetCapacity-=2;
            } else {
                int32_t length; 

                diff=packDiff(diff);
                length=BOCU1_LENGTH_FROM_PACKED(diff);

                
                
                if(length<=targetCapacity) {
                    switch(length) {
                        
                    case 4:
                        *target++=(uint8_t)(diff>>24);
                    case 3: 
                        *target++=(uint8_t)(diff>>16);
                    
                        *target++=(uint8_t)(diff>>8);
                    
                        *target++=(uint8_t)diff;
                    default:
                        
                        break;
                    }
                    targetCapacity-=length;
                } else {
                    uint8_t *charErrorBuffer;

                    





                    
                    length-=targetCapacity;
                    charErrorBuffer=(uint8_t *)cnv->charErrorBuffer;
                    switch(length) {
                        
                    case 3:
                        *charErrorBuffer++=(uint8_t)(diff>>16);
                    case 2: 
                        *charErrorBuffer++=(uint8_t)(diff>>8);
                    case 1: 
                        *charErrorBuffer=(uint8_t)diff;
                    default:
                        
                        break;
                    }
                    cnv->charErrorBufferLength=(int8_t)length;

                    
                    diff>>=8*length; 
                    switch(targetCapacity) {
                        
                    case 3:
                        *target++=(uint8_t)(diff>>16);
                    case 2: 
                        *target++=(uint8_t)(diff>>8);
                    case 1: 
                        *target++=(uint8_t)diff;
                    default:
                        
                        break;
                    }

                    
                    targetCapacity=0;
                    *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                    break;
                }
            }
        } else {
            
            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
            break;
        }
    }

    
    cnv->fromUChar32= c<0 ? -c : 0;
    cnv->fromUnicodeStatus=(uint32_t)prev;

    
    pArgs->source=source;
    pArgs->target=(char *)target;
}










static inline int32_t
decodeBocu1LeadByte(int32_t b) {
    int32_t diff, count;

    if(b>=BOCU1_START_NEG_2) {
        
        if(b<BOCU1_START_POS_3) {
            
            diff=((int32_t)b-BOCU1_START_POS_2)*BOCU1_TRAIL_COUNT+BOCU1_REACH_POS_1+1;
            count=1;
        } else if(b<BOCU1_START_POS_4) {
            
            diff=((int32_t)b-BOCU1_START_POS_3)*BOCU1_TRAIL_COUNT*BOCU1_TRAIL_COUNT+BOCU1_REACH_POS_2+1;
            count=2;
        } else {
            
            diff=BOCU1_REACH_POS_3+1;
            count=3;
        }
    } else {
        
        if(b>=BOCU1_START_NEG_3) {
            
            diff=((int32_t)b-BOCU1_START_NEG_2)*BOCU1_TRAIL_COUNT+BOCU1_REACH_NEG_1;
            count=1;
        } else if(b>BOCU1_MIN) {
            
            diff=((int32_t)b-BOCU1_START_NEG_3)*BOCU1_TRAIL_COUNT*BOCU1_TRAIL_COUNT+BOCU1_REACH_NEG_2;
            count=2;
        } else {
            
            diff=-BOCU1_TRAIL_COUNT*BOCU1_TRAIL_COUNT*BOCU1_TRAIL_COUNT+BOCU1_REACH_NEG_3;
            count=3;
        }
    }

    
    return (diff<<2)|count;
}










static inline int32_t
decodeBocu1TrailByte(int32_t count, int32_t b) {
    if(b<=0x20) {
        
        b=bocu1ByteToTrail[b];
        
#if BOCU1_MAX_TRAIL<0xff
    } else if(b>BOCU1_MAX_TRAIL) {
        return -99;
#endif
    } else {
        b-=BOCU1_TRAIL_BYTE_OFFSET;
    }

    
    if(count==1) {
        return b;
    } else if(count==2) {
        return b*BOCU1_TRAIL_COUNT;
    } else  {
        return b*(BOCU1_TRAIL_COUNT*BOCU1_TRAIL_COUNT);
    }
}

static void
_Bocu1ToUnicodeWithOffsets(UConverterToUnicodeArgs *pArgs,
                           UErrorCode *pErrorCode) {
    UConverter *cnv;
    const uint8_t *source, *sourceLimit;
    UChar *target;
    const UChar *targetLimit;
    int32_t *offsets;

    int32_t prev, count, diff, c;

    int8_t byteIndex;
    uint8_t *bytes;

    int32_t sourceIndex, nextSourceIndex;

    
    cnv=pArgs->converter;
    source=(const uint8_t *)pArgs->source;
    sourceLimit=(const uint8_t *)pArgs->sourceLimit;
    target=pArgs->target;
    targetLimit=pArgs->targetLimit;
    offsets=pArgs->offsets;

    
    prev=(int32_t)cnv->toUnicodeStatus;
    if(prev==0) {
        prev=BOCU1_ASCII_PREV;
    }
    diff=cnv->mode; 
    count=diff&3;
    diff>>=2;

    byteIndex=cnv->toULength;
    bytes=cnv->toUBytes;

    
    sourceIndex=byteIndex==0 ? 0 : -1;
    nextSourceIndex=0;

    
    if(count>0 && byteIndex>0 && target<targetLimit) {
        goto getTrail;
    }

fastSingle:
    
    
    diff=(int32_t)(sourceLimit-source);
    count=(int32_t)(pArgs->targetLimit-target);
    if(count>diff) {
        count=diff;
    }
    while(count>0) {
        if(BOCU1_START_NEG_2<=(c=*source) && c<BOCU1_START_POS_2) {
            c=prev+(c-BOCU1_MIDDLE);
            if(c<0x3000) {
                *target++=(UChar)c;
                *offsets++=nextSourceIndex++;
                prev=BOCU1_SIMPLE_PREV(c);
            } else {
                break;
            }
        } else if(c<=0x20) {
            if(c!=0x20) {
                prev=BOCU1_ASCII_PREV;
            }
            *target++=(UChar)c;
            *offsets++=nextSourceIndex++;
        } else {
            break;
        }
        ++source;
        --count;
    }
    sourceIndex=nextSourceIndex; 

    
    while(source<sourceLimit) {
        if(target>=targetLimit) {
            
            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
            break;
        }

        ++nextSourceIndex;
        c=*source++;
        if(BOCU1_START_NEG_2<=c && c<BOCU1_START_POS_2) {
            
            c=prev+(c-BOCU1_MIDDLE);
            if(c<0x3000) {
                *target++=(UChar)c;
                *offsets++=sourceIndex;
                prev=BOCU1_SIMPLE_PREV(c);
                sourceIndex=nextSourceIndex;
                goto fastSingle;
            }
        } else if(c<=0x20) {
            



            if(c!=0x20) {
                prev=BOCU1_ASCII_PREV;
            }
            *target++=(UChar)c;
            *offsets++=sourceIndex;
            sourceIndex=nextSourceIndex;
            continue;
        } else if(BOCU1_START_NEG_3<=c && c<BOCU1_START_POS_3 && source<sourceLimit) {
            
            if(c>=BOCU1_MIDDLE) {
                diff=((int32_t)c-BOCU1_START_POS_2)*BOCU1_TRAIL_COUNT+BOCU1_REACH_POS_1+1;
            } else {
                diff=((int32_t)c-BOCU1_START_NEG_2)*BOCU1_TRAIL_COUNT+BOCU1_REACH_NEG_1;
            }

            
            ++nextSourceIndex;
            c=decodeBocu1TrailByte(1, *source++);
            if(c<0 || (uint32_t)(c=prev+diff+c)>0x10ffff) {
                bytes[0]=source[-2];
                bytes[1]=source[-1];
                byteIndex=2;
                *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                break;
            }
        } else if(c==BOCU1_RESET) {
            
            prev=BOCU1_ASCII_PREV;
            sourceIndex=nextSourceIndex;
            continue;
        } else {
            




            bytes[0]=(uint8_t)c;
            byteIndex=1;

            diff=decodeBocu1LeadByte(c);
            count=diff&3;
            diff>>=2;
getTrail:
            for(;;) {
                if(source>=sourceLimit) {
                    goto endloop;
                }
                ++nextSourceIndex;
                c=bytes[byteIndex++]=*source++;

                
                c=decodeBocu1TrailByte(count, c);
                if(c<0) {
                    *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                    goto endloop;
                }

                diff+=c;
                if(--count==0) {
                    
                    byteIndex=0;
                    c=prev+diff;
                    if((uint32_t)c>0x10ffff) {
                        *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                        goto endloop;
                    }
                    break;
                }
            }
        }

        
        prev=BOCU1_PREV(c);
        if(c<=0xffff) {
            *target++=(UChar)c;
            *offsets++=sourceIndex;
        } else {
            
            *target++=U16_LEAD(c);
            if(target<targetLimit) {
                *target++=U16_TRAIL(c);
                *offsets++=sourceIndex;
                *offsets++=sourceIndex;
            } else {
                
                *offsets++=sourceIndex;
                cnv->UCharErrorBuffer[0]=U16_TRAIL(c);
                cnv->UCharErrorBufferLength=1;
                *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                break;
            }
        }
        sourceIndex=nextSourceIndex;
    }
endloop:

    if(*pErrorCode==U_ILLEGAL_CHAR_FOUND) {
        
        cnv->toUnicodeStatus=BOCU1_ASCII_PREV;
        cnv->mode=0;
    } else {
        
        cnv->toUnicodeStatus=(uint32_t)prev;
        cnv->mode=(diff<<2)|count;
    }
    cnv->toULength=byteIndex;

    
    pArgs->source=(const char *)source;
    pArgs->target=target;
    pArgs->offsets=offsets;
    return;
}








static void
_Bocu1ToUnicode(UConverterToUnicodeArgs *pArgs,
                UErrorCode *pErrorCode) {
    UConverter *cnv;
    const uint8_t *source, *sourceLimit;
    UChar *target;
    const UChar *targetLimit;

    int32_t prev, count, diff, c;

    int8_t byteIndex;
    uint8_t *bytes;

U_ALIGN_CODE(16)

    
    cnv=pArgs->converter;
    source=(const uint8_t *)pArgs->source;
    sourceLimit=(const uint8_t *)pArgs->sourceLimit;
    target=pArgs->target;
    targetLimit=pArgs->targetLimit;

    
    prev=(int32_t)cnv->toUnicodeStatus;
    if(prev==0) {
        prev=BOCU1_ASCII_PREV;
    }
    diff=cnv->mode; 
    count=diff&3;
    diff>>=2;

    byteIndex=cnv->toULength;
    bytes=cnv->toUBytes;

    
    if(count>0 && byteIndex>0 && target<targetLimit) {
        goto getTrail;
    }

fastSingle:
    
    
    diff=(int32_t)(sourceLimit-source);
    count=(int32_t)(pArgs->targetLimit-target);
    if(count>diff) {
        count=diff;
    }
    while(count>0) {
        if(BOCU1_START_NEG_2<=(c=*source) && c<BOCU1_START_POS_2) {
            c=prev+(c-BOCU1_MIDDLE);
            if(c<0x3000) {
                *target++=(UChar)c;
                prev=BOCU1_SIMPLE_PREV(c);
            } else {
                break;
            }
        } else if(c<=0x20) {
            if(c!=0x20) {
                prev=BOCU1_ASCII_PREV;
            }
            *target++=(UChar)c;
        } else {
            break;
        }
        ++source;
        --count;
    }

    
    while(source<sourceLimit) {
        if(target>=targetLimit) {
            
            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
            break;
        }

        c=*source++;
        if(BOCU1_START_NEG_2<=c && c<BOCU1_START_POS_2) {
            
            c=prev+(c-BOCU1_MIDDLE);
            if(c<0x3000) {
                *target++=(UChar)c;
                prev=BOCU1_SIMPLE_PREV(c);
                goto fastSingle;
            }
        } else if(c<=0x20) {
            



            if(c!=0x20) {
                prev=BOCU1_ASCII_PREV;
            }
            *target++=(UChar)c;
            continue;
        } else if(BOCU1_START_NEG_3<=c && c<BOCU1_START_POS_3 && source<sourceLimit) {
            
            if(c>=BOCU1_MIDDLE) {
                diff=((int32_t)c-BOCU1_START_POS_2)*BOCU1_TRAIL_COUNT+BOCU1_REACH_POS_1+1;
            } else {
                diff=((int32_t)c-BOCU1_START_NEG_2)*BOCU1_TRAIL_COUNT+BOCU1_REACH_NEG_1;
            }

            
            c=decodeBocu1TrailByte(1, *source++);
            if(c<0 || (uint32_t)(c=prev+diff+c)>0x10ffff) {
                bytes[0]=source[-2];
                bytes[1]=source[-1];
                byteIndex=2;
                *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                break;
            }
        } else if(c==BOCU1_RESET) {
            
            prev=BOCU1_ASCII_PREV;
            continue;
        } else {
            




            bytes[0]=(uint8_t)c;
            byteIndex=1;

            diff=decodeBocu1LeadByte(c);
            count=diff&3;
            diff>>=2;
getTrail:
            for(;;) {
                if(source>=sourceLimit) {
                    goto endloop;
                }
                c=bytes[byteIndex++]=*source++;

                
                c=decodeBocu1TrailByte(count, c);
                if(c<0) {
                    *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                    goto endloop;
                }

                diff+=c;
                if(--count==0) {
                    
                    byteIndex=0;
                    c=prev+diff;
                    if((uint32_t)c>0x10ffff) {
                        *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                        goto endloop;
                    }
                    break;
                }
            }
        }

        
        prev=BOCU1_PREV(c);
        if(c<=0xffff) {
            *target++=(UChar)c;
        } else {
            
            *target++=U16_LEAD(c);
            if(target<targetLimit) {
                *target++=U16_TRAIL(c);
            } else {
                
                cnv->UCharErrorBuffer[0]=U16_TRAIL(c);
                cnv->UCharErrorBufferLength=1;
                *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                break;
            }
        }
    }
endloop:

    if(*pErrorCode==U_ILLEGAL_CHAR_FOUND) {
        
        cnv->toUnicodeStatus=BOCU1_ASCII_PREV;
        cnv->mode=0;
    } else {
        
        cnv->toUnicodeStatus=(uint32_t)prev;
        cnv->mode=(diff<<2)|count;
    }
    cnv->toULength=byteIndex;

    
    pArgs->source=(const char *)source;
    pArgs->target=target;
    return;
}



static const UConverterImpl _Bocu1Impl={
    UCNV_BOCU1,

    NULL,
    NULL,

    NULL,
    NULL,
    NULL,

    _Bocu1ToUnicode,
    _Bocu1ToUnicodeWithOffsets,
    _Bocu1FromUnicode,
    _Bocu1FromUnicodeWithOffsets,
    NULL,

    NULL,
    NULL,
    NULL,
    NULL,
    ucnv_getCompleteUnicodeSet,

    NULL,
    NULL
};

static const UConverterStaticData _Bocu1StaticData={
    sizeof(UConverterStaticData),
    "BOCU-1",
    1214, 
    UCNV_IBM, UCNV_BOCU1,
    1, 4, 
    { 0x1a, 0, 0, 0 }, 1, 
    FALSE, FALSE,
    0,
    0,
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } 
};

const UConverterSharedData _Bocu1Data={
    sizeof(UConverterSharedData), ~((uint32_t)0),
    NULL, NULL, &_Bocu1StaticData, FALSE, &_Bocu1Impl,
    0,
    UCNV_MBCS_TABLE_INITIALIZER
};

#endif
