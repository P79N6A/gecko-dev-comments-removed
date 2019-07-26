










#ifndef __DTPTNGEN_H__
#define __DTPTNGEN_H__

#include "unicode/datefmt.h"
#include "unicode/locid.h"
#include "unicode/udat.h"
#include "unicode/udatpg.h"

U_NAMESPACE_BEGIN







class Hashtable;
class FormatParser;
class DateTimeMatcher;
class DistanceInfo;
class PatternMap;
class PtnSkeleton;















class U_I18N_API DateTimePatternGenerator : public UObject {
public:
    





    static DateTimePatternGenerator* U_EXPORT2 createInstance(UErrorCode& status);

    






    static DateTimePatternGenerator* U_EXPORT2 createInstance(const Locale& uLocale, UErrorCode& status);

    





     static DateTimePatternGenerator* U_EXPORT2 createEmptyInstance(UErrorCode& status);
     
    



    virtual ~DateTimePatternGenerator();

    




    DateTimePatternGenerator* clone() const;

     






    UBool operator==(const DateTimePatternGenerator& other) const;
    
    






    UBool operator!=(const DateTimePatternGenerator& other) const;

    









    UnicodeString getSkeleton(const UnicodeString& pattern, UErrorCode& status);

    












    UnicodeString getBaseSkeleton(const UnicodeString& pattern, UErrorCode& status);

    



















    UDateTimePatternConflict addPattern(const UnicodeString& pattern, 
                                        UBool override, 
                                        UnicodeString& conflictingPattern,
                                        UErrorCode& status);

    

















    void setAppendItemFormat(UDateTimePatternField field, const UnicodeString& value);

    







    const UnicodeString& getAppendItemFormat(UDateTimePatternField field) const;

    










    void setAppendItemName(UDateTimePatternField field, const UnicodeString& value);

    







    const UnicodeString& getAppendItemName(UDateTimePatternField field) const;

    


















    void setDateTimeFormat(const UnicodeString& dateTimeFormat);

    




    const UnicodeString& getDateTimeFormat() const;

    












     UnicodeString getBestPattern(const UnicodeString& skeleton, UErrorCode& status);


    


















     UnicodeString getBestPattern(const UnicodeString& skeleton,
                                  UDateTimePatternMatchOptions options,
                                  UErrorCode& status);


    















     UnicodeString replaceFieldTypes(const UnicodeString& pattern, 
                                     const UnicodeString& skeleton, 
                                     UErrorCode& status);

    





















     UnicodeString replaceFieldTypes(const UnicodeString& pattern, 
                                     const UnicodeString& skeleton, 
                                     UDateTimePatternMatchOptions options,
                                     UErrorCode& status);

    










     StringEnumeration* getSkeletons(UErrorCode& status) const;

     





     const UnicodeString& getPatternForSkeleton(const UnicodeString& skeleton) const;
     
    








     StringEnumeration* getBaseSkeletons(UErrorCode& status) const;

#ifndef U_HIDE_INTERNAL_API
     











     StringEnumeration* getRedundants(UErrorCode& status);
#endif  

    










    void setDecimal(const UnicodeString& decimal);

    




    const UnicodeString& getDecimal() const;

    




    virtual UClassID getDynamicClassID() const;

    




    static UClassID U_EXPORT2 getStaticClassID(void);

private:
    



    DateTimePatternGenerator(UErrorCode & status);

    



    DateTimePatternGenerator(const Locale& locale, UErrorCode & status);

    




    DateTimePatternGenerator(const DateTimePatternGenerator& other);

    




    DateTimePatternGenerator& operator=(const DateTimePatternGenerator& other);

    Locale pLocale;  
    FormatParser *fp;
    DateTimeMatcher* dtMatcher;
    DistanceInfo *distanceInfo;
    PatternMap *patternMap;
    UnicodeString appendItemFormats[UDATPG_FIELD_COUNT];
    UnicodeString appendItemNames[UDATPG_FIELD_COUNT];
    UnicodeString dateTimeFormat;
    UnicodeString decimal;
    DateTimeMatcher *skipMatcher;
    Hashtable *fAvailableFormatKeyHash;
    UnicodeString hackPattern;
    UnicodeString emptyString;
    UChar fDefaultHourFormatChar;

    void initData(const Locale &locale, UErrorCode &status);
    void addCanonicalItems();
    void addICUPatterns(const Locale& locale, UErrorCode& status);
    void hackTimes(const UnicodeString& hackPattern, UErrorCode& status);
    void addCLDRData(const Locale& locale, UErrorCode& status);
    UDateTimePatternConflict addPatternWithSkeleton(const UnicodeString& pattern, const UnicodeString * skeletonToUse, UBool override, UnicodeString& conflictingPattern, UErrorCode& status);
    void initHashtable(UErrorCode& status);
    void setDateTimeFromCalendar(const Locale& locale, UErrorCode& status);
    void setDecimalSymbols(const Locale& locale, UErrorCode& status);
    UDateTimePatternField getAppendFormatNumber(const char* field) const;
    UDateTimePatternField getAppendNameNumber(const char* field) const;
    void getAppendName(UDateTimePatternField field, UnicodeString& value);
    int32_t getCanonicalIndex(const UnicodeString& field);
    const UnicodeString* getBestRaw(DateTimeMatcher& source, int32_t includeMask, DistanceInfo* missingFields, const PtnSkeleton** specifiedSkeletonPtr = 0);
    UnicodeString adjustFieldTypes(const UnicodeString& pattern, const PtnSkeleton* specifiedSkeleton, UBool fixFractionalSeconds, UDateTimePatternMatchOptions options = UDATPG_MATCH_NO_OPTIONS);
    UnicodeString getBestAppending(int32_t missingFields, UDateTimePatternMatchOptions options = UDATPG_MATCH_NO_OPTIONS);
    int32_t getTopBitNumber(int32_t foundMask);
    void setAvailableFormat(const UnicodeString &key, UErrorCode& status);
    UBool isAvailableFormatSet(const UnicodeString &key) const;
    void copyHashtable(Hashtable *other, UErrorCode &status);
    UBool isCanonicalItem(const UnicodeString& item) const;
} ;

U_NAMESPACE_END

#endif
