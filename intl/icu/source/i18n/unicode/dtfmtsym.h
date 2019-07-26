















     
#ifndef DTFMTSYM_H
#define DTFMTSYM_H
 
#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/calendar.h"
#include "unicode/uobject.h"
#include "unicode/locid.h"
#include "unicode/udat.h"
#include "unicode/ures.h"






U_NAMESPACE_BEGIN


class SimpleDateFormat;
class Hashtable;






































class U_I18N_API DateFormatSymbols : public UObject {
public:
    












    DateFormatSymbols(UErrorCode& status);

    









    DateFormatSymbols(const Locale& locale,
                      UErrorCode& status);

#ifndef U_HIDE_INTERNAL_API
    















    DateFormatSymbols(const char *type, UErrorCode& status);

    












    DateFormatSymbols(const Locale& locale,
                      const char *type,
                      UErrorCode& status);
#endif  

    



    DateFormatSymbols(const DateFormatSymbols&);

    



    DateFormatSymbols& operator=(const DateFormatSymbols&);

    




    virtual ~DateFormatSymbols();

    






    UBool operator==(const DateFormatSymbols& other) const;

    






    UBool operator!=(const DateFormatSymbols& other) const { return !operator==(other); }

    






    const UnicodeString* getEras(int32_t& count) const;

    





    void setEras(const UnicodeString* eras, int32_t count);

    






    const UnicodeString* getEraNames(int32_t& count) const;

    





    void setEraNames(const UnicodeString* eraNames, int32_t count);

    






    const UnicodeString* getNarrowEras(int32_t& count) const;

    





    void setNarrowEras(const UnicodeString* narrowEras, int32_t count);

    





    const UnicodeString* getMonths(int32_t& count) const;

    






    void setMonths(const UnicodeString* months, int32_t count);

    






    const UnicodeString* getShortMonths(int32_t& count) const;

    





    void setShortMonths(const UnicodeString* shortMonths, int32_t count);

    



    enum DtContextType {
         FORMAT,
         STANDALONE,
         DT_CONTEXT_COUNT
    };

    



    enum DtWidthType {
         ABBREVIATED,
         WIDE,
         NARROW,
         DT_WIDTH_COUNT
    };

    







    const UnicodeString* getMonths(int32_t& count, DtContextType context, DtWidthType width) const;

    








    void setMonths(const UnicodeString* months, int32_t count, DtContextType context, DtWidthType width);

    





    const UnicodeString* getWeekdays(int32_t& count) const;


    





    void setWeekdays(const UnicodeString* weekdays, int32_t count);

    





    const UnicodeString* getShortWeekdays(int32_t& count) const;

    





    void setShortWeekdays(const UnicodeString* shortWeekdays, int32_t count);

    







    const UnicodeString* getWeekdays(int32_t& count, DtContextType context, DtWidthType width) const;

    







    void setWeekdays(const UnicodeString* weekdays, int32_t count, DtContextType context, DtWidthType width);

    








    const UnicodeString* getQuarters(int32_t& count, DtContextType context, DtWidthType width) const;

    









    void setQuarters(const UnicodeString* quarters, int32_t count, DtContextType context, DtWidthType width);

    





    const UnicodeString* getAmPmStrings(int32_t& count) const;

    





    void setAmPmStrings(const UnicodeString* ampms, int32_t count);

#ifndef U_HIDE_INTERNAL_API
    







    enum EMonthPatternType
    {
        kLeapMonthPatternFormatWide,
        kLeapMonthPatternFormatAbbrev,
        kLeapMonthPatternFormatNarrow,
        kLeapMonthPatternStandaloneWide,
        kLeapMonthPatternStandaloneAbbrev,
        kLeapMonthPatternStandaloneNarrow,
        kLeapMonthPatternNumeric,
        kMonthPatternsCount
    };

    











    const UnicodeString* getLeapMonthPatterns(int32_t& count) const;

#endif  

#ifndef U_HIDE_DEPRECATED_API
    






    const UnicodeString** getZoneStrings(int32_t& rowCount, int32_t& columnCount) const;
#endif  

    










    void setZoneStrings(const UnicodeString* const* strings, int32_t rowCount, int32_t columnCount);

    




    static const UChar * U_EXPORT2 getPatternUChars(void);

    









    UnicodeString& getLocalPatternChars(UnicodeString& result) const;

    





    void setLocalPatternChars(const UnicodeString& newLocalPatternChars);

    




    Locale getLocale(ULocDataLocaleType type, UErrorCode& status) const;

#ifndef U_HIDE_INTERNAL_API
    



