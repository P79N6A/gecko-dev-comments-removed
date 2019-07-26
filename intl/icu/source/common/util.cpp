









#include "unicode/unimatch.h"
#include "unicode/utf16.h"
#include "patternprops.h"
#include "util.h"



static const UChar BACKSLASH  = 0x005C; 
static const UChar UPPER_U    = 0x0055; 
static const UChar LOWER_U    = 0x0075; 
static const UChar APOSTROPHE = 0x0027; 
static const UChar SPACE      = 0x0020; 


static const UChar DIGITS[] = {
    48,49,50,51,52,53,54,55,56,57,
    65,66,67,68,69,70,71,72,73,74,
    75,76,77,78,79,80,81,82,83,84,
    85,86,87,88,89,90
};

U_NAMESPACE_BEGIN

UnicodeString& ICU_Utility::appendNumber(UnicodeString& result, int32_t n,
                                     int32_t radix, int32_t minDigits) {
    if (radix < 2 || radix > 36) {
        
        return result.append((UChar)63);
    }
    
    if (n < 0) {
        n = -n;
        result.append((UChar)45);
    }
    
    int32_t nn = n;
    int32_t r = 1;
    while (nn >= radix) {
        nn /= radix;
        r *= radix;
        --minDigits;
    }
    
    while (--minDigits > 0) {
        result.append(DIGITS[0]);
    }
    while (r > 0) {
        int32_t digit = n / r;
        result.append(DIGITS[digit]);
        n -= digit * r;
        r /= radix;
    }
    return result;
}




UBool ICU_Utility::isUnprintable(UChar32 c) {
    return !(c >= 0x20 && c <= 0x7E);
}







UBool ICU_Utility::escapeUnprintable(UnicodeString& result, UChar32 c) {
    if (isUnprintable(c)) {
        result.append(BACKSLASH);
        if (c & ~0xFFFF) {
            result.append(UPPER_U);
            result.append(DIGITS[0xF&(c>>28)]);
            result.append(DIGITS[0xF&(c>>24)]);
            result.append(DIGITS[0xF&(c>>20)]);
            result.append(DIGITS[0xF&(c>>16)]);
        } else {
            result.append(LOWER_U);
        }
        result.append(DIGITS[0xF&(c>>12)]);
        result.append(DIGITS[0xF&(c>>8)]);
        result.append(DIGITS[0xF&(c>>4)]);
        result.append(DIGITS[0xF&c]);
        return TRUE;
    }
    return FALSE;
}


































int32_t ICU_Utility::skipWhitespace(const UnicodeString& str, int32_t& pos,
                                    UBool advance) {
    int32_t p = pos;
    const UChar* s = str.getBuffer();
    p = (int32_t)(PatternProps::skipWhiteSpace(s + p, str.length() - p) - s);
    if (advance) {
        pos = p;
    }
    return p;
}
























































UBool ICU_Utility::parseChar(const UnicodeString& id, int32_t& pos, UChar ch) {
    int32_t start = pos;
    skipWhitespace(id, pos, TRUE);
    if (pos == id.length() ||
        id.charAt(pos) != ch) {
        pos = start;
        return FALSE;
    }
    ++pos;
    return TRUE;
}

















int32_t ICU_Utility::parsePattern(const UnicodeString& pat,
                                  const Replaceable& text,
                                  int32_t index,
                                  int32_t limit) {
    int32_t ipat = 0;

    
    if (ipat == pat.length()) {
        return index;
    }

    UChar32 cpat = pat.char32At(ipat);

    while (index < limit) {
        UChar32 c = text.char32At(index);

        
        if (cpat == 126 ) {
            if (PatternProps::isWhiteSpace(c)) {
                index += U16_LENGTH(c);
                continue;
            } else {
                if (++ipat == pat.length()) {
                    return index; 
                }
                
            }
        }

        
        else if (c == cpat) {
            index += U16_LENGTH(c);
            ipat += U16_LENGTH(cpat);
            if (ipat == pat.length()) {
                return index; 
            }
            
        }

        
        else {
            return -1;
        }

        cpat = pat.char32At(ipat);
    }

    return -1; 
}



















void ICU_Utility::appendToRule(UnicodeString& rule,
                               UChar32 c,
                               UBool isLiteral,
                               UBool escapeUnprintable,
                               UnicodeString& quoteBuf) {
    
    
    
    if (isLiteral ||
        (escapeUnprintable && ICU_Utility::isUnprintable(c))) {
        if (quoteBuf.length() > 0) {
            
            
            
            

            
            
            while (quoteBuf.length() >= 2 &&
                   quoteBuf.charAt(0) == APOSTROPHE &&
                   quoteBuf.charAt(1) == APOSTROPHE) {
                rule.append(BACKSLASH).append(APOSTROPHE);
                quoteBuf.remove(0, 2);
            }
            
            
            int32_t trailingCount = 0;
            while (quoteBuf.length() >= 2 &&
                   quoteBuf.charAt(quoteBuf.length()-2) == APOSTROPHE &&
                   quoteBuf.charAt(quoteBuf.length()-1) == APOSTROPHE) {
                quoteBuf.truncate(quoteBuf.length()-2);
                ++trailingCount;
            }
            if (quoteBuf.length() > 0) {
                rule.append(APOSTROPHE);
                rule.append(quoteBuf);
                rule.append(APOSTROPHE);
                quoteBuf.truncate(0);
            }
            while (trailingCount-- > 0) {
                rule.append(BACKSLASH).append(APOSTROPHE);
            }
        }
        if (c != (UChar32)-1) {
            




            if (c == SPACE) {
                int32_t len = rule.length();
                if (len > 0 && rule.charAt(len-1) != c) {
                    rule.append(c);
                }
            } else if (!escapeUnprintable || !ICU_Utility::escapeUnprintable(rule, c)) {
                rule.append(c);
            }
        }
    }

    
    else if (quoteBuf.length() == 0 &&
             (c == APOSTROPHE || c == BACKSLASH)) {
        rule.append(BACKSLASH);
        rule.append(c);
    }

    
    
    
    else if (quoteBuf.length() > 0 ||
             (c >= 0x0021 && c <= 0x007E &&
              !((c >= 0x0030 && c <= 0x0039) ||
                (c >= 0x0041 && c <= 0x005A) ||
                (c >= 0x0061 && c <= 0x007A))) ||
             PatternProps::isWhiteSpace(c)) {
        quoteBuf.append(c);
        
        if (c == APOSTROPHE) {
            quoteBuf.append(c);
        }
    }
    
    
    else {
        rule.append(c);
    }
}

void ICU_Utility::appendToRule(UnicodeString& rule,
                               const UnicodeString& text,
                               UBool isLiteral,
                               UBool escapeUnprintable,
                               UnicodeString& quoteBuf) {
    for (int32_t i=0; i<text.length(); ++i) {
        appendToRule(rule, text[i], isLiteral, escapeUnprintable, quoteBuf);
    }
}





void ICU_Utility::appendToRule(UnicodeString& rule,
                               const UnicodeMatcher* matcher,
                               UBool escapeUnprintable,
                               UnicodeString& quoteBuf) {
    if (matcher != NULL) {
        UnicodeString pat;
        appendToRule(rule, matcher->toPattern(pat, escapeUnprintable),
                     TRUE, escapeUnprintable, quoteBuf);
    }
}

U_NAMESPACE_END
