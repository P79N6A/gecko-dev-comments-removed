









#ifndef __DTITVFMT_H__
#define __DTITVFMT_H__


#include "unicode/utypes.h"






#if !UCONFIG_NO_FORMATTING

#include "unicode/ucal.h"
#include "unicode/smpdtfmt.h"
#include "unicode/dtintrv.h"
#include "unicode/dtitvinf.h"
#include "unicode/dtptngen.h"

U_NAMESPACE_BEGIN



























































































































































































class U_I18N_API DateIntervalFormat : public Format {
public:

    












    static DateIntervalFormat* U_EXPORT2 createInstance(
                                               const UnicodeString& skeleton,
                                               UErrorCode& status);

    




























    static DateIntervalFormat* U_EXPORT2 createInstance(
                                               const UnicodeString& skeleton,
                                               const Locale& locale,
                                               UErrorCode& status);

    














    static DateIntervalFormat* U_EXPORT2 createInstance(
                                              const UnicodeString& skeleton,
                                              const DateIntervalInfo& dtitvinf,
                                              UErrorCode& status);

    


































    static DateIntervalFormat* U_EXPORT2 createInstance(
                                              const UnicodeString& skeleton,
                                              const Locale& locale,
                                              const DateIntervalInfo& dtitvinf,
                                              UErrorCode& status);

    



    virtual ~DateIntervalFormat();

    





    virtual Format* clone(void) const;

    






    virtual UBool operator==(const Format& other) const;

    






    UBool operator!=(const Format& other) const;


    using Format::format;

    















    virtual UnicodeString& format(const Formattable& obj,
                                  UnicodeString& appendTo,
                                  FieldPosition& fieldPosition,
                                  UErrorCode& status) const ;



    











    UnicodeString& format(const DateInterval* dtInterval,
                          UnicodeString& appendTo,
                          FieldPosition& fieldPosition,
                          UErrorCode& status) const ;


    



















    UnicodeString& format(Calendar& fromCalendar,
                          Calendar& toCalendar,
                          UnicodeString& appendTo,
                          FieldPosition& fieldPosition,
                          UErrorCode& status) const ;

    

























    virtual void parseObject(const UnicodeString& source,
                             Formattable& result,
                             ParsePosition& parse_pos) const;


    





    const DateIntervalInfo* getDateIntervalInfo(void) const;


    





    void setDateIntervalInfo(const DateIntervalInfo& newIntervalPatterns,
                             UErrorCode& status);


    




    const DateFormat* getDateFormat(void) const;

    




    virtual const TimeZone& getTimeZone(void) const;

    





    virtual void adoptTimeZone(TimeZone* zoneToAdopt);

    




    virtual void setTimeZone(const TimeZone& zone);

    










    static UClassID U_EXPORT2 getStaticClassID(void);

    










    virtual UClassID getDynamicClassID(void) const;

protected:

    



    DateIntervalFormat(const DateIntervalFormat&);

    



    DateIntervalFormat& operator=(const DateIntervalFormat&);

private:

    













    struct PatternInfo {
        UnicodeString firstPart;
        UnicodeString secondPart;
        












        UBool         laterDateFirst;
    };


    



    DateIntervalFormat();

    


















    DateIntervalFormat(const Locale& locale, DateIntervalInfo* dtItvInfo,
                       const UnicodeString* skeleton, UErrorCode& status);


    












    static DateIntervalFormat* U_EXPORT2 create(const Locale& locale,
                                                DateIntervalInfo* dtitvinf,
                                                const UnicodeString* skeleton,
                                                UErrorCode& status);

    












    static SimpleDateFormat* U_EXPORT2 createSDFPatternInstance(
                                        const UnicodeString& skeleton,
                                        const Locale& locale,
                                        DateTimePatternGenerator* dtpng,
                                        UErrorCode& status);


    




    

















    UnicodeString& fallbackFormat(Calendar& fromCalendar,
                                  Calendar& toCalendar,
                                  UnicodeString& appendTo,
                                  FieldPosition& pos,
                                  UErrorCode& status) const;



    


































    void initializePattern(UErrorCode& status);



    







    void setFallbackPattern(UCalendarDateFields field,
                            const UnicodeString& skeleton,
                            UErrorCode& status);



    
























    static void  U_EXPORT2 getDateTimeSkeleton(const UnicodeString& skeleton,
                                    UnicodeString& date,
                                    UnicodeString& normalizedDate,
                                    UnicodeString& time,
                                    UnicodeString& normalizedTime);



    























    UBool setSeparateDateTimePtn(const UnicodeString& dateSkeleton,
                                 const UnicodeString& timeSkeleton);




    























    UBool setIntervalPattern(UCalendarDateFields field,
                             const UnicodeString* skeleton,
                             const UnicodeString* bestSkeleton,
                             int8_t differenceInfo,
                             UnicodeString* extendedSkeleton = NULL,
                             UnicodeString* extendedBestSkeleton = NULL);

    




























    static void U_EXPORT2 adjustFieldWidth(
                            const UnicodeString& inputSkeleton,
                            const UnicodeString& bestMatchSkeleton,
                            const UnicodeString& bestMatchIntervalPattern,
                            int8_t differenceInfo,
                            UnicodeString& adjustedIntervalPattern);

    












    void concatSingleDate2TimeInterval(const UChar* format,
                                       int32_t formatLen,
                                       const UnicodeString& datePattern,
                                       UCalendarDateFields field,
                                       UErrorCode& status);

    






    static UBool U_EXPORT2 fieldExistsInSkeleton(UCalendarDateFields field,
                                                 const UnicodeString& skeleton);


    





    static int32_t  U_EXPORT2 splitPatternInto2Part(const UnicodeString& intervalPattern);


    





    void setIntervalPattern(UCalendarDateFields field,
                            const UnicodeString& intervalPattern);


    






    void setIntervalPattern(UCalendarDateFields field,
                            const UnicodeString& intervalPattern,
                            UBool laterDateFirst);


    









    void setPatternInfo(UCalendarDateFields field,
                        const UnicodeString* firstPart,
                        const UnicodeString* secondPart,
                        UBool laterDateFirst);


    
    static const UChar fgCalendarFieldToPatternLetter[];


    


    DateIntervalInfo*     fInfo;

    


    SimpleDateFormat*     fDateFormat;

    




    Calendar* fFromCalendar;
    Calendar* fToCalendar;

    


    DateTimePatternGenerator* fDtpng;

    


    UnicodeString fSkeleton;
    PatternInfo fIntervalPatterns[DateIntervalInfo::kIPI_MAX_INDEX];
};

inline UBool
DateIntervalFormat::operator!=(const Format& other) const  {
    return !operator==(other);
}

U_NAMESPACE_END

#endif 

#endif 

