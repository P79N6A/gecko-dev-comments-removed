









































#include "pcre_internal.h"


















static inline void getUTF8CharAndAdvancePointer(int& c, const unsigned char*& subjectPtr)
{
    c = *subjectPtr++;
    if ((c & 0xc0) == 0xc0) {
        int gcaa = jsc_pcre_utf8_table4[c & 0x3f];  
        int gcss = 6 * gcaa;
        c = (c & jsc_pcre_utf8_table3[gcaa]) << gcss;
        while (gcaa-- > 0) {
            gcss -= 6;
            c |= (*subjectPtr++ & 0x3f) << gcss;
        }
    }
}

bool jsc_pcre_xclass(int c, const unsigned char* data)
{
    bool negated = (*data & XCL_NOT);
    
    


    
    if (c < 256) {
        if ((*data & XCL_MAP) != 0 && (data[1 + c/8] & (1 << (c&7))) != 0)
            return !negated;   
    }
    
    


    
    if ((*data++ & XCL_MAP) != 0)
        data += 32;
    
    int t;
    while ((t = *data++) != XCL_END) {
        if (t == XCL_SINGLE) {
            int x;
            getUTF8CharAndAdvancePointer(x, data);
            if (c == x)
                return !negated;
        }
        else if (t == XCL_RANGE) {
            int x, y;
            getUTF8CharAndAdvancePointer(x, data);
            getUTF8CharAndAdvancePointer(y, data);
            if (c >= x && c <= y)
                return !negated;
        }
    }
    
    return negated;   
}
