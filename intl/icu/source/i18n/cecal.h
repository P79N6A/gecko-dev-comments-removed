






#ifndef CECAL_H
#define CECAL_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/calendar.h"

U_NAMESPACE_BEGIN





class U_I18N_API CECalendar : public Calendar {

protected:
    
    
    

    








    CECalendar(const Locale& aLocale, UErrorCode& success);

    



    CECalendar (const CECalendar& other);

    



    virtual ~CECalendar();

    




    CECalendar& operator=(const CECalendar& right);

protected:
    
    
    

    



    virtual int32_t handleComputeMonthStart(int32_t eyear, int32_t month, UBool useMonth) const;

    



    virtual int32_t handleGetLimit(UCalendarDateFields field, ELimitType limitType) const;

    








    virtual UBool inDaylightTime(UErrorCode&) const;

    



    virtual UBool haveDefaultCentury() const;

protected:
    





    virtual int32_t getJDEpochOffset() const = 0;

    









    static int32_t ceToJD(int32_t year, int32_t month, int32_t date,
        int32_t jdEpochOffset);

    









    static void jdToCE(int32_t julianDay, int32_t jdEpochOffset,
        int32_t& year, int32_t& month, int32_t& day);
};

U_NAMESPACE_END

#endif 
#endif

