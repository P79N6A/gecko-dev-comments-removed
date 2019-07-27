





























#include "unicode/utypes.h"
#include "unicode/ustring.h"
#include "unicode/ucnv.h"
#include "unicode/utf16.h"
#include "cmemory.h"
#include "cintltst.h"

























#define BOCU1_ASCII_PREV        0x40


#define BOCU1_MIN               0x21
#define BOCU1_MIDDLE            0x90
#define BOCU1_MAX_LEAD          0xfe


#define BOCU1_MAX_TRAIL         0xffL
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


struct Bocu1Rx {
    int32_t prev, count, diff;
};

typedef struct Bocu1Rx Bocu1Rx;




U_CFUNC int32_t
packDiff(int32_t diff);

U_CFUNC int32_t
encodeBocu1(int32_t *pPrev, int32_t c);

U_CFUNC int32_t
decodeBocu1(Bocu1Rx *pRx, uint8_t b);












static int32_t
bocu1Prev(int32_t c) {
    
    if(0x3040<=c && c<=0x309f) {
        
        return 0x3070;
    } else if(0x4e00<=c && c<=0x9fa5) {
        
        return 0x4e00-BOCU1_REACH_NEG_2;
    } else if(0xac00<=c && c<=0xd7a3) {
        
        return ((int32_t)0xd7a3+(int32_t)0xac00)/2;
    } else {
        
        return (c&~0x7f)+BOCU1_ASCII_PREV;
    }
}















U_CFUNC int32_t
packDiff(int32_t diff) {
    int32_t result, m, lead, count, shift;

    if(diff>=BOCU1_REACH_NEG_1) {
        
        if(diff<=BOCU1_REACH_POS_1) {
            
            return 0x01000000|(BOCU1_MIDDLE+diff);
        } else if(diff<=BOCU1_REACH_POS_2) {
            
            diff-=BOCU1_REACH_POS_1+1;
            lead=BOCU1_START_POS_2;
            count=1;
        } else if(diff<=BOCU1_REACH_POS_3) {
            
            diff-=BOCU1_REACH_POS_2+1;
            lead=BOCU1_START_POS_3;
            count=2;
        } else {
            
            diff-=BOCU1_REACH_POS_3+1;
            lead=BOCU1_START_POS_4;
            count=3;
        }
    } else {
        
        if(diff>=BOCU1_REACH_NEG_2) {
            
            diff-=BOCU1_REACH_NEG_1;
            lead=BOCU1_START_NEG_2;
            count=1;
        } else if(diff>=BOCU1_REACH_NEG_3) {
            
            diff-=BOCU1_REACH_NEG_2;
            lead=BOCU1_START_NEG_3;
            count=2;
        } else {
            
            diff-=BOCU1_REACH_NEG_3;
            lead=BOCU1_START_NEG_4;
            count=3;
        }
    }

    
    if(count<3) {
        result=(count+1)<<24;
    } else  {
        result=0;
    }

    
    shift=0;
    do {
        NEGDIVMOD(diff, BOCU1_TRAIL_COUNT, m);
        result|=BOCU1_TRAIL_TO_BYTE(m)<<shift;
        shift+=8;
    } while(--count>0);

    
    result|=(lead+diff)<<shift;

    return result;
}














U_CFUNC int32_t
encodeBocu1(int32_t *pPrev, int32_t c) {
    int32_t prev;

    if(pPrev==NULL || c<0 || c>0x10ffff) {
        
        return 0;
    }

    prev=*pPrev;
    if(prev==0) {
        
        prev=*pPrev=BOCU1_ASCII_PREV;
    }

    if(c<=0x20) {
        




        if(c!=0x20) {
            *pPrev=BOCU1_ASCII_PREV;
        }
        return 0x01000000|c;
    }

    








    *pPrev=bocu1Prev(c);
    return packDiff(c-prev);
}











