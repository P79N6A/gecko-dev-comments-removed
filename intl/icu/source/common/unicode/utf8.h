






























#ifndef __UTF8_H__
#define __UTF8_H__

#include "unicode/umachine.h"
#ifndef __UTF_H__
#   include "unicode/utf.h"
#endif














#ifdef U_UTF8_IMPL
U_EXPORT const uint8_t 
#elif defined(U_STATIC_IMPLEMENTATION) || defined(U_COMMON_IMPLEMENTATION)
U_CFUNC const uint8_t
#else
U_CFUNC U_IMPORT const uint8_t   
#endif
utf8_countTrailBytes[256];



















#define U8_COUNT_TRAIL_BYTES(leadByte) \
    ((uint8_t)(leadByte)<0xf0 ? \
        ((uint8_t)(leadByte)>=0xc0)+((uint8_t)(leadByte)>=0xe0) : \
        (uint8_t)(leadByte)<0xfe ? 3+((uint8_t)(leadByte)>=0xf8)+((uint8_t)(leadByte)>=0xfc) : 0)












#define U8_COUNT_TRAIL_BYTES_UNSAFE(leadByte) \
    (((leadByte)>=0xc0)+((leadByte)>=0xe0)+((leadByte)>=0xf0))








#define U8_MASK_LEAD_BYTE(leadByte, countTrailBytes) ((leadByte)&=(1<<(6-(countTrailBytes)))-1)










U_STABLE UChar32 U_EXPORT2
utf8_nextCharSafeBody(const uint8_t *s, int32_t *pi, int32_t length, UChar32 c, UBool strict);










U_STABLE int32_t U_EXPORT2
utf8_appendCharSafeBody(uint8_t *s, int32_t i, int32_t length, UChar32 c, UBool *pIsError);










U_STABLE UChar32 U_EXPORT2
utf8_prevCharSafeBody(const uint8_t *s, int32_t start, int32_t *pi, UChar32 c, UBool strict);










U_STABLE int32_t U_EXPORT2
utf8_back1SafeBody(const uint8_t *s, int32_t start, int32_t i);









#define U8_IS_SINGLE(c) (((c)&0x80)==0)







#define U8_IS_LEAD(c) ((uint8_t)((c)-0xc0)<0x3e)







#define U8_IS_TRAIL(c) (((c)&0xc0)==0x80)








#define U8_LENGTH(c) \
    ((uint32_t)(c)<=0x7f ? 1 : \
        ((uint32_t)(c)<=0x7ff ? 2 : \
            ((uint32_t)(c)<=0xd7ff ? 3 : \
                ((uint32_t)(c)<=0xdfff || (uint32_t)(c)>0x10ffff ? 0 : \
                    ((uint32_t)(c)<=0xffff ? 3 : 4)\
                ) \
            ) \
        ) \
    )






#define U8_MAX_LENGTH 4

















#define U8_GET_UNSAFE(s, i, c) { \
    int32_t _u8_get_unsafe_index=(int32_t)(i); \
    U8_SET_CP_START_UNSAFE(s, _u8_get_unsafe_index); \
    U8_NEXT_UNSAFE(s, _u8_get_unsafe_index, c); \
}






















#define U8_GET(s, start, i, length, c) { \
    int32_t _u8_get_index=(i); \
    U8_SET_CP_START(s, start, _u8_get_index); \
    U8_NEXT(s, _u8_get_index, length, c); \
}


























#define U8_GET_OR_FFFD(s, start, i, length, c) { \
    int32_t _u8_get_index=(i); \
    U8_SET_CP_START(s, start, _u8_get_index); \
    U8_NEXT_OR_FFFD(s, _u8_get_index, length, c); \
}




















#define U8_NEXT_UNSAFE(s, i, c) { \
    (c)=(uint8_t)(s)[(i)++]; \
    if((c)>=0x80) { \
        if((c)<0xe0) { \
            (c)=(((c)&0x1f)<<6)|((s)[(i)++]&0x3f); \
        } else if((c)<0xf0) { \
            /* no need for (c&0xf) because the upper bits are truncated after <<12 in the cast to (UChar) */ \
            (c)=(UChar)(((c)<<12)|(((s)[i]&0x3f)<<6)|((s)[(i)+1]&0x3f)); \
            (i)+=2; \
        } else { \
            (c)=(((c)&7)<<18)|(((s)[i]&0x3f)<<12)|(((s)[(i)+1]&0x3f)<<6)|((s)[(i)+2]&0x3f); \
            (i)+=3; \
        } \
    } \
}





















