





#ifndef ZONEMETA_H
#define ZONEMETA_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/unistr.h"
#include "hash.h"

U_NAMESPACE_BEGIN

typedef struct OlsonToMetaMappingEntry {
    const UChar *mzid; 
    UDate from;
    UDate to;
} OlsonToMetaMappingEntry;

class UVector;
class TimeZone;

class U_I18N_API ZoneMeta {
public:
    






    static UnicodeString& U_EXPORT2 getCanonicalCLDRID(const UnicodeString &tzid, UnicodeString &systemID, UErrorCode& status);

    




    static const UChar* U_EXPORT2 getCanonicalCLDRID(const UnicodeString &tzid, UErrorCode& status);

    


    static const UChar* U_EXPORT2 getCanonicalCLDRID(const TimeZone& tz);

    







    static UnicodeString& U_EXPORT2 getCanonicalCountry(const UnicodeString &tzid, UnicodeString &country, UBool *isPrimary = NULL);

    


    static UnicodeString& U_EXPORT2 getMetazoneID(const UnicodeString &tzid, UDate date, UnicodeString &result);
    


    static UnicodeString& U_EXPORT2 getZoneIdByMetazone(const UnicodeString &mzid, const UnicodeString &region, UnicodeString &result);

    static const UVector* U_EXPORT2 getMetazoneMappings(const UnicodeString &tzid);

    static const UVector* U_EXPORT2 getAvailableMetazoneIDs();

    



    static const UChar* U_EXPORT2 findTimeZoneID(const UnicodeString& tzid);

    



    static const UChar* U_EXPORT2 findMetaZoneID(const UnicodeString& mzid);

    




    static TimeZone* createCustomTimeZone(int32_t offset);

    





    static const UChar* U_EXPORT2 getShortID(const TimeZone& tz);

    





    static const UChar* U_EXPORT2 getShortID(const UnicodeString& id);

private:
    ZoneMeta(); 
    static UVector* createMetazoneMappings(const UnicodeString &tzid);
    static UnicodeString& formatCustomID(uint8_t hour, uint8_t min, uint8_t sec, UBool negative, UnicodeString& id);
    static const UChar* getShortIDFromCanonical(const UChar* canonicalID);
};

U_NAMESPACE_END

#endif 
#endif 
