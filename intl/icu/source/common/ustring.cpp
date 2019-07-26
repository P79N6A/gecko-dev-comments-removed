
















#include "unicode/utypes.h"
#include "unicode/putil.h"
#include "unicode/ustring.h"
#include "unicode/utf16.h"
#include "cstring.h"
#include "cwchar.h"
#include "cmemory.h"
#include "ustr_imp.h"




#define U_BMP_MAX 0xffff








static inline UBool
isMatchAtCPBoundary(const UChar *start, const UChar *match, const UChar *matchLimit, const UChar *limit) {
    if(U16_IS_TRAIL(*match) && start!=match && U16_IS_LEAD(*(match-1))) {
        
        return FALSE;
    }
    if(U16_IS_LEAD(*(matchLimit-1)) && match!=limit && U16_IS_TRAIL(*matchLimit)) {
        
        return FALSE;
    }
    return TRUE;
}

U_CAPI UChar * U_EXPORT2
u_strFindFirst(const UChar *s, int32_t length,
               const UChar *sub, int32_t subLength) {
    const UChar *start, *p, *q, *subLimit;
    UChar c, cs, cq;

    if(sub==NULL || subLength<-1) {
        return (UChar *)s;
    }
    if(s==NULL || length<-1) {
        return NULL;
    }

    start=s;

    if(length<0 && subLength<0) {
        
        if((cs=*sub++)==0) {
            return (UChar *)s;
        }
        if(*sub==0 && !U16_IS_SURROGATE(cs)) {
            
            return u_strchr(s, cs);
        }

        while((c=*s++)!=0) {
            if(c==cs) {
                
                p=s;
                q=sub;
                for(;;) {
                    if((cq=*q)==0) {
                        if(isMatchAtCPBoundary(start, s-1, p, NULL)) {
                            return (UChar *)(s-1); 
                        } else {
                            break; 
                        }
                    }
                    if((c=*p)==0) {
                        return NULL; 
                    }
                    if(c!=cq) {
                        break; 
                    }
                    ++p;
                    ++q;
                }
            }
        }

        
        return NULL;
    }

    if(subLength<0) {
        subLength=u_strlen(sub);
    }
    if(subLength==0) {
        return (UChar *)s;
    }

    
    cs=*sub++;
    --subLength;
    subLimit=sub+subLength;

    if(subLength==0 && !U16_IS_SURROGATE(cs)) {
        
        return length<0 ? u_strchr(s, cs) : u_memchr(s, cs, length);
    }

    if(length<0) {
        
        while((c=*s++)!=0) {
            if(c==cs) {
                
                p=s;
                q=sub;
                for(;;) {
                    if(q==subLimit) {
                        if(isMatchAtCPBoundary(start, s-1, p, NULL)) {
                            return (UChar *)(s-1); 
                        } else {
                            break; 
                        }
                    }
                    if((c=*p)==0) {
                        return NULL; 
                    }
                    if(c!=*q) {
                        break; 
                    }
                    ++p;
                    ++q;
                }
            }
        }
    } else {
        const UChar *limit, *preLimit;

        
        if(length<=subLength) {
            return NULL; 
        }

        limit=s+length;

        
        preLimit=limit-subLength;

        while(s!=preLimit) {
            c=*s++;
            if(c==cs) {
                
                p=s;
                q=sub;
                for(;;) {
                    if(q==subLimit) {
                        if(isMatchAtCPBoundary(start, s-1, p, limit)) {
                            return (UChar *)(s-1); 
                        } else {
                            break; 
                        }
                    }
                    if(*p!=*q) {
                        break; 
                    }
                    ++p;
                    ++q;
                }
            }
        }
    }

    
    return NULL;
}

U_CAPI UChar * U_EXPORT2
u_strstr(const UChar *s, const UChar *substring) {
    return u_strFindFirst(s, -1, substring, -1);
}

U_CAPI UChar * U_EXPORT2
u_strchr(const UChar *s, UChar c) {
    if(U16_IS_SURROGATE(c)) {
        
        return u_strFindFirst(s, -1, &c, 1);
    } else {
        UChar cs;

        
        for(;;) {
            if((cs=*s)==c) {
                return (UChar *)s;
            }
            if(cs==0) {
                return NULL;
            }
            ++s;
        }
    }
}

