

























#include "unicode/putil.h"
#include "unicode/ustring.h"
#include "unicode/utf.h"
#include "unicode/utf8.h"
#include "unicode/utf16.h"
#include "cstring.h"
#include "cmemory.h"
#include "ustr_imp.h"
#include "uassert.h"

U_CAPI UChar* U_EXPORT2 
u_strFromUTF32WithSub(UChar *dest,
               int32_t destCapacity,
               int32_t *pDestLength,
               const UChar32 *src,
               int32_t srcLength,
               UChar32 subchar, int32_t *pNumSubstitutions,
               UErrorCode *pErrorCode) {
    const UChar32 *srcLimit;
    UChar32 ch;
    UChar *destLimit;
    UChar *pDest;
    int32_t reqLength;
    int32_t numSubstitutions;

    
    if(U_FAILURE(*pErrorCode)){
        return NULL;
    }
    if( (src==NULL && srcLength!=0) || srcLength < -1 ||
        (destCapacity<0) || (dest == NULL && destCapacity > 0) ||
        subchar > 0x10ffff || U_IS_SURROGATE(subchar)
    ) {
        *pErrorCode = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }

    if(pNumSubstitutions != NULL) {
        *pNumSubstitutions = 0;
    }

    pDest = dest;
    destLimit = (dest!=NULL)?(dest + destCapacity):NULL;
    reqLength = 0;
    numSubstitutions = 0;

    if(srcLength < 0) {
        
        while((ch=*src) != 0 &&
              ((uint32_t)ch < 0xd800 || (0xe000 <= ch && ch <= 0xffff))) {
            ++src;
            if(pDest < destLimit) {
                *pDest++ = (UChar)ch;
            } else {
                ++reqLength;
            }
        }
        srcLimit = src;
        if(ch != 0) {
            
            while(*++srcLimit != 0) {}
        }
    } else {
      srcLimit = (src!=NULL)?(src + srcLength):NULL;
    }

    
    while(src < srcLimit) {
        ch = *src++;
        do {
            
            if((uint32_t)ch < 0xd800 || (0xe000 <= ch && ch <= 0xffff)) {
                if(pDest < destLimit) {
                    *pDest++ = (UChar)ch;
                } else {
                    ++reqLength;
                }
                break;
            } else if(0x10000 <= ch && ch <= 0x10ffff) {
                if(pDest!=NULL && ((pDest + 2) <= destLimit)) {
                    *pDest++ = U16_LEAD(ch);
                    *pDest++ = U16_TRAIL(ch);
                } else {
                    reqLength += 2;
                }
                break;
            } else if((ch = subchar) < 0) {
                
                *pErrorCode = U_INVALID_CHAR_FOUND;
                return NULL;
            } else {
                ++numSubstitutions;
            }
        } while(TRUE);
    }

    reqLength += (int32_t)(pDest - dest);
    if(pDestLength) {
        *pDestLength = reqLength;
    }
    if(pNumSubstitutions != NULL) {
        *pNumSubstitutions = numSubstitutions;
    }

    
    u_terminateUChars(dest, destCapacity, reqLength, pErrorCode);
    
    return dest;
}

U_CAPI UChar* U_EXPORT2 
u_strFromUTF32(UChar *dest,
               int32_t destCapacity, 
               int32_t *pDestLength,
               const UChar32 *src,
               int32_t srcLength,
               UErrorCode *pErrorCode) {
    return u_strFromUTF32WithSub(
            dest, destCapacity, pDestLength,
            src, srcLength,
            U_SENTINEL, NULL,
            pErrorCode);
}

U_CAPI UChar32* U_EXPORT2 
u_strToUTF32WithSub(UChar32 *dest,
             int32_t destCapacity,
             int32_t *pDestLength,
             const UChar *src,
             int32_t srcLength,
             UChar32 subchar, int32_t *pNumSubstitutions,
             UErrorCode *pErrorCode) {
    const UChar *srcLimit;
    UChar32 ch;
    UChar ch2;
    UChar32 *destLimit;
    UChar32 *pDest;
    int32_t reqLength;
    int32_t numSubstitutions;

    
    if(U_FAILURE(*pErrorCode)){
        return NULL;
    }
    if( (src==NULL && srcLength!=0) || srcLength < -1 ||
        (destCapacity<0) || (dest == NULL && destCapacity > 0) ||
        subchar > 0x10ffff || U_IS_SURROGATE(subchar)
    ) {
        *pErrorCode = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }

    if(pNumSubstitutions != NULL) {
        *pNumSubstitutions = 0;
    }

    pDest = dest;
    destLimit = (dest!=NULL)?(dest + destCapacity):NULL;
    reqLength = 0;
    numSubstitutions = 0;

    if(srcLength < 0) {
        
        while((ch=*src) != 0 && !U16_IS_SURROGATE(ch)) {
            ++src;
            if(pDest < destLimit) {
                *pDest++ = ch;
            } else {
                ++reqLength;
            }
        }
        srcLimit = src;
        if(ch != 0) {
            
            while(*++srcLimit != 0) {}
        }
    } else {
        srcLimit = (src!=NULL)?(src + srcLength):NULL;
    }

    
    while(src < srcLimit) {
        ch = *src++;
        if(!U16_IS_SURROGATE(ch)) {
            
        } else if(U16_IS_SURROGATE_LEAD(ch) && src < srcLimit && U16_IS_TRAIL(ch2 = *src)) {
            ++src;
            ch = U16_GET_SUPPLEMENTARY(ch, ch2);
        } else if((ch = subchar) < 0) {
            
            *pErrorCode = U_INVALID_CHAR_FOUND;
            return NULL;
        } else {
            ++numSubstitutions;
        }
        if(pDest < destLimit) {
            *pDest++ = ch;
        } else {
            ++reqLength;
        }
    }

    reqLength += (int32_t)(pDest - dest);
    if(pDestLength) {
        *pDestLength = reqLength;
    }
    if(pNumSubstitutions != NULL) {
        *pNumSubstitutions = numSubstitutions;
    }

    
    u_terminateUChar32s(dest, destCapacity, reqLength, pErrorCode);

    return dest;
}