static int32_t
decodeBocu1LeadByte(Bocu1Rx *pRx, uint8_t b) {
    int32_t c, count;

    if(b>=BOCU1_START_NEG_2) {
        
        if(b<BOCU1_START_POS_3) {
            
            c=((int32_t)b-BOCU1_START_POS_2)*BOCU1_TRAIL_COUNT+BOCU1_REACH_POS_1+1;
            count=1;
        } else if(b<BOCU1_START_POS_4) {
            
            c=((int32_t)b-BOCU1_START_POS_3)*BOCU1_TRAIL_COUNT*BOCU1_TRAIL_COUNT+BOCU1_REACH_POS_2+1;
            count=2;
        } else {
            
            c=BOCU1_REACH_POS_3+1;
            count=3;
        }
    } else {
        
        if(b>=BOCU1_START_NEG_3) {
            
            c=((int32_t)b-BOCU1_START_NEG_2)*BOCU1_TRAIL_COUNT+BOCU1_REACH_NEG_1;
            count=1;
        } else if(b>BOCU1_MIN) {
            
            c=((int32_t)b-BOCU1_START_NEG_3)*BOCU1_TRAIL_COUNT*BOCU1_TRAIL_COUNT+BOCU1_REACH_NEG_2;
            count=2;
        } else {
            
            c=-BOCU1_TRAIL_COUNT*BOCU1_TRAIL_COUNT*BOCU1_TRAIL_COUNT+BOCU1_REACH_NEG_3;
            count=3;
        }
    }

    
    pRx->diff=c;
    pRx->count=count;
    return -1;
}










static int32_t
decodeBocu1TrailByte(Bocu1Rx *pRx, uint8_t b) {
    int32_t t, c, count;

    if(b<=0x20) {
        
        t=bocu1ByteToTrail[b];
        if(t<0) {
            
            pRx->prev=BOCU1_ASCII_PREV;
            pRx->count=0;
            return -99;
        }
#if BOCU1_MAX_TRAIL<0xff
    } else if(b>BOCU1_MAX_TRAIL) {
        return -99;
#endif
    } else {
        t=(int32_t)b-BOCU1_TRAIL_BYTE_OFFSET;
    }

    
    c=pRx->diff;
    count=pRx->count;

    if(count==1) {
        
        c=pRx->prev+c+t;
        if(0<=c && c<=0x10ffff) {
            
            pRx->prev=bocu1Prev(c);
            pRx->count=0;
            return c;
        } else {
            
            pRx->prev=BOCU1_ASCII_PREV;
            pRx->count=0;
            return -99;
        }
    }

    
    if(count==2) {
        pRx->diff=c+t*BOCU1_TRAIL_COUNT;
    } else  {
        pRx->diff=c+t*BOCU1_TRAIL_COUNT*BOCU1_TRAIL_COUNT;
    }
    pRx->count=count-1;
    return -1;
}













U_CFUNC int32_t
decodeBocu1(Bocu1Rx *pRx, uint8_t b) {
    int32_t prev, c, count;

    if(pRx==NULL) {
        
        return -99;
    }

    prev=pRx->prev;
    if(prev==0) {
        
        prev=pRx->prev=BOCU1_ASCII_PREV;
        count=pRx->count=0;
    } else {
        count=pRx->count;
    }

    if(count==0) {
        
        if(b<=0x20) {
            



            if(b!=0x20) {
                pRx->prev=BOCU1_ASCII_PREV;
            }
            return b;
        }

        











        if(b>=BOCU1_START_NEG_2 && b<BOCU1_START_POS_2) {
            
            c=prev+((int32_t)b-BOCU1_MIDDLE);
            pRx->prev=bocu1Prev(c);
            return c;
        } else if(b==BOCU1_RESET) {
            
            pRx->prev=BOCU1_ASCII_PREV;
            return -1;
        } else {
            return decodeBocu1LeadByte(pRx, b);
        }
    } else {
        
        return decodeBocu1TrailByte(pRx, b);
    }
}








#define TEST_IGNORE_COMMA       1












static int32_t
writePacked(int32_t packed, uint8_t *p) {
    int32_t count=BOCU1_LENGTH_FROM_PACKED(packed);
    switch(count) {
    case 4:
        *p++=(uint8_t)(packed>>24);
    case 3:
        *p++=(uint8_t)(packed>>16);
    case 2:
        *p++=(uint8_t)(packed>>8);
    case 1:
        *p++=(uint8_t)packed;
    default:
        break;
    }

    return count;
}















static int32_t
unpackDiff(int32_t initialPrev, int32_t packed) {
    Bocu1Rx rx={ 0, 0, 0 };
    int32_t count;

    rx.prev=initialPrev;
    count=BOCU1_LENGTH_FROM_PACKED(packed);
    switch(count) {
    case 4:
        decodeBocu1(&rx, (uint8_t)(packed>>24));
    case 3:
        decodeBocu1(&rx, (uint8_t)(packed>>16));
    case 2:
        decodeBocu1(&rx, (uint8_t)(packed>>8));
    case 1:
        
        return decodeBocu1(&rx, (uint8_t)packed)-initialPrev;
    default:
        return -0x7fffffff;
    }
}













