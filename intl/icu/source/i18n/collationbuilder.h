










#ifndef __COLLATIONBUILDER_H__
#define __COLLATIONBUILDER_H__

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/uniset.h"
#include "unicode/unistr.h"
#include "collationrootelements.h"
#include "collationruleparser.h"
#include "uvectr32.h"
#include "uvectr64.h"

struct UParseError;

U_NAMESPACE_BEGIN

struct CollationData;
struct CollationTailoring;

class CEFinalizer;
class CollationDataBuilder;
class Normalizer2;
class Normalizer2Impl;

class U_I18N_API CollationBuilder : public CollationRuleParser::Sink {
public:
    CollationBuilder(const CollationTailoring *base, UErrorCode &errorCode);
    virtual ~CollationBuilder();

    void disableFastLatin() { fastLatinEnabled = FALSE; }

    CollationTailoring *parseAndBuild(const UnicodeString &ruleString,
                                      const UVersionInfo rulesVersion,
                                      CollationRuleParser::Importer *importer,
                                      UParseError *outParseError,
                                      UErrorCode &errorCode);

    const char *getErrorReason() const { return errorReason; }

private:
    friend class CEFinalizer;

    
    virtual void addReset(int32_t strength, const UnicodeString &str,
                          const char *&errorReason, UErrorCode &errorCode);
    



    uint32_t getWeight16Before(int32_t index, int64_t node, int32_t level);

    int64_t getSpecialResetPosition(const UnicodeString &str,
                                    const char *&parserErrorReason, UErrorCode &errorCode);

    
    virtual void addRelation(int32_t strength, const UnicodeString &prefix,
                             const UnicodeString &str, const UnicodeString &extension,
                             const char *&errorReason, UErrorCode &errorCode);

    



    int32_t findOrInsertNodeForCEs(int32_t strength, const char *&parserErrorReason,
                                   UErrorCode &errorCode);
    int32_t findOrInsertNodeForRootCE(int64_t ce, int32_t strength, UErrorCode &errorCode);
    
    int32_t findOrInsertNodeForPrimary(uint32_t p, UErrorCode &errorCode);
    
    int32_t findOrInsertWeakNode(int32_t index, uint32_t weight16, int32_t level,
                                 UErrorCode &errorCode);

    





    int32_t insertTailoredNodeAfter(int32_t index, int32_t strength, UErrorCode &errorCode);

    




    int32_t insertNodeBetween(int32_t index, int32_t nextIndex, int64_t node,
                              UErrorCode &errorCode);

    






    int32_t findCommonNode(int32_t index, int32_t strength) const;

    void setCaseBits(const UnicodeString &nfdString,
                     const char *&parserErrorReason, UErrorCode &errorCode);

    
    virtual void suppressContractions(const UnicodeSet &set, const char *&parserErrorReason,
                                      UErrorCode &errorCode);

    
    virtual void optimize(const UnicodeSet &set, const char *&parserErrorReason,
                          UErrorCode &errorCode);

    




    uint32_t addWithClosure(const UnicodeString &nfdPrefix, const UnicodeString &nfdString,
                            const int64_t newCEs[], int32_t newCEsLength, uint32_t ce32,
                            UErrorCode &errorCode);
    uint32_t addOnlyClosure(const UnicodeString &nfdPrefix, const UnicodeString &nfdString,
                            const int64_t newCEs[], int32_t newCEsLength, uint32_t ce32,
                            UErrorCode &errorCode);
    void addTailComposites(const UnicodeString &nfdPrefix, const UnicodeString &nfdString,
                           UErrorCode &errorCode);
    UBool mergeCompositeIntoString(const UnicodeString &nfdString, int32_t indexAfterLastStarter,
                                   UChar32 composite, const UnicodeString &decomp,
                                   UnicodeString &newNFDString, UnicodeString &newString,
                                   UErrorCode &errorCode) const;

    UBool ignorePrefix(const UnicodeString &s, UErrorCode &errorCode) const;
    UBool ignoreString(const UnicodeString &s, UErrorCode &errorCode) const;
    UBool isFCD(const UnicodeString &s, UErrorCode &errorCode) const;

    void closeOverComposites(UErrorCode &errorCode);

    uint32_t addIfDifferent(const UnicodeString &prefix, const UnicodeString &str,
                            const int64_t newCEs[], int32_t newCEsLength, uint32_t ce32,
                            UErrorCode &errorCode);
    static UBool sameCEs(const int64_t ces1[], int32_t ces1Length,
                         const int64_t ces2[], int32_t ces2Length);

    




    void makeTailoredCEs(UErrorCode &errorCode);
    



    static int32_t countTailoredNodes(const int64_t *nodesArray, int32_t i, int32_t strength);

    
    void finalizeCEs(UErrorCode &errorCode);

    












