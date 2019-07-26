





#ifndef TZTRANS_H
#define TZTRANS_H






#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/uobject.h"

U_NAMESPACE_BEGIN


class TimeZoneRule;






class U_I18N_API TimeZoneTransition : public UObject {
public:
    








    TimeZoneTransition(UDate time, const TimeZoneRule& from, const TimeZoneRule& to);

    



    TimeZoneTransition();

    




    TimeZoneTransition(const TimeZoneTransition& source);

    



    ~TimeZoneTransition();

    





    TimeZoneTransition* clone(void) const;

    




    TimeZoneTransition& operator=(const TimeZoneTransition& right);

    






    UBool operator==(const TimeZoneTransition& that) const;

    






    UBool operator!=(const TimeZoneTransition& that) const;

    




    UDate getTime(void) const;

    




    void setTime(UDate time);

    




    const TimeZoneRule* getFrom(void) const;

    





    void setFrom(const TimeZoneRule& from);

    





    void adoptFrom(TimeZoneRule* from);

    





    void setTo(const TimeZoneRule& to);

    





    void adoptTo(TimeZoneRule* to);

    




    const TimeZoneRule* getTo(void) const;

private:
    UDate   fTime;
    TimeZoneRule*   fFrom;
    TimeZoneRule*   fTo;

public:
    










    static UClassID U_EXPORT2 getStaticClassID(void);

    










    virtual UClassID getDynamicClassID(void) const;
};

U_NAMESPACE_END

#endif 

#endif 


