






#ifndef __TMUNIT_H__
#define __TMUNIT_H__








#include "unicode/measunit.h"

#if !UCONFIG_NO_FORMATTING

U_NAMESPACE_BEGIN







class U_I18N_API TimeUnit: public MeasureUnit {
public:
    



    enum UTimeUnitFields {
        UTIMEUNIT_YEAR,
        UTIMEUNIT_MONTH,
        UTIMEUNIT_DAY,
        UTIMEUNIT_WEEK,
        UTIMEUNIT_HOUR,
        UTIMEUNIT_MINUTE,
        UTIMEUNIT_SECOND,
        UTIMEUNIT_FIELD_COUNT
    };

    









    static TimeUnit* U_EXPORT2 createInstance(UTimeUnitFields timeUnitField,
                                              UErrorCode& status);


    



    virtual UObject* clone() const;

    



    TimeUnit(const TimeUnit& other);

    



    TimeUnit& operator=(const TimeUnit& other);

    




    virtual UBool operator==(const UObject& other) const;

    




    UBool operator!=(const UObject& other) const;

    







    virtual UClassID getDynamicClassID() const;

    





    static UClassID U_EXPORT2 getStaticClassID();


    




    UTimeUnitFields getTimeUnitField() const;

    



    virtual ~TimeUnit();

private:
    UTimeUnitFields fTimeUnitField;

    



    TimeUnit(UTimeUnitFields timeUnitField);

};


inline UBool 
TimeUnit::operator!=(const UObject& other) const {
    return !operator==(other);
}


U_NAMESPACE_END

#endif 

#endif 