U_CAPI UChar * U_EXPORT2
u_strchr32(const UChar *s, UChar32 c) {
    if((uint32_t)c<=U_BMP_MAX) {
        
        return u_strchr(s, (UChar)c);
    } else if((uint32_t)c<=UCHAR_MAX_VALUE) {
        
        UChar cs, lead=U16_LEAD(c), trail=U16_TRAIL(c);

        while((cs=*s++)!=0) {
            if(cs==lead && *s==trail) {
                return (UChar *)(s-1);
            }
        }
        return NULL;
    } else {
        
        return NULL;
    }
}

U_CAPI UChar * U_EXPORT2
u_memchr(const UChar *s, UChar c, int32_t count) {
    if(count<=0) {
        return NULL; 
    } else if(U16_IS_SURROGATE(c)) {
        
        return u_strFindFirst(s, count, &c, 1);
    } else {
        
        const UChar *limit=s+count;
        do {
            if(*s==c) {
                return (UChar *)s;
            }
        } while(++s!=limit);
        return NULL;
    }
}

U_CAPI UChar * U_EXPORT2
u_memchr32(const UChar *s, UChar32 c, int32_t count) {
    if((uint32_t)c<=U_BMP_MAX) {
        
        return u_memchr(s, (UChar)c, count);
    } else if(count<2) {
        
        return NULL;
    } else if((uint32_t)c<=UCHAR_MAX_VALUE) {
        
        const UChar *limit=s+count-1; 
        UChar lead=U16_LEAD(c), trail=U16_TRAIL(c);

        do {
            if(*s==lead && *(s+1)==trail) {
                return (UChar *)s;
            }
        } while(++s!=limit);
        return NULL;
    } else {
        
        return NULL;
    }
}



U_CAPI UChar * U_EXPORT2
u_strFindLast(const UChar *s, int32_t length,
              const UChar *sub, int32_t subLength) {
    const UChar *start, *limit, *p, *q, *subLimit;
    UChar c, cs;

    if(sub==NULL || subLength<-1) {
        return (UChar *)s;
    }
    if(s==NULL || length<-1) {
        return NULL;
    }

    









    if(subLength<0) {
        subLength=u_strlen(sub);
    }
    if(subLength==0) {
        return (UChar *)s;
    }

    
    subLimit=sub+subLength;
    cs=*(--subLimit);
    --subLength;

    if(subLength==0 && !U16_IS_SURROGATE(cs)) {
        
        return length<0 ? u_strrchr(s, cs) : u_memrchr(s, cs, length);
    }

    if(length<0) {
        length=u_strlen(s);
    }

    
    if(length<=subLength) {
        return NULL; 
    }

    start=s;
    limit=s+length;

    
    s+=subLength;

    while(s!=limit) {
        c=*(--limit);
        if(c==cs) {
            
            p=limit;
            q=subLimit;
            for(;;) {
                if(q==sub) {
                    if(isMatchAtCPBoundary(start, p, limit+1, start+length)) {
                        return (UChar *)p; 
                    } else {
                        break; 
                    }
                }
                if(*(--p)!=*(--q)) {
                    break; 
                }
            }
        }
    }

    
    return NULL;
}

U_CAPI UChar * U_EXPORT2
u_strrstr(const UChar *s, const UChar *substring) {
    return u_strFindLast(s, -1, substring, -1);
}

U_CAPI UChar * U_EXPORT2
u_strrchr(const UChar *s, UChar c) {
    if(U16_IS_SURROGATE(c)) {
        
        return u_strFindLast(s, -1, &c, 1);
    } else {
        const UChar *result=NULL;
        UChar cs;

        
        for(;;) {
            if((cs=*s)==c) {
                result=s;
            }
            if(cs==0) {
                return (UChar *)result;
            }
            ++s;
        }
    }
}

U_CAPI UChar * U_EXPORT2
u_strrchr32(const UChar *s, UChar32 c) {
    if((uint32_t)c<=U_BMP_MAX) {
        
        return u_strrchr(s, (UChar)c);
    } else if((uint32_t)c<=UCHAR_MAX_VALUE) {
        
        const UChar *result=NULL;
        UChar cs, lead=U16_LEAD(c), trail=U16_TRAIL(c);

        while((cs=*s++)!=0) {
            if(cs==lead && *s==trail) {
                result=s-1;
            }
        }
        return (UChar *)result;
    } else {
        
        return NULL;
    }
}

