





#ifndef USETITER_H
#define USETITER_H

#include "unicode/utypes.h"
#include "unicode/uobject.h"
#include "unicode/unistr.h"






U_NAMESPACE_BEGIN

class UnicodeSet;
class UnicodeString;






































class U_COMMON_API UnicodeSetIterator : public UObject {

 protected:

    





    enum { IS_STRING = -1 };

    




    UChar32 codepoint;

    








    UChar32 codepointEnd;

    





    const UnicodeString* string;

 public:

    





    UnicodeSetIterator(const UnicodeSet& set);

    





    UnicodeSetIterator();

    



    virtual ~UnicodeSetIterator();

    














    inline UBool isString() const;

    




    inline UChar32 getCodepoint() const;

    





    inline UChar32 getCodepointEnd() const;

    










    const UnicodeString& getString();

    





















    UBool next();

    


















    UBool nextRange();

    






    void reset(const UnicodeSet& set);

    



    void reset();

    




    static UClassID U_EXPORT2 getStaticClassID();

    




    virtual UClassID getDynamicClassID() const;

    

 protected:

    
    
    
    


    const UnicodeSet* set;
    


    int32_t endRange;
    


    int32_t range;
    


    int32_t endElement;
    


    int32_t nextElement;
    
    


    int32_t nextString;
    


    int32_t stringCount;

    




    UnicodeString *cpString;

    


    UnicodeSetIterator(const UnicodeSetIterator&); 

    


    UnicodeSetIterator& operator=(const UnicodeSetIterator&); 

    


    virtual void loadRange(int32_t range);

};

inline UBool UnicodeSetIterator::isString() const {
    return codepoint == (UChar32)IS_STRING;
}

inline UChar32 UnicodeSetIterator::getCodepoint() const {
    return codepoint;
}

inline UChar32 UnicodeSetIterator::getCodepointEnd() const {
    return codepointEnd;
}


U_NAMESPACE_END

#endif
