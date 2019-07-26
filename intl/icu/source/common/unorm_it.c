















#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION && !UCONFIG_NO_NORMALIZATION

#include "unicode/uiter.h"
#include "unicode/unorm.h"
#include "unicode/utf.h"
#include "unorm_it.h"
#include "cmemory.h"



enum {
    INITIAL_CAPACITY=100
};

struct UNormIterator {
    UCharIterator api;
    UCharIterator *iter;

    






    UChar *chars;
    uint32_t *states;

    





    int32_t capacity;

    
    uint32_t state;

    
    UBool hasPrevious, hasNext, isStackAllocated;

    UNormalizationMode mode;

    UChar charsBuffer[INITIAL_CAPACITY];
    uint32_t statesBuffer[INITIAL_CAPACITY+1]; 
};

static void
initIndexes(UNormIterator *uni, UCharIterator *iter) {
    
    UCharIterator *api=&uni->api;

    if(!iter->hasPrevious(iter)) {
        
        api->start=api->index=api->limit=0;
        uni->hasPrevious=FALSE;
        uni->hasNext=iter->hasNext(iter);
    } else if(!iter->hasNext(iter)) {
        
        api->start=api->index=api->limit=uni->capacity;
        uni->hasNext=FALSE;
        uni->hasPrevious=iter->hasPrevious(iter);
    } else {
        
        api->start=api->index=api->limit=uni->capacity/2;
        uni->hasPrevious=uni->hasNext=TRUE;
    }
}

static UBool
reallocArrays(UNormIterator *uni, int32_t capacity, UBool addAtStart) {
    
    UCharIterator *api=&uni->api;

    uint32_t *states;
    UChar *chars;
    int32_t start, limit;

    states=(uint32_t *)uprv_malloc((capacity+1)*4+capacity*2);
    if(states==NULL) {
        return FALSE;
    }

    chars=(UChar *)(states+(capacity+1));
    uni->capacity=capacity;

    start=api->start;
    limit=api->limit;

    if(addAtStart) {
        
        int32_t delta;

        delta=capacity-uni->capacity;
        uprv_memcpy(states+delta+start, uni->states+start, (limit-start+1)*4);
        uprv_memcpy(chars+delta+start, uni->chars+start, (limit-start)*4);

        api->start=start+delta;
        api->index+=delta;
        api->limit=limit+delta;
    } else {
        
        uprv_memcpy(states+start, uni->states+start, (limit-start+1)*4);
        uprv_memcpy(chars+start, uni->chars+start, (limit-start)*4);
    }

    uni->chars=chars;
    uni->states=states;

    return TRUE;
}

static void
moveContentsTowardStart(UCharIterator *api, UChar chars[], uint32_t states[], int32_t delta) {
    
    int32_t srcIndex, destIndex, limit;

    limit=api->limit;
    srcIndex=delta;
    if(srcIndex>api->start) {
        
        while(srcIndex<limit && states[srcIndex]==UITER_NO_STATE) {
            ++srcIndex;
        }
    }

    
    api->start=destIndex=0;
    while(srcIndex<limit) {
        chars[destIndex]=chars[srcIndex];
        states[destIndex++]=states[srcIndex++];
    }

    
    states[destIndex]=states[srcIndex];

    api->limit=destIndex;
}

static void
moveContentsTowardEnd(UCharIterator *api, UChar chars[], uint32_t states[], int32_t delta) {
    
    int32_t srcIndex, destIndex, start;

    start=api->start;
    destIndex=((UNormIterator *)api)->capacity;
    srcIndex=destIndex-delta;
    if(srcIndex<api->limit) {
        
        while(srcIndex>start && states[srcIndex]==UITER_NO_STATE) {
            --srcIndex;
        }
    }

    
    api->limit=destIndex;

    
    states[destIndex]=states[srcIndex];

    while(srcIndex>start) {
        chars[--destIndex]=chars[--srcIndex];
        states[destIndex]=states[srcIndex];
    }

    api->start=destIndex;
}


