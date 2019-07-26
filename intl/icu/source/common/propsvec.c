

















#include <stdlib.h>
#include "unicode/utypes.h"
#include "cmemory.h"
#include "utrie.h"
#include "utrie2.h"
#include "uarrsort.h"
#include "propsvec.h"
#include "uassert.h"

struct UPropsVectors {
    uint32_t *v;
    int32_t columns;  
    int32_t maxRows;
    int32_t rows;
    int32_t prevRow;  
    UBool isCompacted;
};

#define UPVEC_INITIAL_ROWS (1<<12)
#define UPVEC_MEDIUM_ROWS ((int32_t)1<<16)
#define UPVEC_MAX_ROWS (UPVEC_MAX_CP+1)

U_CAPI UPropsVectors * U_EXPORT2
upvec_open(int32_t columns, UErrorCode *pErrorCode) {
    UPropsVectors *pv;
    uint32_t *v, *row;
    uint32_t cp;

    if(U_FAILURE(*pErrorCode)) {
        return NULL;
    }
    if(columns<1) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }
    columns+=2; 

    pv=(UPropsVectors *)uprv_malloc(sizeof(UPropsVectors));
    v=(uint32_t *)uprv_malloc(UPVEC_INITIAL_ROWS*columns*4);
    if(pv==NULL || v==NULL) {
        uprv_free(pv);
        uprv_free(v);
        *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    uprv_memset(pv, 0, sizeof(UPropsVectors));
    pv->v=v;
    pv->columns=columns;
    pv->maxRows=UPVEC_INITIAL_ROWS;
    pv->rows=2+(UPVEC_MAX_CP-UPVEC_FIRST_SPECIAL_CP);

    
    row=pv->v;
    uprv_memset(row, 0, pv->rows*columns*4);
    row[0]=0;
    row[1]=0x110000;
    row+=columns;
    for(cp=UPVEC_FIRST_SPECIAL_CP; cp<=UPVEC_MAX_CP; ++cp) {
        row[0]=cp;
        row[1]=cp+1;
        row+=columns;
    }
    return pv;
}

U_CAPI void U_EXPORT2
upvec_close(UPropsVectors *pv) {
    if(pv!=NULL) {
        uprv_free(pv->v);
        uprv_free(pv);
    }
}

static uint32_t *
_findRow(UPropsVectors *pv, UChar32 rangeStart) {
    uint32_t *row;
    int32_t columns, i, start, limit, prevRow;

    columns=pv->columns;
    limit=pv->rows;
    prevRow=pv->prevRow;

    
    row=pv->v+prevRow*columns;
    if(rangeStart>=(UChar32)row[0]) {
        if(rangeStart<(UChar32)row[1]) {
            
            return row;
        } else if(rangeStart<(UChar32)(row+=columns)[1]) {
            
            pv->prevRow=prevRow+1;
            return row;
        } else if(rangeStart<(UChar32)(row+=columns)[1]) {
            
            pv->prevRow=prevRow+2;
            return row;
        } else if((rangeStart-(UChar32)row[1])<10) {
            
            prevRow+=2;
            do {
                ++prevRow;
                row+=columns;
            } while(rangeStart>=(UChar32)row[1]);
            pv->prevRow=prevRow;
            return row;
        }
    } else if(rangeStart<(UChar32)pv->v[1]) {
        
        pv->prevRow=0;
        return pv->v;
    }

    
    start=0;
    while(start<limit-1) {
        i=(start+limit)/2;
        row=pv->v+i*columns;
        if(rangeStart<(UChar32)row[0]) {
            limit=i;
        } else if(rangeStart<(UChar32)row[1]) {
            pv->prevRow=i;
            return row;
        } else {
            start=i;
        }
    }

    
    pv->prevRow=start;
    return pv->v+start*columns;
}

