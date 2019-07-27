




 
#ifndef __TimeZoneTest__
#define __TimeZoneTest__

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/simpletz.h" 
#include "caltztst.h"




class TimeZoneTest: public CalendarTimeZoneTest {
    
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par );
public: 
    static const int32_t millisPerHour;
 
public:
    


    virtual void TestPRTOffset(void);
    


    virtual void TestVariousAPI518(void);
    


    virtual void TestGetAvailableIDs913(void);

    virtual void TestGetAvailableIDsNew(void);

    


    virtual void TestGenericAPI(void);
    


    virtual void TestRuleAPI(void);
 
    void findTransition(const TimeZone& tz,
                        UDate min, UDate max);

   


    void testUsingBinarySearch(const TimeZone& tz,
                               UDate min, UDate max,
                               UDate expectedBoundary);


    

 
    virtual void TestShortZoneIDs(void);


    

 
    virtual void TestCustomParse(void);
    
    

 
    virtual void TestDisplayName(void);

    void TestDSTSavings(void);
    void TestAlternateRules(void);

    void TestCountries(void);

    void TestHistorical(void);

    void TestEquivalentIDs(void);

    void TestAliasedNames(void);
    
    void TestFractionalDST(void);

    void TestFebruary(void);

    void TestCanonicalIDAPI();
    void TestCanonicalID(void);

    virtual void TestDisplayNamesMeta();

    void TestGetRegion(void);
    void TestGetUnknown();

    void TestGetWindowsID(void);
    void TestGetIDForWindowsID(void);

    static const UDate INTERVAL;

private:
    
    static UnicodeString& formatOffset(int32_t offset, UnicodeString& rv);
    static UnicodeString& formatTZID(int32_t offset, UnicodeString& rv);

    
    
    static const int32_t REFERENCE_YEAR;
    static const char *REFERENCE_DATA_VERSION;

    void checkContainsAll(StringEnumeration *s1, const char *name1,
        StringEnumeration *s2, const char *name2);
};

#endif 
 
#endif 
