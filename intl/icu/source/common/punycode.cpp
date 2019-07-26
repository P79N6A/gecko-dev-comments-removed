











































#include "unicode/utypes.h"

#if !UCONFIG_NO_IDNA

#include "unicode/ustring.h"
#include "unicode/utf.h"
#include "unicode/utf16.h"
#include "ustr_imp.h"
#include "cstring.h"
#include "cmemory.h"
#include "punycode.h"
#include "uassert.h"





#define BASE            36
#define TMIN            1
#define TMAX            26
#define SKEW            38
#define DAMP            700
#define INITIAL_BIAS    72
#define INITIAL_N       0x80


#define _HYPHEN         0X2d
#define DELIMITER       _HYPHEN

#define _ZERO_          0X30
#define _NINE           0x39

#define _SMALL_A        0X61
#define _SMALL_Z        0X7a

#define _CAPITAL_A      0X41
#define _CAPITAL_Z      0X5a

#define IS_BASIC(c) ((c)<0x80)
#define IS_BASIC_UPPERCASE(c) (_CAPITAL_A<=(c) && (c)<=_CAPITAL_Z)







static inline char
digitToBasic(int32_t digit, UBool uppercase) {
    
    
    if(digit<26) {
        if(uppercase) {
            return (char)(_CAPITAL_A+digit);
        } else {
            return (char)(_SMALL_A+digit);
        }
    } else {
        return (char)((_ZERO_-26)+digit);
    }
}