U_CAPI UChar * U_EXPORT2
u_memrchr(const UChar *s, UChar c, int32_t count) {
    if(count<=0) {
        return NULL; 
    } else if(U16_IS_SURROGATE(c)) {
        
        return u_strFindLast(s, count, &c, 1);
    } else {
        
        const UChar *limit=s+count;
        do {
            if(*(--limit)==c) {
                return (UChar *)limit;
            }
        } while(s!=limit);
        return NULL;
    }
}

U_CAPI UChar * U_EXPORT2
u_memrchr32(const UChar *s, UChar32 c, int32_t count) {
    if((uint32_t)c<=U_BMP_MAX) {
        
        return u_memrchr(s, (UChar)c, count);
    } else if(count<2) {
        
        return NULL;
    } else if((uint32_t)c<=UCHAR_MAX_VALUE) {
        
        const UChar *limit=s+count-1;
        UChar lead=U16_LEAD(c), trail=U16_TRAIL(c);

        do {
            if(*limit==trail && *(limit-1)==lead) {
                return (UChar *)(limit-1);
            }
        } while(s!=--limit);
        return NULL;
    } else {
        
        return NULL;
    }
}









static int32_t
_matchFromSet(const UChar *string, const UChar *matchSet, UBool polarity) {
    int32_t matchLen, matchBMPLen, strItr, matchItr;
    UChar32 stringCh, matchCh;
    UChar c, c2;

    
    matchBMPLen = 0;
    while((c = matchSet[matchBMPLen]) != 0 && U16_IS_SINGLE(c)) {
        ++matchBMPLen;
    }

    
    matchLen = matchBMPLen;
    while(matchSet[matchLen] != 0) {
        ++matchLen;
    }

    for(strItr = 0; (c = string[strItr]) != 0;) {
        ++strItr;
        if(U16_IS_SINGLE(c)) {
            if(polarity) {
                for(matchItr = 0; matchItr < matchLen; ++matchItr) {
                    if(c == matchSet[matchItr]) {
                        return strItr - 1; 
                    }
                }
            } else {
                for(matchItr = 0; matchItr < matchLen; ++matchItr) {
                    if(c == matchSet[matchItr]) {
                        goto endloop;
                    }
                }
                return strItr - 1; 
            }
        } else {
            



            if(U16_IS_SURROGATE_LEAD(c) && U16_IS_TRAIL(c2 = string[strItr])) {
                ++strItr;
                stringCh = U16_GET_SUPPLEMENTARY(c, c2);
            } else {
                stringCh = c; 
            }

            if(polarity) {
                for(matchItr = matchBMPLen; matchItr < matchLen;) {
                    U16_NEXT(matchSet, matchItr, matchLen, matchCh);
                    if(stringCh == matchCh) {
                        return strItr - U16_LENGTH(stringCh); 
                    }
                }
            } else {
                for(matchItr = matchBMPLen; matchItr < matchLen;) {
                    U16_NEXT(matchSet, matchItr, matchLen, matchCh);
                    if(stringCh == matchCh) {
                        goto endloop;
                    }
                }
                return strItr - U16_LENGTH(stringCh); 
            }
        }
endloop:
        ;
    }

    
    return -strItr-1;
}


U_CAPI UChar * U_EXPORT2
u_strpbrk(const UChar *string, const UChar *matchSet)
{
    int32_t idx = _matchFromSet(string, matchSet, TRUE);
    if(idx >= 0) {
        return (UChar *)string + idx;
    } else {
        return NULL;
    }
}


U_CAPI int32_t U_EXPORT2
u_strcspn(const UChar *string, const UChar *matchSet)
{
    int32_t idx = _matchFromSet(string, matchSet, TRUE);
    if(idx >= 0) {
        return idx;
    } else {
        return -idx - 1; 
    }
}


U_CAPI int32_t U_EXPORT2
u_strspn(const UChar *string, const UChar *matchSet)
{
    int32_t idx = _matchFromSet(string, matchSet, FALSE);
    if(idx >= 0) {
        return idx;
    } else {
        return -idx - 1; 
    }
}



