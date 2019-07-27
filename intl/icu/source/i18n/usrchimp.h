







#ifndef USRCHIMP_H
#define USRCHIMP_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/normalizer2.h"
#include "unicode/ucol.h"
#include "unicode/ucoleitr.h"
#include "unicode/ubrk.h"


#define UCOL_PRIMARYORDERMASK 0xffff0000

#define UCOL_SECONDARYORDERMASK 0x0000ff00

#define UCOL_TERTIARYORDERMASK 0x000000ff

#define UCOL_PRIMARYORDERSHIFT 16

#define UCOL_SECONDARYORDERSHIFT 8

#define UCOL_IGNORABLE 0


#define UCOL_PRIMARYORDER(order) (((order) >> 16) & 0xffff)
#define UCOL_SECONDARYORDER(order) (((order) & UCOL_SECONDARYORDERMASK)>> UCOL_SECONDARYORDERSHIFT)
#define UCOL_TERTIARYORDER(order) ((order) & UCOL_TERTIARYORDERMASK)

#define UCOL_CONTINUATION_MARKER 0xC0

#define isContinuation(CE) (((CE) & UCOL_CONTINUATION_MARKER) == UCOL_CONTINUATION_MARKER)





#define UCOL_PROCESSED_NULLORDER        ((int64_t)U_INT64_MAX)

U_NAMESPACE_BEGIN

class CollationElementIterator;
class Collator;

struct PCEI
{
    uint64_t ce;
    int32_t  low;
    int32_t  high;
};

struct PCEBuffer
{
    PCEI    defaultBuffer[16];
    PCEI   *buffer;
    int32_t bufferIndex;
    int32_t bufferSize;

    PCEBuffer();
    ~PCEBuffer();

    void  reset();
    UBool empty() const;
    void  put(uint64_t ce, int32_t ixLow, int32_t ixHigh);
    const PCEI *get();
};

class UCollationPCE : public UMemory {
private:
    PCEBuffer          pceBuffer;
    CollationElementIterator *cei;
    UCollationStrength strength;
    UBool              toShift;
    UBool              isShifted;
    uint32_t           variableTop;

public:
    UCollationPCE(UCollationElements *elems);
    UCollationPCE(CollationElementIterator *iter);
    ~UCollationPCE();

    void init(UCollationElements *elems);
    void init(CollationElementIterator *iter);

    









    int64_t nextProcessed(int32_t *ixLow, int32_t *ixHigh, UErrorCode *status);
    












    int64_t previousProcessed(int32_t *ixLow, int32_t *ixHigh, UErrorCode *status);

private:
    void init(const Collator &coll);
    uint64_t processCE(uint32_t ce);
};

U_NAMESPACE_END

#define INITIAL_ARRAY_SIZE_       256
#define MAX_TABLE_SIZE_           257

struct USearch {
    
    const UChar              *text;
          int32_t             textLength; 
          UBool               isOverlap;
          UBool               isCanonicalMatch;
          int16_t             elementComparisonType;
          UBreakIterator     *internalBreakIter;  
          UBreakIterator     *breakIter;
    
    
    
    
          int32_t             matchedIndex; 
          int32_t             matchedLength;
          UBool               isForwardSearching;
          UBool               reset;
};

struct UPattern {
    const UChar              *text;
          int32_t             textLength; 
          
          int32_t             cesLength;
          int32_t            *ces;
          int32_t             cesBuffer[INITIAL_ARRAY_SIZE_];
          int32_t             pcesLength;
          int64_t            *pces;
          int64_t             pcesBuffer[INITIAL_ARRAY_SIZE_];
          UBool               hasPrefixAccents;
          UBool               hasSuffixAccents;
          int16_t             defaultShiftSize;
          int16_t             shift[MAX_TABLE_SIZE_];
          int16_t             backShift[MAX_TABLE_SIZE_];
};

struct UStringSearch {
    struct USearch            *search;
    struct UPattern            pattern;
    const  UCollator          *collator;
    const  icu::Normalizer2   *nfd;
    
    
           UCollationElements *textIter;
           icu::UCollationPCE *textProcessedIter;
    
    
           UCollationElements *utilIter;
           UBool               ownCollator;
           UCollationStrength  strength;
           uint32_t            ceMask;
           uint32_t            variableTop;
           UBool               toShift;
           UChar               canonicalPrefixAccents[INITIAL_ARRAY_SIZE_];
           UChar               canonicalSuffixAccents[INITIAL_ARRAY_SIZE_];
};























U_CFUNC
UBool usearch_handleNextExact(UStringSearch *strsrch, UErrorCode *status);









U_CFUNC
UBool usearch_handleNextCanonical(UStringSearch *strsrch, UErrorCode *status);








U_CFUNC
UBool usearch_handlePreviousExact(UStringSearch *strsrch, UErrorCode *status);









U_CFUNC
UBool usearch_handlePreviousCanonical(UStringSearch *strsrch, 
                                      UErrorCode    *status);

#endif 

#endif