static UBool
readNext(UNormIterator *uni, UCharIterator *iter) {
    
    UCharIterator *api=&uni->api;

    
    int32_t limit, capacity, room;
    UErrorCode errorCode;

    limit=api->limit;
    capacity=uni->capacity;
    room=capacity/4;
    if(room>(capacity-limit)) {
        
        moveContentsTowardStart(api, uni->chars, uni->states, room);
        api->index=limit=api->limit;
        uni->hasPrevious=TRUE;
    }

    
    errorCode=U_ZERO_ERROR;
    if(uni->state!=uni->states[limit]) {
        uiter_setState(iter, uni->states[limit], &errorCode);
        if(U_FAILURE(errorCode)) {
            uni->state=UITER_NO_STATE;
            uni->hasNext=FALSE;
            return FALSE;
        }
    }

    room=unorm_next(iter, uni->chars+limit, capacity-limit, uni->mode, 0, TRUE, NULL, &errorCode);
    if(errorCode==U_BUFFER_OVERFLOW_ERROR) {
        if(room<=capacity) {
            
            uni->states[0]=uni->states[limit];
            api->start=api->index=api->limit=limit=0;
            uni->hasPrevious=TRUE;
        } else {
            capacity+=room+100;
            if(!reallocArrays(uni, capacity, FALSE)) {
                uni->state=UITER_NO_STATE;
                uni->hasNext=FALSE;
                return FALSE;
            }
            limit=api->limit;
        }

        errorCode=U_ZERO_ERROR;
        uiter_setState(iter, uni->states[limit], &errorCode);
        room=unorm_next(iter, uni->chars+limit, capacity-limit, uni->mode, 0, TRUE, NULL, &errorCode);
    }
    if(U_FAILURE(errorCode) || room==0) {
        uni->state=UITER_NO_STATE;
        uni->hasNext=FALSE;
        return FALSE;
    }

    
    ++limit; 
    for(--room; room>0; --room) {
        
        uni->states[limit++]=UITER_NO_STATE;
    }
    uni->states[limit]=uni->state=uiter_getState(iter);
    uni->hasNext=iter->hasNext(iter);
    api->limit=limit;
    return TRUE;
}


static UBool
readPrevious(UNormIterator *uni, UCharIterator *iter) {
    
    UCharIterator *api=&uni->api;

    
    int32_t start, capacity, room;
    UErrorCode errorCode;

    start=api->start;
    capacity=uni->capacity;
    room=capacity/4;
    if(room>start) {
        
        moveContentsTowardEnd(api, uni->chars, uni->states, room);
        api->index=start=api->start;
        uni->hasNext=TRUE;
    }

    
    errorCode=U_ZERO_ERROR;
    if(uni->state!=uni->states[start]) {
        uiter_setState(iter, uni->states[start], &errorCode);
        if(U_FAILURE(errorCode)) {
            uni->state=UITER_NO_STATE;
            uni->hasPrevious=FALSE;
            return FALSE;
        }
    }

    room=unorm_previous(iter, uni->chars, start, uni->mode, 0, TRUE, NULL, &errorCode);
    if(errorCode==U_BUFFER_OVERFLOW_ERROR) {
        if(room<=capacity) {
            
            uni->states[capacity]=uni->states[start];
            api->start=api->index=api->limit=start=capacity;
            uni->hasNext=TRUE;
        } else {
            capacity+=room+100;
            if(!reallocArrays(uni, capacity, TRUE)) {
                uni->state=UITER_NO_STATE;
                uni->hasPrevious=FALSE;
                return FALSE;
            }
            start=api->start;
        }

        errorCode=U_ZERO_ERROR;
        uiter_setState(iter, uni->states[start], &errorCode);
        room=unorm_previous(iter, uni->chars, start, uni->mode, 0, TRUE, NULL, &errorCode);
    }
    if(U_FAILURE(errorCode) || room==0) {
        uni->state=UITER_NO_STATE;
        uni->hasPrevious=FALSE;
        return FALSE;
    }

    
    do {
        
        uni->chars[--start]=uni->chars[--room];
        
        uni->states[start]=UITER_NO_STATE;
    } while(room>0);
    uni->states[start]=uni->state=uiter_getState(iter);
    uni->hasPrevious=iter->hasPrevious(iter);
    api->start=start;
    return TRUE;
}