U_CAPI UChar* U_EXPORT2
u_strtok_r(UChar    *src, 
     const UChar    *delim,
           UChar   **saveState)
{
    UChar *tokSource;
    UChar *nextToken;
    uint32_t nonDelimIdx;

    
    if (src != NULL) {
        tokSource = src;
        *saveState = src; 
    }
    else if (*saveState) {
        tokSource = *saveState;
    }
    else {
        
        
        return NULL;
    }

    
    nonDelimIdx = u_strspn(tokSource, delim);
    tokSource = &tokSource[nonDelimIdx];

    if (*tokSource) {
        nextToken = u_strpbrk(tokSource, delim);
        if (nextToken != NULL) {
            
            *(nextToken++) = 0;
            *saveState = nextToken;
            return tokSource;
        }
        else if (*saveState) {
            
            *saveState = NULL;
            return tokSource;
        }
    }
    else {
        
        *saveState = NULL;
    }
    return NULL;
}



U_CAPI UChar* U_EXPORT2
u_strcat(UChar     *dst, 
    const UChar     *src)
{
    UChar *anchor = dst;            

    while(*dst != 0) {              
        ++dst;
    }
    while((*(dst++) = *(src++)) != 0) {     
    }

    return anchor;
}

U_CAPI UChar*  U_EXPORT2
u_strncat(UChar     *dst, 
     const UChar     *src, 
     int32_t     n ) 
{
    if(n > 0) {
        UChar *anchor = dst;            

        while(*dst != 0) {              
            ++dst;
        }
        while((*dst = *src) != 0) {     
            ++dst;
            if(--n == 0) {
                *dst = 0;
                break;
            }
            ++src;
        }

        return anchor;
    } else {
        return dst;
    }
}



U_CAPI int32_t   U_EXPORT2
u_strcmp(const UChar *s1, 
    const UChar *s2) 
{
    UChar  c1, c2;

    for(;;) {
        c1=*s1++;
        c2=*s2++;
        if (c1 != c2 || c1 == 0) {
            break;
        }
    }
    return (int32_t)c1 - (int32_t)c2;
}

U_CFUNC int32_t U_EXPORT2
uprv_strCompare(const UChar *s1, int32_t length1,
                const UChar *s2, int32_t length2,
                UBool strncmpStyle, UBool codePointOrder) {
    const UChar *start1, *start2, *limit1, *limit2;
    UChar c1, c2;

    
    start1=s1;
    start2=s2;

    
    if(length1<0 && length2<0) {
        
        if(s1==s2) {
            return 0;
        }

        for(;;) {
            c1=*s1;
            c2=*s2;
            if(c1!=c2) {
                break;
            }
            if(c1==0) {
                return 0;
            }
            ++s1;
            ++s2;
        }

        
        limit1=limit2=NULL;
    } else if(strncmpStyle) {
        
        if(s1==s2) {
            return 0;
        }

        limit1=start1+length1;

        for(;;) {
            
            if(s1==limit1) {
                return 0;
            }

            c1=*s1;
            c2=*s2;
            if(c1!=c2) {
                break;
            }
            if(c1==0) {
                return 0;
            }
            ++s1;
            ++s2;
        }

        
        limit2=start2+length1; 
    } else {
        
        int32_t lengthResult;

        if(length1<0) {
            length1=u_strlen(s1);
        }
        if(length2<0) {
            length2=u_strlen(s2);
        }

        
        if(length1<length2) {
            lengthResult=-1;
            limit1=start1+length1;
        } else if(length1==length2) {
            lengthResult=0;
            limit1=start1+length1;
        } else  {
            lengthResult=1;
            limit1=start1+length2;
        }

        if(s1==s2) {
            return lengthResult;
        }

        for(;;) {
            
            if(s1==limit1) {
                return lengthResult;
            }

            c1=*s1;
            c2=*s2;
            if(c1!=c2) {
                break;
            }
            ++s1;
            ++s2;
        }

        
        limit1=start1+length1;
        limit2=start2+length2;
    }

    
    if(c1>=0xd800 && c2>=0xd800 && codePointOrder) {
        
        if(
            (c1<=0xdbff && (s1+1)!=limit1 && U16_IS_TRAIL(*(s1+1))) ||
            (U16_IS_TRAIL(c1) && start1!=s1 && U16_IS_LEAD(*(s1-1)))
        ) {
            
        } else {
            
            c1-=0x2800;
        }

        if(
            (c2<=0xdbff && (s2+1)!=limit2 && U16_IS_TRAIL(*(s2+1))) ||
            (U16_IS_TRAIL(c2) && start2!=s2 && U16_IS_LEAD(*(s2-1)))
        ) {
            
        } else {
            
            c2-=0x2800;
        }
    }

    
    return (int32_t)c1-(int32_t)c2;
}







