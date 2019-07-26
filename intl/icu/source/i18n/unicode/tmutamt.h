




 

#ifndef __TMUTAMT_H__
#define __TMUTAMT_H__







#include "unicode/measure.h"
#include "unicode/tmunit.h"

#if !UCONFIG_NO_FORMATTING

U_NAMESPACE_BEGIN








class U_I18N_API TimeUnitAmount: public Measure {
public:
    











    TimeUnitAmount(const Formattable& number, 
                   TimeUnit::UTimeUnitFields timeUnitField,
                   UErrorCode& status);

    











    TimeUnitAmount(double amount, TimeUnit::UTimeUnitFields timeUnitField,
                   UErrorCode& status);


    



    TimeUnitAmount(const TimeUnitAmount& other);


    



    TimeUnitAmount& operator=(const TimeUnitAmount& other);


    




    virtual UObject* clone() const;

    
    



    virtual ~TimeUnitAmount();

    
    





    virtual UBool operator==(const UObject& other) const;


    





    UBool operator!=(const UObject& other) const;


    










    static UClassID U_EXPORT2 getStaticClassID(void);


    










    virtual UClassID getDynamicClassID(void) const;


    




    const TimeUnit& getTimeUnit() const;

    




    TimeUnit::UTimeUnitFields getTimeUnitField() const;
};



inline UBool 
TimeUnitAmount::operator!=(const UObject& other) const {
    return !operator==(other);
}

U_NAMESPACE_END

#endif 

#endif 


