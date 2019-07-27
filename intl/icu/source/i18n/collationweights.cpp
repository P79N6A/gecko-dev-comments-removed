



















#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "cmemory.h"
#include "collation.h"
#include "collationweights.h"
#include "uarrsort.h"
#include "uassert.h"

#ifdef UCOL_DEBUG
#   include <stdio.h>
#endif

U_NAMESPACE_BEGIN





static inline uint32_t
getWeightTrail(uint32_t weight, int32_t length) {
    return (uint32_t)(weight>>(8*(4-length)))&0xff;
}

static inline uint32_t
setWeightTrail(uint32_t weight, int32_t length, uint32_t trail) {
    length=8*(4-length);
    return (uint32_t)((weight&(0xffffff00<<length))|(trail<<length));
}

static inline uint32_t
getWeightByte(uint32_t weight, int32_t idx) {
    return getWeightTrail(weight, idx); 
}

static inline uint32_t
setWeightByte(uint32_t weight, int32_t idx, uint32_t byte) {
    uint32_t mask; 

    idx*=8;
    if(idx<32) {
        mask=((uint32_t)0xffffffff)>>idx;
    } else {
        
        
        
        
        
        
        
        
        
        mask=0;
    }
    idx=32-idx;
    mask|=0xffffff00<<idx;
    return (uint32_t)((weight&mask)|(byte<<idx));
}

static inline uint32_t
truncateWeight(uint32_t weight, int32_t length) {
    return (uint32_t)(weight&(0xffffffff<<(8*(4-length))));
}

static inline uint32_t
incWeightTrail(uint32_t weight, int32_t length) {
    return (uint32_t)(weight+(1UL<<(8*(4-length))));
}

static inline uint32_t
decWeightTrail(uint32_t weight, int32_t length) {
    return (uint32_t)(weight-(1UL<<(8*(4-length))));
}

CollationWeights::CollationWeights()
        : middleLength(0), rangeIndex(0), rangeCount(0) {
    for(int32_t i = 0; i < 5; ++i) {
        minBytes[i] = maxBytes[i] = 0;
    }
}

void
CollationWeights::initForPrimary(UBool compressible) {
    middleLength=1;
    minBytes[1] = Collation::MERGE_SEPARATOR_BYTE + 1;
    maxBytes[1] = Collation::TRAIL_WEIGHT_BYTE;
    if(compressible) {
        minBytes[2] = Collation::PRIMARY_COMPRESSION_LOW_BYTE + 1;
        maxBytes[2] = Collation::PRIMARY_COMPRESSION_HIGH_BYTE - 1;
    } else {
        minBytes[2] = 2;
        maxBytes[2] = 0xff;
    }
    minBytes[3] = 2;
    maxBytes[3] = 0xff;
    minBytes[4] = 2;
    maxBytes[4] = 0xff;
}

void
CollationWeights::initForSecondary() {
    
    middleLength=3;
    minBytes[1] = 0;
    maxBytes[1] = 0;
    minBytes[2] = 0;
    maxBytes[2] = 0;
    minBytes[3] = Collation::LEVEL_SEPARATOR_BYTE + 1;
    maxBytes[3] = 0xff;
    minBytes[4] = 2;
    maxBytes[4] = 0xff;
}

void
CollationWeights::initForTertiary() {
    
    middleLength=3;
    minBytes[1] = 0;
    maxBytes[1] = 0;
    minBytes[2] = 0;
    maxBytes[2] = 0;
    
    
    minBytes[3] = Collation::LEVEL_SEPARATOR_BYTE + 1;
    maxBytes[3] = 0x3f;
    minBytes[4] = 2;
    maxBytes[4] = 0x3f;
}

uint32_t
CollationWeights::incWeight(uint32_t weight, int32_t length) const {
    for(;;) {
        uint32_t byte=getWeightByte(weight, length);
        if(byte<maxBytes[length]) {
            return setWeightByte(weight, length, byte+1);
        } else {
            
            weight=setWeightByte(weight, length, minBytes[length]);
            --length;
            U_ASSERT(length > 0);
        }
    }
}

uint32_t
CollationWeights::incWeightByOffset(uint32_t weight, int32_t length, int32_t offset) const {
    for(;;) {
        offset += getWeightByte(weight, length);
        if((uint32_t)offset <= maxBytes[length]) {
            return setWeightByte(weight, length, offset);
        } else {
            
            offset -= minBytes[length];
            weight = setWeightByte(weight, length, minBytes[length] + offset % countBytes(length));
            offset /= countBytes(length);
            --length;
            U_ASSERT(length > 0);
        }
    }
}

void
CollationWeights::lengthenRange(WeightRange &range) const {
    int32_t length=range.length+1;
    range.start=setWeightTrail(range.start, length, minBytes[length]);
    range.end=setWeightTrail(range.end, length, maxBytes[length]);
    range.count*=countBytes(length);
    range.length=length;
}


