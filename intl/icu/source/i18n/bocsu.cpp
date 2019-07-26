
















#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/bytestream.h"
#include "unicode/utf16.h"
#include "bocsu.h"





U_CFUNC uint8_t *
u_writeDiff(int32_t diff, uint8_t *p) {
    if(diff>=SLOPE_REACH_NEG_1) {
        if(diff<=SLOPE_REACH_POS_1) {
            *p++=(uint8_t)(SLOPE_MIDDLE+diff);
        } else if(diff<=SLOPE_REACH_POS_2) {
            *p++=(uint8_t)(SLOPE_START_POS_2+(diff/SLOPE_TAIL_COUNT));
            *p++=(uint8_t)(SLOPE_MIN+diff%SLOPE_TAIL_COUNT);
        } else if(diff<=SLOPE_REACH_POS_3) {
            p[2]=(uint8_t)(SLOPE_MIN+diff%SLOPE_TAIL_COUNT);
            diff/=SLOPE_TAIL_COUNT;
            p[1]=(uint8_t)(SLOPE_MIN+diff%SLOPE_TAIL_COUNT);
            *p=(uint8_t)(SLOPE_START_POS_3+(diff/SLOPE_TAIL_COUNT));
            p+=3;
        } else {
            p[3]=(uint8_t)(SLOPE_MIN+diff%SLOPE_TAIL_COUNT);
            diff/=SLOPE_TAIL_COUNT;
            p[2]=(uint8_t)(SLOPE_MIN+diff%SLOPE_TAIL_COUNT);
            diff/=SLOPE_TAIL_COUNT;
            p[1]=(uint8_t)(SLOPE_MIN+diff%SLOPE_TAIL_COUNT);
            *p=SLOPE_MAX;
            p+=4;
        }
    } else {
        int32_t m;

        if(diff>=SLOPE_REACH_NEG_2) {
            NEGDIVMOD(diff, SLOPE_TAIL_COUNT, m);
            *p++=(uint8_t)(SLOPE_START_NEG_2+diff);
            *p++=(uint8_t)(SLOPE_MIN+m);
        } else if(diff>=SLOPE_REACH_NEG_3) {
            NEGDIVMOD(diff, SLOPE_TAIL_COUNT, m);
            p[2]=(uint8_t)(SLOPE_MIN+m);
            NEGDIVMOD(diff, SLOPE_TAIL_COUNT, m);
            p[1]=(uint8_t)(SLOPE_MIN+m);
            *p=(uint8_t)(SLOPE_START_NEG_3+diff);
            p+=3;
        } else {
            NEGDIVMOD(diff, SLOPE_TAIL_COUNT, m);
            p[3]=(uint8_t)(SLOPE_MIN+m);
            NEGDIVMOD(diff, SLOPE_TAIL_COUNT, m);
            p[2]=(uint8_t)(SLOPE_MIN+m);
            NEGDIVMOD(diff, SLOPE_TAIL_COUNT, m);
            p[1]=(uint8_t)(SLOPE_MIN+m);
            *p=SLOPE_MIN;
            p+=4;
        }
    }
    return p;
}


















U_CFUNC void
u_writeIdenticalLevelRun(const UChar *s, int32_t length, icu::ByteSink &sink) {
    char scratch[64];
    int32_t capacity;

    UChar32 prev=0;
    int32_t i=0;
    while(i<length) {
        char *buffer=sink.GetAppendBuffer(1, length*2, scratch, (int32_t)sizeof(scratch), &capacity);
        uint8_t *p;
        
        
        
        if(capacity<16) {
            buffer=scratch;
            capacity=(int32_t)sizeof(scratch);
        }
        p=reinterpret_cast<uint8_t *>(buffer);
        uint8_t *lastSafe=p+capacity-SLOPE_MAX_BYTES;
        while(i<length && p<=lastSafe) {
            if(prev<0x4e00 || prev>=0xa000) {
                prev=(prev&~0x7f)-SLOPE_REACH_NEG_1;
            } else {
                



                prev=0x9fff-SLOPE_REACH_POS_2;
            }

            UChar32 c;
            U16_NEXT(s, i, length, c);
            p=u_writeDiff(c-prev, p);
            prev=c;
        }
        sink.Append(buffer, (int32_t)(p-reinterpret_cast<uint8_t *>(buffer)));
    }
}

U_CFUNC int32_t
u_writeIdenticalLevelRunTwoChars(UChar32 first, UChar32 second, uint8_t *p) {
    uint8_t *p0 = p;
    if(first<0x4e00 || first>=0xa000) {
        first=(first&~0x7f)-SLOPE_REACH_NEG_1;
    } else {
        



        first=0x9fff-SLOPE_REACH_POS_2;
    }

    p=u_writeDiff(second-first, p);
    return (int32_t)(p-p0);
}

#endif 