    enum ECapitalizationContextUsageType
    {
        kCapContextUsageOther,
        kCapContextUsageMonthFormat,     
        kCapContextUsageMonthStandalone, 
        kCapContextUsageMonthNarrow,
        kCapContextUsageDayFormat,     
        kCapContextUsageDayStandalone, 
        kCapContextUsageDayNarrow,
        kCapContextUsageEraWide,
        kCapContextUsageEraAbbrev,
        kCapContextUsageEraNarrow,
        kCapContextUsageZoneLong,
        kCapContextUsageZoneShort,
        kCapContextUsageMetazoneLong,
        kCapContextUsageMetazoneShort,
        kCapContextUsageTypeCount
    };
#endif  

    




    virtual UClassID getDynamicClassID() const;

    




    static UClassID U_EXPORT2 getStaticClassID();

private:

    friend class SimpleDateFormat;
    friend class DateFormatSymbolsSingleSetter; 

    


    UnicodeString*  fEras;
    int32_t         fErasCount;

    


    UnicodeString*  fEraNames;
    int32_t         fEraNamesCount;

    


    UnicodeString*  fNarrowEras;
    int32_t         fNarrowErasCount;

    


    UnicodeString*  fMonths;
    int32_t         fMonthsCount;

    


    UnicodeString*  fShortMonths;
    int32_t         fShortMonthsCount;

    


    UnicodeString*  fNarrowMonths;
    int32_t         fNarrowMonthsCount;

    


    UnicodeString*  fStandaloneMonths;
    int32_t         fStandaloneMonthsCount;

    


    UnicodeString*  fStandaloneShortMonths;
    int32_t         fStandaloneShortMonthsCount;

    


    UnicodeString*  fStandaloneNarrowMonths;
    int32_t         fStandaloneNarrowMonthsCount;

    


    UnicodeString*  fWeekdays;
    int32_t         fWeekdaysCount;

    


    UnicodeString*  fShortWeekdays;
    int32_t         fShortWeekdaysCount;

    


    UnicodeString*  fNarrowWeekdays;
    int32_t         fNarrowWeekdaysCount;

    


    UnicodeString*  fStandaloneWeekdays;
    int32_t         fStandaloneWeekdaysCount;

    


    UnicodeString*  fStandaloneShortWeekdays;
    int32_t         fStandaloneShortWeekdaysCount;

    


    UnicodeString*  fStandaloneNarrowWeekdays;
    int32_t         fStandaloneNarrowWeekdaysCount;

    


    UnicodeString*  fAmPms;
    int32_t         fAmPmsCount;

    


    UnicodeString  *fQuarters;
    int32_t         fQuartersCount;

    


    UnicodeString  *fShortQuarters;
    int32_t         fShortQuartersCount;

    


    UnicodeString  *fStandaloneQuarters;
    int32_t         fStandaloneQuartersCount;

    


    UnicodeString  *fStandaloneShortQuarters;
    int32_t         fStandaloneShortQuartersCount;

    


    UnicodeString  *fLeapMonthPatterns;
    int32_t         fLeapMonthPatternsCount;

    


    UnicodeString*  fShortYearNames;
    int32_t         fShortYearNamesCount;

    




































    UnicodeString   **fZoneStrings;         
    UnicodeString   **fLocaleZoneStrings;   
    int32_t         fZoneStringsRowCount;
    int32_t         fZoneStringsColCount;

    Locale                  fZSFLocale;         

    


    UnicodeString   fLocalPatternChars;

#ifndef U_HIDE_INTERNAL_API
    




     UBool fCapitalization[kCapContextUsageTypeCount][2];
#endif


private:
    


    char validLocale[ULOC_FULLNAME_CAPACITY];
    char actualLocale[ULOC_FULLNAME_CAPACITY];

    DateFormatSymbols(); 

    








    void initializeData(const Locale& locale, const char *type, UErrorCode& status, UBool useLastResortData = FALSE);

    







    static void assignArray(UnicodeString*& dstArray,
                            int32_t& dstCount,
                            const UnicodeString* srcArray,
                            int32_t srcCount);

    









    static UBool arrayCompare(const UnicodeString* array1,
                             const UnicodeString* array2,
                             int32_t count);

    




    void createZoneStrings(const UnicodeString *const * otherStrings);

    


    void dispose(void);

    



    void copyData(const DateFormatSymbols& other);

    


    void initZoneStringsArray(void);

    


    void disposeZoneStrings(void);

    



    static UDateFormatField U_EXPORT2 getPatternCharIndex(UChar c);

    


    static UBool U_EXPORT2 isNumericField(UDateFormatField f, int32_t count);

    


    static UBool U_EXPORT2 isNumericPatternChar(UChar c, int32_t count);
};

U_NAMESPACE_END

#endif 

#endif 

