








#ifndef SYMTABLE_H
#define SYMTABLE_H

#include "unicode/utypes.h"
#include "unicode/uobject.h"






 
U_NAMESPACE_BEGIN

class ParsePosition;
class UnicodeFunctor;
class UnicodeSet;
class UnicodeString;


























class U_COMMON_API SymbolTable  {
public:

    



    enum { SYMBOL_REF = 0x0024  };

    



    virtual ~SymbolTable();

    








    virtual const UnicodeString* lookup(const UnicodeString& s) const = 0;

    







    virtual const UnicodeFunctor* lookupMatcher(UChar32 ch) const = 0;

    
















    virtual UnicodeString parseReference(const UnicodeString& text,
                                         ParsePosition& pos, int32_t limit) const = 0;
};
U_NAMESPACE_END

#endif