U_CAPI void U_EXPORT2
upvec_setValue(UPropsVectors *pv,
               UChar32 start, UChar32 end,
               int32_t column,
               uint32_t value, uint32_t mask,
               UErrorCode *pErrorCode) {
    uint32_t *firstRow, *lastRow;
    int32_t columns;
    UChar32 limit;
    UBool splitFirstRow, splitLastRow;

    
    if(U_FAILURE(*pErrorCode)) {
        return;
    }
    if( pv==NULL ||
        start<0 || start>end || end>UPVEC_MAX_CP ||
        column<0 || column>=(pv->columns-2)
    ) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    if(pv->isCompacted) {
        *pErrorCode=U_NO_WRITE_PERMISSION;
        return;
    }
    limit=end+1;

    
    columns=pv->columns;
    column+=2; 
    value&=mask;

    

    
    firstRow=_findRow(pv, start);
    lastRow=_findRow(pv, end);

    




    splitFirstRow= (UBool)(start!=(UChar32)firstRow[0] && value!=(firstRow[column]&mask));
    splitLastRow= (UBool)(limit!=(UChar32)lastRow[1] && value!=(lastRow[column]&mask));

    
    if(splitFirstRow || splitLastRow) {
        int32_t count, rows;

        rows=pv->rows;
        if((rows+splitFirstRow+splitLastRow)>pv->maxRows) {
            uint32_t *newVectors;
            int32_t newMaxRows;

            if(pv->maxRows<UPVEC_MEDIUM_ROWS) {
                newMaxRows=UPVEC_MEDIUM_ROWS;
            } else if(pv->maxRows<UPVEC_MAX_ROWS) {
                newMaxRows=UPVEC_MAX_ROWS;
            } else {
                
                *pErrorCode=U_INTERNAL_PROGRAM_ERROR;
                return;
            }
            newVectors=(uint32_t *)uprv_malloc(newMaxRows*columns*4);
            if(newVectors==NULL) {
                *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
                return;
            }
            uprv_memcpy(newVectors, pv->v, rows*columns*4);
            firstRow=newVectors+(firstRow-pv->v);
            lastRow=newVectors+(lastRow-pv->v);
            uprv_free(pv->v);
            pv->v=newVectors;
            pv->maxRows=newMaxRows;
        }

        
        count = (int32_t)((pv->v+rows*columns)-(lastRow+columns));
        if(count>0) {
            uprv_memmove(
                lastRow+(1+splitFirstRow+splitLastRow)*columns,
                lastRow+columns,
                count*4);
        }
        pv->rows=rows+splitFirstRow+splitLastRow;

        
        if(splitFirstRow) {
            
            count = (int32_t)((lastRow-firstRow)+columns);
            uprv_memmove(firstRow+columns, firstRow, count*4);
            lastRow+=columns;

            
            firstRow[1]=firstRow[columns]=(uint32_t)start;
            firstRow+=columns;
        }

        
        if(splitLastRow) {
            
            uprv_memcpy(lastRow+columns, lastRow, columns*4);

            
            lastRow[1]=lastRow[columns]=(uint32_t)limit;
        }
    }

    
    pv->prevRow=(int32_t)((lastRow-(pv->v))/columns);

    
    firstRow+=column;
    lastRow+=column;
    mask=~mask;
    for(;;) {
        *firstRow=(*firstRow&mask)|value;
        if(firstRow==lastRow) {
            break;
        }
        firstRow+=columns;
    }
}

U_CAPI uint32_t U_EXPORT2
upvec_getValue(const UPropsVectors *pv, UChar32 c, int32_t column) {
    uint32_t *row;
    UPropsVectors *ncpv;

    if(pv->isCompacted || c<0 || c>UPVEC_MAX_CP || column<0 || column>=(pv->columns-2)) {
        return 0;
    }
    ncpv=(UPropsVectors *)pv;
    row=_findRow(ncpv, c);
    return row[2+column];
}

U_CAPI uint32_t * U_EXPORT2
upvec_getRow(const UPropsVectors *pv, int32_t rowIndex,
             UChar32 *pRangeStart, UChar32 *pRangeEnd) {
    uint32_t *row;
    int32_t columns;

    if(pv->isCompacted || rowIndex<0 || rowIndex>=pv->rows) {
        return NULL;
    }

    columns=pv->columns;
    row=pv->v+rowIndex*columns;
    if(pRangeStart!=NULL) {
        *pRangeStart=(UChar32)row[0];
    }
    if(pRangeEnd!=NULL) {
        *pRangeEnd=(UChar32)row[1]-1;
    }
    return row+2;
}

static int32_t U_CALLCONV
upvec_compareRows(const void *context, const void *l, const void *r) {
    const uint32_t *left=(const uint32_t *)l, *right=(const uint32_t *)r;
    const UPropsVectors *pv=(const UPropsVectors *)context;
    int32_t i, count, columns;

    count=columns=pv->columns; 

    
    i=2;
    do {
        if(left[i]!=right[i]) {
            return left[i]<right[i] ? -1 : 1;
        }
        if(++i==columns) {
            i=0;
        }
    } while(--count>0);

    return 0;
}

