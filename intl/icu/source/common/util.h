









#ifndef ICU_UTIL_H
#define ICU_UTIL_H

#include "unicode/utypes.h"
#include "unicode/uobject.h"
#include "unicode/unistr.h"






U_NAMESPACE_BEGIN

class UnicodeMatcher;

class U_COMMON_API ICU_Utility  {
 public:

    












    static UnicodeString& appendNumber(UnicodeString& result, int32_t n,
                                       int32_t radix = 10,
                                       int32_t minDigits = 1);

    





    static UBool isUnprintable(UChar32 c);

    





    static UBool escapeUnprintable(UnicodeString& result, UChar32 c);

    
















    







    static int32_t skipWhitespace(const UnicodeString& str, int32_t& pos,
                                  UBool advance = FALSE);

    




















    











    static UBool parseChar(const UnicodeString& id, int32_t& pos, UChar ch);

    


















    static int32_t parsePattern(const UnicodeString& rule, int32_t pos, int32_t limit,
                                const UnicodeString& pattern, int32_t* parsedInts);
        
    















    static int32_t parsePattern(const UnicodeString& pat,
                                const Replaceable& text,
                                int32_t index,
                                int32_t limit);

    







    static int32_t parseInteger(const UnicodeString& rule, int32_t& pos, int32_t limit);

    













    static UnicodeString parseUnicodeIdentifier(const UnicodeString& str, int32_t& pos);

    















    static int32_t parseNumber(const UnicodeString& text,
                               int32_t& pos, int8_t radix);

    static void appendToRule(UnicodeString& rule,
                             UChar32 c,
                             UBool isLiteral,
                             UBool escapeUnprintable,
                             UnicodeString& quoteBuf);
    
    static void appendToRule(UnicodeString& rule,
                             const UnicodeString& text,
                             UBool isLiteral,
                             UBool escapeUnprintable,
                             UnicodeString& quoteBuf);

    static void appendToRule(UnicodeString& rule,
                             const UnicodeMatcher* matcher,
                             UBool escapeUnprintable,
                             UnicodeString& quoteBuf);

private:
    
    ICU_Utility();
};

U_NAMESPACE_END

#endif

