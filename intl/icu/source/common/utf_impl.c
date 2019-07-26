



















#ifndef U_UTF8_IMPL
#   define U_UTF8_IMPL
#endif

#include "unicode/utypes.h"
#include "unicode/utf.h"
#include "unicode/utf8.h"
#include "unicode/utf_old.h"
#include "uassert.h"






















U_EXPORT const uint8_t 
utf8_countTrailBytes[256]={
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,

    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3,
    3, 3, 3,    
    4, 4, 4, 4, 
    5, 5,       
    0, 0        
};

static const UChar32
utf8_minLegal[4]={ 0, 0x80, 0x800, 0x10000 };

static const UChar32
utf8_errorValue[6]={
    UTF8_ERROR_VALUE_1, UTF8_ERROR_VALUE_2, UTF_ERROR_VALUE, 0x10ffff,
    0x3ffffff, 0x7fffffff
};























U_CAPI UChar32 U_EXPORT2
utf8_nextCharSafeBody(const uint8_t *s, int32_t *pi, int32_t length, UChar32 c, UBool strict) {
    int32_t i=*pi;
    uint8_t count=U8_COUNT_TRAIL_BYTES(c);
    U_ASSERT(count <= 5); 
    if((i)+count<=(length)) {
        uint8_t trail, illegal=0;

        U8_MASK_LEAD_BYTE((c), count);
        
        switch(count) {
        
        case 5:
        case 4:
            
            illegal=1;
            break;
        case 3:
            trail=s[(i)++];
            (c)=((c)<<6)|(trail&0x3f);
            if(c<0x110) {
                illegal|=(trail&0xc0)^0x80;
            } else {
                
                illegal=1;
                break;
            }
        case 2:
            trail=s[(i)++];
            (c)=((c)<<6)|(trail&0x3f);
            illegal|=(trail&0xc0)^0x80;
        case 1:
            trail=s[(i)++];
            (c)=((c)<<6)|(trail&0x3f);
            illegal|=(trail&0xc0)^0x80;
            break;
        case 0:
            if(strict>=0) {
                return UTF8_ERROR_VALUE_1;
            } else {
                return U_SENTINEL;
            }
        
        }

        










        
        
        if(illegal || (c)<utf8_minLegal[count] || (U_IS_SURROGATE(c) && strict!=-2)) {
            
            uint8_t errorCount=count;
            
            i=*pi;
            while(count>0 && U8_IS_TRAIL(s[i])) {
                ++(i);
                --count;
            }
            if(strict>=0) {
                c=utf8_errorValue[errorCount-count];
            } else {
                c=U_SENTINEL;
            }
        } else if((strict)>0 && U_IS_UNICODE_NONCHAR(c)) {
            
            c=utf8_errorValue[count];
        }
    } else  {
        
        int32_t i0=i;
        
        while((i)<(length) && U8_IS_TRAIL(s[i])) {
            ++(i);
        }
        if(strict>=0) {
            c=utf8_errorValue[i-i0];
        } else {
            c=U_SENTINEL;
        }
    }
    *pi=i;
    return c;
}

U_CAPI int32_t U_EXPORT2
utf8_appendCharSafeBody(uint8_t *s, int32_t i, int32_t length, UChar32 c, UBool *pIsError) {
    if((uint32_t)(c)<=0x7ff) {
        if((i)+1<(length)) {
            (s)[(i)++]=(uint8_t)(((c)>>6)|0xc0);
            (s)[(i)++]=(uint8_t)(((c)&0x3f)|0x80);
            return i;
        }
    } else if((uint32_t)(c)<=0xffff) {
        
        if((i)+2<(length) && !U_IS_SURROGATE(c)) {
            (s)[(i)++]=(uint8_t)(((c)>>12)|0xe0);
            (s)[(i)++]=(uint8_t)((((c)>>6)&0x3f)|0x80);
            (s)[(i)++]=(uint8_t)(((c)&0x3f)|0x80);
            return i;
        }
    } else if((uint32_t)(c)<=0x10ffff) {
        if((i)+3<(length)) {
            (s)[(i)++]=(uint8_t)(((c)>>18)|0xf0);
            (s)[(i)++]=(uint8_t)((((c)>>12)&0x3f)|0x80);
            (s)[(i)++]=(uint8_t)((((c)>>6)&0x3f)|0x80);
            (s)[(i)++]=(uint8_t)(((c)&0x3f)|0x80);
            return i;
        }
    }
    
    if(pIsError!=NULL) {
        *pIsError=TRUE;
    } else {
        length-=i;
        if(length>0) {
            int32_t offset;
            if(length>3) {
                length=3;
            }
            s+=i;
            offset=0;
            c=utf8_errorValue[length-1];
            UTF8_APPEND_CHAR_UNSAFE(s, offset, c);
            i=i+offset;
        }
    }
    return i;
}

U_CAPI UChar32 U_EXPORT2
utf8_prevCharSafeBody(const uint8_t *s, int32_t start, int32_t *pi, UChar32 c, UBool strict) {
    int32_t i=*pi;
    uint8_t b, count=1, shift=6;

    
    c&=0x3f;

    for(;;) {
        if(i<=start) {
            
            if(strict>=0) {
                return UTF8_ERROR_VALUE_1;
            } else {
                return U_SENTINEL;
            }
            
        }

        
        b=s[--i];
        if((uint8_t)(b-0x80)<0x7e) { 
            if(b&0x40) {
                
                uint8_t shouldCount=U8_COUNT_TRAIL_BYTES(b);

                if(count==shouldCount) {
                    
                    *pi=i;
                    U8_MASK_LEAD_BYTE(b, count);
                    c|=(UChar32)b<<shift;
                    if(count>=4 || c>0x10ffff || c<utf8_minLegal[count] || (U_IS_SURROGATE(c) && strict!=-2) || (strict>0 && U_IS_UNICODE_NONCHAR(c))) {
                        
                        if(count>=4) {
                            count=3;
                        }
                        if(strict>=0) {
                            c=utf8_errorValue[count];
                        } else {
                            c=U_SENTINEL;
                        }
                    } else {
                        
                    }
                } else {
                    
                    

                    if(count<shouldCount) {
                        *pi=i;
                        if(strict>=0) {
                            c=utf8_errorValue[count];
                        } else {
                            c=U_SENTINEL;
                        }
                    } else {
                        if(strict>=0) {
                            c=UTF8_ERROR_VALUE_1;
                        } else {
                            c=U_SENTINEL;
                        }
                    }
                }
                break;
            } else if(count<5) {
                
                c|=(UChar32)(b&0x3f)<<shift;
                ++count;
                shift+=6;
            } else {
                
                if(strict>=0) {
                    c=UTF8_ERROR_VALUE_1;
                } else {
                    c=U_SENTINEL;
                }
                break;
            }
        } else {
            
            if(strict>=0) {
                c=UTF8_ERROR_VALUE_1;
            } else {
                c=U_SENTINEL;
            }
            break;
        }
    }
    return c;
}

U_CAPI int32_t U_EXPORT2
utf8_back1SafeBody(const uint8_t *s, int32_t start, int32_t i) {
    
    int32_t I=i, Z;
    uint8_t b;

    
    if(I-5>start) {
        Z=I-5;
    } else {
        Z=start;
    }

    
    do {
        b=s[I];
        if((uint8_t)(b-0x80)>=0x7e) { 
            break;
        } else if(b>=0xc0) {
            if(U8_COUNT_TRAIL_BYTES(b)>=(i-I)) {
                return I;
            } else {
                break;
            }
        }
    } while(Z<=--I);

    
    return i;
}