U_CAPI void U_EXPORT2
upvec_compact(UPropsVectors *pv, UPVecCompactHandler *handler, void *context, UErrorCode *pErrorCode) {
    uint32_t *row;
    int32_t i, columns, valueColumns, rows, count;
    UChar32 start, limit;

    
    if(U_FAILURE(*pErrorCode)) {
        return;
    }
    if(handler==NULL) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    if(pv->isCompacted) {
        return;
    }

    
    pv->isCompacted=TRUE;

    rows=pv->rows;
    columns=pv->columns;
    U_ASSERT(columns>=3); 
    valueColumns=columns-2; 

    
    uprv_sortArray(pv->v, rows, columns*4,
                   upvec_compareRows, pv, FALSE, pErrorCode);
    if(U_FAILURE(*pErrorCode)) {
        return;
    }

    




    row=pv->v;
    count=-valueColumns;
    for(i=0; i<rows; ++i) {
        start=(UChar32)row[0];

        
        if(count<0 || 0!=uprv_memcmp(row+2, row-valueColumns, valueColumns*4)) {
            count+=valueColumns;
        }

        if(start>=UPVEC_FIRST_SPECIAL_CP) {
            handler(context, start, start, count, row+2, valueColumns, pErrorCode);
            if(U_FAILURE(*pErrorCode)) {
                return;
            }
        }

        row+=columns;
    }

    
    count+=valueColumns;

    
    handler(context, UPVEC_START_REAL_VALUES_CP, UPVEC_START_REAL_VALUES_CP,
            count, row-valueColumns, valueColumns, pErrorCode);
    if(U_FAILURE(*pErrorCode)) {
        return;
    }

    






    row=pv->v;
    count=-valueColumns;
    for(i=0; i<rows; ++i) {
        
        start=(UChar32)row[0];
        limit=(UChar32)row[1];

        
        if(count<0 || 0!=uprv_memcmp(row+2, pv->v+count, valueColumns*4)) {
            count+=valueColumns;
            uprv_memmove(pv->v+count, row+2, valueColumns*4);
        }

        if(start<UPVEC_FIRST_SPECIAL_CP) {
            handler(context, start, limit-1, count, pv->v+count, valueColumns, pErrorCode);
            if(U_FAILURE(*pErrorCode)) {
                return;
            }
        }

        row+=columns;
    }

    
    pv->rows=count/valueColumns+1;
}

U_CAPI const uint32_t * U_EXPORT2
upvec_getArray(const UPropsVectors *pv, int32_t *pRows, int32_t *pColumns) {
    if(!pv->isCompacted) {
        return NULL;
    }
    if(pRows!=NULL) {
        *pRows=pv->rows;
    }
    if(pColumns!=NULL) {
        *pColumns=pv->columns-2;
    }
    return pv->v;
}

U_CAPI uint32_t * U_EXPORT2
upvec_cloneArray(const UPropsVectors *pv,
                 int32_t *pRows, int32_t *pColumns, UErrorCode *pErrorCode) {
    uint32_t *clonedArray;
    int32_t byteLength;

    if(U_FAILURE(*pErrorCode)) {
        return NULL;
    }
    if(!pv->isCompacted) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }
    byteLength=pv->rows*(pv->columns-2)*4;
    clonedArray=(uint32_t *)uprv_malloc(byteLength);
    if(clonedArray==NULL) {
        *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    uprv_memcpy(clonedArray, pv->v, byteLength);
    if(pRows!=NULL) {
        *pRows=pv->rows;
    }
    if(pColumns!=NULL) {
        *pColumns=pv->columns-2;
    }
    return clonedArray;
}

U_CAPI UTrie2 * U_EXPORT2
upvec_compactToUTrie2WithRowIndexes(UPropsVectors *pv, UErrorCode *pErrorCode) {
    UPVecToUTrie2Context toUTrie2={ NULL };
    upvec_compact(pv, upvec_compactToUTrie2Handler, &toUTrie2, pErrorCode);
    utrie2_freeze(toUTrie2.trie, UTRIE2_16_VALUE_BITS, pErrorCode);
    if(U_FAILURE(*pErrorCode)) {
        utrie2_close(toUTrie2.trie);
        toUTrie2.trie=NULL;
    }
    return toUTrie2.trie;
}






U_CAPI void U_CALLCONV
upvec_compactToUTrie2Handler(void *context,
                             UChar32 start, UChar32 end,
                             int32_t rowIndex, uint32_t *row, int32_t columns,
                             UErrorCode *pErrorCode) {
    UPVecToUTrie2Context *toUTrie2=(UPVecToUTrie2Context *)context;
    if(start<UPVEC_FIRST_SPECIAL_CP) {
        utrie2_setRange32(toUTrie2->trie, start, end, (uint32_t)rowIndex, TRUE, pErrorCode);
    } else {
        switch(start) {
        case UPVEC_INITIAL_VALUE_CP:
            toUTrie2->initialValue=rowIndex;
            break;
        case UPVEC_ERROR_VALUE_CP:
            toUTrie2->errorValue=rowIndex;
            break;
        case UPVEC_START_REAL_VALUES_CP:
            toUTrie2->maxValue=rowIndex;
            if(rowIndex>0xffff) {
                
                *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
            } else {
                toUTrie2->trie=utrie2_open(toUTrie2->initialValue,
                                           toUTrie2->errorValue, pErrorCode);
            }
            break;
        default:
            break;
        }
    }
}
