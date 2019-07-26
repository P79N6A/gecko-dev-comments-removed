









#ifndef __CURRENCYUNIT_H__
#define __CURRENCYUNIT_H__

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/measunit.h"





 
U_NAMESPACE_BEGIN









class U_I18N_API CurrencyUnit: public MeasureUnit {
 public:
    







    CurrencyUnit(const UChar* isoCode, UErrorCode &ec);

    



    CurrencyUnit(const CurrencyUnit& other);

    



    CurrencyUnit& operator=(const CurrencyUnit& other);

    




    virtual UObject* clone() const;

    



    virtual ~CurrencyUnit();

    




    UBool operator==(const UObject& other) const;

    







    virtual UClassID getDynamicClassID() const;

    





    static UClassID U_EXPORT2 getStaticClassID();

    



    inline const UChar* getISOCurrency() const;

 private:
    


    UChar isoCode[4];
};

inline const UChar* CurrencyUnit::getISOCurrency() const {
    return isoCode;
}

U_NAMESPACE_END

#endif 
#endif 
