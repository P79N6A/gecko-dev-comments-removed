






#ifndef UNIMATCH_H
#define UNIMATCH_H

#include "unicode/utypes.h"







U_NAMESPACE_BEGIN

class Replaceable;
class UnicodeString;
class UnicodeSet;






enum UMatchDegree {
    






    U_MISMATCH,
    
    









    U_PARTIAL_MATCH,
    
    







    U_MATCH
};






class U_COMMON_API UnicodeMatcher  {

public:
    



    virtual ~UnicodeMatcher();

    













































    virtual UMatchDegree matches(const Replaceable& text,
                                 int32_t& offset,
                                 int32_t limit,
                                 UBool incremental) = 0;

    











    virtual UnicodeString& toPattern(UnicodeString& result,
                                     UBool escapeUnprintable = FALSE) const = 0;

    






    virtual UBool matchesIndexValue(uint8_t v) const = 0;

    





    virtual void addMatchSetTo(UnicodeSet& toUnionTo) const = 0;
};

U_NAMESPACE_END

#endif
