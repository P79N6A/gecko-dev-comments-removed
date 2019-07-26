








#ifndef UNIREPL_H
#define UNIREPL_H

#include "unicode/utypes.h"






U_NAMESPACE_BEGIN

class Replaceable;
class UnicodeString;
class UnicodeSet;











class U_I18N_API UnicodeReplacer  {

 public:

    



    virtual ~UnicodeReplacer();

    

















    virtual int32_t replace(Replaceable& text,
                            int32_t start,
                            int32_t limit,
                            int32_t& cursor) = 0;

    













    virtual UnicodeString& toReplacerPattern(UnicodeString& result,
                                             UBool escapeUnprintable) const = 0;

    





    virtual void addReplacementSetTo(UnicodeSet& toUnionTo) const = 0;
};

U_NAMESPACE_END

#endif