U_CAPI UChar32* U_EXPORT2 
u_strToUTF32(UChar32 *dest, 
             int32_t destCapacity,
             int32_t *pDestLength,
             const UChar *src, 
             int32_t srcLength,
             UErrorCode *pErrorCode) {
    return u_strToUTF32WithSub(
            dest, destCapacity, pDestLength,
            src, srcLength,
            U_SENTINEL, NULL,
            pErrorCode);
}


static const UChar32
utf8_minLegal[4]={ 0, 0x80, 0x800, 0x10000 };











static UChar32
utf8_nextCharSafeBodyTerminated(const uint8_t **ps, UChar32 c) {
    const uint8_t *s=*ps;
    uint8_t trail, illegal=0;
    uint8_t count=U8_COUNT_TRAIL_BYTES(c);
    U_ASSERT(count<6);
    U8_MASK_LEAD_BYTE((c), count);
    
    switch(count) {
    
    case 5:
    case 4:
        
        illegal=1;
        break;
    case 3:
        trail=(uint8_t)(*s++ - 0x80);
        c=(c<<6)|trail;
        if(trail>0x3f || c>=0x110) {
            
            illegal=1;
            break;
        }
    case 2: 
        trail=(uint8_t)(*s++ - 0x80);
        if(trail>0x3f) {
            
            illegal=1;
            break;
        }
        c=(c<<6)|trail;
    case 1: 
        trail=(uint8_t)(*s++ - 0x80);
        if(trail>0x3f) {
            
            illegal=1;
        }
        c=(c<<6)|trail;
        break;
    case 0:
        return U_SENTINEL;
    
    }

    
    
    if(illegal || c<utf8_minLegal[count] || U_IS_SURROGATE(c)) {
        
        
        s=*ps;
        while(count>0 && U8_IS_TRAIL(*s)) {
            ++s;
            --count;
        }
        c=U_SENTINEL;
    }
    *ps=s;
    return c;
}










static UChar32
utf8_nextCharSafeBodyPointer(const uint8_t **ps, const uint8_t *limit, UChar32 c) {
    const uint8_t *s=*ps;
    uint8_t trail, illegal=0;
    uint8_t count=U8_COUNT_TRAIL_BYTES(c);
    if((limit-s)>=count) {
        U8_MASK_LEAD_BYTE((c), count);
        
        switch(count) {
        
        case 5:
        case 4:
            
            illegal=1;
            break;
        case 3:
            trail=*s++;
            c=(c<<6)|(trail&0x3f);
            if(c<0x110) {
                illegal|=(trail&0xc0)^0x80;
            } else {
                
                illegal=1;
                break;
            }
        case 2: 
            trail=*s++;
            c=(c<<6)|(trail&0x3f);
            illegal|=(trail&0xc0)^0x80;
        case 1: 
            trail=*s++;
            c=(c<<6)|(trail&0x3f);
            illegal|=(trail&0xc0)^0x80;
            break;
        case 0:
            return U_SENTINEL;
        
        }
    } else {
        illegal=1; 
    }

    
    
    U_ASSERT(count<sizeof(utf8_minLegal)/sizeof(utf8_minLegal[0]));
    if(illegal || c<utf8_minLegal[count] || U_IS_SURROGATE(c)) {
        
        
        s=*ps;
        while(count>0 && s<limit && U8_IS_TRAIL(*s)) {
            ++s;
            --count;
        }
        c=U_SENTINEL;
    }
    *ps=s;
    return c;
}

