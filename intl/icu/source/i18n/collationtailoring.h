










#ifndef __COLLATIONTAILORING_H__
#define __COLLATIONTAILORING_H__

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/locid.h"
#include "unicode/unistr.h"
#include "unicode/uversion.h"
#include "collationsettings.h"
#include "uhash.h"
#include "umutex.h"

struct UDataMemory;
struct UResourceBundle;
struct UTrie2;

U_NAMESPACE_BEGIN

struct CollationData;

class UnicodeSet;











struct U_I18N_API CollationTailoring : public SharedObject {
    CollationTailoring(const CollationSettings *baseSettings);
    virtual ~CollationTailoring();

    


    UBool isBogus() { return settings == NULL; }

    UBool ensureOwnedData(UErrorCode &errorCode);

    static void makeBaseVersion(const UVersionInfo ucaVersion, UVersionInfo version);
    void setVersion(const UVersionInfo baseVersion, const UVersionInfo rulesVersion);
    int32_t getUCAVersion() const;

    
    const CollationData *data;  
    const CollationSettings *settings;  
    UnicodeString rules;
    
    
    mutable Locale actualLocale;
    
    
    
    
    
    UVersionInfo version;

    
    CollationData *ownedData;
    UObject *builder;
    UDataMemory *memory;
    UResourceBundle *bundle;
    UTrie2 *trie;
    UnicodeSet *unsafeBackwardSet;
    mutable UHashtable *maxExpansions;
    mutable UInitOnce maxExpansionsInitOnce;

private:
    



    CollationTailoring(const CollationTailoring &other);
};

struct CollationCacheEntry : public SharedObject {
    CollationCacheEntry(const Locale &loc, const CollationTailoring *t)
            : validLocale(loc), tailoring(t) {
        if(t != NULL) {
            t->addRef();
        }
    }
    ~CollationCacheEntry();

    Locale validLocale;
    const CollationTailoring *tailoring;
};

U_NAMESPACE_END

#endif  
#endif  