U_CAPI int32_t U_EXPORT2
u_strCompareIter(UCharIterator *iter1, UCharIterator *iter2, UBool codePointOrder) {
    UChar32 c1, c2;

    
    if(iter1==NULL || iter2==NULL) {
        return 0; 
    }
    if(iter1==iter2) {
        return 0; 
    }

    
    iter1->move(iter1, 0, UITER_START);
    iter2->move(iter2, 0, UITER_START);

    
    for(;;) {
        c1=iter1->next(iter1);
        c2=iter2->next(iter2);
        if(c1!=c2) {
            break;
        }
        if(c1==-1) {
            return 0;
        }
    }

    
    if(c1>=0xd800 && c2>=0xd800 && codePointOrder) {
        
        if(
            (c1<=0xdbff && U16_IS_TRAIL(iter1->current(iter1))) ||
            (U16_IS_TRAIL(c1) && (iter1->previous(iter1), U16_IS_LEAD(iter1->previous(iter1))))
        ) {
            
        } else {
            
            c1-=0x2800;
        }

        if(
            (c2<=0xdbff && U16_IS_TRAIL(iter2->current(iter2))) ||
            (U16_IS_TRAIL(c2) && (iter2->previous(iter2), U16_IS_LEAD(iter2->previous(iter2))))
        ) {
            
        } else {
            
            c2-=0x2800;
        }
    }

    
    return (int32_t)c1-(int32_t)c2;
}

#if 0
















void fragment {
        
        if(c1<=0xdbff) {
            if(!U16_IS_TRAIL(iter1->current(iter1))) {
                
                c1-=0x2800;
            }
        } else if(c1<=0xdfff) {
            int32_t idx=iter1->getIndex(iter1, UITER_CURRENT);
            iter1->previous(iter1); 
            if(!U16_IS_LEAD(iter1->previous(iter1))) {
                
                c1-=0x2800;
            }
            
            iter1->move(iter1, idx, UITER_ZERO);
        } else  {
            
            c1-=0x2800;
        }
}
#endif

U_CAPI int32_t U_EXPORT2
u_strCompare(const UChar *s1, int32_t length1,
             const UChar *s2, int32_t length2,
             UBool codePointOrder) {
    
    if(s1==NULL || length1<-1 || s2==NULL || length2<-1) {
        return 0;
    }
    return uprv_strCompare(s1, length1, s2, length2, FALSE, codePointOrder);
}


U_CAPI int32_t U_EXPORT2
u_strcmpCodePointOrder(const UChar *s1, const UChar *s2) {
    return uprv_strCompare(s1, -1, s2, -1, FALSE, TRUE);
}

U_CAPI int32_t   U_EXPORT2
u_strncmp(const UChar     *s1, 
     const UChar     *s2, 
     int32_t     n) 
{
    if(n > 0) {
        int32_t rc;
        for(;;) {
            rc = (int32_t)*s1 - (int32_t)*s2;
            if(rc != 0 || *s1 == 0 || --n == 0) {
                return rc;
            }
            ++s1;
            ++s2;
        }
    } else {
        return 0;
    }
}

U_CAPI int32_t U_EXPORT2
u_strncmpCodePointOrder(const UChar *s1, const UChar *s2, int32_t n) {
    return uprv_strCompare(s1, n, s2, n, TRUE, TRUE);
}

U_CAPI UChar* U_EXPORT2
u_strcpy(UChar     *dst, 
    const UChar     *src) 
{
    UChar *anchor = dst;            

    while((*(dst++) = *(src++)) != 0) {     
    }

    return anchor;
}

U_CAPI UChar*  U_EXPORT2
u_strncpy(UChar     *dst, 
     const UChar     *src, 
     int32_t     n) 
{
    UChar *anchor = dst;            

    
    while(n > 0 && (*(dst++) = *(src++)) != 0) {
        --n;
    }

    return anchor;
}

U_CAPI int32_t   U_EXPORT2
u_strlen(const UChar *s) 
{
#if U_SIZEOF_WCHAR_T == U_SIZEOF_UCHAR
    return (int32_t)uprv_wcslen(s);
#else
    const UChar *t = s;
    while(*t != 0) {
      ++t;
    }
    return t - s;
#endif
}

