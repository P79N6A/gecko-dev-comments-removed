









#ifndef _USTRENUM_H_
#define _USTRENUM_H_

#include "unicode/uenum.h"
#include "unicode/strenum.h"


U_NAMESPACE_BEGIN





class U_COMMON_API UStringEnumeration : public StringEnumeration {

public:
    







    UStringEnumeration(UEnumeration* uenum);

    



    virtual ~UStringEnumeration();

    




    virtual int32_t count(UErrorCode& status) const;

    virtual const char* next(int32_t *resultLength, UErrorCode& status);

    





    virtual const UnicodeString* snext(UErrorCode& status);

    



    virtual void reset(UErrorCode& status);

    


    virtual UClassID getDynamicClassID() const;

    


    static UClassID U_EXPORT2 getStaticClassID();

private:
    UEnumeration *uenum; 
};

U_NAMESPACE_END

#endif