static int32_t U_CALLCONV
unormIteratorGetIndex(UCharIterator *api, UCharIteratorOrigin origin) {
    switch(origin) {
    case UITER_ZERO:
    case UITER_START:
        return 0;
    case UITER_CURRENT:
    case UITER_LIMIT:
    case UITER_LENGTH:
        return UITER_UNKNOWN_INDEX;
    default:
        
        
        return -1;
    }
}

static int32_t U_CALLCONV
unormIteratorMove(UCharIterator *api, int32_t delta, UCharIteratorOrigin origin) {
    UNormIterator *uni=(UNormIterator *)api;
    UCharIterator *iter=uni->iter;
    int32_t pos;

    switch(origin) {
    case UITER_ZERO:
    case UITER_START:
        
        if(uni->hasPrevious) {
            iter->move(iter, 0, UITER_START);
            api->start=api->index=api->limit=0;
            uni->states[api->limit]=uni->state=uiter_getState(iter);
            uni->hasPrevious=FALSE;
            uni->hasNext=iter->hasNext(iter);
        } else {
            
            api->index=api->start;
        }
        break;
    case UITER_CURRENT:
        break;
    case UITER_LIMIT:
    case UITER_LENGTH:
        
        if(uni->hasNext) {
            iter->move(iter, 0, UITER_LIMIT);
            api->start=api->index=api->limit=uni->capacity;
            uni->states[api->limit]=uni->state=uiter_getState(iter);
            uni->hasPrevious=iter->hasPrevious(iter);
            uni->hasNext=FALSE;
        } else {
            
            api->index=api->limit;
        }
        break;
    default:
        return -1;  
    }

    
    if(delta==0) {
        
    } else if(delta>0) {
        
        for(;;) {
            pos=api->index+delta;   
            delta=pos-api->limit;   
            if(delta<=0) {
                api->index=pos;     
                break;
            }

            
            api->index=api->limit;
            if(!uni->hasNext || !readNext(uni, iter)) {
                break;              
            }
        }
    } else  {
        
        for(;;) {
            pos=api->index+delta;   
            delta=pos-api->start;   
            if(delta>=0) {
                api->index=pos;     
                break;
            }

            
            api->index=api->start;
            if(!uni->hasPrevious || !readPrevious(uni, iter)) {
                break;              
            }
        }
    }

    if(api->index==api->start && !uni->hasPrevious) {
        return 0;
    } else {
        return UITER_UNKNOWN_INDEX;
    }
}

static UBool U_CALLCONV
unormIteratorHasNext(UCharIterator *api) {
    return api->index<api->limit || ((UNormIterator *)api)->hasNext;
}

static UBool U_CALLCONV
unormIteratorHasPrevious(UCharIterator *api) {
    return api->index>api->start || ((UNormIterator *)api)->hasPrevious;
}

static UChar32 U_CALLCONV
unormIteratorCurrent(UCharIterator *api) {
    UNormIterator *uni=(UNormIterator *)api;

    if( api->index<api->limit ||
        (uni->hasNext && readNext(uni, uni->iter))
    ) {
        return uni->chars[api->index];
    } else {
        return U_SENTINEL;
    }
}

static UChar32 U_CALLCONV
unormIteratorNext(UCharIterator *api) {
    UNormIterator *uni=(UNormIterator *)api;

    if( api->index<api->limit ||
        (uni->hasNext && readNext(uni, uni->iter))
    ) {
        return uni->chars[api->index++];
    } else {
        return U_SENTINEL;
    }
}

static UChar32 U_CALLCONV
unormIteratorPrevious(UCharIterator *api) {
    UNormIterator *uni=(UNormIterator *)api;

    if( api->index>api->start ||
        (uni->hasPrevious && readPrevious(uni, uni->iter))
    ) {
        return uni->chars[--api->index];
    } else {
        return U_SENTINEL;
    }
}