#define U8_NEXT(s, i, length, c) { \
    (c)=(uint8_t)(s)[(i)++]; \
    if((c)>=0x80) { \
        uint8_t __t1, __t2; \
        if( /* handle U+1000..U+CFFF inline */ \
            (0xe0<(c) && (c)<=0xec) && \
            (((i)+1)<(length) || (length)<0) && \
            (__t1=(uint8_t)((s)[i]-0x80))<=0x3f && \
            (__t2=(uint8_t)((s)[(i)+1]-0x80))<= 0x3f \
        ) { \
            /* no need for (c&0xf) because the upper bits are truncated after <<12 in the cast to (UChar) */ \
            (c)=(UChar)(((c)<<12)|(__t1<<6)|__t2); \
            (i)+=2; \
        } else if( /* handle U+0080..U+07FF inline */ \
            ((c)<0xe0 && (c)>=0xc2) && \
            ((i)!=(length)) && \
            (__t1=(uint8_t)((s)[i]-0x80))<=0x3f \
        ) { \
            (c)=(((c)&0x1f)<<6)|__t1; \
            ++(i); \
        } else { \
            /* function call for "complicated" and error cases */ \
            (c)=utf8_nextCharSafeBody((const uint8_t *)s, &(i), (length), c, -1); \
        } \
    } \
}

























#define U8_NEXT_OR_FFFD(s, i, length, c) { \
    (c)=(uint8_t)(s)[(i)++]; \
    if((c)>=0x80) { \
        uint8_t __t1, __t2; \
        if( /* handle U+1000..U+CFFF inline */ \
            (0xe0<(c) && (c)<=0xec) && \
            (((i)+1)<(length) || (length)<0) && \
            (__t1=(uint8_t)((s)[i]-0x80))<=0x3f && \
            (__t2=(uint8_t)((s)[(i)+1]-0x80))<= 0x3f \
        ) { \
            /* no need for (c&0xf) because the upper bits are truncated after <<12 in the cast to (UChar) */ \
            (c)=(UChar)(((c)<<12)|(__t1<<6)|__t2); \
            (i)+=2; \
        } else if( /* handle U+0080..U+07FF inline */ \
            ((c)<0xe0 && (c)>=0xc2) && \
            ((i)!=(length)) && \
            (__t1=(uint8_t)((s)[i]-0x80))<=0x3f \
        ) { \
            (c)=(((c)&0x1f)<<6)|__t1; \
            ++(i); \
        } else { \
            /* function call for "complicated" and error cases */ \
            (c)=utf8_nextCharSafeBody((const uint8_t *)s, &(i), (length), c, -3); \
        } \
    } \
}














#define U8_APPEND_UNSAFE(s, i, c) { \
    if((uint32_t)(c)<=0x7f) { \
        (s)[(i)++]=(uint8_t)(c); \
    } else { \
        if((uint32_t)(c)<=0x7ff) { \
            (s)[(i)++]=(uint8_t)(((c)>>6)|0xc0); \
        } else { \
            if((uint32_t)(c)<=0xffff) { \
                (s)[(i)++]=(uint8_t)(((c)>>12)|0xe0); \
            } else { \
                (s)[(i)++]=(uint8_t)(((c)>>18)|0xf0); \
                (s)[(i)++]=(uint8_t)((((c)>>12)&0x3f)|0x80); \
            } \
            (s)[(i)++]=(uint8_t)((((c)>>6)&0x3f)|0x80); \
        } \
        (s)[(i)++]=(uint8_t)(((c)&0x3f)|0x80); \
    } \
}


















#define U8_APPEND(s, i, capacity, c, isError) { \
    if((uint32_t)(c)<=0x7f) { \
        (s)[(i)++]=(uint8_t)(c); \
    } else if((uint32_t)(c)<=0x7ff && (i)+1<(capacity)) { \
        (s)[(i)++]=(uint8_t)(((c)>>6)|0xc0); \
        (s)[(i)++]=(uint8_t)(((c)&0x3f)|0x80); \
    } else if((uint32_t)(c)<=0xd7ff && (i)+2<(capacity)) { \
        (s)[(i)++]=(uint8_t)(((c)>>12)|0xe0); \
        (s)[(i)++]=(uint8_t)((((c)>>6)&0x3f)|0x80); \
        (s)[(i)++]=(uint8_t)(((c)&0x3f)|0x80); \
    } else { \
        (i)=utf8_appendCharSafeBody(s, (i), (capacity), c, &(isError)); \
    } \
}











