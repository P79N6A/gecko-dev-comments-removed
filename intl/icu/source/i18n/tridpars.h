








#ifndef TRIDPARS_H
#define TRIDPARS_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/uobject.h"
#include "unicode/unistr.h"

U_NAMESPACE_BEGIN

class Transliterator;
class UnicodeSet;
class UVector;























class TransliteratorIDParser  {

 public:

    













    class Specs : public UMemory {
    public:
        UnicodeString source; 
        UnicodeString target; 
        UnicodeString variant; 
        UnicodeString filter; 
        UBool sawSource;
        Specs(const UnicodeString& s, const UnicodeString& t,
              const UnicodeString& v, UBool sawS,
              const UnicodeString& f);

    private:

        Specs(const Specs &other); 
        Specs &operator=(const Specs &other); 
    };

    














    class SingleID : public UMemory {
    public:
        UnicodeString canonID;
        UnicodeString basicID;
        UnicodeString filter;
        SingleID(const UnicodeString& c, const UnicodeString& b,
                 const UnicodeString& f);
        SingleID(const UnicodeString& c, const UnicodeString& b);
        Transliterator* createInstance();

    private:

        SingleID(const SingleID &other); 
        SingleID &operator=(const SingleID &other); 
    };

    








    static SingleID* parseFilterID(const UnicodeString& id, int32_t& pos);

    











    static SingleID* parseSingleID(const UnicodeString& id, int32_t& pos,
                                  int32_t dir, UErrorCode& status);

    





















    static UnicodeSet* parseGlobalFilter(const UnicodeString& id, int32_t& pos,
                                         int32_t dir,
                                         int32_t& withParens,
                                         UnicodeString* canonID);

    





















    static UBool parseCompoundID(const UnicodeString& id, int32_t dir,
                                 UnicodeString& canonID,
                                 UVector& list,
                                 UnicodeSet*& globalFilter);

    

















    static void instantiateList(UVector& list,
                                UErrorCode& ec);

    
















    static void IDtoSTV(const UnicodeString& id,
                        UnicodeString& source,
                        UnicodeString& target,
                        UnicodeString& variant,
                        UBool& isSourcePresent);

    




    static void STVtoID(const UnicodeString& source,
                        const UnicodeString& target,
                        const UnicodeString& variant,
                        UnicodeString& id);

    































    static void registerSpecialInverse(const UnicodeString& target,
                                       const UnicodeString& inverseTarget,
                                       UBool bidirectional,
                                       UErrorCode &status);

    


    static void cleanup();

 private:
    
    
    

    
    TransliteratorIDParser();

    


















    static Specs* parseFilterID(const UnicodeString& id, int32_t& pos,
                                UBool allowFilter);

    








    static SingleID* specsToID(const Specs* specs, int32_t dir);

    







    static SingleID* specsToSpecialInverse(const Specs& specs, UErrorCode &status);

    




    static Transliterator* createBasicInstance(const UnicodeString& id,
                                               const UnicodeString* canonID);

    


    static void init(UErrorCode &status);

    friend class SingleID;
};

U_NAMESPACE_END

#endif 

#endif