U_CAPI UChar* U_EXPORT2
u_strFromUTF8WithSub(UChar *dest,
              int32_t destCapacity,
              int32_t *pDestLength,
              const char* src,
              int32_t srcLength,
              UChar32 subchar, int32_t *pNumSubstitutions,
              UErrorCode *pErrorCode){
    UChar *pDest = dest;
    UChar *pDestLimit = dest+destCapacity;
    UChar32 ch;
    int32_t reqLength = 0;
    const uint8_t* pSrc = (const uint8_t*) src;
    uint8_t t1, t2; 
    int32_t numSubstitutions;

    
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)){
        return NULL;
    }
        
    if( (src==NULL && srcLength!=0) || srcLength < -1 ||
        (destCapacity<0) || (dest == NULL && destCapacity > 0) ||
        subchar > 0x10ffff || U_IS_SURROGATE(subchar)
    ) {
        *pErrorCode = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }

    if(pNumSubstitutions!=NULL) {
        *pNumSubstitutions=0;
    }
    numSubstitutions=0;

    












    if(srcLength < 0){
        




        while(((ch = *pSrc) != 0) && (pDest < pDestLimit)) {
            if(ch <= 0x7f){
                *pDest++=(UChar)ch;
                ++pSrc;
            } else {
                if(ch > 0xe0) {
                    if( 
                        ch <= 0xec &&
                        (t1 = (uint8_t)(pSrc[1] - 0x80)) <= 0x3f &&
                        (t2 = (uint8_t)(pSrc[2] - 0x80)) <= 0x3f
                    ) {
                        
                        *pDest++ = (UChar)((ch << 12) | (t1 << 6) | t2);
                        pSrc += 3;
                        continue;
                    }
                } else if(ch < 0xe0) {
                    if( 
                        ch >= 0xc2 &&
                        (t1 = (uint8_t)(pSrc[1] - 0x80)) <= 0x3f
                    ) {
                        *pDest++ = (UChar)(((ch & 0x1f) << 6) | t1);
                        pSrc += 2;
                        continue;
                    }
                }

                
                ++pSrc; 
                ch=utf8_nextCharSafeBodyTerminated(&pSrc, ch);
                if(ch<0 && (++numSubstitutions, ch = subchar) < 0) {
                    *pErrorCode = U_INVALID_CHAR_FOUND;
                    return NULL;
                } else if(ch<=0xFFFF) {
                    *(pDest++)=(UChar)ch;
                } else {
                    *(pDest++)=U16_LEAD(ch);
                    if(pDest<pDestLimit) {
                        *(pDest++)=U16_TRAIL(ch);
                    } else {
                        reqLength++;
                        break;
                    }
                }
            }
        }

        
        while((ch = *pSrc) != 0) {
            if(ch <= 0x7f){
                ++reqLength;
                ++pSrc;
            } else {
                if(ch > 0xe0) {
                    if( 
                        ch <= 0xec &&
                        (uint8_t)(pSrc[1] - 0x80) <= 0x3f &&
                        (uint8_t)(pSrc[2] - 0x80) <= 0x3f
                    ) {
                        ++reqLength;
                        pSrc += 3;
                        continue;
                    }
                } else if(ch < 0xe0) {
                    if( 
                        ch >= 0xc2 &&
                        (uint8_t)(pSrc[1] - 0x80) <= 0x3f
                    ) {
                        ++reqLength;
                        pSrc += 2;
                        continue;
                    }
                }

                
                ++pSrc; 
                ch=utf8_nextCharSafeBodyTerminated(&pSrc, ch);
                if(ch<0 && (++numSubstitutions, ch = subchar) < 0) {
                    *pErrorCode = U_INVALID_CHAR_FOUND;
                    return NULL;
                }
                reqLength += U16_LENGTH(ch);
            }
        }
    } else  {
        const uint8_t *pSrcLimit = pSrc + srcLength;
        int32_t count;

        
        for(;;) {
            





            count = (int32_t)(pDestLimit - pDest);
            srcLength = (int32_t)((pSrcLimit - pSrc) / 3);
            if(count > srcLength) {
                count = srcLength; 
            }
            if(count < 3) {
                



                break;
            }

            do {
                ch = *pSrc;
                if(ch <= 0x7f){
                    *pDest++=(UChar)ch;
                    ++pSrc;
                } else {
                    if(ch > 0xe0) {
                        if( 
                            ch <= 0xec &&
                            (t1 = (uint8_t)(pSrc[1] - 0x80)) <= 0x3f &&
                            (t2 = (uint8_t)(pSrc[2] - 0x80)) <= 0x3f
                        ) {
                            
                            *pDest++ = (UChar)((ch << 12) | (t1 << 6) | t2);
                            pSrc += 3;
                            continue;
                        }
                    } else if(ch < 0xe0) {
                        if( 
                            ch >= 0xc2 &&
                            (t1 = (uint8_t)(pSrc[1] - 0x80)) <= 0x3f
                        ) {
                            *pDest++ = (UChar)(((ch & 0x1f) << 6) | t1);
                            pSrc += 2;
                            continue;
                        }
                    }

                    if(ch >= 0xf0 || subchar > 0xffff) {
                        




                        if(--count == 0) {
                            break;
                        }
                    }

                    
                    ++pSrc; 
                    ch=utf8_nextCharSafeBodyPointer(&pSrc, pSrcLimit, ch);
                    if(ch<0 && (++numSubstitutions, ch = subchar) < 0){
                        *pErrorCode = U_INVALID_CHAR_FOUND;
                        return NULL;
                    }else if(ch<=0xFFFF){
                        *(pDest++)=(UChar)ch;
                    }else{
                        *(pDest++)=U16_LEAD(ch);
                        *(pDest++)=U16_TRAIL(ch);
                    }
                }
            } while(--count > 0);
        }

        while((pSrc<pSrcLimit) && (pDest<pDestLimit)) {
            ch = *pSrc;
            if(ch <= 0x7f){
                *pDest++=(UChar)ch;
                ++pSrc;
            } else {
                if(ch > 0xe0) {
                    if( 
                        ch <= 0xec &&
                        ((pSrcLimit - pSrc) >= 3) &&
                        (t1 = (uint8_t)(pSrc[1] - 0x80)) <= 0x3f &&
                        (t2 = (uint8_t)(pSrc[2] - 0x80)) <= 0x3f
                    ) {
                        
                        *pDest++ = (UChar)((ch << 12) | (t1 << 6) | t2);
                        pSrc += 3;
                        continue;
                    }
                } else if(ch < 0xe0) {
                    if( 
                        ch >= 0xc2 &&
                        ((pSrcLimit - pSrc) >= 2) &&
                        (t1 = (uint8_t)(pSrc[1] - 0x80)) <= 0x3f
                    ) {
                        *pDest++ = (UChar)(((ch & 0x1f) << 6) | t1);
                        pSrc += 2;
                        continue;
                    }
                }

                
                ++pSrc; 
                ch=utf8_nextCharSafeBodyPointer(&pSrc, pSrcLimit, ch);
                if(ch<0 && (++numSubstitutions, ch = subchar) < 0){
                    *pErrorCode = U_INVALID_CHAR_FOUND;
                    return NULL;
                }else if(ch<=0xFFFF){
                    *(pDest++)=(UChar)ch;
                }else{
                    *(pDest++)=U16_LEAD(ch);
                    if(pDest<pDestLimit){
                        *(pDest++)=U16_TRAIL(ch);
                    }else{
                        reqLength++;
                        break;
                    }
                }
            }
        }
        
        while(pSrc < pSrcLimit){
            ch = *pSrc;
            if(ch <= 0x7f){
                reqLength++;
                ++pSrc;
            } else {
                if(ch > 0xe0) {
                    if( 
                        ch <= 0xec &&
                        ((pSrcLimit - pSrc) >= 3) &&
                        (uint8_t)(pSrc[1] - 0x80) <= 0x3f &&
                        (uint8_t)(pSrc[2] - 0x80) <= 0x3f
                    ) {
                        reqLength++;
                        pSrc += 3;
                        continue;
                    }
                } else if(ch < 0xe0) {
                    if( 
                        ch >= 0xc2 &&
                        ((pSrcLimit - pSrc) >= 2) &&
                        (uint8_t)(pSrc[1] - 0x80) <= 0x3f
                    ) {
                        reqLength++;
                        pSrc += 2;
                        continue;
                    }
                }

                
                ++pSrc; 
                ch=utf8_nextCharSafeBodyPointer(&pSrc, pSrcLimit, ch);
                if(ch<0 && (++numSubstitutions, ch = subchar) < 0){
                    *pErrorCode = U_INVALID_CHAR_FOUND;
                    return NULL;
                }
                reqLength+=U16_LENGTH(ch);
            }
        }
    }

    reqLength+=(int32_t)(pDest - dest);

    if(pNumSubstitutions!=NULL) {
        *pNumSubstitutions=numSubstitutions;
    }

    if(pDestLength){
        *pDestLength = reqLength;
    }

    
    u_terminateUChars(dest,destCapacity,reqLength,pErrorCode);

    return dest;
}