U_CAPI int32_t U_EXPORT2
u_countChar32(const UChar *s, int32_t length) {
    int32_t count;

    if(s==NULL || length<-1) {
        return 0;
    }

    count=0;
    if(length>=0) {
        while(length>0) {
            ++count;
            if(U16_IS_LEAD(*s) && length>=2 && U16_IS_TRAIL(*(s+1))) {
                s+=2;
                length-=2;
            } else {
                ++s;
                --length;
            }
        }
    } else  {
        UChar c;

        for(;;) {
            if((c=*s++)==0) {
                break;
            }
            ++count;

            



            if(U16_IS_LEAD(c) && U16_IS_TRAIL(*s)) {
                ++s;
            }
        }
    }
    return count;
}

U_CAPI UBool U_EXPORT2
u_strHasMoreChar32Than(const UChar *s, int32_t length, int32_t number) {

    if(number<0) {
        return TRUE;
    }
    if(s==NULL || length<-1) {
        return FALSE;
    }

    if(length==-1) {
        
        UChar c;

        
        for(;;) {
            if((c=*s++)==0) {
                return FALSE;
            }
            if(number==0) {
                return TRUE;
            }
            if(U16_IS_LEAD(c) && U16_IS_TRAIL(*s)) {
                ++s;
            }
            --number;
        }
    } else {
        
        const UChar *limit;
        int32_t maxSupplementary;

        
        if(((length+1)/2)>number) {
            return TRUE;
        }

        
        maxSupplementary=length-number;
        if(maxSupplementary<=0) {
            return FALSE;
        }
        

        



        limit=s+length;
        for(;;) {
            if(s==limit) {
                return FALSE;
            }
            if(number==0) {
                return TRUE;
            }
            if(U16_IS_LEAD(*s++) && s!=limit && U16_IS_TRAIL(*s)) {
                ++s;
                if(--maxSupplementary<=0) {
                    
                    return FALSE;
                }
            }
            --number;
        }
    }
}

U_CAPI UChar * U_EXPORT2
u_memcpy(UChar *dest, const UChar *src, int32_t count) {
    if(count > 0) {
        uprv_memcpy(dest, src, count*U_SIZEOF_UCHAR);
    }
    return dest;
}

U_CAPI UChar * U_EXPORT2
u_memmove(UChar *dest, const UChar *src, int32_t count) {
    if(count > 0) {
        uprv_memmove(dest, src, count*U_SIZEOF_UCHAR);
    }
    return dest;
}

U_CAPI UChar * U_EXPORT2
u_memset(UChar *dest, UChar c, int32_t count) {
    if(count > 0) {
        UChar *ptr = dest;
        UChar *limit = dest + count;

        while (ptr < limit) {
            *(ptr++) = c;
        }
    }
    return dest;
}

U_CAPI int32_t U_EXPORT2
u_memcmp(const UChar *buf1, const UChar *buf2, int32_t count) {
    if(count > 0) {
        const UChar *limit = buf1 + count;
        int32_t result;

        while (buf1 < limit) {
            result = (int32_t)(uint16_t)*buf1 - (int32_t)(uint16_t)*buf2;
            if (result != 0) {
                return result;
            }
            buf1++;
            buf2++;
        }
    }
    return 0;
}

U_CAPI int32_t U_EXPORT2
u_memcmpCodePointOrder(const UChar *s1, const UChar *s2, int32_t count) {
    return uprv_strCompare(s1, count, s2, count, FALSE, TRUE);
}




static const UChar UNESCAPE_MAP[] = {
    
    
    
    
     0x61, 0x07,
     0x62, 0x08,
     0x65, 0x1b,
     0x66, 0x0c,
     0x6E, 0x0a,
     0x72, 0x0d,
     0x74, 0x09,
     0x76, 0x0b
};
enum { UNESCAPE_MAP_LENGTH = sizeof(UNESCAPE_MAP) / sizeof(UNESCAPE_MAP[0]) };


static int8_t _digit8(UChar c) {
    if (c >= 0x0030 && c <= 0x0037) {
        return (int8_t)(c - 0x0030);
    }
    return -1;
}


static int8_t _digit16(UChar c) {
    if (c >= 0x0030 && c <= 0x0039) {
        return (int8_t)(c - 0x0030);
    }
    if (c >= 0x0041 && c <= 0x0046) {
        return (int8_t)(c - (0x0041 - 10));
    }
    if (c >= 0x0061 && c <= 0x0066) {
        return (int8_t)(c - (0x0061 - 10));
    }
    return -1;
}




