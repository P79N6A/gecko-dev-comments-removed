





#ifndef __TZNAMES_H
#define __TZNAMES_H





#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/uloc.h"
#include "unicode/unistr.h"

U_CDECL_BEGIN





typedef enum UTimeZoneNameType {
    



    UTZNM_UNKNOWN           = 0x00,
    



    UTZNM_LONG_GENERIC      = 0x01,
    



    UTZNM_LONG_STANDARD     = 0x02,
    



    UTZNM_LONG_DAYLIGHT     = 0x04,
    



    UTZNM_SHORT_GENERIC     = 0x08,
    



    UTZNM_SHORT_STANDARD    = 0x10,
    



    UTZNM_SHORT_DAYLIGHT    = 0x20,
    



    UTZNM_EXEMPLAR_LOCATION = 0x40
} UTimeZoneNameType;

U_CDECL_END

U_NAMESPACE_BEGIN

class UVector;
struct MatchInfo;



















































class U_I18N_API TimeZoneNames : public UObject {
public:
    



    virtual ~TimeZoneNames();

    





    virtual UBool operator==(const TimeZoneNames& other) const = 0;

    






    UBool operator!=(const TimeZoneNames& other) const { return !operator==(other); }

    





    virtual TimeZoneNames* clone() const = 0;

    







    static TimeZoneNames* U_EXPORT2 createInstance(const Locale& locale, UErrorCode& status);

#ifndef U_HIDE_DRAFT_API
    









     static TimeZoneNames* U_EXPORT2 createTZDBInstance(const Locale& locale, UErrorCode& status);
#endif 

    





    virtual StringEnumeration* getAvailableMetaZoneIDs(UErrorCode& status) const = 0;

    






    virtual StringEnumeration* getAvailableMetaZoneIDs(const UnicodeString& tzID, UErrorCode& status) const = 0;

    









    virtual UnicodeString& getMetaZoneID(const UnicodeString& tzID, UDate date, UnicodeString& mzID) const = 0;

    















    virtual UnicodeString& getReferenceZoneID(const UnicodeString& mzID, const char* region, UnicodeString& tzID) const = 0;

    









    virtual UnicodeString& getMetaZoneDisplayName(const UnicodeString& mzID, UTimeZoneNameType type, UnicodeString& name) const = 0;

    









    virtual UnicodeString& getTimeZoneDisplayName(const UnicodeString& tzID, UTimeZoneNameType type, UnicodeString& name) const = 0;

    


















    virtual UnicodeString& getExemplarLocationName(const UnicodeString& tzID, UnicodeString& name) const;

    














    virtual UnicodeString& getDisplayName(const UnicodeString& tzID, UTimeZoneNameType type, UDate date, UnicodeString& name) const;

    




    class U_I18N_API MatchInfoCollection : public UMemory {
    public:
        



        MatchInfoCollection();
        



        virtual ~MatchInfoCollection();

#ifndef U_HIDE_INTERNAL_API
        







        void addZone(UTimeZoneNameType nameType, int32_t matchLength,
            const UnicodeString& tzID, UErrorCode& status);

        







        void addMetaZone(UTimeZoneNameType nameType, int32_t matchLength,
            const UnicodeString& mzID, UErrorCode& status);

        




        int32_t size() const;

        







        UTimeZoneNameType getNameTypeAt(int32_t idx) const;

        






        int32_t getMatchLengthAt(int32_t idx) const;

        






        UBool getTimeZoneIDAt(int32_t idx, UnicodeString& tzID) const;

        






        UBool getMetaZoneIDAt(int32_t idx, UnicodeString& mzID) const;
#endif  

    private:
        UVector* fMatches;  

        UVector* matches(UErrorCode& status);
    };

    












    virtual MatchInfoCollection* find(const UnicodeString& text, int32_t start, uint32_t types, UErrorCode& status) const = 0;
};

U_NAMESPACE_END

#endif
#endif