static uint8_t *
writeDiff(int32_t diff, uint8_t *p) {
    
    int32_t packed, initialPrev;

    packed=packDiff(diff);

    



    if(diff<=0) {
        initialPrev=0x10ffff;
    } else {
        initialPrev=-1;
    }

    if(diff!=unpackDiff(initialPrev, packed)) {
        log_err("error: unpackDiff(packDiff(diff=%ld)=0x%08lx)=%ld!=diff\n",
                diff, packed, unpackDiff(initialPrev, packed));
    }
    return p+writePacked(packed, p);
}










static int32_t
writeString(const UChar *s, int32_t length, uint8_t *p) {
    uint8_t *p0;
    int32_t c, prev, i;

    prev=0;
    p0=p;
    i=0;
    while(i<length) {
        U16_NEXT(s, i, length, c);
        p+=writePacked(encodeBocu1(&prev, c), p);
    }
    return (int32_t)(p-p0);
}










static int32_t
readString(const uint8_t *p, int32_t length, UChar *s) {
    Bocu1Rx rx={ 0, 0, 0 };
    int32_t c, i, sLength;

    i=sLength=0;
    while(i<length) {
        c=decodeBocu1(&rx, p[i++]);
        if(c<-1) {
            log_err("error: readString detects encoding error at string index %ld\n", i);
            return -1;
        }
        if(c>=0) {
            U16_APPEND_UNSAFE(s, sLength, c);
        }
    }
    return sLength;
}

static char
hexDigit(uint8_t digit) {
    return digit<=9 ? (char)('0'+digit) : (char)('a'-10+digit);
}







static void
printBytes(uint8_t *bytes, char *out) {
    int i;
    uint8_t b;

    i=0;
    while((b=*bytes++)!=0) {
        *out++=' ';
        *out++=hexDigit((uint8_t)(b>>4));
        *out++=hexDigit((uint8_t)(b&0xf));
        ++i;
    }
    i=3*(5-i);
    while(i>0) {
        *out++=' ';
        --i;
    }
    *out=0;
}






static void
TestBOCU1RefDiff(void) {
    char buf1[80], buf2[80];
    uint8_t prev[5], level[5];
    int32_t i, cmp, countErrors;

    log_verbose("reach of single bytes: %ld\n", 1+BOCU1_REACH_POS_1-BOCU1_REACH_NEG_1);
    log_verbose("reach of 2 bytes     : %ld\n", 1+BOCU1_REACH_POS_2-BOCU1_REACH_NEG_2);
    log_verbose("reach of 3 bytes     : %ld\n\n", 1+BOCU1_REACH_POS_3-BOCU1_REACH_NEG_3);

    log_verbose("    BOCU1_REACH_NEG_1 %8ld    BOCU1_REACH_POS_1 %8ld\n", BOCU1_REACH_NEG_1, BOCU1_REACH_POS_1);
    log_verbose("    BOCU1_REACH_NEG_2 %8ld    BOCU1_REACH_POS_2 %8ld\n", BOCU1_REACH_NEG_2, BOCU1_REACH_POS_2);
    log_verbose("    BOCU1_REACH_NEG_3 %8ld    BOCU1_REACH_POS_3 %8ld\n\n", BOCU1_REACH_NEG_3, BOCU1_REACH_POS_3);

    log_verbose("    BOCU1_MIDDLE      0x%02x\n", BOCU1_MIDDLE);
    log_verbose("    BOCU1_START_NEG_2 0x%02x    BOCU1_START_POS_2 0x%02x\n", BOCU1_START_NEG_2, BOCU1_START_POS_2);
    log_verbose("    BOCU1_START_NEG_3 0x%02x    BOCU1_START_POS_3 0x%02x\n\n", BOCU1_START_NEG_3, BOCU1_START_POS_3);

    
    writeDiff(0, level);
    writeDiff(1, level);
    writeDiff(65, level);
    writeDiff(130, level);
    writeDiff(30000, level);
    writeDiff(1000000, level);
    writeDiff(-65, level);
    writeDiff(-130, level);
    writeDiff(-30000, level);
    writeDiff(-1000000, level);

    
    countErrors=0;
    i=-0x10ffff;
    *writeDiff(i, prev)=0;

    
    printBytes(prev, buf1);
    log_verbose("              wD(%8ld)                    %s\n", i, buf1);

    for(++i; i<=0x10ffff; ++i) {
        *writeDiff(i, level)=0;
        cmp=strcmp((const char *)prev, (const char *)level);
        if(BOCU1_LENGTH_FROM_LEAD(level[0])!=(int32_t)strlen((const char *)level)) {
            log_verbose("BOCU1_LENGTH_FROM_LEAD(0x%02x)=%ld!=%ld=strlen(writeDiff(%ld))\n",
                   level[0], BOCU1_LENGTH_FROM_LEAD(level[0]), strlen((const char *)level), i);
        }
        if(cmp<0) {
            if(i==0 || i==1 || strlen((const char *)prev)!=strlen((const char *)level)) {
                



                printBytes(prev, buf1);
                printBytes(level, buf2);
                log_verbose("ok:    strcmp(wD(%8ld), wD(%8ld))=%2d  %s%s\n", i-1, i, cmp, buf1, buf2);
            }
        } else {
            ++countErrors;
            printBytes(prev, buf1);
            printBytes(level, buf2);
            log_verbose("wrong: strcmp(wD(%8ld), wD(%8ld))=%2d  %s%s\n", i-1, i, cmp, buf1, buf2);
        }
        
        memcpy(prev, level, 4);
    }

    
    printBytes((uint8_t *)"", buf1);
    printBytes(prev, buf2);
    log_verbose("                            wD(%8ld)      %s%s\n", i-1, buf1, buf2);

    if(countErrors==0) {
        log_verbose("writeDiff(-0x10ffff..0x10ffff) works fine\n");
    } else {
        log_err("writeDiff(-0x10ffff..0x10ffff) violates lexical ordering in %d cases\n", countErrors);
    }

    
    i=0;
    writePacked(encodeBocu1(&i, 0xfeff), level);
    log_verbose("\nBOCU-1 signature byte sequence: %02x %02x %02x\n",
            level[0], level[1], level[2]);
}



