














#ifndef BUDDHCAL_H
#define BUDDHCAL_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/calendar.h"
#include "unicode/gregocal.h"

U_NAMESPACE_BEGIN




















class BuddhistCalendar : public GregorianCalendar {
public:

    



    enum EEras {
       BE
    };

    








    BuddhistCalendar(const Locale& aLocale, UErrorCode& success);


    



    virtual ~BuddhistCalendar();

    




    BuddhistCalendar(const BuddhistCalendar& source);

    




    BuddhistCalendar& operator=(const BuddhistCalendar& right);

    




    virtual Calendar* clone(void) const;

public:
    









    virtual UClassID getDynamicClassID(void) const;

    










    U_I18N_API static UClassID U_EXPORT2 getStaticClassID(void);

    





    virtual const char * getType() const;

private:
    BuddhistCalendar(); 

 protected:
    







    virtual int32_t handleGetExtendedYear();
    




    virtual void handleComputeFields(int32_t julianDay, UErrorCode& status);
    






    virtual int32_t handleGetLimit(UCalendarDateFields field, ELimitType limitType) const;
        












    virtual int32_t handleComputeMonthStart(int32_t eyear, int32_t month,
                                            UBool useMonth) const;

    



    virtual UBool haveDefaultCentury() const;

    




    virtual UDate defaultCenturyStart() const;

    



    virtual int32_t defaultCenturyStartYear() const;

 private: 
    





    static UDate         fgSystemDefaultCenturyStart;

    


    static int32_t          fgSystemDefaultCenturyStartYear;

    


    static const int32_t    fgSystemDefaultCenturyYear;

    


    static const UDate        fgSystemDefaultCentury;

    



    UDate         internalGetDefaultCenturyStart(void) const;

    



    int32_t          internalGetDefaultCenturyStartYear(void) const;

    




    static void  initializeSystemDefaultCentury(void);
};

U_NAMESPACE_END

#endif

#endif


