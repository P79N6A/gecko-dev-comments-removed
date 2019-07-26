









#ifndef __CURRENCYAMOUNT_H__
#define __CURRENCYAMOUNT_H__

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/measure.h"
#include "unicode/currunit.h"





 
U_NAMESPACE_BEGIN








class U_I18N_API CurrencyAmount: public Measure {
 public:
    









    CurrencyAmount(const Formattable& amount, const UChar* isoCode,
                   UErrorCode &ec);

    









    CurrencyAmount(double amount, const UChar* isoCode,
                   UErrorCode &ec);

    



    CurrencyAmount(const CurrencyAmount& other);
 
    



    CurrencyAmount& operator=(const CurrencyAmount& other);

    




    virtual UObject* clone() const;

    



    virtual ~CurrencyAmount();
    
    







    virtual UClassID getDynamicClassID() const;

    





    static UClassID U_EXPORT2 getStaticClassID();

    



    inline const CurrencyUnit& getCurrency() const;

    



    inline const UChar* getISOCurrency() const;
};

inline const CurrencyUnit& CurrencyAmount::getCurrency() const {
    return (const CurrencyUnit&) getUnit();
}

inline const UChar* CurrencyAmount::getISOCurrency() const {
    return getCurrency().getISOCurrency();
}

U_NAMESPACE_END

#endif 
#endif 
