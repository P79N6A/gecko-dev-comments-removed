













#include "unicode/utypes.h"
#include "patternprops.h"

U_NAMESPACE_BEGIN








static const uint8_t latin1[256]={
    
    0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 5, 5, 5, 5, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    
    5, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 3, 3, 3, 3,
    3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 3, 3, 0,
    
    3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 3, 3, 0,
    
    0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    
    0, 3, 3, 3, 3, 3, 3, 3, 0, 3, 0, 3, 3, 0, 3, 0,
    
    3, 3, 0, 0, 0, 0, 3, 0, 0, 0, 0, 3, 0, 0, 0, 3,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    
    0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    
    0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0
};






static const uint8_t index2000[130]={
    2, 3, 4, 0, 0, 0, 0, 0,  
    0, 0, 0, 0, 5, 1, 1, 1,  
    1, 1, 1, 1, 1, 1, 1, 1,  
    1, 1, 1, 1, 1, 1, 1, 1,  
    1, 1, 1, 0, 0, 0, 0, 0,  
    1, 1, 1, 1, 1, 1, 1, 1,  
    1, 1, 1, 1, 1, 1, 1, 1,  
    1, 1, 1, 6, 7, 1, 1, 1,  
    1, 1, 1, 1, 1, 1, 1, 1,  
    1, 1, 1, 1, 1, 1, 1, 1,  
    1, 1, 1, 1, 1, 1, 1, 1,  
    1, 1, 1, 1, 1, 1, 1, 1,  
    0, 0, 0, 0, 0, 0, 0, 0,  
    0, 0, 0, 0, 0, 0, 0, 0,  
    1, 1, 1, 1, 0, 0, 0, 0,  
    0, 0, 0, 0, 0, 0, 0, 0,  
    8, 9  
};





static const uint32_t syntax2000[]={
    0,
    0xffffffff,
    0xffff0000,  
    0x7fff00ff,  
    0x7feffffe,  
    0xffff0000,  
    0x003fffff,  
    0xfff00000,  
    0xffffff0e,  
    0x00010001   
};





static const uint32_t syntaxOrWhiteSpace2000[]={
    0,
    0xffffffff,
    0xffffc000,  
    0x7fff03ff,  
    0x7feffffe,  
    0xffff0000,  
    0x003fffff,  
    0xfff00000,  
    0xffffff0e,  
    0x00010001   
};

UBool
PatternProps::isSyntax(UChar32 c) {
    if(c<0) {
        return FALSE;
    } else if(c<=0xff) {
        return (UBool)(latin1[c]>>1)&1;
    } else if(c<0x2010) {
        return FALSE;
    } else if(c<=0x3030) {
        uint32_t bits=syntax2000[index2000[(c-0x2000)>>5]];
        return (UBool)((bits>>(c&0x1f))&1);
    } else if(0xfd3e<=c && c<=0xfe46) {
        return c<=0xfd3f || 0xfe45<=c;
    } else {
        return FALSE;
    }
}

UBool
PatternProps::isSyntaxOrWhiteSpace(UChar32 c) {
    if(c<0) {
        return FALSE;
    } else if(c<=0xff) {
        return (UBool)(latin1[c]&1);
    } else if(c<0x200e) {
        return FALSE;
    } else if(c<=0x3030) {
        uint32_t bits=syntaxOrWhiteSpace2000[index2000[(c-0x2000)>>5]];
        return (UBool)((bits>>(c&0x1f))&1);
    } else if(0xfd3e<=c && c<=0xfe46) {
        return c<=0xfd3f || 0xfe45<=c;
    } else {
        return FALSE;
    }
}

UBool
PatternProps::isWhiteSpace(UChar32 c) {
    if(c<0) {
        return FALSE;
    } else if(c<=0xff) {
        return (UBool)(latin1[c]>>2)&1;
    } else if(0x200e<=c && c<=0x2029) {
        return c<=0x200f || 0x2028<=c;
    } else {
        return FALSE;
    }
}

const UChar *
PatternProps::skipWhiteSpace(const UChar *s, int32_t length) {
    while(length>0 && isWhiteSpace(*s)) {
        ++s;
        --length;
    }
    return s;
}

const UChar *
PatternProps::trimWhiteSpace(const UChar *s, int32_t &length) {
    if(length<=0 || (!isWhiteSpace(s[0]) && !isWhiteSpace(s[length-1]))) {
        return s;
    }
    int32_t start=0;
    int32_t limit=length;
    while(start<limit && isWhiteSpace(s[start])) {
        ++start;
    }
    if(start<limit) {
        
        
        while(isWhiteSpace(s[limit-1])) {
            --limit;
        }
    }
    length=limit-start;
    return s+start;
}

UBool
PatternProps::isIdentifier(const UChar *s, int32_t length) {
    if(length<=0) {
        return FALSE;
    }
    const UChar *limit=s+length;
    do {
        if(isSyntaxOrWhiteSpace(*s++)) {
            return FALSE;
        }
    } while(s<limit);
    return TRUE;
}

const UChar *
PatternProps::skipIdentifier(const UChar *s, int32_t length) {
    while(length>0 && !isSyntaxOrWhiteSpace(*s)) {
        ++s;
        --length;
    }
    return s;
}

U_NAMESPACE_END