static const int32_t DEFAULT_BUFFER_SIZE = 30000;



static void
roundtripBOCU1(UConverter *bocu1, int32_t number, const UChar *text, int32_t length) {
    UChar *roundtripRef, *roundtripICU;
    char *bocu1Ref, *bocu1ICU;

    int32_t bocu1RefLength, bocu1ICULength, roundtripRefLength, roundtripICULength;
    UErrorCode errorCode;

    roundtripRef = malloc(DEFAULT_BUFFER_SIZE * sizeof(UChar));
    roundtripICU = malloc(DEFAULT_BUFFER_SIZE * sizeof(UChar));
    bocu1Ref = malloc(DEFAULT_BUFFER_SIZE);
    bocu1ICU = malloc(DEFAULT_BUFFER_SIZE);

    
    bocu1RefLength=writeString(text, length, (uint8_t *)bocu1Ref);

    errorCode=U_ZERO_ERROR;
    bocu1ICULength=ucnv_fromUChars(bocu1, bocu1ICU, DEFAULT_BUFFER_SIZE, text, length, &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("ucnv_fromUChars(BOCU-1, text(%d)[%d]) failed: %s\n", number, length, u_errorName(errorCode));
        goto cleanup; 
    }

    if(bocu1RefLength!=bocu1ICULength || 0!=uprv_memcmp(bocu1Ref, bocu1ICU, bocu1RefLength)) {
        log_err("Unicode(%d)[%d] -> BOCU-1: reference[%d]!=ICU[%d]\n", number, length, bocu1RefLength, bocu1ICULength);
        goto cleanup;
    }

    
    roundtripRefLength=readString((uint8_t *)bocu1Ref, bocu1RefLength, roundtripRef);
    if(roundtripRefLength<0) {
        goto cleanup; 
    }

    roundtripICULength=ucnv_toUChars(bocu1, roundtripICU, DEFAULT_BUFFER_SIZE, bocu1ICU, bocu1ICULength, &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("ucnv_toUChars(BOCU-1, text(%d)[%d]) failed: %s\n", number, length, u_errorName(errorCode));
        goto cleanup;
    }

    if(length!=roundtripRefLength || 0!=u_memcmp(text, roundtripRef, length)) {
        log_err("BOCU-1 -> Unicode: original(%d)[%d]!=reference[%d]\n", number, length, roundtripRefLength);
        goto cleanup;
    }
    if(roundtripRefLength!=roundtripICULength || 0!=u_memcmp(roundtripRef, roundtripICU, roundtripRefLength)) {
        log_err("BOCU-1 -> Unicode: reference(%d)[%d]!=ICU[%d]\n", number, roundtripRefLength, roundtripICULength);
        goto cleanup;
    }
cleanup:
    free(roundtripRef);
    free(roundtripICU);
    free(bocu1Ref);
    free(bocu1ICU);
}

