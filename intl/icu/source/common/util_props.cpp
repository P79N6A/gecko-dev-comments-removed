









#include "unicode/uchar.h"
#include "unicode/utf16.h"
#include "patternprops.h"
#include "util.h"

U_NAMESPACE_BEGIN









int32_t ICU_Utility::parseInteger(const UnicodeString& rule, int32_t& pos, int32_t limit) {
    int32_t count = 0;
    int32_t value = 0;
    int32_t p = pos;
    int8_t radix = 10;

    if (p < limit && rule.charAt(p) == 48 ) {
        if (p+1 < limit && (rule.charAt(p+1) == 0x78  || rule.charAt(p+1) == 0x58 )) {
            p += 2;
            radix = 16;
        }
        else {
            p++;
            count = 1;
            radix = 8;
        }
    }

    while (p < limit) {
        int32_t d = u_digit(rule.charAt(p++), radix);
        if (d < 0) {
            --p;
            break;
        }
        ++count;
        int32_t v = (value * radix) + d;
        if (v <= value) {
            
            
            
            
            return 0;
        }
        value = v;
    }
    if (count > 0) {
        pos = p;
    }
    return value;
}




















int32_t ICU_Utility::parsePattern(const UnicodeString& rule, int32_t pos, int32_t limit,
                              const UnicodeString& pattern, int32_t* parsedInts) {
    
    int32_t p;
    int32_t intCount = 0; 
    for (int32_t i=0; i<pattern.length(); ++i) {
        UChar cpat = pattern.charAt(i);
        UChar c;
        switch (cpat) {
        case 32 :
            if (pos >= limit) {
                return -1;
            }
            c = rule.charAt(pos++);
            if (!PatternProps::isWhiteSpace(c)) {
                return -1;
            }
            
        case 126 :
            pos = skipWhitespace(rule, pos);
            break;
        case 35 :
            p = pos;
            parsedInts[intCount++] = parseInteger(rule, p, limit);
            if (p == pos) {
                
                return -1;
            }
            pos = p;
            break;
        default:
            if (pos >= limit) {
                return -1;
            }
            c = (UChar) u_tolower(rule.charAt(pos++));
            if (c != cpat) {
                return -1;
            }
            break;
        }
    }
    return pos;
}














UnicodeString ICU_Utility::parseUnicodeIdentifier(const UnicodeString& str, int32_t& pos) {
    
    UnicodeString buf;
    int p = pos;
    while (p < str.length()) {
        UChar32 ch = str.char32At(p);
        if (buf.length() == 0) {
            if (u_isIDStart(ch)) {
                buf.append(ch);
            } else {
                buf.truncate(0);
                return buf;
            }
        } else {
            if (u_isIDPart(ch)) {
                buf.append(ch);
            } else {
                break;
            }
        }
        p += U16_LENGTH(ch);
    }
    pos = p;
    return buf;
}

















int32_t ICU_Utility::parseNumber(const UnicodeString& text,
                                 int32_t& pos, int8_t radix) {
    
    
    
    int32_t n = 0;
    int32_t p = pos;
    while (p < text.length()) {
        UChar32 ch = text.char32At(p);
        int32_t d = u_digit(ch, radix);
        if (d < 0) {
            break;
        }
        n = radix*n + d;
        
        
        if (n < 0) {
            return -1;
        }
        ++p;
    }
    if (p == pos) {
        return -1;
    }
    pos = p;
    return n;
}

U_NAMESPACE_END

