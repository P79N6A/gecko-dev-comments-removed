

























#ifndef TIMEZONE_H
#define TIMEZONE_H

#include "unicode/utypes.h"






#if !UCONFIG_NO_FORMATTING

#include "unicode/uobject.h"
#include "unicode/unistr.h"
#include "unicode/ures.h"
#include "unicode/ucal.h"

U_NAMESPACE_BEGIN

class StringEnumeration;


















































































class U_I18N_API TimeZone : public UObject {
public:
    


    virtual ~TimeZone();

#ifndef U_HIDE_DRAFT_API
    











    static const TimeZone& U_EXPORT2 getUnknown();
#endif  

    











    static const TimeZone* U_EXPORT2 getGMT(void);

    










    static TimeZone* U_EXPORT2 createTimeZone(const UnicodeString& ID);

    














    static StringEnumeration* U_EXPORT2 createTimeZoneIDEnumeration(
        USystemTimeZoneType zoneType,
        const char* region,
        const int32_t* rawOffset,
        UErrorCode& ec);

    






    static StringEnumeration* U_EXPORT2 createEnumeration();

    
















    static StringEnumeration* U_EXPORT2 createEnumeration(int32_t rawOffset);

    









    static StringEnumeration* U_EXPORT2 createEnumeration(const char* country);

    













    static int32_t U_EXPORT2 countEquivalentIDs(const UnicodeString& id);

    


















    static const UnicodeString U_EXPORT2 getEquivalentID(const UnicodeString& id,
                                               int32_t index);

    











    static TimeZone* U_EXPORT2 createDefault(void);

    








    static void U_EXPORT2 adoptDefault(TimeZone* zone);

#ifndef U_HIDE_SYSTEM_API
    







    static void U_EXPORT2 setDefault(const TimeZone& zone);
#endif  

    





    static const char* U_EXPORT2 getTZDataVersion(UErrorCode& status);

    












    static UnicodeString& U_EXPORT2 getCanonicalID(const UnicodeString& id,
        UnicodeString& canonicalID, UErrorCode& status);

    














    static UnicodeString& U_EXPORT2 getCanonicalID(const UnicodeString& id,
        UnicodeString& canonicalID, UBool& isSystemID, UErrorCode& status);

    








    virtual UBool operator==(const TimeZone& that) const;

    








    UBool operator!=(const TimeZone& that) const {return !operator==(that);}

    























    virtual int32_t getOffset(uint8_t era, int32_t year, int32_t month, int32_t day,
                              uint8_t dayOfWeek, int32_t millis, UErrorCode& status) const = 0;

    



















    virtual int32_t getOffset(uint8_t era, int32_t year, int32_t month, int32_t day,
                           uint8_t dayOfWeek, int32_t milliseconds,
                           int32_t monthLength, UErrorCode& status) const = 0;

    






















    virtual void getOffset(UDate date, UBool local, int32_t& rawOffset,
                           int32_t& dstOffset, UErrorCode& ec) const;

    






    virtual void setRawOffset(int32_t offsetMillis) = 0;

    






    virtual int32_t getRawOffset(void) const = 0;

    






    UnicodeString& getID(UnicodeString& ID) const;

    












    void setID(const UnicodeString& ID);

    



    enum EDisplayType {
        



        SHORT = 1,
        



        LONG,
        



        SHORT_GENERIC,
        



        LONG_GENERIC,
        




        SHORT_GMT,
        




        LONG_GMT,
        




        SHORT_COMMONLY_USED,
        




        GENERIC_LOCATION
    };

    










    UnicodeString& getDisplayName(UnicodeString& result) const;

    












    UnicodeString& getDisplayName(const Locale& locale, UnicodeString& result) const;

    











    UnicodeString& getDisplayName(UBool daylight, EDisplayType style, UnicodeString& result) const;

    













    UnicodeString& getDisplayName(UBool daylight, EDisplayType style, const Locale& locale, UnicodeString& result) const;
    
    

































    virtual UBool useDaylightTime(void) const = 0;

    












    virtual UBool inDaylightTime(UDate date, UErrorCode& status) const = 0;

    







    virtual UBool hasSameRules(const TimeZone& other) const;

    






    virtual TimeZone* clone(void) const = 0;

    





    static UClassID U_EXPORT2 getStaticClassID(void);

    










    virtual UClassID getDynamicClassID(void) const = 0;
    
    














    virtual int32_t getDSTSavings() const;

    















 
    static int32_t U_EXPORT2 getRegion(const UnicodeString& id, 
        char *region, int32_t capacity, UErrorCode& status); 

protected:

    



    TimeZone();

    




    TimeZone(const UnicodeString &id);

    




    TimeZone(const TimeZone& source);

    




    TimeZone& operator=(const TimeZone& right);

#ifndef U_HIDE_INTERNAL_API
    








    static UResourceBundle* loadRule(const UResourceBundle* top, const UnicodeString& ruleid, UResourceBundle* oldbundle, UErrorCode&status);
#endif  

private:
    friend class ZoneMeta;


    static TimeZone*        createCustomTimeZone(const UnicodeString&); 

    







    static const UChar* findID(const UnicodeString& id);

    







    static const UChar* dereferOlsonLink(const UnicodeString& id);

    





    static const UChar* getRegion(const UnicodeString& id);

    






    static const UChar* getRegion(const UnicodeString& id, UErrorCode& status);

    









    static UBool parseCustomID(const UnicodeString& id, int32_t& sign, int32_t& hour,
        int32_t& minute, int32_t& second);

    









    static UnicodeString& getCustomID(const UnicodeString& id, UnicodeString& normalized,
        UErrorCode& status);

    








    static UnicodeString& formatCustomID(int32_t hour, int32_t min, int32_t sec,
        UBool negative, UnicodeString& id);

    




    static void             initDefault(void);

    
    






    static TimeZone*        createSystemTimeZone(const UnicodeString& name);
    static TimeZone*        createSystemTimeZone(const UnicodeString& name, UErrorCode& ec);

    UnicodeString           fID;    

    friend class TZEnumeration;
};




inline UnicodeString&
TimeZone::getID(UnicodeString& ID) const
{
    ID = fID;
    return ID;
}



inline void
TimeZone::setID(const UnicodeString& ID)
{
    fID = ID;
}
U_NAMESPACE_END

#endif 

#endif 