static const UChar feff[]={ 0xfeff };
static const UChar ascii[]={ 0x61, 0x62, 0x20, 0x63, 0x61 };
static const UChar crlf[]={ 0xd, 0xa, 0x20 };
static const UChar nul[]={ 0 };
static const UChar latin[]={ 0xdf, 0xe6 };
static const UChar devanagari[]={ 0x930, 0x20, 0x918, 0x909 };
static const UChar hiragana[]={ 0x3086, 0x304d, 0x20, 0x3053, 0x4000 };
static const UChar unihan[]={ 0x4e00, 0x7777, 0x20, 0x9fa5, 0x4e00 };
static const UChar hangul[]={ 0xac00, 0xbcde, 0x20, 0xd7a3 };
static const UChar surrogates[]={ 0xdc00, 0xd800 }; 
static const UChar plane1[]={ 0xd800, 0xdc00 };
static const UChar plane2[]={ 0xd845, 0xdddd };
static const UChar plane15[]={ 0xdbbb, 0xddee, 0x20 };
static const UChar plane16[]={ 0xdbff, 0xdfff };
static const UChar c0[]={ 1, 0xe40, 0x20, 9 };

static const struct {
    const UChar *s;
    int32_t length;
} strings[]={
    { feff,         UPRV_LENGTHOF(feff) },
    { ascii,        UPRV_LENGTHOF(ascii) },
    { crlf,         UPRV_LENGTHOF(crlf) },
    { nul,          UPRV_LENGTHOF(nul) },
    { latin,        UPRV_LENGTHOF(latin) },
    { devanagari,   UPRV_LENGTHOF(devanagari) },
    { hiragana,     UPRV_LENGTHOF(hiragana) },
    { unihan,       UPRV_LENGTHOF(unihan) },
    { hangul,       UPRV_LENGTHOF(hangul) },
    { surrogates,   UPRV_LENGTHOF(surrogates) },
    { plane1,       UPRV_LENGTHOF(plane1) },
    { plane2,       UPRV_LENGTHOF(plane2) },
    { plane15,      UPRV_LENGTHOF(plane15) },
    { plane16,      UPRV_LENGTHOF(plane16) },
    { c0,           UPRV_LENGTHOF(c0) }
};







static void
TestBOCU1(void) {
    UChar *text;
    int32_t i, length;

    UConverter *bocu1;
    UErrorCode errorCode;

    errorCode=U_ZERO_ERROR;
    bocu1=ucnv_open("BOCU-1", &errorCode);
    if(U_FAILURE(errorCode)) {
        log_data_err("error: unable to open BOCU-1 converter: %s\n", u_errorName(errorCode));
        return;
    }

    text = malloc(DEFAULT_BUFFER_SIZE * sizeof(UChar));

    
    length=0;
    for(i=0; i<UPRV_LENGTHOF(strings); ++i) {
        u_memcpy(text+length, strings[i].s, strings[i].length);
        length+=strings[i].length;
    }
    roundtripBOCU1(bocu1, 1, text, length);

    
    length=0;
    for(i=0; i<UPRV_LENGTHOF(strings); ++i) {
        u_memcpy(text+length, strings[i].s, strings[i].length);
        length+=strings[i].length;
        u_memcpy(text+length, strings[i].s, strings[i].length);
        length+=strings[i].length;
    }
    roundtripBOCU1(bocu1, 2, text, length);

    
    length=0;
    for(i=1; length<5000; i+=7) {
        if(i>=UPRV_LENGTHOF(strings)) {
            i-=UPRV_LENGTHOF(strings);
        }
        u_memcpy(text+length, strings[i].s, strings[i].length);
        length+=strings[i].length;
    }
    roundtripBOCU1(bocu1, 3, text, length);

    ucnv_close(bocu1);
    free(text);
}

U_CFUNC void addBOCU1Tests(TestNode** root);

U_CFUNC void
addBOCU1Tests(TestNode** root) {
    addTest(root, TestBOCU1RefDiff, "tsconv/bocu1tst/TestBOCU1RefDiff");
    addTest(root, TestBOCU1, "tsconv/bocu1tst/TestBOCU1");
}
