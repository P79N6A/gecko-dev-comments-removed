



















#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "ucol_imp.h"
#include "ucol_wgt.h"
#include "cmemory.h"
#include "uarrsort.h"

#ifdef UCOL_DEBUG
#   include <stdio.h>
#endif





static inline int32_t
lengthOfWeight(uint32_t weight) {
    if((weight&0xffffff)==0) {
        return 1;
    } else if((weight&0xffff)==0) {
        return 2;
    } else if((weight&0xff)==0) {
        return 3;
    } else {
        return 4;
    }
}

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

static inline uint32_t
incWeight(uint32_t weight, int32_t length, uint32_t maxByte) {
    uint32_t byte;

    for(;;) {
        byte=getWeightByte(weight, length);
        if(byte<maxByte) {
            return setWeightByte(weight, length, byte+1);
        } else {
            
            weight=setWeightByte(weight, length, UCOL_BYTE_FIRST_TAILORED);
            --length;
        }
    }
}

static inline int32_t
lengthenRange(WeightRange *range, uint32_t maxByte, uint32_t countBytes) {
    int32_t length;

    length=range->length2+1;
    range->start=setWeightTrail(range->start, length, UCOL_BYTE_FIRST_TAILORED);
    range->end=setWeightTrail(range->end, length, maxByte);
    range->count2*=countBytes;
    range->length2=length;
    return length;
}


static int32_t U_CALLCONV
compareRanges(const void * , const void *left, const void *right) {
    uint32_t l, r;

    l=((const WeightRange *)left)->start;
    r=((const WeightRange *)right)->start;
    if(l<r) {
        return -1;
    } else if(l>r) {
        return 1;
    } else {
        return 0;
    }
}






