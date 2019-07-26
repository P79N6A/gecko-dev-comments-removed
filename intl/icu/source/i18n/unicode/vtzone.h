





#ifndef VTZONE_H
#define VTZONE_H

#include "unicode/utypes.h"






#if !UCONFIG_NO_FORMATTING

#include "unicode/basictz.h"

U_NAMESPACE_BEGIN

class VTZWriter;
class VTZReader;
class UVector;















class U_I18N_API VTimeZone : public BasicTimeZone {
public:
    




    VTimeZone(const VTimeZone& source);

    



    virtual ~VTimeZone();

    




    VTimeZone& operator=(const VTimeZone& right);

    







    virtual UBool operator==(const TimeZone& that) const;

    







    virtual UBool operator!=(const TimeZone& that) const;

    






    static VTimeZone* createVTimeZoneByID(const UnicodeString& ID);

    






    static VTimeZone* createVTimeZoneFromBasicTimeZone(const BasicTimeZone& basicTZ,
                                                       UErrorCode &status);

    








    static VTimeZone* createVTimeZone(const UnicodeString& vtzdata, UErrorCode& status);

    







    UBool getTZURL(UnicodeString& url) const;

    




    void setTZURL(const UnicodeString& url);

    







    UBool getLastModified(UDate& lastModified) const;

    




    void setLastModified(UDate lastModified);

    





    void write(UnicodeString& result, UErrorCode& status) const;

    







    void write(UDate start, UnicodeString& result, UErrorCode& status) ;

    












    void writeSimple(UDate time, UnicodeString& result, UErrorCode& status) ;

    





    virtual TimeZone* clone(void) const;

    























    virtual int32_t getOffset(uint8_t era, int32_t year, int32_t month, int32_t day,
                              uint8_t dayOfWeek, int32_t millis, UErrorCode& status) const;

    


















    virtual int32_t getOffset(uint8_t era, int32_t year, int32_t month, int32_t day,
                           uint8_t dayOfWeek, int32_t millis,
                           int32_t monthLength, UErrorCode& status) const;

    





















    virtual void getOffset(UDate date, UBool local, int32_t& rawOffset,
                           int32_t& dstOffset, UErrorCode& ec) const;

    






    virtual void setRawOffset(int32_t offsetMillis);

    






    virtual int32_t getRawOffset(void) const;

    





    virtual UBool useDaylightTime(void) const;

    












    virtual UBool inDaylightTime(UDate date, UErrorCode& status) const;

    







    virtual UBool hasSameRules(const TimeZone& other) const;

    







    virtual UBool getNextTransition(UDate base, UBool inclusive, TimeZoneTransition& result) ;

    







    virtual UBool getPreviousTransition(UDate base, UBool inclusive, TimeZoneTransition& result) ;

    







    virtual int32_t countTransitionRules(UErrorCode& status) ;

    















    virtual void getTimeZoneRules(const InitialTimeZoneRule*& initial,
        const TimeZoneRule* trsrules[], int32_t& trscount, UErrorCode& status) ;

private:
    enum { DEFAULT_VTIMEZONE_LINES = 100 };

    


    VTimeZone();
    static VTimeZone* createVTimeZone(VTZReader* reader);
    void write(VTZWriter& writer, UErrorCode& status) const;
    void write(UDate start, VTZWriter& writer, UErrorCode& status) ;
    void writeSimple(UDate time, VTZWriter& writer, UErrorCode& status) ;
    void load(VTZReader& reader, UErrorCode& status);
    void parse(UErrorCode& status);

    void writeZone(VTZWriter& w, BasicTimeZone& basictz, UVector* customProps,
        UErrorCode& status) const;

    void writeHeaders(VTZWriter& w, UErrorCode& status) const;
    void writeFooter(VTZWriter& writer, UErrorCode& status) const;

    void writeZonePropsByTime(VTZWriter& writer, UBool isDst, const UnicodeString& zonename,
                              int32_t fromOffset, int32_t toOffset, UDate time, UBool withRDATE,
                              UErrorCode& status) const;
    void writeZonePropsByDOM(VTZWriter& writer, UBool isDst, const UnicodeString& zonename,
                             int32_t fromOffset, int32_t toOffset,
                             int32_t month, int32_t dayOfMonth, UDate startTime, UDate untilTime,
                             UErrorCode& status) const;
    void writeZonePropsByDOW(VTZWriter& writer, UBool isDst, const UnicodeString& zonename,
                             int32_t fromOffset, int32_t toOffset,
                             int32_t month, int32_t weekInMonth, int32_t dayOfWeek,
                             UDate startTime, UDate untilTime, UErrorCode& status) const;
    void writeZonePropsByDOW_GEQ_DOM(VTZWriter& writer, UBool isDst, const UnicodeString& zonename,
                                     int32_t fromOffset, int32_t toOffset,
                                     int32_t month, int32_t dayOfMonth, int32_t dayOfWeek,
                                     UDate startTime, UDate untilTime, UErrorCode& status) const;
    void writeZonePropsByDOW_GEQ_DOM_sub(VTZWriter& writer, int32_t month, int32_t dayOfMonth,
                                         int32_t dayOfWeek, int32_t numDays,
                                         UDate untilTime, int32_t fromOffset, UErrorCode& status) const;
    void writeZonePropsByDOW_LEQ_DOM(VTZWriter& writer, UBool isDst, const UnicodeString& zonename,
                                     int32_t fromOffset, int32_t toOffset,
                                     int32_t month, int32_t dayOfMonth, int32_t dayOfWeek,
                                     UDate startTime, UDate untilTime, UErrorCode& status) const;
    void writeFinalRule(VTZWriter& writer, UBool isDst, const AnnualTimeZoneRule* rule,
                        int32_t fromRawOffset, int32_t fromDSTSavings,
                        UDate startTime, UErrorCode& status) const;

    void beginZoneProps(VTZWriter& writer, UBool isDst, const UnicodeString& zonename,
                        int32_t fromOffset, int32_t toOffset, UDate startTime, UErrorCode& status) const;
    void endZoneProps(VTZWriter& writer, UBool isDst, UErrorCode& status) const;
    void beginRRULE(VTZWriter& writer, int32_t month, UErrorCode& status) const;
    void appendUNTIL(VTZWriter& writer, const UnicodeString& until, UErrorCode& status) const;

    BasicTimeZone   *tz;
    UVector         *vtzlines;
    UnicodeString   tzurl;
    UDate           lastmod;
    UnicodeString   olsonzid;
    UnicodeString   icutzver;

public:
    










    static UClassID U_EXPORT2 getStaticClassID(void);

    










    virtual UClassID getDynamicClassID(void) const;
};

U_NAMESPACE_END

#endif

#endif

