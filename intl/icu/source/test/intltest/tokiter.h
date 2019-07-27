









#ifndef __ICU_INTLTEST_TOKITER__
#define __ICU_INTLTEST_TOKITER__

#include "intltest.h"

class TextFile;








class TokenIterator {
 public:

    





    TokenIterator(TextFile* r);

    virtual ~TokenIterator();

    




    UBool next(UnicodeString& token, UErrorCode& ec);

    




    int32_t getLineNumber() const;
    
    



    
    
    
    
 private:
    UBool nextToken(UnicodeString& token, UErrorCode& ec);

    TextFile* reader; 
    UnicodeString line;
    UBool done;
    UBool haveLine;
    int32_t pos;
    int32_t lastpos;
};

#endif