U_CAPI UChar* U_EXPORT2
u_strFromUTF8(UChar *dest,
              int32_t destCapacity,
              int32_t *pDestLength,
              const char* src,
              int32_t srcLength,
              UErrorCode *pErrorCode){
    return u_strFromUTF8WithSub(
            dest, destCapacity, pDestLength,
            src, srcLength,
            U_SENTINEL, NULL,
            pErrorCode);
}

U_CAPI UChar * U_EXPORT2
u_strFromUTF8Lenient(UChar *dest,
                     int32_t destCapacity,
                     int32_t *pDestLength,
                     const char *src,
                     int32_t srcLength,
                     UErrorCode *pErrorCode) {
    UChar *pDest = dest;
    UChar32 ch;
    int32_t reqLength = 0;
    uint8_t* pSrc = (uint8_t*) src;

    
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)){
        return NULL;
    }
        
    if( (src==NULL && srcLength!=0) || srcLength < -1 ||
        (destCapacity<0) || (dest == NULL && destCapacity > 0)
    ) {
        *pErrorCode = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }

    if(srcLength < 0) {
        
        UChar *pDestLimit = (dest!=NULL)?(dest+destCapacity):NULL;
        uint8_t t1, t2, t3; 

        while(((ch = *pSrc) != 0) && (pDest < pDestLimit)) {
            if(ch < 0xc0) {
                




                *pDest++=(UChar)ch;
                ++pSrc;
                continue;
            } else if(ch < 0xe0) { 
                if((t1 = pSrc[1]) != 0) {
                    
                    *pDest++ = (UChar)((ch << 6) + t1 - 0x3080);
                    pSrc += 2;
                    continue;
                }
            } else if(ch < 0xf0) { 
                if((t1 = pSrc[1]) != 0 && (t2 = pSrc[2]) != 0) {
                    
                    
                    *pDest++ = (UChar)((ch << 12) + (t1 << 6) + t2 - 0x2080);
                    pSrc += 3;
                    continue;
                }
            } else  { 
                if((t1 = pSrc[1]) != 0 && (t2 = pSrc[2]) != 0 && (t3 = pSrc[3]) != 0) {
                    pSrc += 4;
                    
                    ch = (ch << 18) + (t1 << 12) + (t2 << 6) + t3 - 0x3c82080;
                    *(pDest++) = U16_LEAD(ch);
                    if(pDest < pDestLimit) {
                        *(pDest++) = U16_TRAIL(ch);
                    } else {
                        reqLength = 1;
                        break;
                    }
                    continue;
                }
            }

            
            *pDest++ = 0xfffd;
            while(*++pSrc != 0) {}
            break;
        }

        
        while((ch = *pSrc) != 0) {
            if(ch < 0xc0) {
                




                ++reqLength;
                ++pSrc;
                continue;
            } else if(ch < 0xe0) { 
                if(pSrc[1] != 0) {
                    ++reqLength;
                    pSrc += 2;
                    continue;
                }
            } else if(ch < 0xf0) { 
                if(pSrc[1] != 0 && pSrc[2] != 0) {
                    ++reqLength;
                    pSrc += 3;
                    continue;
                }
            } else  { 
                if(pSrc[1] != 0 && pSrc[2] != 0 && pSrc[3] != 0) {
                    reqLength += 2;
                    pSrc += 4;
                    continue;
                }
            }

            
            ++reqLength;
            break;
        }
    } else  {
      const uint8_t *pSrcLimit = (pSrc!=NULL)?(pSrc + srcLength):NULL;

        




        if(destCapacity < srcLength) {
            if(pDestLength != NULL) {
                *pDestLength = srcLength; 
            }
            *pErrorCode = U_BUFFER_OVERFLOW_ERROR;
            return NULL;
        }

        if((pSrcLimit - pSrc) >= 4) {
            pSrcLimit -= 3; 

            
            do {
                ch = *pSrc++;
                if(ch < 0xc0) {
                    




                    *pDest++=(UChar)ch;
                } else if(ch < 0xe0) { 
                    
                    *pDest++ = (UChar)((ch << 6) + *pSrc++ - 0x3080);
                } else if(ch < 0xf0) { 
                    
                    
                    ch = (ch << 12) + (*pSrc++ << 6);
                    *pDest++ = (UChar)(ch + *pSrc++ - 0x2080);
                } else  { 
                    
                    ch = (ch << 18) + (*pSrc++ << 12);
                    ch += *pSrc++ << 6;
                    ch += *pSrc++ - 0x3c82080;
                    *(pDest++) = U16_LEAD(ch);
                    *(pDest++) = U16_TRAIL(ch);
                }
            } while(pSrc < pSrcLimit);

            pSrcLimit += 3; 
        }

        while(pSrc < pSrcLimit) {
            ch = *pSrc++;
            if(ch < 0xc0) {
                




                *pDest++=(UChar)ch;
                continue;
            } else if(ch < 0xe0) { 
                if(pSrc < pSrcLimit) {
                    
                    *pDest++ = (UChar)((ch << 6) + *pSrc++ - 0x3080);
                    continue;
                }
            } else if(ch < 0xf0) { 
                if((pSrcLimit - pSrc) >= 2) {
                    
                    
                    ch = (ch << 12) + (*pSrc++ << 6);
                    *pDest++ = (UChar)(ch + *pSrc++ - 0x2080);
                    pSrc += 3;
                    continue;
                }
            } else  { 
                if((pSrcLimit - pSrc) >= 3) {
                    
                    ch = (ch << 18) + (*pSrc++ << 12);
                    ch += *pSrc++ << 6;
                    ch += *pSrc++ - 0x3c82080;
                    *(pDest++) = U16_LEAD(ch);
                    *(pDest++) = U16_TRAIL(ch);
                    pSrc += 4;
                    continue;
                }
            }

            
            *pDest++ = 0xfffd;
            break;
        }
    }

    reqLength+=(int32_t)(pDest - dest);

    if(pDestLength){
        *pDestLength = reqLength;
    }

    
    u_terminateUChars(dest,destCapacity,reqLength,pErrorCode);

    return dest;
}

