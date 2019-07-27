










#ifndef __COLLATIONSETS_H__
#define __COLLATIONSETS_H__

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/uniset.h"
#include "collation.h"

U_NAMESPACE_BEGIN

struct CollationData;
















class TailoredSet : public UMemory {
public:
    TailoredSet(UnicodeSet *t)
            : data(NULL), baseData(NULL),
              tailored(t),
              suffix(NULL),
              errorCode(U_ZERO_ERROR) {}

    void forData(const CollationData *d, UErrorCode &errorCode);

    



    UBool handleCE32(UChar32 start, UChar32 end, uint32_t ce32);

private:
    void compare(UChar32 c, uint32_t ce32, uint32_t baseCE32);
    void comparePrefixes(UChar32 c, const UChar *p, const UChar *q);
    void compareContractions(UChar32 c, const UChar *p, const UChar *q);

    void addPrefixes(const CollationData *d, UChar32 c, const UChar *p);
    void addPrefix(const CollationData *d, const UnicodeString &pfx, UChar32 c, uint32_t ce32);
    void addContractions(UChar32 c, const UChar *p);
    void addSuffix(UChar32 c, const UnicodeString &sfx);
    void add(UChar32 c);

    
    void setPrefix(const UnicodeString &pfx) {
        unreversedPrefix = pfx;
        unreversedPrefix.reverse();
    }
    void resetPrefix() {
        unreversedPrefix.remove();
    }

    const CollationData *data;
    const CollationData *baseData;
    UnicodeSet *tailored;
    UnicodeString unreversedPrefix;
    const UnicodeString *suffix;
    UErrorCode errorCode;
};

class ContractionsAndExpansions : public UMemory {
public:
    class CESink : public UMemory {
    public:
        virtual ~CESink();
        virtual void handleCE(int64_t ce) = 0;
        virtual void handleExpansion(const int64_t ces[], int32_t length) = 0;
    };

    ContractionsAndExpansions(UnicodeSet *con, UnicodeSet *exp, CESink *s, UBool prefixes)
            : data(NULL),
              contractions(con), expansions(exp),
              sink(s),
              addPrefixes(prefixes),
              checkTailored(0),
              suffix(NULL),
              errorCode(U_ZERO_ERROR) {}

    void forData(const CollationData *d, UErrorCode &errorCode);
    void forCodePoint(const CollationData *d, UChar32 c, UErrorCode &ec);

    

    void handleCE32(UChar32 start, UChar32 end, uint32_t ce32);

    void handlePrefixes(UChar32 start, UChar32 end, uint32_t ce32);
    void handleContractions(UChar32 start, UChar32 end, uint32_t ce32);

    void addExpansions(UChar32 start, UChar32 end);
    void addStrings(UChar32 start, UChar32 end, UnicodeSet *set);

    
    void setPrefix(const UnicodeString &pfx) {
        unreversedPrefix = pfx;
        unreversedPrefix.reverse();
    }
    void resetPrefix() {
        unreversedPrefix.remove();
    }

    const CollationData *data;
    UnicodeSet *contractions;
    UnicodeSet *expansions;
    CESink *sink;
    UBool addPrefixes;
    int8_t checkTailored;  
    UnicodeSet tailored;
    UnicodeSet ranges;
    UnicodeString unreversedPrefix;
    const UnicodeString *suffix;
    int64_t ces[Collation::MAX_EXPANSION_LENGTH];
    UErrorCode errorCode;
};

U_NAMESPACE_END

#endif  
#endif  
