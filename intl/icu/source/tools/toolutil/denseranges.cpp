















#include "unicode/utypes.h"
#include "denseranges.h"


namespace {




class LargestGaps {
public:
    LargestGaps(int32_t max) : maxLength(max<=kCapacity ? max : kCapacity), length(0) {}

    void add(int32_t gapStart, int64_t gapLength) {
        int32_t i=length;
        while(i>0 && gapLength>gapLengths[i-1]) {
            --i;
        }
        if(i<maxLength) {
            
            
            
            int32_t j= length<maxLength ? length++ : maxLength-1;
            while(j>i) {
                gapStarts[j]=gapStarts[j-1];
                gapLengths[j]=gapLengths[j-1];
                --j;
            }
            gapStarts[i]=gapStart;
            gapLengths[i]=gapLength;
        }
    }

    void truncate(int32_t newLength) {
        if(newLength<length) {
            length=newLength;
        }
    }

    int32_t count() const { return length; }
    int32_t gapStart(int32_t i) const { return gapStarts[i]; }
    int64_t gapLength(int32_t i) const { return gapLengths[i]; }

    int32_t firstAfter(int32_t value) const {
        if(length==0) {
            return -1;
        }
        int32_t minValue=0;
        int32_t minIndex=-1;
        for(int32_t i=0; i<length; ++i) {
            if(value<gapStarts[i] && (minIndex<0 || gapStarts[i]<minValue)) {
                minValue=gapStarts[i];
                minIndex=i;
            }
        }
        return minIndex;
    }

private:
    static const int32_t kCapacity=15;

    int32_t maxLength;
    int32_t length;
    int32_t gapStarts[kCapacity];
    int64_t gapLengths[kCapacity];
};

}  













U_CAPI int32_t U_EXPORT2
uprv_makeDenseRanges(const int32_t values[], int32_t length,
                     int32_t density,
                     int32_t ranges[][2], int32_t capacity) {
    if(length<=2) {
        return 0;
    }
    int32_t minValue=values[0];
    int32_t maxValue=values[length-1];  
    
    
    int64_t maxLength=(int64_t)maxValue-(int64_t)minValue+1;
    if(length>=(density*maxLength)/0x100) {
        
        ranges[0][0]=minValue;
        ranges[0][1]=maxValue;
        return 1;
    }
    if(length<=4) {
        return 0;
    }
    
    
    LargestGaps gaps(capacity-1);
    int32_t i;
    int32_t expectedValue=minValue;
    for(i=1; i<length; ++i) {
        ++expectedValue;
        int32_t actualValue=values[i];
        if(expectedValue!=actualValue) {
            gaps.add(expectedValue, (int64_t)actualValue-(int64_t)expectedValue);
            expectedValue=actualValue;
        }
    }
    
    
    
    int32_t num;
    for(i=0, num=2;; ++i, ++num) {
        if(i>=gaps.count()) {
            
            
            return 0;
        }
        maxLength-=gaps.gapLength(i);
        if(length>num*2 && length>=(density*maxLength)/0x100) {
            break;
        }
    }
    
    gaps.truncate(num-1);
    ranges[0][0]=minValue;
    for(i=0; i<=num-2; ++i) {
        int32_t gapIndex=gaps.firstAfter(minValue);
        int32_t gapStart=gaps.gapStart(gapIndex);
        ranges[i][1]=gapStart-1;
        ranges[i+1][0]=minValue=(int32_t)(gapStart+gaps.gapLength(gapIndex));
    }
    ranges[num-1][1]=maxValue;
    return num;
}
