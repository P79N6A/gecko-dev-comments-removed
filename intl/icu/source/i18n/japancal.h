














#ifndef JAPANCAL_H
#define JAPANCAL_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/calendar.h"
#include "unicode/gregocal.h"

U_NAMESPACE_BEGIN





























class JapaneseCalendar : public GregorianCalendar {
public:

    



    U_I18N_API static uint32_t U_EXPORT2 getCurrentEra(void); 

    








    JapaneseCalendar(const Locale& aLocale, UErrorCode& success);


    



    virtual ~JapaneseCalendar();

    




    JapaneseCalendar(const JapaneseCalendar& source);

    




    JapaneseCalendar& operator=(const JapaneseCalendar& right);

    




    virtual Calendar* clone(void) const;

    




    virtual int32_t handleGetExtendedYear();

    



    virtual int32_t getActualMaximum(UCalendarDateFields field, UErrorCode& status) const;


public:
    









    virtual UClassID getDynamicClassID(void) const;

    










    U_I18N_API static UClassID U_EXPORT2 getStaticClassID(void);

    





    virtual const char * getType() const;

    



    virtual UBool haveDefaultCentury() const;

    



    virtual UDate defaultCenturyStart() const;
    



    virtual int32_t defaultCenturyStartYear() const;

private:
    JapaneseCalendar(); 

protected:
    



    virtual int32_t internalGetEra() const;

    



    virtual void handleComputeFields(int32_t julianDay, UErrorCode& status);

    



    virtual int32_t handleGetLimit(UCalendarDateFields field, ELimitType limitType) const;

    






    virtual int32_t getDefaultMonthInYear(int32_t eyear);

    







    virtual int32_t getDefaultDayInMonth(int32_t eyear, int32_t month);
};

U_NAMESPACE_END

#endif 

#endif