static inline int32_t
getWeightRanges(uint32_t lowerLimit, uint32_t upperLimit,
                uint32_t maxByte, uint32_t countBytes,
                WeightRange ranges[7]) {
    WeightRange lower[5], middle, upper[5]; 
    uint32_t weight, trail;
    int32_t length, lowerLength, upperLength, rangeCount;

    

    
    lowerLength=lengthOfWeight(lowerLimit);
    upperLength=lengthOfWeight(upperLimit);

#ifdef UCOL_DEBUG
    printf("length of lower limit 0x%08lx is %ld\n", lowerLimit, lowerLength);
    printf("length of upper limit 0x%08lx is %ld\n", upperLimit, upperLength);
#endif

    if(lowerLimit>=upperLimit) {
#ifdef UCOL_DEBUG
        printf("error: no space between lower & upper limits\n");
#endif
        return 0;
    }

    
    if(lowerLength<upperLength) {
        if(lowerLimit==truncateWeight(upperLimit, lowerLength)) {
#ifdef UCOL_DEBUG
            printf("error: lower limit 0x%08lx is a prefix of upper limit 0x%08lx\n", lowerLimit, upperLimit);
#endif
            return 0;
        }
    }
    

    
    uprv_memset(lower, 0, sizeof(lower));
    uprv_memset(&middle, 0, sizeof(middle));
    uprv_memset(upper, 0, sizeof(upper));

    













    weight=lowerLimit;
    for(length=lowerLength; length>=2; --length) {
        trail=getWeightTrail(weight, length);
        if(trail<maxByte) {
            lower[length].start=incWeightTrail(weight, length);
            lower[length].end=setWeightTrail(weight, length, maxByte);
            lower[length].length=length;
            lower[length].count=maxByte-trail;
        }
        weight=truncateWeight(weight, length-1);
    }
    middle.start=incWeightTrail(weight, 1);

    weight=upperLimit;
    for(length=upperLength; length>=2; --length) {
        trail=getWeightTrail(weight, length);
        if(trail>UCOL_BYTE_FIRST_TAILORED) {
            upper[length].start=setWeightTrail(weight, length, UCOL_BYTE_FIRST_TAILORED);
            upper[length].end=decWeightTrail(weight, length);
            upper[length].length=length;
            upper[length].count=trail-UCOL_BYTE_FIRST_TAILORED;
        }
        weight=truncateWeight(weight, length-1);
    }
    middle.end=decWeightTrail(weight, 1);

    
    middle.length=1;
    if(middle.end>=middle.start) {
        middle.count=(int32_t)((middle.end-middle.start)>>24)+1;
    } else {
        
        uint32_t start, end;

        
        middle.count=0;

        
        for(length=4; length>=2; --length) {
            if(lower[length].count>0 && upper[length].count>0) {
                start=upper[length].start;
                end=lower[length].end;

                if(end>=start || incWeight(end, length, maxByte)==start) {
                    
                    start=lower[length].start;
                    end=lower[length].end=upper[length].end;
                    



                    lower[length].count=
                        (int32_t)(getWeightTrail(end, length)-getWeightTrail(start, length)+1+
                                  countBytes*(getWeightByte(end, length-1)-getWeightByte(start, length-1)));
                    upper[length].count=0;
                    while(--length>=2) {
                        lower[length].count=upper[length].count=0;
                    }
                    break;
                }
            }
        }
    }

#ifdef UCOL_DEBUG
    
    for(length=4; length>=2; --length) {
        if(lower[length].count>0) {
            printf("lower[%ld] .start=0x%08lx .end=0x%08lx .count=%ld\n", length, lower[length].start, lower[length].end, lower[length].count);
        }
    }
    if(middle.count>0) {
        printf("middle   .start=0x%08lx .end=0x%08lx .count=%ld\n", middle.start, middle.end, middle.count);
    }
    for(length=2; length<=4; ++length) {
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
    for(length=2; length<=4; ++length) {
        
        if(upper[length].count>0) {
            uprv_memcpy(ranges+rangeCount, upper+length, sizeof(WeightRange));
            ++rangeCount;
        }
        if(lower[length].count>0) {
            uprv_memcpy(ranges+rangeCount, lower+length, sizeof(WeightRange));
            ++rangeCount;
        }
    }
    return rangeCount;
}






U_CFUNC int32_t
ucol_allocWeights(uint32_t lowerLimit, uint32_t upperLimit,
                  uint32_t n,
                  uint32_t maxByte,
                  WeightRange ranges[7]) {
    
    uint32_t countBytes=maxByte-UCOL_BYTE_FIRST_TAILORED+1;

    uint32_t lengthCounts[6]; 
    uint32_t maxCount;
    int32_t i, rangeCount, minLength;

    
    uint32_t powers[5];
    
    powers[0] = 1;
    powers[1] = countBytes;
    powers[2] = countBytes*countBytes;
    powers[3] = countBytes*countBytes*countBytes;
    powers[4] = countBytes*countBytes*countBytes*countBytes;

#ifdef UCOL_DEBUG
    puts("");
#endif

    rangeCount=getWeightRanges(lowerLimit, upperLimit, maxByte, countBytes, ranges);
    if(rangeCount<=0) {
#ifdef UCOL_DEBUG
        printf("error: unable to get Weight ranges\n");
#endif
        return 0;
    }

    
    maxCount=0;
    for(i=0; i<rangeCount; ++i) {
        maxCount+=(uint32_t)ranges[i].count*powers[4-ranges[i].length];
    }
    if(maxCount>=n) {
#ifdef UCOL_DEBUG
        printf("the maximum number of %lu weights is sufficient for n=%lu\n", maxCount, n);
#endif
    } else {
#ifdef UCOL_DEBUG
        printf("error: the maximum number of %lu weights is insufficient for n=%lu\n", maxCount, n);
#endif
        return 0;
    }

    
    for(i=0; i<rangeCount; ++i) {
        ranges[i].length2=ranges[i].length;
        ranges[i].count2=(uint32_t)ranges[i].count;
    }

    
    for(;;) {
        
        minLength=ranges[0].length2;

        
        uprv_memset(lengthCounts, 0, sizeof(lengthCounts));
        for(i=0; i<rangeCount; ++i) {
            lengthCounts[ranges[i].length2]+=ranges[i].count2;
        }

        
        if(n<=(lengthCounts[minLength]+lengthCounts[minLength+1])) {
            
            maxCount=0;
            rangeCount=0;
            do {
                maxCount+=ranges[rangeCount].count2;
                ++rangeCount;
            } while(n>maxCount);
#ifdef UCOL_DEBUG
            printf("take first %ld ranges\n", rangeCount);
#endif
            break;
        } else if(n<=ranges[0].count2*countBytes) {
            
            uint32_t count1, count2, power_1, power;

            

            
            power_1=powers[minLength-ranges[0].length];
            power=power_1*countBytes;
            count2=(n+power-1)/power;
            count1=ranges[0].count-count2;

            
#ifdef UCOL_DEBUG
            printf("split the first range %ld:%ld\n", count1, count2);
#endif
            if(count1<1) {
                rangeCount=1;

                
                lengthenRange(ranges, maxByte, countBytes);
            } else {
                
                uint32_t byte;

                
                rangeCount=2;
                ranges[1].end=ranges[0].end;
                ranges[1].length=ranges[0].length;
                ranges[1].length2=minLength;

                
                i=ranges[0].length;
                byte=getWeightByte(ranges[0].start, i)+count1-1;

                




                if(byte<=maxByte) {
                    ranges[0].end=setWeightByte(ranges[0].start, i, byte);
                } else  {
                    ranges[0].end=setWeightByte(incWeight(ranges[0].start, i-1, maxByte), i, byte-countBytes);
                }

                
                byte=(maxByte<<24)|(maxByte<<16)|(maxByte<<8)|maxByte; 
                ranges[0].end=truncateWeight(ranges[0].end, i)|
                              ((byte>>(8*i))&(byte<<(8*(4-minLength))));

                
                ranges[1].start=incWeight(ranges[0].end, minLength, maxByte);

                
                ranges[0].count=count1;
                ranges[1].count=count2;

                ranges[0].count2=count1*power_1;
                ranges[1].count2=count2*power_1; 

                
                lengthenRange(ranges+1, maxByte, countBytes);
            }
            break;
        }

        
#ifdef UCOL_DEBUG
        printf("lengthen the short ranges from %ld bytes to %ld and iterate\n", minLength, minLength+1);
#endif
        for(i=0; ranges[i].length2==minLength; ++i) {
            lengthenRange(ranges+i, maxByte, countBytes);
        }
    }

    if(rangeCount>1) {
        
        UErrorCode errorCode=U_ZERO_ERROR;
        uprv_sortArray(ranges, rangeCount, sizeof(WeightRange), compareRanges, NULL, FALSE, &errorCode);
        
    }

#ifdef UCOL_DEBUG
    puts("final ranges:");
    for(i=0; i<rangeCount; ++i) {
        printf("ranges[%ld] .start=0x%08lx .end=0x%08lx .length=%ld .length2=%ld .count=%ld .count2=%lu\n",
               i, ranges[i].start, ranges[i].end, ranges[i].length, ranges[i].length2, ranges[i].count, ranges[i].count2);
    }
#endif

    
    ranges[0].count=maxByte;

    return rangeCount;
}





U_CFUNC uint32_t
ucol_nextWeight(WeightRange ranges[], int32_t *pRangeCount) {
    if(*pRangeCount<=0) {
        return 0xffffffff;
    } else {
        uint32_t weight, maxByte;

        
        maxByte=ranges[0].count;

        
        weight=ranges[0].start;
        if(weight==ranges[0].end) {
            
            if(--*pRangeCount>0) {
                uprv_memmove(ranges, ranges+1, *pRangeCount*sizeof(WeightRange));
                ranges[0].count=maxByte; 
            }
        } else {
            
            ranges[0].start=incWeight(weight, ranges[0].length2, maxByte);
        }

        return weight;
    }
}

#if 0 

static void
testAlloc(uint32_t lowerLimit, uint32_t upperLimit, uint32_t n, UBool enumerate) {
    WeightRange ranges[8];
    int32_t rangeCount;

    rangeCount=ucol_allocWeights(lowerLimit, upperLimit, n, ranges);
    if(enumerate) {
        uint32_t weight;

        while(n>0) {
            weight=ucol_nextWeight(ranges, &rangeCount);
            if(weight==0xffffffff) {
                printf("error: 0xffffffff with %lu more weights to go\n", n);
                break;
            }
            printf("    0x%08lx\n", weight);
            --n;
        }
    }
}

extern int
main(int argc, const char *argv[]) {
#if 0
#endif
    testAlloc(0x364214fc, 0x44b87d23, 5, FALSE);
    testAlloc(0x36421500, 0x44b87d23, 5, FALSE);
    testAlloc(0x36421500, 0x44b87d23, 20, FALSE);
    testAlloc(0x36421500, 0x44b87d23, 13700, FALSE);
    testAlloc(0x36421500, 0x38b87d23, 1, FALSE);
    testAlloc(0x36421500, 0x38b87d23, 20, FALSE);
    testAlloc(0x36421500, 0x38b87d23, 200, TRUE);
    testAlloc(0x36421500, 0x38b87d23, 13700, FALSE);
    testAlloc(0x36421500, 0x37b87d23, 13700, FALSE);
    testAlloc(0x36ef1500, 0x37b87d23, 13700, FALSE);
    testAlloc(0x36421500, 0x36b87d23, 13700, FALSE);
    testAlloc(0x36b87122, 0x36b87d23, 13700, FALSE);
    testAlloc(0x49000000, 0x4a600000, 13700, FALSE);
    testAlloc(0x9fffffff, 0xd0000000, 13700, FALSE);
    testAlloc(0x9fffffff, 0xd0000000, 67400, FALSE);
    testAlloc(0x9fffffff, 0xa0030000, 67400, FALSE);
    testAlloc(0x9fffffff, 0xa0030000, 40000, FALSE);
    testAlloc(0xa0000000, 0xa0030000, 40000, FALSE);
    testAlloc(0xa0031100, 0xa0030000, 40000, FALSE);
#if 0
#endif
    return 0;
}

#endif

#endif 