U_CAPI UChar32 U_EXPORT2
u_unescapeAt(UNESCAPE_CHAR_AT charAt,
             int32_t *offset,
             int32_t length,
             void *context) {

    int32_t start = *offset;
    UChar c;
    UChar32 result = 0;
    int8_t n = 0;
    int8_t minDig = 0;
    int8_t maxDig = 0;
    int8_t bitsPerDigit = 4; 
    int8_t dig;
    int32_t i;
    UBool braces = FALSE;

    
    if (*offset < 0 || *offset >= length) {
        goto err;
    }

    
    c = charAt((*offset)++, context);

    
    switch (c) {
    case 0x0075 :
        minDig = maxDig = 4;
        break;
    case 0x0055 :
        minDig = maxDig = 8;
        break;
    case 0x0078 :
        minDig = 1;
        if (*offset < length && charAt(*offset, context) == 0x7B ) {
            ++(*offset);
            braces = TRUE;
            maxDig = 8;
        } else {
            maxDig = 2;
        }
        break;
    default:
        dig = _digit8(c);
        if (dig >= 0) {
            minDig = 1;
            maxDig = 3;
            n = 1; 
            bitsPerDigit = 3;
            result = dig;
        }
        break;
    }
    if (minDig != 0) {
        while (*offset < length && n < maxDig) {
            c = charAt(*offset, context);
            dig = (int8_t)((bitsPerDigit == 3) ? _digit8(c) : _digit16(c));
            if (dig < 0) {
                break;
            }
            result = (result << bitsPerDigit) | dig;
            ++(*offset);
            ++n;
        }
        if (n < minDig) {
            goto err;
        }
        if (braces) {
            if (c != 0x7D ) {
                goto err;
            }
            ++(*offset);
        }
        if (result < 0 || result >= 0x110000) {
            goto err;
        }
        



        if (*offset < length && U16_IS_LEAD(result)) {
            int32_t ahead = *offset + 1;
            c = charAt(*offset, context);
            if (c == 0x5C  && ahead < length) {
                c = (UChar) u_unescapeAt(charAt, &ahead, length, context);
            }
            if (U16_IS_TRAIL(c)) {
                *offset = ahead;
                result = U16_GET_SUPPLEMENTARY(result, c);
            }
        }
        return result;
    }

    
    for (i=0; i<UNESCAPE_MAP_LENGTH; i+=2) {
        if (c == UNESCAPE_MAP[i]) {
            return UNESCAPE_MAP[i+1];
        } else if (c < UNESCAPE_MAP[i]) {
            break;
        }
    }

    
    if (c == 0x0063  && *offset < length) {
        c = charAt((*offset)++, context);
        if (U16_IS_LEAD(c) && *offset < length) {
            UChar c2 = charAt(*offset, context);
            if (U16_IS_TRAIL(c2)) {
                ++(*offset);
                c = (UChar) U16_GET_SUPPLEMENTARY(c, c2); 
            }
        }
        return 0x1F & c;
    }

    


    if (U16_IS_LEAD(c) && *offset < length) {
        UChar c2 = charAt(*offset, context);
        if (U16_IS_TRAIL(c2)) {
            ++(*offset);
            return U16_GET_SUPPLEMENTARY(c, c2);
        }
    }
    return c;

 err:
    
    *offset = start; 
    return (UChar32)0xFFFFFFFF;
}


static UChar U_CALLCONV
_charPtr_charAt(int32_t offset, void *context) {
    UChar c16;
    

    u_charsToUChars(((char*) context) + offset, &c16, 1);
    return c16;
}


static void _appendUChars(UChar *dest, int32_t destCapacity,
                          const char *src, int32_t srcLen) {
    if (destCapacity < 0) {
        destCapacity = 0;
    }
    if (srcLen > destCapacity) {
        srcLen = destCapacity;
    }
    u_charsToUChars(src, dest, srcLen);
}