#define U8_FWD_1_UNSAFE(s, i) { \
    (i)+=1+U8_COUNT_TRAIL_BYTES_UNSAFE((uint8_t)(s)[i]); \
}














#define U8_FWD_1(s, i, length) { \
    uint8_t __b=(uint8_t)(s)[(i)++]; \
    if(U8_IS_LEAD(__b)) { \
        uint8_t __count=U8_COUNT_TRAIL_BYTES(__b); \
        if((i)+__count>(length) && (length)>=0) { \
            __count=(uint8_t)((length)-(i)); \
        } \
        while(__count>0 && U8_IS_TRAIL((s)[i])) { \
            ++(i); \
            --__count; \
        } \
    } \
}













#define U8_FWD_N_UNSAFE(s, i, n) { \
    int32_t __N=(n); \
    while(__N>0) { \
        U8_FWD_1_UNSAFE(s, i); \
        --__N; \
    } \
}
















#define U8_FWD_N(s, i, length, n) { \
    int32_t __N=(n); \
    while(__N>0 && ((i)<(length) || ((length)<0 && (s)[i]!=0))) { \
        U8_FWD_1(s, i, length); \
        --__N; \
    } \
}














#define U8_SET_CP_START_UNSAFE(s, i) { \
    while(U8_IS_TRAIL((s)[i])) { --(i); } \
}















#define U8_SET_CP_START(s, start, i) { \
    if(U8_IS_TRAIL((s)[(i)])) { \
        (i)=utf8_back1SafeBody(s, start, (i)); \
    } \
}






















#define U8_PREV_UNSAFE(s, i, c) { \
    (c)=(uint8_t)(s)[--(i)]; \
    if(U8_IS_TRAIL(c)) { \
        uint8_t __b, __count=1, __shift=6; \
\
        /* c is a trail byte */ \
        (c)&=0x3f; \
        for(;;) { \
            __b=(uint8_t)(s)[--(i)]; \
            if(__b>=0xc0) { \
                U8_MASK_LEAD_BYTE(__b, __count); \
                (c)|=(UChar32)__b<<__shift; \
                break; \
            } else { \
                (c)|=(UChar32)(__b&0x3f)<<__shift; \
                ++__count; \
                __shift+=6; \
            } \
        } \
    } \
}





















#define U8_PREV(s, start, i, c) { \
    (c)=(uint8_t)(s)[--(i)]; \
    if((c)>=0x80) { \
        (c)=utf8_prevCharSafeBody((const uint8_t *)s, start, &(i), c, -1); \
    } \
}

























#define U8_PREV_OR_FFFD(s, start, i, c) { \
    (c)=(uint8_t)(s)[--(i)]; \
    if((c)>=0x80) { \
        (c)=utf8_prevCharSafeBody((const uint8_t *)s, start, &(i), c, -3); \
    } \
}












#define U8_BACK_1_UNSAFE(s, i) { \
    while(U8_IS_TRAIL((s)[--(i)])) {} \
}













#define U8_BACK_1(s, start, i) { \
    if(U8_IS_TRAIL((s)[--(i)])) { \
        (i)=utf8_back1SafeBody(s, start, (i)); \
    } \
}














#define U8_BACK_N_UNSAFE(s, i, n) { \
    int32_t __N=(n); \
    while(__N>0) { \
        U8_BACK_1_UNSAFE(s, i); \
        --__N; \
    } \
}















#define U8_BACK_N(s, start, i, n) { \
    int32_t __N=(n); \
    while(__N>0 && (i)>(start)) { \
        U8_BACK_1(s, start, i); \
        --__N; \
    } \
}














#define U8_SET_CP_LIMIT_UNSAFE(s, i) { \
    U8_BACK_1_UNSAFE(s, i); \
    U8_FWD_1_UNSAFE(s, i); \
}


















#define U8_SET_CP_LIMIT(s, start, i, length) { \
    if((start)<(i) && ((i)<(length) || ((length)<0 && (s)[i]!=0))) { \
        U8_BACK_1(s, start, i); \
        U8_FWD_1(s, i, length); \
    } \
}

#endif
