















#ifndef __NORMALIZER2IMPL_H__
#define __NORMALIZER2IMPL_H__

#include "unicode/utypes.h"

#if !UCONFIG_NO_NORMALIZATION

#include "unicode/normalizer2.h"
#include "unicode/udata.h"
#include "unicode/unistr.h"
#include "unicode/unorm.h"
#include "unicode/utf16.h"
#include "mutex.h"
#include "uset_imp.h"
#include "utrie2.h"

U_NAMESPACE_BEGIN

struct CanonIterData;

class Hangul {
public:
    
    enum {
        JAMO_L_BASE=0x1100,     
        JAMO_V_BASE=0x1161,     
        JAMO_T_BASE=0x11a7,     

        HANGUL_BASE=0xac00,

        JAMO_L_COUNT=19,
        JAMO_V_COUNT=21,
        JAMO_T_COUNT=28,

        JAMO_VT_COUNT=JAMO_V_COUNT*JAMO_T_COUNT,

        HANGUL_COUNT=JAMO_L_COUNT*JAMO_V_COUNT*JAMO_T_COUNT,
        HANGUL_LIMIT=HANGUL_BASE+HANGUL_COUNT
    };

    static inline UBool isHangul(UChar32 c) {
        return HANGUL_BASE<=c && c<HANGUL_LIMIT;
    }
    static inline UBool
    isHangulWithoutJamoT(UChar c) {
        c-=HANGUL_BASE;
        return c<HANGUL_COUNT && c%JAMO_T_COUNT==0;
    }
    static inline UBool isJamoL(UChar32 c) {
        return (uint32_t)(c-JAMO_L_BASE)<JAMO_L_COUNT;
    }
    static inline UBool isJamoV(UChar32 c) {
        return (uint32_t)(c-JAMO_V_BASE)<JAMO_V_COUNT;
    }

    



    static inline int32_t decompose(UChar32 c, UChar buffer[3]) {
        c-=HANGUL_BASE;
        UChar32 c2=c%JAMO_T_COUNT;
        c/=JAMO_T_COUNT;
        buffer[0]=(UChar)(JAMO_L_BASE+c/JAMO_V_COUNT);
        buffer[1]=(UChar)(JAMO_V_BASE+c%JAMO_V_COUNT);
        if(c2==0) {
            return 2;
        } else {
            buffer[2]=(UChar)(JAMO_T_BASE+c2);
            return 3;
        }
    }

    



    static inline void getRawDecomposition(UChar32 c, UChar buffer[2]) {
        UChar32 orig=c;
        c-=HANGUL_BASE;
        UChar32 c2=c%JAMO_T_COUNT;
        if(c2==0) {
            c/=JAMO_T_COUNT;
            buffer[0]=(UChar)(JAMO_L_BASE+c/JAMO_V_COUNT);
            buffer[1]=(UChar)(JAMO_V_BASE+c%JAMO_V_COUNT);
        } else {
            buffer[0]=orig-c2;  
            buffer[1]=(UChar)(JAMO_T_BASE+c2);
        }
    }
private:
    Hangul();  
};

class Normalizer2Impl;

class ReorderingBuffer : public UMemory {
public:
    ReorderingBuffer(const Normalizer2Impl &ni, UnicodeString &dest) :
        impl(ni), str(dest),
        start(NULL), reorderStart(NULL), limit(NULL),
        remainingCapacity(0), lastCC(0) {}
    ~ReorderingBuffer() {
        if(start!=NULL) {
            str.releaseBuffer((int32_t)(limit-start));
        }
    }
    UBool init(int32_t destCapacity, UErrorCode &errorCode);

    UBool isEmpty() const { return start==limit; }
    int32_t length() const { return (int32_t)(limit-start); }
    UChar *getStart() { return start; }
    UChar *getLimit() { return limit; }
    uint8_t getLastCC() const { return lastCC; }

    UBool equals(const UChar *start, const UChar *limit) const;

    
    void setLastChar(UChar c) {
        *(limit-1)=c;
    }

    UBool append(UChar32 c, uint8_t cc, UErrorCode &errorCode) {
        return (c<=0xffff) ?
            appendBMP((UChar)c, cc, errorCode) :
            appendSupplementary(c, cc, errorCode);
    }
    
