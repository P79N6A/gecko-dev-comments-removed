









#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/uniset.h"
#include "unicode/utf16.h"
#include "strrepl.h"
#include "rbt_data.h"
#include "util.h"

U_NAMESPACE_BEGIN

UnicodeReplacer::~UnicodeReplacer() {}
UOBJECT_DEFINE_RTTI_IMPLEMENTATION(StringReplacer)












StringReplacer::StringReplacer(const UnicodeString& theOutput,
                               int32_t theCursorPos,
                               const TransliterationRuleData* theData) {
    output = theOutput;
    cursorPos = theCursorPos;
    hasCursor = TRUE;
    data = theData;
    isComplex = TRUE;
}










StringReplacer::StringReplacer(const UnicodeString& theOutput,
                               const TransliterationRuleData* theData) {
    output = theOutput;
    cursorPos = 0;
    hasCursor = FALSE;
    data = theData;
    isComplex = TRUE;
}




StringReplacer::StringReplacer(const StringReplacer& other) :
    UnicodeFunctor(other),
    UnicodeReplacer(other)
{
    output = other.output;
    cursorPos = other.cursorPos;
    hasCursor = other.hasCursor;
    data = other.data;
    isComplex = other.isComplex;
}




StringReplacer::~StringReplacer() {
}




UnicodeFunctor* StringReplacer::clone() const {
    return new StringReplacer(*this);
}




UnicodeReplacer* StringReplacer::toReplacer() const {
  return const_cast<StringReplacer *>(this);
}




int32_t StringReplacer::replace(Replaceable& text,
                                int32_t start,
                                int32_t limit,
                                int32_t& cursor) {
    int32_t outLen;
    int32_t newStart = 0;

    
    
    

    
    if (!isComplex) {
        text.handleReplaceBetween(start, limit, output);
        outLen = output.length();

        
        newStart = cursorPos;
    }

    
    else {
        





        UnicodeString buf;
        int32_t oOutput; 
        isComplex = FALSE;

        
        
        
        
        
        
        
        
        
        int32_t tempStart = text.length(); 
        int32_t destStart = tempStart; 
        if (start > 0) {
            int32_t len = U16_LENGTH(text.char32At(start-1));
            text.copy(start-len, start, tempStart);
            destStart += len;
        } else {
            UnicodeString str((UChar) 0xFFFF);
            text.handleReplaceBetween(tempStart, tempStart, str);
            destStart++;
        }
        int32_t destLimit = destStart;

        for (oOutput=0; oOutput<output.length(); ) {
            if (oOutput == cursorPos) {
                
                newStart = destLimit - destStart; 
            }
            UChar32 c = output.char32At(oOutput);
            UnicodeReplacer* r = data->lookupReplacer(c);
            if (r == NULL) {
                
                buf.append(c);
            } else {
                isComplex = TRUE;

                
                if (buf.length() > 0) {
                    text.handleReplaceBetween(destLimit, destLimit, buf);
                    destLimit += buf.length();
                    buf.truncate(0);
                }

                
                int32_t len = r->replace(text, destLimit, destLimit, cursor);
                destLimit += len;
            }
            oOutput += U16_LENGTH(c);
        }
        
        if (buf.length() > 0) {
            text.handleReplaceBetween(destLimit, destLimit, buf);
            destLimit += buf.length();
        }
        if (oOutput == cursorPos) {
            
            newStart = destLimit - destStart; 
        }

        outLen = destLimit - destStart;

        
        text.copy(destStart, destLimit, start);
        text.handleReplaceBetween(tempStart + outLen, destLimit + outLen, UnicodeString());

        
        text.handleReplaceBetween(start + outLen, limit + outLen, UnicodeString());
    }        

    if (hasCursor) {
        
        
        
        
        if (cursorPos < 0) {
            newStart = start;
            int32_t n = cursorPos;
            
            while (n < 0 && newStart > 0) {
                newStart -= U16_LENGTH(text.char32At(newStart-1));
                ++n;
            }
            newStart += n;
        } else if (cursorPos > output.length()) {
            newStart = start + outLen;
            int32_t n = cursorPos - output.length();
            
            while (n > 0 && newStart < text.length()) {
                newStart += U16_LENGTH(text.char32At(newStart));
                --n;
            }
            newStart += n;
        } else {
            
            
            newStart += start;
        }

        cursor = newStart;
    }

    return outLen;
}




UnicodeString& StringReplacer::toReplacerPattern(UnicodeString& rule,
                                                 UBool escapeUnprintable) const {
    rule.truncate(0);
    UnicodeString quoteBuf;

    int32_t cursor = cursorPos;

    
    if (hasCursor && cursor < 0) {
        while (cursor++ < 0) {
            ICU_Utility::appendToRule(rule, (UChar)0x0040 , TRUE, escapeUnprintable, quoteBuf);
        }
        
    }

    for (int32_t i=0; i<output.length(); ++i) {
        if (hasCursor && i == cursor) {
            ICU_Utility::appendToRule(rule, (UChar)0x007C , TRUE, escapeUnprintable, quoteBuf);
        }
        UChar c = output.charAt(i); 

        UnicodeReplacer* r = data->lookupReplacer(c);
        if (r == NULL) {
            ICU_Utility::appendToRule(rule, c, FALSE, escapeUnprintable, quoteBuf);
        } else {
            UnicodeString buf;
            r->toReplacerPattern(buf, escapeUnprintable);
            buf.insert(0, (UChar)0x20);
            buf.append((UChar)0x20);
            ICU_Utility::appendToRule(rule, buf,
                                      TRUE, escapeUnprintable, quoteBuf);
        }
    }

    
    
    
    if (hasCursor && cursor > output.length()) {
        cursor -= output.length();
        while (cursor-- > 0) {
            ICU_Utility::appendToRule(rule, (UChar)0x0040 , TRUE, escapeUnprintable, quoteBuf);
        }
        ICU_Utility::appendToRule(rule, (UChar)0x007C , TRUE, escapeUnprintable, quoteBuf);
    }
    
    ICU_Utility::appendToRule(rule, -1,
                              TRUE, escapeUnprintable, quoteBuf);

    return rule;
}




void StringReplacer::addReplacementSetTo(UnicodeSet& toUnionTo) const {
    UChar32 ch;
    for (int32_t i=0; i<output.length(); i+=U16_LENGTH(ch)) {
    ch = output.char32At(i);
    UnicodeReplacer* r = data->lookupReplacer(ch);
    if (r == NULL) {
        toUnionTo.add(ch);
    } else {
        r->addReplacementSetTo(toUnionTo);
    }
    }
}




void StringReplacer::setData(const TransliterationRuleData* d) {
    data = d;
    int32_t i = 0;
    while (i<output.length()) {
        UChar32 c = output.char32At(i);
        UnicodeFunctor* f = data->lookup(c);
        if (f != NULL) {
            f->setData(data);
        }
        i += U16_LENGTH(c);
    }
}

U_NAMESPACE_END

#endif 


