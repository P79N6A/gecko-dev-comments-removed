


















#include "base/third_party/icu/icu_utf.h"

namespace base_icu {















#define CBUTF8_ERROR_VALUE_1 0x15






#define CBUTF8_ERROR_VALUE_2 0x9f








#define CBUTF_ERROR_VALUE 0xffff






















const uint8
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
    CBUTF8_ERROR_VALUE_1, CBUTF8_ERROR_VALUE_2, CBUTF_ERROR_VALUE, 0x10ffff,
    0x3ffffff, 0x7fffffff
};























UChar32
utf8_nextCharSafeBody(const uint8 *s, int32 *pi, int32 length, UChar32 c, UBool strict) {
    int32 i=*pi;
    uint8 count=CBU8_COUNT_TRAIL_BYTES(c);
    if((i)+count<=(length)) {
        uint8 trail, illegal=0;

        CBU8_MASK_LEAD_BYTE((c), count);
        
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
                return CBUTF8_ERROR_VALUE_1;
            } else {
                return CBU_SENTINEL;
            }
        
        }

        










        
        
        if(illegal || (c)<utf8_minLegal[count] || (CBU_IS_SURROGATE(c) && strict!=-2)) {
            
            uint8 errorCount=count;
            
            i=*pi;
            while(count>0 && CBU8_IS_TRAIL(s[i])) {
                ++(i);
                --count;
            }
            if(strict>=0) {
                c=utf8_errorValue[errorCount-count];
            } else {
                c=CBU_SENTINEL;
            }
        } else if((strict)>0 && CBU_IS_UNICODE_NONCHAR(c)) {
            
            c=utf8_errorValue[count];
        }
    } else  {
        
        int32 i0=i;
        
        while((i)<(length) && CBU8_IS_TRAIL(s[i])) {
            ++(i);
        }
        if(strict>=0) {
            c=utf8_errorValue[i-i0];
        } else {
            c=CBU_SENTINEL;
        }
    }
    *pi=i;
    return c;
}

}  