    UBool append(const UChar *s, int32_t length,
                 uint8_t leadCC, uint8_t trailCC,
                 UErrorCode &errorCode);
    UBool appendBMP(UChar c, uint8_t cc, UErrorCode &errorCode) {
        if(remainingCapacity==0 && !resize(1, errorCode)) {
            return FALSE;
        }
        if(lastCC<=cc || cc==0) {
            *limit++=c;
            lastCC=cc;
            if(cc<=1) {
                reorderStart=limit;
            }
        } else {
            insert(c, cc);
        }
        --remainingCapacity;
        return TRUE;
    }
    UBool appendZeroCC(UChar32 c, UErrorCode &errorCode);
    UBool appendZeroCC(const UChar *s, const UChar *sLimit, UErrorCode &errorCode);
    void remove();
    void removeSuffix(int32_t suffixLength);
    void setReorderingLimit(UChar *newLimit) {
        remainingCapacity+=(int32_t)(limit-newLimit);
        reorderStart=limit=newLimit;
        lastCC=0;
    }
    void copyReorderableSuffixTo(UnicodeString &s) const {
        s.setTo(reorderStart, (int32_t)(limit-reorderStart));
    }
private:
    












    UBool appendSupplementary(UChar32 c, uint8_t cc, UErrorCode &errorCode);
    void insert(UChar32 c, uint8_t cc);
    static void writeCodePoint(UChar *p, UChar32 c) {
        if(c<=0xffff) {
            *p=(UChar)c;
        } else {
            p[0]=U16_LEAD(c);
            p[1]=U16_TRAIL(c);
        }
    }
    UBool resize(int32_t appendLength, UErrorCode &errorCode);

    const Normalizer2Impl &impl;
    UnicodeString &str;
    UChar *start, *reorderStart, *limit;
    int32_t remainingCapacity;
    uint8_t lastCC;

    
    void setIterator() { codePointStart=limit; }
    void skipPrevious();  
    uint8_t previousCC();  

    UChar *codePointStart, *codePointLimit;
};

class U_COMMON_API Normalizer2Impl : public UMemory {
public:
    Normalizer2Impl() : memory(NULL), normTrie(NULL) {
        canonIterDataSingleton.fInstance=NULL;
    }
    ~Normalizer2Impl();

    void load(const char *packageName, const char *name, UErrorCode &errorCode);

    void addPropertyStarts(const USetAdder *sa, UErrorCode &errorCode) const;
    void addCanonIterPropertyStarts(const USetAdder *sa, UErrorCode &errorCode) const;

    

    const UTrie2 *getNormTrie() const { return normTrie; }

    UBool ensureCanonIterData(UErrorCode &errorCode) const;

    uint16_t getNorm16(UChar32 c) const { return UTRIE2_GET16(normTrie, c); }

    UNormalizationCheckResult getCompQuickCheck(uint16_t norm16) const {
        if(norm16<minNoNo || MIN_YES_YES_WITH_CC<=norm16) {
            return UNORM_YES;
        } else if(minMaybeYes<=norm16) {
            return UNORM_MAYBE;
        } else {
            return UNORM_NO;
        }
    }
    UBool isCompNo(uint16_t norm16) const { return minNoNo<=norm16 && norm16<minMaybeYes; }
    UBool isDecompYes(uint16_t norm16) const { return norm16<minYesNo || minMaybeYes<=norm16; }

    uint8_t getCC(uint16_t norm16) const {
        if(norm16>=MIN_NORMAL_MAYBE_YES) {
            return (uint8_t)norm16;
        }
        if(norm16<minNoNo || limitNoNo<=norm16) {
            return 0;
        }
        return getCCFromNoNo(norm16);
    }
    static uint8_t getCCFromYesOrMaybe(uint16_t norm16) {
        return norm16>=MIN_NORMAL_MAYBE_YES ? (uint8_t)norm16 : 0;
    }

    




    uint16_t getFCD16(UChar32 c) const {
        if(c<0) {
            return 0;
        } else if(c<0x180) {
            return tccc180[c];
        } else if(c<=0xffff) {
            if(!singleLeadMightHaveNonZeroFCD16(c)) { return 0; }
        }
        return getFCD16FromNormData(c);
    }
    







