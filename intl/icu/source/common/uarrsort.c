

















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



U_CAPI int32_t U_EXPORT2
uprv_stableBinarySearch(char *array, int32_t limit, void *item, int32_t itemSize,
                        UComparator *cmp, const void *context) {
    int32_t start=0;
    UBool found=FALSE;

    
    while((limit-start)>=MIN_QSORT) {
        int32_t i=(start+limit)/2;
        int32_t diff=cmp(context, item, array+i*itemSize);
        if(diff==0) {
            










            found=TRUE;
            start=i+1;
        } else if(diff<0) {
            limit=i;
        } else {
            start=i;
        }
    }

    
    while(start<limit) {
        int32_t diff=cmp(context, item, array+start*itemSize);
        if(diff==0) {
            found=TRUE;
        } else if(diff<0) {
            break;
        }
        ++start;
    }
    return found ? (start-1) : ~start;
}

static void
doInsertionSort(char *array, int32_t length, int32_t itemSize,
                UComparator *cmp, const void *context, void *pv) {
    int32_t j;

    for(j=1; j<length; ++j) {
        char *item=array+j*itemSize;
        int32_t insertionPoint=uprv_stableBinarySearch(array, j, item, itemSize, cmp, context);
        if(insertionPoint<0) {
            insertionPoint=~insertionPoint;
        } else {
            ++insertionPoint;  
        }
        if(insertionPoint<j) {
            char *dest=array+insertionPoint*itemSize;
            uprv_memcpy(pv, item, itemSize);  
            uprv_memmove(dest+itemSize, dest, (j-insertionPoint)*itemSize);
            uprv_memcpy(dest, pv, itemSize);  
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

    doInsertionSort(array, length, itemSize, cmp, context, pv);

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
            doInsertionSort(array+start*itemSize, limit-start, itemSize, cmp, context, px);
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
