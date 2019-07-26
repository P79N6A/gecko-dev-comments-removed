









#ifndef _RULEITER_H_
#define _RULEITER_H_

#include "unicode/uobject.h"

U_NAMESPACE_BEGIN

class UnicodeString;
class ParsePosition;
class SymbolTable;








class RuleCharacterIterator : public UMemory {

    
    
    
    
    

private:
    

    
    const UnicodeString& text;

    


    ParsePosition& pos;

    


    const SymbolTable* sym;
    
    


    const UnicodeString* buf;

    


    int32_t bufPos;

public:
    


    enum { DONE = -1 };

    




    enum { PARSE_VARIABLES = 1 };

    




    enum { PARSE_ESCAPES   = 2 };

    




    enum { SKIP_WHITESPACE = 4 };

    










    RuleCharacterIterator(const UnicodeString& text, const SymbolTable* sym,
                          ParsePosition& pos);
    
    


    UBool atEnd() const;

    













    UChar32 next(int32_t options, UBool& isEscaped, UErrorCode& ec);

    


    inline UBool inVariable() const;

    


    struct Pos : public UMemory {
    private:
        const UnicodeString* buf;
        int32_t pos;
        int32_t bufPos;
        friend class RuleCharacterIterator;
    };

    
















    void getPos(Pos& p) const;

    




    void setPos(const Pos& p);

    







    void skipIgnored(int32_t options);

    













    UnicodeString& lookahead(UnicodeString& result, int32_t maxLookAhead = -1) const;

    




    void jumpahead(int32_t count);

    







    
private:
    




    UChar32 _current() const;
    
    



    void _advance(int32_t count);
};

inline UBool RuleCharacterIterator::inVariable() const {
    return buf != 0;
}

U_NAMESPACE_END

#endif