static uint32_t U_CALLCONV
unormIteratorGetState(const UCharIterator *api) {
    
    return ((UNormIterator *)api)->states[api->index];
}

static void U_CALLCONV
unormIteratorSetState(UCharIterator *api, uint32_t state, UErrorCode *pErrorCode) {
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        
    } else if(api==NULL) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
    } else if(state==UITER_NO_STATE) {
        *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
    } else {
        UNormIterator *uni=(UNormIterator *)api;
        UCharIterator *iter=((UNormIterator *)api)->iter;
        if(state!=uni->state) {
            uni->state=state;
            uiter_setState(iter, state, pErrorCode);
        }

        





        if(state==uni->states[api->index]) {
            return;
        } else if(state==uni->states[api->limit]) {
            api->index=api->limit;
            return;
        } else {
            
            int32_t i;

            for(i=api->start; i<api->limit; ++i) {
                if(state==uni->states[i]) {
                    api->index=i;
                    return;
                }
            }
        }

        
        initIndexes((UNormIterator *)api, iter);
        uni->states[api->limit]=state;
    }
}

static const UCharIterator unormIterator={
    NULL, 0, 0, 0, 0, 0,
    unormIteratorGetIndex,
    unormIteratorMove,
    unormIteratorHasNext,
    unormIteratorHasPrevious,
    unormIteratorCurrent,
    unormIteratorNext,
    unormIteratorPrevious,
    NULL,
    unormIteratorGetState,
    unormIteratorSetState
};



U_CAPI UNormIterator * U_EXPORT2
unorm_openIter(void *stackMem, int32_t stackMemSize, UErrorCode *pErrorCode) {
    UNormIterator *uni;

    
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return NULL;
    }

    
    uni=NULL;
    if(stackMem!=NULL && stackMemSize>=sizeof(UNormIterator)) {
        if(U_ALIGNMENT_OFFSET(stackMem)==0) {
            
            uni=(UNormIterator *)stackMem;
        } else {
            int32_t align=(int32_t)U_ALIGNMENT_OFFSET_UP(stackMem);
            if((stackMemSize-=align)>=(int32_t)sizeof(UNormIterator)) {
                
                uni=(UNormIterator *)((char *)stackMem+align);
            }
        }
        
    }

    if(uni!=NULL) {
        uni->isStackAllocated=TRUE;
    } else {
        uni=(UNormIterator *)uprv_malloc(sizeof(UNormIterator));
        if(uni==NULL) {
            *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
            return NULL;
        }
        uni->isStackAllocated=FALSE;
    }

    



    uni->iter=NULL;
    uni->chars=uni->charsBuffer;
    uni->states=uni->statesBuffer;
    uni->capacity=INITIAL_CAPACITY;
    uni->state=UITER_NO_STATE;
    uni->hasPrevious=uni->hasNext=FALSE;
    uni->mode=UNORM_NONE;

    
    uiter_setString(&uni->api, NULL, 0);
    return uni;
}

U_CAPI void U_EXPORT2
unorm_closeIter(UNormIterator *uni) {
    if(uni!=NULL) {
        if(uni->states!=uni->statesBuffer) {
            
            uprv_free(uni->states);
        }
        if(!uni->isStackAllocated) {
            uprv_free(uni);
        }
    }
}

U_CAPI UCharIterator * U_EXPORT2
unorm_setIter(UNormIterator *uni, UCharIterator *iter, UNormalizationMode mode, UErrorCode *pErrorCode) {
    
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return NULL;
    }
    if(uni==NULL) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }
    if( iter==NULL || iter->getState==NULL || iter->setState==NULL ||
        mode<UNORM_NONE || UNORM_MODE_COUNT<=mode
    ) {
        
        uiter_setString(&uni->api, NULL, 0);
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }

    
    uprv_memcpy(&uni->api, &unormIterator, sizeof(unormIterator));

    uni->iter=iter;
    uni->mode=mode;

    initIndexes(uni, iter);
    uni->states[uni->api.limit]=uni->state=uiter_getState(iter);

    return &uni->api;
}

#endif 
