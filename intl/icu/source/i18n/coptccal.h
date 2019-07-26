






#ifndef COPTCCAL_H
#define COPTCCAL_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/calendar.h"
#include "cecal.h"

U_NAMESPACE_BEGIN





class CopticCalendar : public CECalendar {
  
public:
    



    enum EMonths {
        



        TOUT,
        
        



        BABA,

        



        HATOR,

        



        KIAHK,

        



        TOBA,

        



        AMSHIR,

        



        BARAMHAT,

        



        BARAMOUDA,

        



        BASHANS,

        



        PAONA,

        



        EPEP,

        



        MESRA,

        





        NASIE
    };

    enum EEras {
        BCE,    
        CE      
    };

    








    CopticCalendar(const Locale& aLocale, UErrorCode& success);

    



    CopticCalendar (const CopticCalendar& other);

    



    virtual ~CopticCalendar();

    




    virtual Calendar* clone(void) const;

    




    const char * getType() const;

protected:
    
    
    

    



    virtual int32_t handleGetExtendedYear();

    



    virtual void handleComputeFields(int32_t julianDay, UErrorCode &status);

    




    virtual UDate defaultCenturyStart() const;

    



    virtual int32_t defaultCenturyStartYear() const;

    



    virtual int32_t getJDEpochOffset() const;

private:
    





    static UDate fgSystemDefaultCenturyStart;

    


    static int32_t fgSystemDefaultCenturyStartYear;

    


    static const int32_t fgSystemDefaultCenturyYear;

    


    static const UDate fgSystemDefaultCentury;

    




    static void initializeSystemDefaultCentury(void);

public:
    









    virtual UClassID getDynamicClassID(void) const;

    










    U_I18N_API static UClassID U_EXPORT2 getStaticClassID(void);  

#if 0
    
    
public:
    
    
    
    








    static int32_t copticToJD(int32_t year, int32_t month, int32_t day);
#endif
};

U_NAMESPACE_END

#endif
#endif

