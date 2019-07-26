










#ifndef __DTITVINF_H__
#define __DTITVINF_H__

#include "unicode/utypes.h"






#if !UCONFIG_NO_FORMATTING

#include "unicode/udat.h"
#include "unicode/locid.h"
#include "unicode/ucal.h"
#include "unicode/dtptngen.h"

U_NAMESPACE_BEGIN

























































































































class U_I18N_API DateIntervalInfo : public UObject {
public:
#ifndef U_HIDE_INTERNAL_API
    












    DateIntervalInfo(UErrorCode& status);
#endif  


    






    DateIntervalInfo(const Locale& locale, UErrorCode& status);


    



    DateIntervalInfo(const DateIntervalInfo&);

    



    DateIntervalInfo& operator=(const DateIntervalInfo&);

    





    virtual DateIntervalInfo* clone(void) const;

    




    virtual ~DateIntervalInfo();


    






    virtual UBool operator==(const DateIntervalInfo& other) const;

    






    UBool operator!=(const DateIntervalInfo& other) const;



    































    void setIntervalPattern(const UnicodeString& skeleton, 
                            UCalendarDateFields lrgDiffCalUnit, 
                            const UnicodeString& intervalPattern,
                            UErrorCode& status);

    









    UnicodeString& getIntervalPattern(const UnicodeString& skeleton,
                                      UCalendarDateFields field,
                                      UnicodeString& result,
                                      UErrorCode& status) const; 

    





    UnicodeString& getFallbackIntervalPattern(UnicodeString& result) const;


    












    void setFallbackIntervalPattern(const UnicodeString& fallbackPattern,
                                    UErrorCode& status);


    





    UBool getDefaultOrder() const;


    




    virtual UClassID getDynamicClassID() const;

    




    static UClassID U_EXPORT2 getStaticClassID();


private:
    







    friend class DateIntervalFormat;

    




    enum IntervalPatternIndex
    {
        kIPI_ERA,
        kIPI_YEAR,
        kIPI_MONTH,
        kIPI_DATE,
        kIPI_AM_PM,
        kIPI_HOUR,
        kIPI_MINUTE,
        kIPI_MAX_INDEX
    };
public:
#ifndef U_HIDE_INTERNAL_API
    



     enum {
         kMaxIntervalPatternIndex = kIPI_MAX_INDEX
     };
#endif  
private:


    





    void initializeData(const Locale& locale, UErrorCode& status);


    










    void setIntervalPatternInternally(const UnicodeString& skeleton,
                                      UCalendarDateFields lrgDiffCalUnit,
                                      const UnicodeString& intervalPattern,
                                      UErrorCode& status); 


    


















    const UnicodeString* getBestSkeleton(const UnicodeString& skeleton,
                                         int8_t& bestMatchDistanceInfo) const;


    







    static void U_EXPORT2 parseSkeleton(const UnicodeString& skeleton, 
                                        int32_t* skeletonFieldWidth);


    











    static UBool U_EXPORT2 stringNumeric(int32_t fieldWidth,
                                         int32_t anotherFieldWidth,
                                         char patternLetter);


    













    static IntervalPatternIndex U_EXPORT2 calendarFieldToIntervalIndex(
                                                      UCalendarDateFields field,
                                                      UErrorCode& status);


    





    void deleteHash(Hashtable* hTable);


    






    Hashtable* initHash(UErrorCode& status);



    







    void copyHash(const Hashtable* source, Hashtable* target, UErrorCode& status);


    
    
    UnicodeString fFallbackIntervalPattern;
    
    UBool fFirstDateInPtnIsLaterDate;

    
    
    Hashtable* fIntervalPatterns;

};


inline UBool
DateIntervalInfo::operator!=(const DateIntervalInfo& other) const {
    return !operator==(other);
}


U_NAMESPACE_END

#endif

#endif