    uint16_t nextFCD16(const UChar *&s, const UChar *limit) const {
        UChar32 c=*s++;
        if(c<0x180) {
            return tccc180[c];
        } else if(!singleLeadMightHaveNonZeroFCD16(c)) {
            return 0;
        }
        UChar c2;
        if(U16_IS_LEAD(c) && s!=limit && U16_IS_TRAIL(c2=*s)) {
            c=U16_GET_SUPPLEMENTARY(c, c2);
            ++s;
        }
        return getFCD16FromNormData(c);
    }
    





    uint16_t previousFCD16(const UChar *start, const UChar *&s) const {
        UChar32 c=*--s;
        if(c<0x180) {
            return tccc180[c];
        }
        if(!U16_IS_TRAIL(c)) {
            if(!singleLeadMightHaveNonZeroFCD16(c)) {
                return 0;
            }
        } else {
            UChar c2;
            if(start<s && U16_IS_LEAD(c2=*(s-1))) {
                c=U16_GET_SUPPLEMENTARY(c2, c);
                --s;
            }
        }
        return getFCD16FromNormData(c);
    }

    
    uint16_t getFCD16FromBelow180(UChar32 c) const { return tccc180[c]; }
    
    UBool singleLeadMightHaveNonZeroFCD16(UChar32 lead) const {
        
        uint8_t bits=smallFCD[lead>>8];
        if(bits==0) { return false; }
        return (UBool)((bits>>((lead>>5)&7))&1);
    }
    
    uint16_t getFCD16FromNormData(UChar32 c) const;

    void makeCanonIterDataFromNorm16(UChar32 start, UChar32 end, uint16_t norm16,
                                     CanonIterData &newData, UErrorCode &errorCode) const;

    






    const UChar *getDecomposition(UChar32 c, UChar buffer[4], int32_t &length) const;

    






    const UChar *getRawDecomposition(UChar32 c, UChar buffer[30], int32_t &length) const;

    UChar32 composePair(UChar32 a, UChar32 b) const;

    UBool isCanonSegmentStarter(UChar32 c) const;
    UBool getCanonStartSet(UChar32 c, UnicodeSet &set) const;

    enum {
        MIN_CCC_LCCC_CP=0x300
    };

    enum {
        MIN_YES_YES_WITH_CC=0xff01,
        JAMO_VT=0xff00,
        MIN_NORMAL_MAYBE_YES=0xfe00,
        JAMO_L=1,
        MAX_DELTA=0x40
    };

    enum {
        
        IX_NORM_TRIE_OFFSET,
        IX_EXTRA_DATA_OFFSET,
        IX_SMALL_FCD_OFFSET,
        IX_RESERVED3_OFFSET,
        IX_RESERVED4_OFFSET,
        IX_RESERVED5_OFFSET,
        IX_RESERVED6_OFFSET,
        IX_TOTAL_SIZE,

        
        IX_MIN_DECOMP_NO_CP,
        IX_MIN_COMP_NO_MAYBE_CP,

        
        IX_MIN_YES_NO,  
        IX_MIN_NO_NO,
        IX_LIMIT_NO_NO,
        IX_MIN_MAYBE_YES,

        IX_MIN_YES_NO_MAPPINGS_ONLY,  

        IX_RESERVED15,
        IX_COUNT
    };

    enum {
        MAPPING_HAS_CCC_LCCC_WORD=0x80,
        MAPPING_HAS_RAW_MAPPING=0x40,
        MAPPING_NO_COMP_BOUNDARY_AFTER=0x20,
        MAPPING_LENGTH_MASK=0x1f
    };

    enum {
        COMP_1_LAST_TUPLE=0x8000,
        COMP_1_TRIPLE=1,
        COMP_1_TRAIL_LIMIT=0x3400,
        COMP_1_TRAIL_MASK=0x7ffe,
        COMP_1_TRAIL_SHIFT=9,  
        COMP_2_TRAIL_SHIFT=6,
        COMP_2_TRAIL_MASK=0xffc0
    };

    

