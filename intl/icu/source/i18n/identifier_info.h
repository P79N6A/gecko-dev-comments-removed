











#ifndef __IDENTIFIER_INFO_H__
#define __IDENTIFIER_INFO_H__

#include "unicode/utypes.h"

#include "unicode/uniset.h"
#include "unicode/uspoof.h"
#include "uhash.h"

U_NAMESPACE_BEGIN

class ScriptSet;



















class U_I18N_API IdentifierInfo : public UMemory {

  public:
    



    IdentifierInfo(UErrorCode &status);

    


    virtual ~IdentifierInfo();

  private:
    
    IdentifierInfo(const IdentifierInfo &other);

  public:
     
    






    IdentifierInfo &setIdentifierProfile(const UnicodeSet &identifierProfile);

    





    const UnicodeSet &getIdentifierProfile() const;


    







    IdentifierInfo &setIdentifier(const UnicodeString &identifier, UErrorCode &status);


    






    const UnicodeString *getIdentifier() const;
    

    





    const ScriptSet *getScripts() const;

    










    const UHashtable *getAlternates() const;

    





    const UnicodeSet *getNumerics() const;

    





    const ScriptSet *getCommonAmongAlternates() const;

    







    int32_t getScriptCount() const;

#if !UCONFIG_NO_NORMALIZATION

    





    URestrictionLevel getRestrictionLevel(UErrorCode &status) const;

#endif 

    UnicodeString toString() const;

    







    static UnicodeString &displayAlternates(UnicodeString &dest, const UHashtable *alternates, UErrorCode &status);

  private:

    IdentifierInfo  & clear();
    UBool             containsWithAlternates(const ScriptSet &container, const ScriptSet &containee) const;

    UnicodeString     *fIdentifier;
    ScriptSet         *fRequiredScripts;
    UHashtable        *fScriptSetSet;
    ScriptSet         *fCommonAmongAlternates;
    UnicodeSet        *fNumerics;
    UnicodeSet        *fIdentifierProfile;
};

U_NAMESPACE_END

#endif 