static inline uint8_t *
_appendUTF8(uint8_t *pDest, UChar32 c) {
    
    if((c)<=0x7f) {
        *pDest++=(uint8_t)c;
    } else if(c<=0x7ff) {
        *pDest++=(uint8_t)((c>>6)|0xc0);
        *pDest++=(uint8_t)((c&0x3f)|0x80);
    } else if(c<=0xffff) {
        *pDest++=(uint8_t)((c>>12)|0xe0);
        *pDest++=(uint8_t)(((c>>6)&0x3f)|0x80);
        *pDest++=(uint8_t)(((c)&0x3f)|0x80);
    } else  {
        *pDest++=(uint8_t)(((c)>>18)|0xf0);
        *pDest++=(uint8_t)((((c)>>12)&0x3f)|0x80);
        *pDest++=(uint8_t)((((c)>>6)&0x3f)|0x80);
        *pDest++=(uint8_t)(((c)&0x3f)|0x80);
    }
    return pDest;
}

   
U_CAPI char* U_EXPORT2 
u_strToUTF8WithSub(char *dest,
            int32_t destCapacity,
            int32_t *pDestLength,
            const UChar *pSrc,
            int32_t srcLength,
            UChar32 subchar, int32_t *pNumSubstitutions,
            UErrorCode *pErrorCode){
    int32_t reqLength=0;
    uint32_t ch=0,ch2=0;
    uint8_t *pDest = (uint8_t *)dest;
    uint8_t *pDestLimit = (pDest!=NULL)?(pDest + destCapacity):NULL;
    int32_t numSubstitutions;

    
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)){
        return NULL;
    }
        
    if( (pSrc==NULL && srcLength!=0) || srcLength < -1 ||
        (destCapacity<0) || (dest == NULL && destCapacity > 0) ||
        subchar > 0x10ffff || U_IS_SURROGATE(subchar)
    ) {
        *pErrorCode = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }

    if(pNumSubstitutions!=NULL) {
        *pNumSubstitutions=0;
    }
    numSubstitutions=0;

    if(srcLength==-1) {
        while((ch=*pSrc)!=0) {
            ++pSrc;
            if(ch <= 0x7f) {
                if(pDest<pDestLimit) {
                    *pDest++ = (uint8_t)ch;
                } else {
                    reqLength = 1;
                    break;
                }
            } else if(ch <= 0x7ff) {
                if((pDestLimit - pDest) >= 2) {
                    *pDest++=(uint8_t)((ch>>6)|0xc0);
                    *pDest++=(uint8_t)((ch&0x3f)|0x80);
                } else {
                    reqLength = 2;
                    break;
                }
            } else if(ch <= 0xd7ff || ch >= 0xe000) {
                if((pDestLimit - pDest) >= 3) {
                    *pDest++=(uint8_t)((ch>>12)|0xe0);
                    *pDest++=(uint8_t)(((ch>>6)&0x3f)|0x80);
                    *pDest++=(uint8_t)((ch&0x3f)|0x80);
                } else {
                    reqLength = 3;
                    break;
                }
            } else  {
                int32_t length;

                
                if(U16_IS_SURROGATE_LEAD(ch) && U16_IS_TRAIL(ch2=*pSrc)) { 
                    ++pSrc;
                    ch=U16_GET_SUPPLEMENTARY(ch, ch2);
                } else if(subchar>=0) {
                    ch=subchar;
                    ++numSubstitutions;
                } else {
                    
                    *pErrorCode = U_INVALID_CHAR_FOUND;
                    return NULL;
                }

                length = U8_LENGTH(ch);
                if((pDestLimit - pDest) >= length) {
                    
                    pDest=_appendUTF8(pDest, ch);
                } else {
                    reqLength = length;
                    break;
                }
            }
        }
        while((ch=*pSrc++)!=0) {
            if(ch<=0x7f) {
                ++reqLength;
            } else if(ch<=0x7ff) {
                reqLength+=2;
            } else if(!U16_IS_SURROGATE(ch)) {
                reqLength+=3;
            } else if(U16_IS_SURROGATE_LEAD(ch) && U16_IS_TRAIL(ch2=*pSrc)) {
                ++pSrc;
                reqLength+=4;
            } else if(subchar>=0) {
                reqLength+=U8_LENGTH(subchar);
                ++numSubstitutions;
            } else {
                
                *pErrorCode = U_INVALID_CHAR_FOUND;
                return NULL;
            }
        }
    } else {
        const UChar *pSrcLimit = (pSrc!=NULL)?(pSrc+srcLength):NULL;
        int32_t count;

        
        for(;;) {
            





            count = (int32_t)((pDestLimit - pDest) / 3);
            srcLength = (int32_t)(pSrcLimit - pSrc);
            if(count > srcLength) {
                count = srcLength; 
            }
            if(count < 3) {
                



                break;
            }
            do {
                ch=*pSrc++;
                if(ch <= 0x7f) {
                    *pDest++ = (uint8_t)ch;
                } else if(ch <= 0x7ff) {
                    *pDest++=(uint8_t)((ch>>6)|0xc0);
                    *pDest++=(uint8_t)((ch&0x3f)|0x80);
                } else if(ch <= 0xd7ff || ch >= 0xe000) {
                    *pDest++=(uint8_t)((ch>>12)|0xe0);
                    *pDest++=(uint8_t)(((ch>>6)&0x3f)|0x80);
                    *pDest++=(uint8_t)((ch&0x3f)|0x80);
                } else  {
                    




                    if(--count == 0) {
                        --pSrc; 
                        break;  
                    }

                    if(U16_IS_SURROGATE_LEAD(ch) && U16_IS_TRAIL(ch2=*pSrc)) { 
                        ++pSrc;
                        ch=U16_GET_SUPPLEMENTARY(ch, ch2);

                        
                        *pDest++=(uint8_t)((ch>>18)|0xf0);
                        *pDest++=(uint8_t)(((ch>>12)&0x3f)|0x80);
                        *pDest++=(uint8_t)(((ch>>6)&0x3f)|0x80);
                        *pDest++=(uint8_t)((ch&0x3f)|0x80);
                    } else  {
                        
                        if(subchar>=0) {
                            ch=subchar;
                            ++numSubstitutions;
                        } else {
                            *pErrorCode = U_INVALID_CHAR_FOUND;
                            return NULL;
                        }

                        
                        pDest=_appendUTF8(pDest, ch);
                    }
                }
            } while(--count > 0);
        }

        while(pSrc<pSrcLimit) {
            ch=*pSrc++;
            if(ch <= 0x7f) {
                if(pDest<pDestLimit) {
                    *pDest++ = (uint8_t)ch;
                } else {
                    reqLength = 1;
                    break;
                }
            } else if(ch <= 0x7ff) {
                if((pDestLimit - pDest) >= 2) {
                    *pDest++=(uint8_t)((ch>>6)|0xc0);
                    *pDest++=(uint8_t)((ch&0x3f)|0x80);
                } else {
                    reqLength = 2;
                    break;
                }
            } else if(ch <= 0xd7ff || ch >= 0xe000) {
                if((pDestLimit - pDest) >= 3) {
                    *pDest++=(uint8_t)((ch>>12)|0xe0);
                    *pDest++=(uint8_t)(((ch>>6)&0x3f)|0x80);
                    *pDest++=(uint8_t)((ch&0x3f)|0x80);
                } else {
                    reqLength = 3;
                    break;
                }
            } else  {
                int32_t length;

                if(U16_IS_SURROGATE_LEAD(ch) && pSrc<pSrcLimit && U16_IS_TRAIL(ch2=*pSrc)) { 
                    ++pSrc;
                    ch=U16_GET_SUPPLEMENTARY(ch, ch2);
                } else if(subchar>=0) {
                    ch=subchar;
                    ++numSubstitutions;
                } else {
                    
                    *pErrorCode = U_INVALID_CHAR_FOUND;
                    return NULL;
                }

                length = U8_LENGTH(ch);
                if((pDestLimit - pDest) >= length) {
                    
                    pDest=_appendUTF8(pDest, ch);
                } else {
                    reqLength = length;
                    break;
                }
            }
        }
        while(pSrc<pSrcLimit) {
            ch=*pSrc++;
            if(ch<=0x7f) {
                ++reqLength;
            } else if(ch<=0x7ff) {
                reqLength+=2;
            } else if(!U16_IS_SURROGATE(ch)) {
                reqLength+=3;
            } else if(U16_IS_SURROGATE_LEAD(ch) && pSrc<pSrcLimit && U16_IS_TRAIL(ch2=*pSrc)) {
                ++pSrc;
                reqLength+=4;
            } else if(subchar>=0) {
                reqLength+=U8_LENGTH(subchar);
                ++numSubstitutions;
            } else {
                
                *pErrorCode = U_INVALID_CHAR_FOUND;
                return NULL;
            }
        }
    }

    reqLength+=(int32_t)(pDest - (uint8_t *)dest);

    if(pNumSubstitutions!=NULL) {
        *pNumSubstitutions=numSubstitutions;
    }

    if(pDestLength){
        *pDestLength = reqLength;
    }

    
    u_terminateChars(dest, destCapacity, reqLength, pErrorCode);
    return dest;
}

