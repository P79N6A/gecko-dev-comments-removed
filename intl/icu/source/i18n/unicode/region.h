






#ifndef REGION_H
#define REGION_H






#include "unicode/utypes.h"
#include "unicode/uregion.h"

#if !UCONFIG_NO_FORMATTING
#ifndef U_HIDE_DRAFT_API

#include "unicode/uobject.h"
#include "unicode/uniset.h"
#include "unicode/unistr.h"
#include "unicode/strenum.h"

U_NAMESPACE_BEGIN









































class U_I18N_API Region : public UObject {
public:
    



    virtual ~Region();

    



    UBool operator==(const Region &that) const;

    



    UBool operator!=(const Region &that) const;
 
    






    static const Region* U_EXPORT2 getInstance(const char *region_code, UErrorCode &status);

    




    static const Region* U_EXPORT2 getInstance (int32_t code, UErrorCode &status);

    



    static StringEnumeration* U_EXPORT2 getAvailable(URegionType type);
   
    





    const Region* getContainingRegion() const;

    







    const Region* getContainingRegion(URegionType type) const;

    








    StringEnumeration* getContainedRegions() const;

    






    StringEnumeration* getContainedRegions( URegionType type ) const;
 
    



    UBool contains(const Region &other) const;

    





    StringEnumeration* getPreferredValues() const;
 

    



    const char* getRegionCode() const;

    




    int32_t getNumericCode() const;

    



    URegionType getType() const;

#ifndef U_HIDE_INTERNAL_API
    



    static void cleanupRegionData();
#endif  

private:
    char id[4];
    UnicodeString idStr;
    int32_t code;
    URegionType type;
    Region *containingRegion;
    UVector *containedRegions;
    UVector *preferredValues;

    


    Region();


    








    static void loadRegionData();

};

U_NAMESPACE_END

#endif  
#endif 
#endif 


