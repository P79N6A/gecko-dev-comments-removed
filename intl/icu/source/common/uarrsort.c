

















#include "unicode/utypes.h"
#include "cmemory.h"
#include "uarrsort.h"

enum {
    MIN_QSORT=9, 
    STACK_ITEM_SIZE=200
};



U_CAPI int32_t U_EXPORT2
uprv_uint16Comparator(const void *context, const void *left, const void *right) {
    return (int32_t)*(const uint16_t *)left - (int32_t)*(const uint16_t *)right;
}

U_CAPI int32_t U_EXPORT2
uprv_int32Comparator(const void *context, const void *left, const void *right) {
    return *(const int32_t *)left - *(const int32_t *)right;
}

U_CAPI int32_t U_EXPORT2
uprv_uint32Comparator(const void *context, const void *left, const void *right) {
    uint32_t l=*(const uint32_t *)left, r=*(const uint32_t *)right;

    
    if(l<r) {
        return -1;
    } else if(l==r) {
        return 0;
    } else  {
        return 1;
    }
}



static void
doInsertionSort(char *array, int32_t start, int32_t limit, int32_t itemSize,
                UComparator *cmp, const void *context, void *pv) {
    int32_t i, j;

    for(j=start+1; j<limit; ++j) {
        
        uprv_memcpy(pv, array+j*itemSize, itemSize);

        for(i=j; i>start; --i) {
            if( cmp(context, pv, array+(i-1)*itemSize)>=0) {
                break;
            }

            
            uprv_memcpy(array+i*itemSize, array+(i-1)*itemSize, itemSize);
        }

        if(i!=j) {
            
            uprv_memcpy(array+i*itemSize, pv, itemSize);
        }
    }
}

static void
insertionSort(char *array, int32_t length, int32_t itemSize,
              UComparator *cmp, const void *context, UErrorCode *pErrorCode) {
    UAlignedMemory v[STACK_ITEM_SIZE/sizeof(UAlignedMemory)+1];
    void *pv;

    
    if(itemSize<=STACK_ITEM_SIZE) {
        pv=v;
    } else {
        pv=uprv_malloc(itemSize);
        if(pv==NULL) {
            *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
            return;
        }
    }

    doInsertionSort(array, 0, length, itemSize, cmp, context, pv);

    if(pv!=v) {
        uprv_free(pv);
    }
}















static void
subQuickSort(char *array, int32_t start, int32_t limit, int32_t itemSize,
             UComparator *cmp, const void *context,
             void *px, void *pw) {
    int32_t left, right;

    
    do {
        if((start+MIN_QSORT)>=limit) {
            doInsertionSort(array, start, limit, itemSize, cmp, context, px);
            break;
        }

        left=start;
        right=limit;

        
        uprv_memcpy(px, array+((start+limit)/2)*itemSize, itemSize);

        do {
            while(
                  cmp(context, array+left*itemSize, px)<0
            ) {
                ++left;
            }
            while(
                  cmp(context, px, array+(right-1)*itemSize)<0
            ) {
                --right;
            }

            
            if(left<right) {
                --right;

                if(left<right) {
                    uprv_memcpy(pw, array+left*itemSize, itemSize);
                    uprv_memcpy(array+left*itemSize, array+right*itemSize, itemSize);
                    uprv_memcpy(array+right*itemSize, pw, itemSize);
                }

                ++left;
            }
        } while(left<right);

        
        if((right-start)<(limit-left)) {
            
            if(start<(right-1)) {
                subQuickSort(array, start, right, itemSize, cmp, context, px, pw);
            }

            
            start=left;
        } else {
            
            if(left<(limit-1)) {
                subQuickSort(array, left, limit, itemSize, cmp, context, px, pw);
            }

            
            limit=right;
        }
    } while(start<(limit-1));
}

static void
quickSort(char *array, int32_t length, int32_t itemSize,
            UComparator *cmp, const void *context, UErrorCode *pErrorCode) {
    UAlignedMemory xw[(2*STACK_ITEM_SIZE)/sizeof(UAlignedMemory)+1];
    void *p;

    
    if(itemSize<=STACK_ITEM_SIZE) {
        p=xw;
    } else {
        p=uprv_malloc(2*itemSize);
        if(p==NULL) {
            *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
            return;
        }
    }

    subQuickSort(array, 0, length, itemSize,
                 cmp, context, p, (char *)p+itemSize);

    if(p!=xw) {
        uprv_free(p);
    }
}







U_CAPI void U_EXPORT2
uprv_sortArray(void *array, int32_t length, int32_t itemSize,
               UComparator *cmp, const void *context,
               UBool sortStable, UErrorCode *pErrorCode) {
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return;
    }
    if((length>0 && array==NULL) || length<0 || itemSize<=0 || cmp==NULL) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }

    if(length<=1) {
        return;
    } else if(length<MIN_QSORT || sortStable) {
        insertionSort((char *)array, length, itemSize, cmp, context, pErrorCode);
        
    } else {
        quickSort((char *)array, length, itemSize, cmp, context, pErrorCode);
    }
}
