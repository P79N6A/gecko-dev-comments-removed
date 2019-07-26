





#ifndef DTRULE_H
#define DTRULE_H

#include "unicode/utypes.h"






#if !UCONFIG_NO_FORMATTING

#include "unicode/uobject.h"

U_NAMESPACE_BEGIN







class U_I18N_API DateTimeRule : public UObject {
public:

    



    enum DateRuleType {
        DOM = 0,        

        DOW,            

        DOW_GEQ_DOM,    

        DOW_LEQ_DOM     

    };

    



    enum TimeRuleType {
        WALL_TIME = 0,  
        STANDARD_TIME,  
        UTC_TIME        
    };

    











    DateTimeRule(int32_t month, int32_t dayOfMonth,
        int32_t millisInDay, TimeRuleType timeType);

    














    DateTimeRule(int32_t month, int32_t weekInMonth, int32_t dayOfWeek,
        int32_t millisInDay, TimeRuleType timeType);

    














    DateTimeRule(int32_t month, int32_t dayOfMonth, int32_t dayOfWeek, UBool after,
        int32_t millisInDay, TimeRuleType timeType);

    




    DateTimeRule(const DateTimeRule& source);

    



    ~DateTimeRule();

    





    DateTimeRule* clone(void) const;

    




    DateTimeRule& operator=(const DateTimeRule& right);

    






    UBool operator==(const DateTimeRule& that) const;

    






    UBool operator!=(const DateTimeRule& that) const;

    




    DateRuleType getDateRuleType(void) const;

    





    TimeRuleType getTimeRuleType(void) const;

    




    int32_t getRuleMonth(void) const;

    





    int32_t getRuleDayOfMonth(void) const;

    





    int32_t getRuleDayOfWeek(void) const;

    






    int32_t getRuleWeekInMonth(void) const;

    




    int32_t getRuleMillisInDay(void) const;

private:
    int32_t fMonth;
    int32_t fDayOfMonth;
    int32_t fDayOfWeek;
    int32_t fWeekInMonth;
    int32_t fMillisInDay;
    DateRuleType fDateRuleType;
    TimeRuleType fTimeRuleType;

public:
    










    static UClassID U_EXPORT2 getStaticClassID(void);

    










    virtual UClassID getDynamicClassID(void) const;
};

U_NAMESPACE_END

#endif

#endif