U_CAPI char* U_EXPORT2 
u_strToUTF8(char *dest,
            int32_t destCapacity,
            int32_t *pDestLength,
            const UChar *pSrc,
            int32_t srcLength,
            UErrorCode *pErrorCode){
    return u_strToUTF8WithSub(
            dest, destCapacity, pDestLength,
            pSrc, srcLength,
            U_SENTINEL, NULL,
            pErrorCode);
}

U_CAPI UChar* U_EXPORT2
u_strFromJavaModifiedUTF8WithSub(
        UChar *dest,
        int32_t destCapacity,
        int32_t *pDestLength,
        const char *src,
        int32_t srcLength,
        UChar32 subchar, int32_t *pNumSubstitutions,
        UErrorCode *pErrorCode) {
    UChar *pDest = dest;
    UChar *pDestLimit = dest+destCapacity;
    UChar32 ch;
    int32_t reqLength = 0;
    const uint8_t* pSrc = (const uint8_t*) src;
    const uint8_t *pSrcLimit;
    int32_t count;
    uint8_t t1, t2; 
    int32_t numSubstitutions;

    
    if(U_FAILURE(*pErrorCode)){
        return NULL;
    }
    if( (src==NULL && srcLength!=0) || srcLength < -1 ||
        (dest==NULL && destCapacity!=0) || destCapacity<0 ||
        subchar > 0x10ffff || U_IS_SURROGATE(subchar)
    ) {
        *pErrorCode = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }

    if(pNumSubstitutions!=NULL) {
        *pNumSubstitutions=0;
    }
    numSubstitutions=0;

    if(srcLength < 0) {
        



        while(((ch = *pSrc) != 0) && ch <= 0x7f && (pDest < pDestLimit)) {
            *pDest++=(UChar)ch;
            ++pSrc;
        }
        if(ch == 0) {
            reqLength=(int32_t)(pDest - dest);
            if(pDestLength) {
                *pDestLength = reqLength;
            }

            
            u_terminateUChars(dest, destCapacity, reqLength, pErrorCode);
            return dest;
        }
        srcLength = uprv_strlen((const char *)pSrc);
    }

    
    pSrcLimit = (pSrc == NULL) ? NULL : pSrc + srcLength;
    for(;;) {
        count = (int32_t)(pDestLimit - pDest);
        srcLength = (int32_t)(pSrcLimit - pSrc);
        if(count >= srcLength && srcLength > 0 && *pSrc <= 0x7f) {
            
            const uint8_t *prevSrc = pSrc;
            int32_t delta;
            while(pSrc < pSrcLimit && (ch = *pSrc) <= 0x7f) {
                *pDest++=(UChar)ch;
                ++pSrc;
            }
            delta = (int32_t)(pSrc - prevSrc);
            count -= delta;
            srcLength -= delta;
        }
        



        srcLength /= 3;
        if(count > srcLength) {
            count = srcLength; 
        }
        if(count < 3) {
            



            break;
        }
        do {
            ch = *pSrc;
            if(ch <= 0x7f){
                *pDest++=(UChar)ch;
                ++pSrc;
            } else {
                if(ch >= 0xe0) {
                    if( 
                        ch <= 0xef &&
                        (t1 = (uint8_t)(pSrc[1] - 0x80)) <= 0x3f &&
                        (t2 = (uint8_t)(pSrc[2] - 0x80)) <= 0x3f
                    ) {
                        
                        *pDest++ = (UChar)((ch << 12) | (t1 << 6) | t2);
                        pSrc += 3;
                        continue;
                    }
                } else {
                    if( 
                        ch >= 0xc0 &&
                        (t1 = (uint8_t)(pSrc[1] - 0x80)) <= 0x3f
                    ) {
                        *pDest++ = (UChar)(((ch & 0x1f) << 6) | t1);
                        pSrc += 2;
                        continue;
                    }
                }

                if(subchar < 0) {
                    *pErrorCode = U_INVALID_CHAR_FOUND;
                    return NULL;
                } else if(subchar > 0xffff && --count == 0) {
                    



                    break;
                } else {
                    
                    ++pSrc; 
                    utf8_nextCharSafeBodyPointer(&pSrc, pSrcLimit, ch);
                    ++numSubstitutions;
                    if(subchar<=0xFFFF) {
                        *(pDest++)=(UChar)subchar;
                    } else {
                        *(pDest++)=U16_LEAD(subchar);
                        *(pDest++)=U16_TRAIL(subchar);
                    }
                }
            }
        } while(--count > 0);
    }

    while((pSrc<pSrcLimit) && (pDest<pDestLimit)) {
        ch = *pSrc;
        if(ch <= 0x7f){
            *pDest++=(UChar)ch;
            ++pSrc;
        } else {
            if(ch >= 0xe0) {
                if( 
                    ch <= 0xef &&
                    ((pSrcLimit - pSrc) >= 3) &&
                    (t1 = (uint8_t)(pSrc[1] - 0x80)) <= 0x3f &&
                    (t2 = (uint8_t)(pSrc[2] - 0x80)) <= 0x3f
                ) {
                    
                    *pDest++ = (UChar)((ch << 12) | (t1 << 6) | t2);
                    pSrc += 3;
                    continue;
                }
            } else {
                if( 
                    ch >= 0xc0 &&
                    ((pSrcLimit - pSrc) >= 2) &&
                    (t1 = (uint8_t)(pSrc[1] - 0x80)) <= 0x3f
                ) {
                    *pDest++ = (UChar)(((ch & 0x1f) << 6) | t1);
                    pSrc += 2;
                    continue;
                }
            }

            if(subchar < 0) {
                *pErrorCode = U_INVALID_CHAR_FOUND;
                return NULL;
            } else {
                
                ++pSrc; 
                utf8_nextCharSafeBodyPointer(&pSrc, pSrcLimit, ch);
                ++numSubstitutions;
                if(subchar<=0xFFFF) {
                    *(pDest++)=(UChar)subchar;
                } else {
                    *(pDest++)=U16_LEAD(subchar);
                    if(pDest<pDestLimit) {
                        *(pDest++)=U16_TRAIL(subchar);
                    } else {
                        reqLength++;
                        break;
                    }
                }
            }
        }
    }

    
    while(pSrc < pSrcLimit){
        ch = *pSrc;
        if(ch <= 0x7f) {
            reqLength++;
            ++pSrc;
        } else {
            if(ch >= 0xe0) {
                if( 
                    ch <= 0xef &&
                    ((pSrcLimit - pSrc) >= 3) &&
                    (uint8_t)(pSrc[1] - 0x80) <= 0x3f &&
                    (uint8_t)(pSrc[2] - 0x80) <= 0x3f
                ) {
                    reqLength++;
                    pSrc += 3;
                    continue;
                }
            } else {
                if( 
                    ch >= 0xc0 &&
                    ((pSrcLimit - pSrc) >= 2) &&
                    (uint8_t)(pSrc[1] - 0x80) <= 0x3f
                ) {
                    reqLength++;
                    pSrc += 2;
                    continue;
                }
            }

            if(subchar < 0) {
                *pErrorCode = U_INVALID_CHAR_FOUND;
                return NULL;
            } else {
                
                ++pSrc; 
                utf8_nextCharSafeBodyPointer(&pSrc, pSrcLimit, ch);
                ++numSubstitutions;
                reqLength+=U16_LENGTH(ch);
            }
        }
    }

    if(pNumSubstitutions!=NULL) {
        *pNumSubstitutions=numSubstitutions;
    }

    reqLength+=(int32_t)(pDest - dest);
    if(pDestLength) {
        *pDestLength = reqLength;
    }

    
    u_terminateUChars(dest, destCapacity, reqLength, pErrorCode);
    return dest;
}