    const UChar *decompose(const UChar *src, const UChar *limit,
                           ReorderingBuffer *buffer, UErrorCode &errorCode) const;
    void decomposeAndAppend(const UChar *src, const UChar *limit,
                            UBool doDecompose,
                            UnicodeString &safeMiddle,
                            ReorderingBuffer &buffer,
                            UErrorCode &errorCode) const;
    UBool compose(const UChar *src, const UChar *limit,
                  UBool onlyContiguous,
                  UBool doCompose,
                  ReorderingBuffer &buffer,
                  UErrorCode &errorCode) const;
    const UChar *composeQuickCheck(const UChar *src, const UChar *limit,
                                   UBool onlyContiguous,
                                   UNormalizationCheckResult *pQCResult) const;
    void composeAndAppend(const UChar *src, const UChar *limit,
                          UBool doCompose,
                          UBool onlyContiguous,
                          UnicodeString &safeMiddle,
                          ReorderingBuffer &buffer,
                          UErrorCode &errorCode) const;
    const UChar *makeFCD(const UChar *src, const UChar *limit,
                         ReorderingBuffer *buffer, UErrorCode &errorCode) const;
    void makeFCDAndAppend(const UChar *src, const UChar *limit,
                          UBool doMakeFCD,
                          UnicodeString &safeMiddle,
                          ReorderingBuffer &buffer,
                          UErrorCode &errorCode) const;

    UBool hasDecompBoundary(UChar32 c, UBool before) const;
    UBool isDecompInert(UChar32 c) const { return isDecompYesAndZeroCC(getNorm16(c)); }

    UBool hasCompBoundaryBefore(UChar32 c) const {
        return c<minCompNoMaybeCP || hasCompBoundaryBefore(c, getNorm16(c));
    }
    UBool hasCompBoundaryAfter(UChar32 c, UBool onlyContiguous, UBool testInert) const;

    UBool hasFCDBoundaryBefore(UChar32 c) const { return c<MIN_CCC_LCCC_CP || getFCD16(c)<=0xff; }
    UBool hasFCDBoundaryAfter(UChar32 c) const {
        uint16_t fcd16=getFCD16(c);
        return fcd16<=1 || (fcd16&0xff)==0;
    }
    UBool isFCDInert(UChar32 c) const { return getFCD16(c)<=1; }
private:
    static UBool U_CALLCONV
    isAcceptable(void *context, const char *type, const char *name, const UDataInfo *pInfo);

    UBool isMaybe(uint16_t norm16) const { return minMaybeYes<=norm16 && norm16<=JAMO_VT; }
    UBool isMaybeOrNonZeroCC(uint16_t norm16) const { return norm16>=minMaybeYes; }
    static UBool isInert(uint16_t norm16) { return norm16==0; }
    static UBool isJamoL(uint16_t norm16) { return norm16==1; }
    static UBool isJamoVT(uint16_t norm16) { return norm16==JAMO_VT; }
    UBool isHangul(uint16_t norm16) const { return norm16==minYesNo; }
    UBool isCompYesAndZeroCC(uint16_t norm16) const { return norm16<minNoNo; }
    
    
    
    
    
    
    
    
    
    UBool isDecompYesAndZeroCC(uint16_t norm16) const {
        return norm16<minYesNo ||
               norm16==JAMO_VT ||
               (minMaybeYes<=norm16 && norm16<=MIN_NORMAL_MAYBE_YES);
    }
    




    UBool isMostDecompYesAndZeroCC(uint16_t norm16) const {
        return norm16<minYesNo || norm16==MIN_NORMAL_MAYBE_YES || norm16==JAMO_VT;
    }
    UBool isDecompNoAlgorithmic(uint16_t norm16) const { return norm16>=limitNoNo; }

    
    
    
    
    
    uint8_t getCCFromNoNo(uint16_t norm16) const {
        const uint16_t *mapping=getMapping(norm16);
        if(*mapping&MAPPING_HAS_CCC_LCCC_WORD) {
            return (uint8_t)*(mapping-1);
        } else {
            return 0;
        }
    }
    
    uint8_t getTrailCCFromCompYesAndZeroCC(const UChar *cpStart, const UChar *cpLimit) const;

    
    UChar32 mapAlgorithmic(UChar32 c, uint16_t norm16) const {
        return c+norm16-(minMaybeYes-MAX_DELTA-1);
    }

    
    const uint16_t *getMapping(uint16_t norm16) const { return extraData+norm16; }
    const uint16_t *getCompositionsListForDecompYes(uint16_t norm16) const {
        if(norm16==0 || MIN_NORMAL_MAYBE_YES<=norm16) {
            return NULL;
        } else if(norm16<minMaybeYes) {
            return extraData+norm16;  
        } else {
            return maybeYesCompositions+norm16-minMaybeYes;
        }
    }
    const uint16_t *getCompositionsListForComposite(uint16_t norm16) const {
        const uint16_t *list=extraData+norm16;  
        return list+  
            1+  
            (*list&MAPPING_LENGTH_MASK);  
    }
    



