





#ifndef _REGIONTEST_
#define _REGIONTEST_

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/region.h"
#include "intltest.h"




class RegionTest: public IntlTest {
    
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par );

public:
    RegionTest();
    virtual ~RegionTest();
    
    void TestKnownRegions(void);
    void TestGetInstanceString(void);
    void TestGetInstanceInt(void);
    void TestGetContainedRegions(void);
    void TestGetContainedRegionsWithType(void);
    void TestGetContainingRegion(void);
    void TestGetContainingRegionWithType(void);
    void TestGetPreferredValues(void);
    void TestContains(void);
    void TestAvailableTerritories(void);
    void TestNoContainedRegions(void);

private:

    UBool optionv; 
};

#endif 
 
#endif 