static int32_t U_CALLCONV
compareRanges(const void * , const void *left, const void *right) {
    uint32_t l, r;

    l=((const CollationWeights::WeightRange *)left)->start;
    r=((const CollationWeights::WeightRange *)right)->start;
    if(l<r) {
        return -1;
    } else if(l>r) {
        return 1;
    } else {
        return 0;
    }
}

UBool
CollationWeights::getWeightRanges(uint32_t lowerLimit, uint32_t upperLimit) {
    U_ASSERT(lowerLimit != 0);
    U_ASSERT(upperLimit != 0);

    
    int32_t lowerLength=lengthOfWeight(lowerLimit);
    int32_t upperLength=lengthOfWeight(upperLimit);

#ifdef UCOL_DEBUG
    printf("length of lower limit 0x%08lx is %ld\n", lowerLimit, lowerLength);
    printf("length of upper limit 0x%08lx is %ld\n", upperLimit, upperLength);
#endif
    U_ASSERT(lowerLength>=middleLength);
    

    if(lowerLimit>=upperLimit) {
#ifdef UCOL_DEBUG
        printf("error: no space between lower & upper limits\n");
#endif
        return FALSE;
    }

    
    if(lowerLength<upperLength) {
        if(lowerLimit==truncateWeight(upperLimit, lowerLength)) {
#ifdef UCOL_DEBUG
            printf("error: lower limit 0x%08lx is a prefix of upper limit 0x%08lx\n", lowerLimit, upperLimit);
#endif
            return FALSE;
        }
    }
    

    WeightRange lower[5], middle, upper[5]; 
    uprv_memset(lower, 0, sizeof(lower));
    uprv_memset(&middle, 0, sizeof(middle));
    uprv_memset(upper, 0, sizeof(upper));

    













    uint32_t weight=lowerLimit;
    for(int32_t length=lowerLength; length>middleLength; --length) {
        uint32_t trail=getWeightTrail(weight, length);
        if(trail<maxBytes[length]) {
            lower[length].start=incWeightTrail(weight, length);
            lower[length].end=setWeightTrail(weight, length, maxBytes[length]);
            lower[length].length=length;
            lower[length].count=maxBytes[length]-trail;
        }
        weight=truncateWeight(weight, length-1);
    }
    if(weight<0xff000000) {
        middle.start=incWeightTrail(weight, middleLength);
    } else {
        
        
        middle.start=0xffffffff;  
    }

    weight=upperLimit;
    for(int32_t length=upperLength; length>middleLength; --length) {
        uint32_t trail=getWeightTrail(weight, length);
        if(trail>minBytes[length]) {
            upper[length].start=setWeightTrail(weight, length, minBytes[length]);
            upper[length].end=decWeightTrail(weight, length);
            upper[length].length=length;
            upper[length].count=trail-minBytes[length];
        }
        weight=truncateWeight(weight, length-1);
    }
    middle.end=decWeightTrail(weight, middleLength);

    
    middle.length=middleLength;
    if(middle.end>=middle.start) {
        middle.count=(int32_t)((middle.end-middle.start)>>(8*(4-middleLength)))+1;
    } else {
        

        
        for(int32_t length=4; length>middleLength; --length) {
            if(lower[length].count>0 && upper[length].count>0) {
                uint32_t start=upper[length].start;
                uint32_t end=lower[length].end;

                if(end>=start || incWeight(end, length)==start) {
                    
                    start=lower[length].start;
                    end=lower[length].end=upper[length].end;
                    



                    lower[length].count=
                        (int32_t)(getWeightTrail(end, length)-getWeightTrail(start, length)+1+
                                  countBytes(length)*(getWeightByte(end, length-1)-getWeightByte(start, length-1)));
                    upper[length].count=0;
                    while(--length>middleLength) {
                        lower[length].count=upper[length].count=0;
                    }
                    break;
                }
            }
        }
    }

#ifdef UCOL_DEBUG
    
    for(int32_t length=4; length>=2; --length) {
        if(lower[length].count>0) {
            printf("lower[%ld] .start=0x%08lx .end=0x%08lx .count=%ld\n", length, lower[length].start, lower[length].end, lower[length].count);
        }
    }
    if(middle.count>0) {
        printf("middle   .start=0x%08lx .end=0x%08lx .count=%ld\n", middle.start, middle.end, middle.count);
    }
    for(int32_t length=2; length<=4; ++length) {
        if(upper[length].count>0) {
            printf("upper[%ld] .start=0x%08lx .end=0x%08lx .count=%ld\n", length, upper[length].start, upper[length].end, upper[length].count);
        }
    }
#endif

    
    rangeCount=0;
    if(middle.count>0) {
        uprv_memcpy(ranges, &middle, sizeof(WeightRange));
        rangeCount=1;
    }
    for(int32_t length=middleLength+1; length<=4; ++length) {
        
        if(upper[length].count>0) {
            uprv_memcpy(ranges+rangeCount, upper+length, sizeof(WeightRange));
            ++rangeCount;
        }
        if(lower[length].count>0) {
            uprv_memcpy(ranges+rangeCount, lower+length, sizeof(WeightRange));
            ++rangeCount;
        }
    }
    return rangeCount>0;
}

