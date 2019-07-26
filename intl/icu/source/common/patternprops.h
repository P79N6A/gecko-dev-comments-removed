













#ifndef __PATTERNPROPS_H__
#define __PATTERNPROPS_H__

#include "unicode/utypes.h"

U_NAMESPACE_BEGIN




















class U_COMMON_API PatternProps {
public:
    


    static UBool isSyntax(UChar32 c);

    


    static UBool isSyntaxOrWhiteSpace(UChar32 c);

    


    static UBool isWhiteSpace(UChar32 c);

    



    static const UChar *skipWhiteSpace(const UChar *s, int32_t length);

    


    static const UChar *trimWhiteSpace(const UChar *s, int32_t &length);

    




    static UBool isIdentifier(const UChar *s, int32_t length);

    




    static const UChar *skipIdentifier(const UChar *s, int32_t length);

private:
    PatternProps();  
};

U_NAMESPACE_END

#endif  