    static inline int64_t tempCEFromIndexAndStrength(int32_t index, int32_t strength) {
        return
            
            INT64_C(0x4040000006002000) +
            
            ((int64_t)(index & 0xfe000) << 43) +
            
            ((int64_t)(index & 0x1fc0) << 42) +
            
            ((index & 0x3f) << 24) +
            
            (strength << 8);
    }
    static inline int32_t indexFromTempCE(int64_t tempCE) {
        tempCE -= INT64_C(0x4040000006002000);
        return
            ((int32_t)(tempCE >> 43) & 0xfe000) |
            ((int32_t)(tempCE >> 42) & 0x1fc0) |
            ((int32_t)(tempCE >> 24) & 0x3f);
    }
    static inline int32_t strengthFromTempCE(int64_t tempCE) {
        return ((int32_t)tempCE >> 8) & 3;
    }
    static inline UBool isTempCE(int64_t ce) {
        uint32_t sec = (uint32_t)ce >> 24;
        return 6 <= sec && sec <= 0x45;
    }

    static inline int32_t indexFromTempCE32(uint32_t tempCE32) {
        tempCE32 -= 0x40400620;
        return
            ((int32_t)(tempCE32 >> 11) & 0xfe000) |
            ((int32_t)(tempCE32 >> 10) & 0x1fc0) |
            ((int32_t)(tempCE32 >> 8) & 0x3f);
    }
    static inline UBool isTempCE32(uint32_t ce32) {
        return
            (ce32 & 0xff) >= 2 &&  
            6 <= ((ce32 >> 8) & 0xff) && ((ce32 >> 8) & 0xff) <= 0x45;
    }

    static int32_t ceStrength(int64_t ce);

    
    static const int32_t MAX_INDEX = 0xfffff;
    



    static const int32_t HAS_BEFORE2 = 0x40;
    



    static const int32_t HAS_BEFORE3 = 0x20;
    



    static const int32_t IS_TAILORED = 8;

    static inline int64_t nodeFromWeight32(uint32_t weight32) {
        return (int64_t)weight32 << 32;
    }
    static inline int64_t nodeFromWeight16(uint32_t weight16) {
        return (int64_t)weight16 << 48;
    }
    static inline int64_t nodeFromPreviousIndex(int32_t previous) {
        return (int64_t)previous << 28;
    }
    static inline int64_t nodeFromNextIndex(int32_t next) {
        return next << 8;
    }
    static inline int64_t nodeFromStrength(int32_t strength) {
        return strength;
    }

    static inline uint32_t weight32FromNode(int64_t node) {
        return (uint32_t)(node >> 32);
    }
    static inline uint32_t weight16FromNode(int64_t node) {
        return (uint32_t)(node >> 48) & 0xffff;
    }
    static inline int32_t previousIndexFromNode(int64_t node) {
        return (int32_t)(node >> 28) & MAX_INDEX;
    }
    static inline int32_t nextIndexFromNode(int64_t node) {
        return ((int32_t)node >> 8) & MAX_INDEX;
    }
    static inline int32_t strengthFromNode(int64_t node) {
        return (int32_t)node & 3;
    }

    static inline UBool nodeHasBefore2(int64_t node) {
        return (node & HAS_BEFORE2) != 0;
    }
    static inline UBool nodeHasBefore3(int64_t node) {
        return (node & HAS_BEFORE3) != 0;
    }
    static inline UBool nodeHasAnyBefore(int64_t node) {
        return (node & (HAS_BEFORE2 | HAS_BEFORE3)) != 0;
    }
    static inline UBool isTailoredNode(int64_t node) {
        return (node & IS_TAILORED) != 0;
    }

    static inline int64_t changeNodePreviousIndex(int64_t node, int32_t previous) {
        return (node & INT64_C(0xffff00000fffffff)) | nodeFromPreviousIndex(previous);
    }
    static inline int64_t changeNodeNextIndex(int64_t node, int32_t next) {
        return (node & INT64_C(0xfffffffff00000ff)) | nodeFromNextIndex(next);
    }

    const Normalizer2 &nfd, &fcd;
    const Normalizer2Impl &nfcImpl;

    const CollationTailoring *base;
    const CollationData *baseData;
    const CollationRootElements rootElements;
    uint32_t variableTop;

    CollationDataBuilder *dataBuilder;
    UBool fastLatinEnabled;
    UnicodeSet optimizeSet;
    const char *errorReason;

    int64_t ces[Collation::MAX_EXPANSION_LENGTH];
    int32_t cesLength;

    








    UVector32 rootPrimaryIndexes;
    

















































































    UVector64 nodes;
};

U_NAMESPACE_END

#endif  
#endif  