static const int8_t
basicToDigit[256]={
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    26, 27, 28, 29, 30, 31, 32, 33, 34, 35, -1, -1, -1, -1, -1, -1,

    -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,

    -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,

    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

static inline char
asciiCaseMap(char b, UBool uppercase) {
    if(uppercase) {
        if(_SMALL_A<=b && b<=_SMALL_Z) {
            b-=(_SMALL_A-_CAPITAL_A);
        }
    } else {
        if(_CAPITAL_A<=b && b<=_CAPITAL_Z) {
            b+=(_SMALL_A-_CAPITAL_A);
        }
    }
    return b;
}









static int32_t
adaptBias(int32_t delta, int32_t length, UBool firstTime) {
    int32_t count;

    if(firstTime) {
        delta/=DAMP;
    } else {
        delta/=2;
    }

    delta+=delta/length;
    for(count=0; delta>((BASE-TMIN)*TMAX)/2; count+=BASE) {
        delta/=(BASE-TMIN);
    }

    return count+(((BASE-TMIN+1)*delta)/(delta+SKEW));
}

#define MAX_CP_COUNT    200

U_CFUNC int32_t
u_strToPunycode(const UChar *src, int32_t srcLength,
                UChar *dest, int32_t destCapacity,
                const UBool *caseFlags,
                UErrorCode *pErrorCode) {

    int32_t cpBuffer[MAX_CP_COUNT];
    int32_t n, delta, handledCPCount, basicLength, destLength, bias, j, m, q, k, t, srcCPCount;
    UChar c, c2;

    
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    if(src==NULL || srcLength<-1 || (dest==NULL && destCapacity!=0)) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    



    srcCPCount=destLength=0;
    if(srcLength==-1) {
        
        for(j=0; ; ++j) {
            if((c=src[j])==0) {
                break;
            }
            if(srcCPCount==MAX_CP_COUNT) {
                
                *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
                return 0;
            }
            if(IS_BASIC(c)) {
                cpBuffer[srcCPCount++]=0;
                if(destLength<destCapacity) {
                    dest[destLength]=
                        caseFlags!=NULL ?
                            asciiCaseMap((char)c, caseFlags[j]) :
                            (char)c;
                }
                ++destLength;
            } else {
                n=(caseFlags!=NULL && caseFlags[j])<<31L;
                if(U16_IS_SINGLE(c)) {
                    n|=c;
                } else if(U16_IS_LEAD(c) && U16_IS_TRAIL(c2=src[j+1])) {
                    ++j;
                    n|=(int32_t)U16_GET_SUPPLEMENTARY(c, c2);
                } else {
                    
                    *pErrorCode=U_INVALID_CHAR_FOUND;
                    return 0;
                }
                cpBuffer[srcCPCount++]=n;
            }
        }
    } else {
        
        for(j=0; j<srcLength; ++j) {
            if(srcCPCount==MAX_CP_COUNT) {
                
                *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
                return 0;
            }
            c=src[j];
            if(IS_BASIC(c)) {
                cpBuffer[srcCPCount++]=0;
                if(destLength<destCapacity) {
                    dest[destLength]=
                        caseFlags!=NULL ?
                            asciiCaseMap((char)c, caseFlags[j]) :
                            (char)c;
                }
                ++destLength;
            } else {
                n=(caseFlags!=NULL && caseFlags[j])<<31L;
                if(U16_IS_SINGLE(c)) {
                    n|=c;
                } else if(U16_IS_LEAD(c) && (j+1)<srcLength && U16_IS_TRAIL(c2=src[j+1])) {
                    ++j;
                    n|=(int32_t)U16_GET_SUPPLEMENTARY(c, c2);
                } else {
                    
                    *pErrorCode=U_INVALID_CHAR_FOUND;
                    return 0;
                }
                cpBuffer[srcCPCount++]=n;
            }
        }
    }

    
    basicLength=destLength;
    if(basicLength>0) {
        if(destLength<destCapacity) {
            dest[destLength]=DELIMITER;
        }
        ++destLength;
    }

    





    
    n=INITIAL_N;
    delta=0;
    bias=INITIAL_BIAS;

    
    for(handledCPCount=basicLength; handledCPCount<srcCPCount; ) {
        



        for(m=0x7fffffff, j=0; j<srcCPCount; ++j) {
            q=cpBuffer[j]&0x7fffffff; 
            if(n<=q && q<m) {
                m=q;
            }
        }

        



        if(m-n>(0x7fffffff-MAX_CP_COUNT-delta)/(handledCPCount+1)) {
            *pErrorCode=U_INTERNAL_PROGRAM_ERROR;
            return 0;
        }
        delta+=(m-n)*(handledCPCount+1);
        n=m;

        
        for(j=0; j<srcCPCount; ++j) {
            q=cpBuffer[j]&0x7fffffff; 
            if(q<n) {
                ++delta;
            } else if(q==n) {
                
                for(q=delta, k=BASE; ; k+=BASE) {

                    









                    t=k-bias;
                    if(t<TMIN) {
                        t=TMIN;
                    } else if(k>=(bias+TMAX)) {
                        t=TMAX;
                    }

                    if(q<t) {
                        break;
                    }

                    if(destLength<destCapacity) {
                        dest[destLength]=digitToBasic(t+(q-t)%(BASE-t), 0);
                    }
                    ++destLength;
                    q=(q-t)/(BASE-t);
                }

                if(destLength<destCapacity) {
                    dest[destLength]=digitToBasic(q, (UBool)(cpBuffer[j]<0));
                }
                ++destLength;
                bias=adaptBias(delta, handledCPCount+1, (UBool)(handledCPCount==basicLength));
                delta=0;
                ++handledCPCount;
            }
        }

        ++delta;
        ++n;
    }

    return u_terminateUChars(dest, destCapacity, destLength, pErrorCode);
}

U_CFUNC int32_t
u_strFromPunycode(const UChar *src, int32_t srcLength,
                  UChar *dest, int32_t destCapacity,
                  UBool *caseFlags,
                  UErrorCode *pErrorCode) {
    int32_t n, destLength, i, bias, basicLength, j, in, oldi, w, k, digit, t,
            destCPCount, firstSupplementaryIndex, cpLength;
    UChar b;

    
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    if(src==NULL || srcLength<-1 || (dest==NULL && destCapacity!=0)) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    if(srcLength==-1) {
        srcLength=u_strlen(src);
    }

    







    for(j=srcLength; j>0;) {
        if(src[--j]==DELIMITER) {
            break;
        }
    }
    destLength=basicLength=destCPCount=j;
    U_ASSERT(destLength>=0);

    while(j>0) {
        b=src[--j];
        if(!IS_BASIC(b)) {
            *pErrorCode=U_INVALID_CHAR_FOUND;
            return 0;
        }

        if(j<destCapacity) {
            dest[j]=(UChar)b;

            if(caseFlags!=NULL) {
                caseFlags[j]=IS_BASIC_UPPERCASE(b);
            }
        }
    }

    
    n=INITIAL_N;
    i=0;
    bias=INITIAL_BIAS;
    firstSupplementaryIndex=1000000000;

    




    for(in=basicLength>0 ? basicLength+1 : 0; in<srcLength; ) {
        








        for(oldi=i, w=1, k=BASE; ; k+=BASE) {
            if(in>=srcLength) {
                *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                return 0;
            }

            digit=basicToDigit[(uint8_t)src[in++]];
            if(digit<0) {
                *pErrorCode=U_INVALID_CHAR_FOUND;
                return 0;
            }
            if(digit>(0x7fffffff-i)/w) {
                
                *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                return 0;
            }

            i+=digit*w;
            







            t=k-bias;
            if(t<TMIN) {
                t=TMIN;
            } else if(k>=(bias+TMAX)) {
                t=TMAX;
            }
            if(digit<t) {
                break;
            }

            if(w>0x7fffffff/(BASE-t)) {
                
                *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                return 0;
            }
            w*=BASE-t;
        }

        




        ++destCPCount;
        bias=adaptBias(i-oldi, destCPCount, (UBool)(oldi==0));

        



        if(i/destCPCount>(0x7fffffff-n)) {
            
            *pErrorCode=U_ILLEGAL_CHAR_FOUND;
            return 0;
        }

        n+=i/destCPCount;
        i%=destCPCount;
        
        

        if(n>0x10ffff || U_IS_SURROGATE(n)) {
            
            *pErrorCode=U_ILLEGAL_CHAR_FOUND;
            return 0;
        }

        
        cpLength=U16_LENGTH(n);
        if(dest!=NULL && ((destLength+cpLength)<=destCapacity)) {
            int32_t codeUnitIndex;

            









            if(i<=firstSupplementaryIndex) {
                codeUnitIndex=i;
                if(cpLength>1) {
                    firstSupplementaryIndex=codeUnitIndex;
                } else {
                    ++firstSupplementaryIndex;
                }
            } else {
                codeUnitIndex=firstSupplementaryIndex;
                U16_FWD_N(dest, codeUnitIndex, destLength, i-codeUnitIndex);
            }

            
            if(codeUnitIndex<destLength) {
                uprv_memmove(dest+codeUnitIndex+cpLength,
                             dest+codeUnitIndex,
                             (destLength-codeUnitIndex)*U_SIZEOF_UCHAR);
                if(caseFlags!=NULL) {
                    uprv_memmove(caseFlags+codeUnitIndex+cpLength,
                                 caseFlags+codeUnitIndex,
                                 destLength-codeUnitIndex);
                }
            }
            if(cpLength==1) {
                
                dest[codeUnitIndex]=(UChar)n;
            } else {
                
                dest[codeUnitIndex]=U16_LEAD(n);
                dest[codeUnitIndex+1]=U16_TRAIL(n);
            }
            if(caseFlags!=NULL) {
                
                caseFlags[codeUnitIndex]=IS_BASIC_UPPERCASE(src[in-1]);
                if(cpLength==2) {
                    caseFlags[codeUnitIndex+1]=FALSE;
                }
            }
        }
        destLength+=cpLength;
        U_ASSERT(destLength>=0);
        ++i;
    }

    return u_terminateUChars(dest, destCapacity, destLength, pErrorCode);
}



#endif 
