








#ifndef _TRANSREG_H
#define _TRANSREG_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/uobject.h"
#include "unicode/translit.h"
#include "hash.h"
#include "uvector.h"

U_NAMESPACE_BEGIN

class TransliteratorEntry;
class TransliteratorSpec;
class UnicodeString;














class TransliteratorAlias : public UMemory {
 public:
    



    TransliteratorAlias(const UnicodeString& aliasID, const UnicodeSet* compoundFilter);

    


    TransliteratorAlias(const UnicodeString& ID, const UnicodeString& idBlocks,
                        UVector* adoptedTransliterators,
                        const UnicodeSet* compoundFilter);

    


    TransliteratorAlias(const UnicodeString& theID,
                        const UnicodeString& rules,
                        UTransDirection dir);

    ~TransliteratorAlias();

    









    Transliterator* create(UParseError&, UErrorCode&);

    



    UBool isRuleBased() const;

    










    void parse(TransliteratorParser& parser,
               UParseError& pe, UErrorCode& ec) const;

 private:
    
    
    
    
    
    
    
    
    
    
    
    
    UnicodeString ID;
    UnicodeString aliasesOrRules;
    UVector* transes; 
    const UnicodeSet* compoundFilter; 
    UTransDirection direction;
    enum { SIMPLE, COMPOUND, RULES } type;

    TransliteratorAlias(const TransliteratorAlias &other); 
    TransliteratorAlias &operator=(const TransliteratorAlias &other); 
};




















class TransliteratorRegistry : public UMemory {

 public:

    



    TransliteratorRegistry(UErrorCode& status);

    


    ~TransliteratorRegistry();

    
    
    

    















    Transliterator* get(const UnicodeString& ID,
                        TransliteratorAlias*& aliasReturn,
                        UErrorCode& status);

    














    Transliterator* reget(const UnicodeString& ID,
                          TransliteratorParser& parser,
                          TransliteratorAlias*& aliasReturn,
                          UErrorCode& status);

    




    void put(Transliterator* adoptedProto,
             UBool visible,
             UErrorCode& ec);

    




    void put(const UnicodeString& ID,
             Transliterator::Factory factory,
             Transliterator::Token context,
             UBool visible,
             UErrorCode& ec);

    




    void put(const UnicodeString& ID,
             const UnicodeString& resourceName,
             UTransDirection dir,
             UBool readonlyResourceAlias,
             UBool visible,
             UErrorCode& ec);

    




    void put(const UnicodeString& ID,
             const UnicodeString& alias,
             UBool readonlyAliasAlias,
             UBool visible,
             UErrorCode& ec);

    





    void remove(const UnicodeString& ID);

    
    
    

    




    StringEnumeration* getAvailableIDs() const;

    







    int32_t countAvailableIDs(void) const;

    










    const UnicodeString& getAvailableID(int32_t index) const;

    



    int32_t countAvailableSources(void) const;

    







    UnicodeString& getAvailableSource(int32_t index,
                                      UnicodeString& result) const;

    






    int32_t countAvailableTargets(const UnicodeString& source) const;

    









    UnicodeString& getAvailableTarget(int32_t index,
                                      const UnicodeString& source,
                                      UnicodeString& result) const;

    










    int32_t countAvailableVariants(const UnicodeString& source,
                                   const UnicodeString& target) const;

    












    UnicodeString& getAvailableVariant(int32_t index,
                                       const UnicodeString& source,
                                       const UnicodeString& target,
                                       UnicodeString& result) const;

 private:

    
    
    

    TransliteratorEntry* find(const UnicodeString& ID);

    TransliteratorEntry* find(UnicodeString& source,
                UnicodeString& target,
                UnicodeString& variant);

    TransliteratorEntry* findInDynamicStore(const TransliteratorSpec& src,
                              const TransliteratorSpec& trg,
                              const UnicodeString& variant) const;

    TransliteratorEntry* findInStaticStore(const TransliteratorSpec& src,
                             const TransliteratorSpec& trg,
                             const UnicodeString& variant);

    static TransliteratorEntry* findInBundle(const TransliteratorSpec& specToOpen,
                               const TransliteratorSpec& specToFind,
                               const UnicodeString& variant,
                               UTransDirection direction);

    void registerEntry(const UnicodeString& source,
                       const UnicodeString& target,
                       const UnicodeString& variant,
                       TransliteratorEntry* adopted,
                       UBool visible);

    void registerEntry(const UnicodeString& ID,
                       TransliteratorEntry* adopted,
                       UBool visible);

    void registerEntry(const UnicodeString& ID,
                       const UnicodeString& source,
                       const UnicodeString& target,
                       const UnicodeString& variant,
                       TransliteratorEntry* adopted,
                       UBool visible);

    void registerSTV(const UnicodeString& source,
                     const UnicodeString& target,
                     const UnicodeString& variant);

    void removeSTV(const UnicodeString& source,
                   const UnicodeString& target,
                   const UnicodeString& variant);

    Transliterator* instantiateEntry(const UnicodeString& ID,
                                     TransliteratorEntry *entry,
                                     TransliteratorAlias*& aliasReturn,
                                     UErrorCode& status);

    


    class Enumeration : public StringEnumeration {
    public:
        Enumeration(const TransliteratorRegistry& reg);
        virtual ~Enumeration();
        virtual int32_t count(UErrorCode& status) const;
        virtual const UnicodeString* snext(UErrorCode& status);
        virtual void reset(UErrorCode& status);
        static UClassID U_EXPORT2 getStaticClassID();
        virtual UClassID getDynamicClassID() const;
    private:
        int32_t index;
        const TransliteratorRegistry& reg;
    };
    friend class Enumeration;

 private:

    





    Hashtable registry;

    






    Hashtable specDAG;

    


    UVector availableIDs;

    TransliteratorRegistry(const TransliteratorRegistry &other); 
    TransliteratorRegistry &operator=(const TransliteratorRegistry &other); 
};

U_NAMESPACE_END

#endif 

#endif

