









#ifndef __MEASURE_H__
#define __MEASURE_H__

#include "unicode/utypes.h"





 
#if !UCONFIG_NO_FORMATTING

#include "unicode/fmtable.h"

U_NAMESPACE_BEGIN

class MeasureUnit;

















class U_I18N_API Measure: public UObject {
 public:
    









    Measure(const Formattable& number, MeasureUnit* adoptedUnit,
            UErrorCode& ec);

    



    Measure(const Measure& other);

    



    Measure& operator=(const Measure& other);

    




    virtual UObject* clone() const = 0;

    



    virtual ~Measure();
    
    




    UBool operator==(const UObject& other) const;

    





    inline const Formattable& getNumber() const;

    



    inline const MeasureUnit& getUnit() const;

 protected:
    



    Measure();

 private:
    


    Formattable number;

    



    MeasureUnit* unit;
};

inline const Formattable& Measure::getNumber() const {
    return number;
}

inline const MeasureUnit& Measure::getUnit() const {
    return *unit;
}

U_NAMESPACE_END

#endif 
#endif 