    const uint16_t *getCompositionsList(uint16_t norm16) const {
        return isDecompYes(norm16) ?
                getCompositionsListForDecompYes(norm16) :
                getCompositionsListForComposite(norm16);
    }

    const UChar *copyLowPrefixFromNulTerminated(const UChar *src,
                                                UChar32 minNeedDataCP,
                                                ReorderingBuffer *buffer,
                                                UErrorCode &errorCode) const;
    UBool decomposeShort(const UChar *src, const UChar *limit,
                         ReorderingBuffer &buffer, UErrorCode &errorCode) const;
    UBool decompose(UChar32 c, uint16_t norm16,
                    ReorderingBuffer &buffer, UErrorCode &errorCode) const;

    static int32_t combine(const uint16_t *list, UChar32 trail);
    void addComposites(const uint16_t *list, UnicodeSet &set) const;
    void recompose(ReorderingBuffer &buffer, int32_t recomposeStartIndex,
                   UBool onlyContiguous) const;

    UBool hasCompBoundaryBefore(UChar32 c, uint16_t norm16) const;
    const UChar *findPreviousCompBoundary(const UChar *start, const UChar *p) const;
    const UChar *findNextCompBoundary(const UChar *p, const UChar *limit) const;

    const UChar *findPreviousFCDBoundary(const UChar *start, const UChar *p) const;
    const UChar *findNextFCDBoundary(const UChar *p, const UChar *limit) const;

    int32_t getCanonValue(UChar32 c) const;
    const UnicodeSet &getCanonStartSet(int32_t n) const;

    UDataMemory *memory;
    UVersionInfo dataVersion;

    
    UChar32 minDecompNoCP;
    UChar32 minCompNoMaybeCP;

    
    uint16_t minYesNo;
    uint16_t minYesNoMappingsOnly;
    uint16_t minNoNo;
    uint16_t limitNoNo;
    uint16_t minMaybeYes;

    UTrie2 *normTrie;
    const uint16_t *maybeYesCompositions;
    const uint16_t *extraData;  
    const uint8_t *smallFCD;  
    uint8_t tccc180[0x180];  

    SimpleSingleton canonIterDataSingleton;
};


#define CANON_NOT_SEGMENT_STARTER 0x80000000
#define CANON_HAS_COMPOSITIONS 0x40000000
#define CANON_HAS_SET 0x200000
#define CANON_VALUE_MASK 0x1fffff




class U_COMMON_API Normalizer2Factory {
public:
    static const Normalizer2 *getNFCInstance(UErrorCode &errorCode);
    static const Normalizer2 *getNFDInstance(UErrorCode &errorCode);
    static const Normalizer2 *getFCDInstance(UErrorCode &errorCode);
    static const Normalizer2 *getFCCInstance(UErrorCode &errorCode);
    static const Normalizer2 *getNFKCInstance(UErrorCode &errorCode);
    static const Normalizer2 *getNFKDInstance(UErrorCode &errorCode);
    static const Normalizer2 *getNFKC_CFInstance(UErrorCode &errorCode);
    static const Normalizer2 *getNoopInstance(UErrorCode &errorCode);

    static const Normalizer2 *getInstance(UNormalizationMode mode, UErrorCode &errorCode);

    static const Normalizer2Impl *getNFCImpl(UErrorCode &errorCode);
    static const Normalizer2Impl *getNFKCImpl(UErrorCode &errorCode);
    static const Normalizer2Impl *getNFKC_CFImpl(UErrorCode &errorCode);

    
    
    static const Normalizer2Impl *getImpl(const Normalizer2 *norm2);
private:
    Normalizer2Factory();  
};

U_NAMESPACE_END

U_CAPI int32_t U_EXPORT2
unorm2_swap(const UDataSwapper *ds,
            const void *inData, int32_t length, void *outData,
            UErrorCode *pErrorCode);





U_CFUNC UNormalizationCheckResult
unorm_getQuickCheck(UChar32 c, UNormalizationMode mode);





U_CFUNC uint16_t
unorm_getFCD16(UChar32 c);



































































































































#endif
#endif
