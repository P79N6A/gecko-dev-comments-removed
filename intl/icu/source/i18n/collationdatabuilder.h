










#ifndef __COLLATIONDATABUILDER_H__
#define __COLLATIONDATABUILDER_H__

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/uniset.h"
#include "unicode/unistr.h"
#include "unicode/uversion.h"
#include "collation.h"
#include "collationdata.h"
#include "collationsettings.h"
#include "normalizer2impl.h"
#include "utrie2.h"
#include "uvectr32.h"
#include "uvectr64.h"
#include "uvector.h"

U_NAMESPACE_BEGIN

struct ConditionalCE32;

class CollationFastLatinBuilder;
class CopyHelper;
class DataBuilderCollationIterator;
class UCharsTrieBuilder;






class U_I18N_API CollationDataBuilder : public UObject {
public:
    




    class CEModifier : public UObject {
    public:
        virtual ~CEModifier();
        
        virtual int64_t modifyCE32(uint32_t ce32) const = 0;
        
        virtual int64_t modifyCE(int64_t ce) const = 0;
    };

    CollationDataBuilder(UErrorCode &errorCode);

    virtual ~CollationDataBuilder();

    void initForTailoring(const CollationData *b, UErrorCode &errorCode);

    virtual UBool isCompressibleLeadByte(uint32_t b) const;

    inline UBool isCompressiblePrimary(uint32_t p) const {
        return isCompressibleLeadByte(p >> 24);
    }

    


    UBool hasMappings() const { return modified; }

    


    UBool isAssigned(UChar32 c) const;

    



    uint32_t getLongPrimaryIfSingleCE(UChar32 c) const;

    



    int64_t getSingleCE(UChar32 c, UErrorCode &errorCode) const;

    void add(const UnicodeString &prefix, const UnicodeString &s,
             const int64_t ces[], int32_t cesLength,
             UErrorCode &errorCode);

    





    virtual uint32_t encodeCEs(const int64_t ces[], int32_t cesLength, UErrorCode &errorCode);
    void addCE32(const UnicodeString &prefix, const UnicodeString &s,
                 uint32_t ce32, UErrorCode &errorCode);

    











    UBool maybeSetPrimaryRange(UChar32 start, UChar32 end,
                               uint32_t primary, int32_t step,
                               UErrorCode &errorCode);

    











    uint32_t setPrimaryRangeAndReturnNext(UChar32 start, UChar32 end,
                                          uint32_t primary, int32_t step,
                                          UErrorCode &errorCode);

    



    void copyFrom(const CollationDataBuilder &src, const CEModifier &modifier,
                  UErrorCode &errorCode);

    void optimize(const UnicodeSet &set, UErrorCode &errorCode);
    void suppressContractions(const UnicodeSet &set, UErrorCode &errorCode);

    void enableFastLatin() { fastLatinEnabled = TRUE; }
    virtual void build(CollationData &data, UErrorCode &errorCode);

    








    int32_t getCEs(const UnicodeString &s, int64_t ces[], int32_t cesLength);
    int32_t getCEs(const UnicodeString &prefix, const UnicodeString &s,
                   int64_t ces[], int32_t cesLength);

protected:
    friend class CopyHelper;
    friend class DataBuilderCollationIterator;

    uint32_t getCE32FromOffsetCE32(UBool fromBase, UChar32 c, uint32_t ce32) const;

    int32_t addCE(int64_t ce, UErrorCode &errorCode);
    int32_t addCE32(uint32_t ce32, UErrorCode &errorCode);
    int32_t addConditionalCE32(const UnicodeString &context, uint32_t ce32, UErrorCode &errorCode);

    inline ConditionalCE32 *getConditionalCE32(int32_t index) const {
        return static_cast<ConditionalCE32 *>(conditionalCE32s[index]);
    }
    inline ConditionalCE32 *getConditionalCE32ForCE32(uint32_t ce32) const {
        return getConditionalCE32(Collation::indexFromCE32(ce32));
    }

    static uint32_t makeBuilderContextCE32(int32_t index) {
        return Collation::makeCE32FromTagAndIndex(Collation::BUILDER_DATA_TAG, index);
    }
    static inline UBool isBuilderContextCE32(uint32_t ce32) {
        return Collation::hasCE32Tag(ce32, Collation::BUILDER_DATA_TAG);
    }

    static uint32_t encodeOneCEAsCE32(int64_t ce);
    uint32_t encodeOneCE(int64_t ce, UErrorCode &errorCode);
    uint32_t encodeExpansion(const int64_t ces[], int32_t length, UErrorCode &errorCode);
    uint32_t encodeExpansion32(const int32_t newCE32s[], int32_t length, UErrorCode &errorCode);

    uint32_t copyFromBaseCE32(UChar32 c, uint32_t ce32, UBool withContext, UErrorCode &errorCode);
    




    int32_t copyContractionsFromBaseCE32(UnicodeString &context, UChar32 c, uint32_t ce32,
                                         ConditionalCE32 *cond, UErrorCode &errorCode);

    UBool getJamoCE32s(uint32_t jamoCE32s[], UErrorCode &errorCode);
    void setDigitTags(UErrorCode &errorCode);
    void setLeadSurrogates(UErrorCode &errorCode);

    void buildMappings(CollationData &data, UErrorCode &errorCode);

    void clearContexts();
    void buildContexts(UErrorCode &errorCode);
    uint32_t buildContext(ConditionalCE32 *head, UErrorCode &errorCode);
    int32_t addContextTrie(uint32_t defaultCE32, UCharsTrieBuilder &trieBuilder,
                           UErrorCode &errorCode);

    void buildFastLatinTable(CollationData &data, UErrorCode &errorCode);

    int32_t getCEs(const UnicodeString &s, int32_t start, int64_t ces[], int32_t cesLength);

    static UChar32 jamoCpFromIndex(int32_t i) {
        
        if(i < Hangul::JAMO_L_COUNT) { return Hangul::JAMO_L_BASE + i; }
        i -= Hangul::JAMO_L_COUNT;
        if(i < Hangul::JAMO_V_COUNT) { return Hangul::JAMO_V_BASE + i; }
        i -= Hangul::JAMO_V_COUNT;
        
        return Hangul::JAMO_T_BASE + 1 + i;
    }

    
    static const uint32_t IS_BUILDER_JAMO_CE32 = 0x100;

    const Normalizer2Impl &nfcImpl;
    const CollationData *base;
    const CollationSettings *baseSettings;
    UTrie2 *trie;
    UVector32 ce32s;
    UVector64 ce64s;
    UVector conditionalCE32s;  
    
    UnicodeSet contextChars;
    
    UnicodeString contexts;
    UnicodeSet unsafeBackwardSet;
    UBool modified;

    UBool fastLatinEnabled;
    CollationFastLatinBuilder *fastLatinBuilder;

    DataBuilderCollationIterator *collIter;
};

U_NAMESPACE_END

#endif  
#endif  
