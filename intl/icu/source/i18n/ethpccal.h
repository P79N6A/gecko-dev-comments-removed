






#ifndef ETHPCCAL_H
#define ETHPCCAL_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/calendar.h"
#include "cecal.h"

U_NAMESPACE_BEGIN





class EthiopicCalendar : public CECalendar {

public:
    



    enum EEraType {
        AMETE_MIHRET_ERA,
        AMETE_ALEM_ERA
    };

    



    enum EMonths {
        


        MESKEREM,

        


        TEKEMT,

        


        HEDAR,

        


        TAHSAS,

        


        TER,

        


        YEKATIT,

        


        MEGABIT,

        


        MIAZIA,

        


        GENBOT,

        


        SENE,

        


        HAMLE,

        


        NEHASSA,

        


        PAGUMEN
    };

    enum EEras {
        AMETE_ALEM,     
        AMETE_MIHRET    
    };

    










    EthiopicCalendar(const Locale& aLocale, UErrorCode& success, EEraType type = AMETE_MIHRET_ERA);

    



    EthiopicCalendar(const EthiopicCalendar& other);

    



    virtual ~EthiopicCalendar();

    




    virtual Calendar* clone() const;

    




    virtual const char * getType() const;

    




    void setAmeteAlemEra (UBool onOff);

    




    UBool isAmeteAlemEra() const;

protected:
    
    
    

    



    virtual int32_t handleGetExtendedYear();

    



    virtual void handleComputeFields(int32_t julianDay, UErrorCode &status);

    



    virtual int32_t handleGetLimit(UCalendarDateFields field, ELimitType limitType) const;

    




    virtual UDate defaultCenturyStart() const;

    



    virtual int32_t defaultCenturyStartYear() const;

    



    virtual int32_t getJDEpochOffset() const;

private:
    





    static UDate fgSystemDefaultCenturyStart;

    


    static int32_t fgSystemDefaultCenturyStartYear;

    


    static const int32_t fgSystemDefaultCenturyYear;

    


    static const UDate fgSystemDefaultCentury;
 
    




    static void initializeSystemDefaultCentury(void);

    








    EEraType eraType;

public:
    









    virtual UClassID getDynamicClassID(void) const;

    










    U_I18N_API static UClassID U_EXPORT2 getStaticClassID(void);  

#if 0



public:
    
    
    

    








    int32_t ethiopicToJD(int32_t year, int32_t month, int32_t day);
#endif
};

U_NAMESPACE_END
#endif
#endif

