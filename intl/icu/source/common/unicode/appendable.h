













#ifndef __APPENDABLE_H__
#define __APPENDABLE_H__






#include "unicode/utypes.h"
#include "unicode/uobject.h"

U_NAMESPACE_BEGIN

class UnicodeString;




















class U_COMMON_API Appendable : public UObject {
public:
    



    ~Appendable();

    





    virtual UBool appendCodeUnit(UChar c) = 0;

    






    virtual UBool appendCodePoint(UChar32 c);

    







    virtual UBool appendString(const UChar *s, int32_t length);

    








    virtual UBool reserveAppendCapacity(int32_t appendCapacity);

    











































    virtual UChar *getAppendBuffer(int32_t minCapacity,
                                   int32_t desiredCapacityHint,
                                   UChar *scratch, int32_t scratchCapacity,
                                   int32_t *resultCapacity);

private:
    
    virtual UClassID getDynamicClassID() const;
};







class U_COMMON_API UnicodeStringAppendable : public Appendable {
public:
    




    explicit UnicodeStringAppendable(UnicodeString &s) : str(s) {}

    



    ~UnicodeStringAppendable();

    





    virtual UBool appendCodeUnit(UChar c);

    





    virtual UBool appendCodePoint(UChar32 c);

    






    virtual UBool appendString(const UChar *s, int32_t length);

    






    virtual UBool reserveAppendCapacity(int32_t appendCapacity);

    




















    virtual UChar *getAppendBuffer(int32_t minCapacity,
                                   int32_t desiredCapacityHint,
                                   UChar *scratch, int32_t scratchCapacity,
                                   int32_t *resultCapacity);

private:
    UnicodeString &str;
};

U_NAMESPACE_END

#endif  