UBool
CollationWeights::allocWeightsInShortRanges(int32_t n, int32_t minLength) {
    
    for(int32_t i = 0; i < rangeCount && ranges[i].length <= (minLength + 1); ++i) {
        if(n <= ranges[i].count) {
            
            if(ranges[i].length > minLength) {
                
                
                
                ranges[i].count = n;
            }
            rangeCount = i + 1;
#ifdef UCOL_DEBUG
            printf("take first %ld ranges\n", rangeCount);
#endif

            if(rangeCount>1) {
                
                UErrorCode errorCode=U_ZERO_ERROR;
                uprv_sortArray(ranges, rangeCount, sizeof(WeightRange),
                               compareRanges, NULL, FALSE, &errorCode);
                
            }
            return TRUE;
        }
        n -= ranges[i].count;  
    }
    return FALSE;
}

UBool
CollationWeights::allocWeightsInMinLengthRanges(int32_t n, int32_t minLength) {
    
    
    int32_t count = 0;
    int32_t minLengthRangeCount;
    for(minLengthRangeCount = 0;
            minLengthRangeCount < rangeCount &&
                ranges[minLengthRangeCount].length == minLength;
            ++minLengthRangeCount) {
        count += ranges[minLengthRangeCount].count;
    }

    int32_t nextCountBytes = countBytes(minLength + 1);
    if(n > count * nextCountBytes) { return FALSE; }

    
    uint32_t start = ranges[0].start;
    uint32_t end = ranges[0].end;
    for(int32_t i = 1; i < minLengthRangeCount; ++i) {
        if(ranges[i].start < start) { start = ranges[i].start; }
        if(ranges[i].end > end) { end = ranges[i].end; }
    }

    
    
    
    
    
    
    
    int32_t count2 = (n - count) / (nextCountBytes - 1);  
    int32_t count1 = count - count2;  
    if(count2 == 0 || (count1 + count2 * nextCountBytes) < n) {
        
        ++count2;
        --count1;
        U_ASSERT((count1 + count2 * nextCountBytes) >= n);
    }

    ranges[0].start = start;

    if(count1 == 0) {
        
        ranges[0].end = end;
        ranges[0].count = count;
        lengthenRange(ranges[0]);
        rangeCount = 1;
    } else {
        
#ifdef UCOL_DEBUG
        printf("split the range number %ld (out of %ld minLength ranges) by %ld:%ld\n",
               splitRange, rangeCount, count1, count2);
#endif

        
        ranges[0].end = incWeightByOffset(start, minLength, count1 - 1);
        ranges[0].count = count1;

        ranges[1].start = incWeight(ranges[0].end, minLength);
        ranges[1].end = end;
        ranges[1].length = minLength;  
        ranges[1].count = count2;  
        lengthenRange(ranges[1]);
        rangeCount = 2;
    }
    return TRUE;
}






UBool
CollationWeights::allocWeights(uint32_t lowerLimit, uint32_t upperLimit, int32_t n) {
#ifdef UCOL_DEBUG
    puts("");
#endif

    if(!getWeightRanges(lowerLimit, upperLimit)) {
#ifdef UCOL_DEBUG
        printf("error: unable to get Weight ranges\n");
#endif
        return FALSE;
    }

    
    for(;;) {
        
        int32_t minLength=ranges[0].length;

        if(allocWeightsInShortRanges(n, minLength)) { break; }

        if(minLength == 4) {
#ifdef UCOL_DEBUG
            printf("error: the maximum number of %ld weights is insufficient for n=%ld\n",
                   minLengthCount, n);
#endif
            return FALSE;
        }

        if(allocWeightsInMinLengthRanges(n, minLength)) { break; }

        
#ifdef UCOL_DEBUG
        printf("lengthen the short ranges from %ld bytes to %ld and iterate\n", minLength, minLength+1);
#endif
        for(int32_t i=0; ranges[i].length==minLength; ++i) {
            lengthenRange(ranges[i]);
        }
    }

#ifdef UCOL_DEBUG
    puts("final ranges:");
    for(int32_t i=0; i<rangeCount; ++i) {
        printf("ranges[%ld] .start=0x%08lx .end=0x%08lx .length=%ld .count=%ld\n",
               i, ranges[i].start, ranges[i].end, ranges[i].length, ranges[i].count);
    }
#endif

    rangeIndex = 0;
    return TRUE;
}

uint32_t
CollationWeights::nextWeight() {
    if(rangeIndex >= rangeCount) {
        return 0xffffffff;
    } else {
        
        WeightRange &range = ranges[rangeIndex];
        uint32_t weight = range.start;
        if(--range.count == 0) {
            
            ++rangeIndex;
        } else {
            
            range.start = incWeight(weight, range.length);
            U_ASSERT(range.start <= range.end);
        }

        return weight;
    }
}

U_NAMESPACE_END

#endif 