U_CAPI char* U_EXPORT2 
u_strToJavaModifiedUTF8(
        char *dest,
        int32_t destCapacity,
        int32_t *pDestLength,
        const UChar *src, 
        int32_t srcLength,
        UErrorCode *pErrorCode) {
    int32_t reqLength=0;
    uint32_t ch=0;
    uint8_t *pDest = (uint8_t *)dest;
    uint8_t *pDestLimit = pDest + destCapacity;
    const UChar *pSrcLimit;
    int32_t count;

    
    if(U_FAILURE(*pErrorCode)){
        return NULL;
    }
    if( (src==NULL && srcLength!=0) || srcLength < -1 ||
        (dest==NULL && destCapacity!=0) || destCapacity<0
    ) {
        *pErrorCode = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }

    if(srcLength==-1) {
        
        while((ch=*src)<=0x7f && ch != 0 && pDest<pDestLimit) {
            *pDest++ = (uint8_t)ch;
            ++src;
        }
        if(ch == 0) {
            reqLength=(int32_t)(pDest - (uint8_t *)dest);
            if(pDestLength) {
                *pDestLength = reqLength;
            }

            
            u_terminateChars(dest, destCapacity, reqLength, pErrorCode);
            return dest;
        }
        srcLength = u_strlen(src);
    }

    
    pSrcLimit = (src!=NULL)?(src+srcLength):NULL;
    for(;;) {
        count = (int32_t)(pDestLimit - pDest);
        srcLength = (int32_t)(pSrcLimit - src);
        if(count >= srcLength && srcLength > 0 && *src <= 0x7f) {
            
            const UChar *prevSrc = src;
            int32_t delta;
            while(src < pSrcLimit && (ch = *src) <= 0x7f && ch != 0) {
                *pDest++=(uint8_t)ch;
                ++src;
            }
            delta = (int32_t)(src - prevSrc);
            count -= delta;
            srcLength -= delta;
        }
        



        count /= 3;
        if(count > srcLength) {
            count = srcLength; 
        }
        if(count < 3) {
            



            break;
        }
        do {
            ch=*src++;
            if(ch <= 0x7f && ch != 0) {
                *pDest++ = (uint8_t)ch;
            } else if(ch <= 0x7ff) {
                *pDest++=(uint8_t)((ch>>6)|0xc0);
                *pDest++=(uint8_t)((ch&0x3f)|0x80);
            } else {
                *pDest++=(uint8_t)((ch>>12)|0xe0);
                *pDest++=(uint8_t)(((ch>>6)&0x3f)|0x80);
                *pDest++=(uint8_t)((ch&0x3f)|0x80);
            }
        } while(--count > 0);
    }

    while(src<pSrcLimit) {
        ch=*src++;
        if(ch <= 0x7f && ch != 0) {
            if(pDest<pDestLimit) {
                *pDest++ = (uint8_t)ch;
            } else {
                reqLength = 1;
                break;
            }
        } else if(ch <= 0x7ff) {
            if((pDestLimit - pDest) >= 2) {
                *pDest++=(uint8_t)((ch>>6)|0xc0);
                *pDest++=(uint8_t)((ch&0x3f)|0x80);
            } else {
                reqLength = 2;
                break;
            }
        } else {
            if((pDestLimit - pDest) >= 3) {
                *pDest++=(uint8_t)((ch>>12)|0xe0);
                *pDest++=(uint8_t)(((ch>>6)&0x3f)|0x80);
                *pDest++=(uint8_t)((ch&0x3f)|0x80);
            } else {
                reqLength = 3;
                break;
            }
        }
    }
    while(src<pSrcLimit) {
        ch=*src++;
        if(ch <= 0x7f && ch != 0) {
            ++reqLength;
        } else if(ch<=0x7ff) {
            reqLength+=2;
        } else {
            reqLength+=3;
        }
    }

    reqLength+=(int32_t)(pDest - (uint8_t *)dest);
    if(pDestLength){
        *pDestLength = reqLength;
    }

    
    u_terminateChars(dest, destCapacity, reqLength, pErrorCode);
    return dest;
}
