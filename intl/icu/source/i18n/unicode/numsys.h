














#ifndef NUMSYS
#define NUMSYS






#define NUMSYS_NAME_CAPACITY 8

#include "unicode/utypes.h"






#if !UCONFIG_NO_FORMATTING


#include "unicode/format.h"
#include "unicode/uobject.h"

U_NAMESPACE_BEGIN
















class U_I18N_API NumberingSystem : public UObject {
public:

    




    NumberingSystem();

    



    NumberingSystem(const NumberingSystem& other);

    



    virtual ~NumberingSystem();

    





    static NumberingSystem* U_EXPORT2 createInstance(const Locale & inLocale, UErrorCode& status);

    



    static NumberingSystem* U_EXPORT2 createInstance(UErrorCode& status);

    








    static NumberingSystem* U_EXPORT2 createInstance(int32_t radix, UBool isAlgorithmic, const UnicodeString& description, UErrorCode& status );

    




     static StringEnumeration * U_EXPORT2 getAvailableNames(UErrorCode& status);

    





    static NumberingSystem* U_EXPORT2 createInstanceByName(const char* name, UErrorCode& status);


    



    int32_t getRadix();

#ifndef U_HIDE_DRAFT_API
    




    const char * getName();
#endif  

    





    virtual UnicodeString getDescription();



    






    UBool isAlgorithmic() const;

    





    static UClassID U_EXPORT2 getStaticClassID(void);

    




    virtual UClassID getDynamicClassID() const;


private:
    UnicodeString   desc;
    int32_t         radix;
    UBool           algorithmic;
    char            name[NUMSYS_NAME_CAPACITY+1];

    void setRadix(int32_t radix);

    void setAlgorithmic(UBool algorithmic);

    void setDesc(UnicodeString desc);

    void setName(const char* name);

    static UBool isValidDigitString(const UnicodeString &str);

    UBool hasContiguousDecimalDigits() const;
};

U_NAMESPACE_END

#endif 

#endif 