U_CAPI int32_t U_EXPORT2
u_unescape(const char *src, UChar *dest, int32_t destCapacity) {
    const char *segment = src;
    int32_t i = 0;
    char c;

    while ((c=*src) != 0) {
        


        if (c == '\\') {
            int32_t lenParsed = 0;
            UChar32 c32;
            if (src != segment) {
                if (dest != NULL) {
                    _appendUChars(dest + i, destCapacity - i,
                                  segment, (int32_t)(src - segment));
                }
                i += (int32_t)(src - segment);
            }
            ++src; 
            c32 = (UChar32)u_unescapeAt(_charPtr_charAt, &lenParsed, (int32_t)uprv_strlen(src), (void*)src);
            if (lenParsed == 0) {
                goto err;
            }
            src += lenParsed; 
            if (dest != NULL && U16_LENGTH(c32) <= (destCapacity - i)) {
                U16_APPEND_UNSAFE(dest, i, c32);
            } else {
                i += U16_LENGTH(c32);
            }
            segment = src;
        } else {
            ++src;
        }
    }
    if (src != segment) {
        if (dest != NULL) {
            _appendUChars(dest + i, destCapacity - i,
                          segment, (int32_t)(src - segment));
        }
        i += (int32_t)(src - segment);
    }
    if (dest != NULL && i < destCapacity) {
        dest[i] = 0;
    }
    return i;

 err:
    if (dest != NULL && destCapacity > 0) {
        *dest = 0;
    }
    return 0;
}







#define __TERMINATE_STRING(dest, destCapacity, length, pErrorCode)      \
    if(pErrorCode!=NULL && U_SUCCESS(*pErrorCode)) {                    \
        /* not a public function, so no complete argument checking */   \
                                                                        \
        if(length<0) {                                                  \
            /* assume that the caller handles this */                   \
        } else if(length<destCapacity) {                                \
            /* NUL-terminate the string, the NUL fits */                \
            dest[length]=0;                                             \
            /* unset the not-terminated warning but leave all others */ \
            if(*pErrorCode==U_STRING_NOT_TERMINATED_WARNING) {          \
                *pErrorCode=U_ZERO_ERROR;                               \
            }                                                           \
        } else if(length==destCapacity) {                               \
            /* unable to NUL-terminate, but the string itself fit - set a warning code */ \
            *pErrorCode=U_STRING_NOT_TERMINATED_WARNING;                \
        } else /* length>destCapacity */ {                              \
            /* even the string itself did not fit - set an error code */ \
            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;                        \
        }                                                               \
    }

U_CAPI int32_t U_EXPORT2
u_terminateUChars(UChar *dest, int32_t destCapacity, int32_t length, UErrorCode *pErrorCode) {
    __TERMINATE_STRING(dest, destCapacity, length, pErrorCode);
    return length;
}

U_CAPI int32_t U_EXPORT2
u_terminateChars(char *dest, int32_t destCapacity, int32_t length, UErrorCode *pErrorCode) {
    __TERMINATE_STRING(dest, destCapacity, length, pErrorCode);
    return length;
}

U_CAPI int32_t U_EXPORT2
u_terminateUChar32s(UChar32 *dest, int32_t destCapacity, int32_t length, UErrorCode *pErrorCode) {
    __TERMINATE_STRING(dest, destCapacity, length, pErrorCode);
    return length;
}

U_CAPI int32_t U_EXPORT2
u_terminateWChars(wchar_t *dest, int32_t destCapacity, int32_t length, UErrorCode *pErrorCode) {
    __TERMINATE_STRING(dest, destCapacity, length, pErrorCode);
    return length;
}















#define STRING_HASH(TYPE, STR, STRLEN, DEREF) \
    int32_t hash = 0;                         \
    const TYPE *p = (const TYPE*) STR;        \
    if (p != NULL) {                          \
        int32_t len = (int32_t)(STRLEN);      \
        int32_t inc = ((len - 32) / 32) + 1;  \
        const TYPE *limit = p + len;          \
        while (p<limit) {                     \
            hash = (hash * 37) + DEREF;       \
            p += inc;                         \
        }                                     \
    }                                         \
    return hash


U_CAPI int32_t U_EXPORT2
ustr_hashUCharsN(const UChar *str, int32_t length) {
    STRING_HASH(UChar, str, length, *p);
}

U_CAPI int32_t U_EXPORT2
ustr_hashCharsN(const char *str, int32_t length) {
    STRING_HASH(uint8_t, str, length, *p);
}

U_CAPI int32_t U_EXPORT2
ustr_hashICharsN(const char *str, int32_t length) {
    STRING_HASH(char, str, length, (uint8_t)uprv_tolower(*p));
}
