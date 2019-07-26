















#include "cmemory.h"
#include "unicode/utypes.h"
#include "unicode/ustring.h"
#include "unicode/uchar.h"
#include "unicode/ubidi.h"
#include "ubidiimp.h"
#include "uassert.h"

#ifndef U_COMMON_IMPLEMENTATION
#error U_COMMON_IMPLEMENTATION not set - must be set for all ICU source files in common/ - see http://userguide.icu-project.org/howtouseicu
#endif

































































static void
setTrailingWSStart(UBiDi *pBiDi) {
    

    const DirProp *dirProps=pBiDi->dirProps;
    UBiDiLevel *levels=pBiDi->levels;
    int32_t start=pBiDi->length;
    UBiDiLevel paraLevel=pBiDi->paraLevel;

    





    if(NO_CONTEXT_RTL(dirProps[start-1])==B) {
        pBiDi->trailingWSStart=start;   
        return;
    }
    
    while(start>0 && DIRPROP_FLAG_NC(dirProps[start-1])&MASK_WS) {
        --start;
    }

    
    while(start>0 && levels[start-1]==paraLevel) {
        --start;
    }

    pBiDi->trailingWSStart=start;
}



U_CAPI void U_EXPORT2
ubidi_setLine(const UBiDi *pParaBiDi,
              int32_t start, int32_t limit,
              UBiDi *pLineBiDi,
              UErrorCode *pErrorCode) {
    int32_t length;

    
    RETURN_VOID_IF_NULL_OR_FAILING_ERRCODE(pErrorCode);
    RETURN_VOID_IF_NOT_VALID_PARA(pParaBiDi, *pErrorCode);
    RETURN_VOID_IF_BAD_RANGE(start, 0, limit, *pErrorCode);
    RETURN_VOID_IF_BAD_RANGE(limit, 0, pParaBiDi->length+1, *pErrorCode);
    if(pLineBiDi==NULL) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    if(ubidi_getParagraph(pParaBiDi, start, NULL, NULL, NULL, pErrorCode) !=
       ubidi_getParagraph(pParaBiDi, limit-1, NULL, NULL, NULL, pErrorCode)) {
        
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }

    
    pLineBiDi->pParaBiDi=NULL;          
    pLineBiDi->text=pParaBiDi->text+start;
    length=pLineBiDi->length=limit-start;
    pLineBiDi->resultLength=pLineBiDi->originalLength=length;
    pLineBiDi->paraLevel=GET_PARALEVEL(pParaBiDi, start);
    pLineBiDi->paraCount=pParaBiDi->paraCount;
    pLineBiDi->runs=NULL;
    pLineBiDi->flags=0;
    pLineBiDi->reorderingMode=pParaBiDi->reorderingMode;
    pLineBiDi->reorderingOptions=pParaBiDi->reorderingOptions;
    pLineBiDi->controlCount=0;
    if(pParaBiDi->controlCount>0) {
        int32_t j;
        for(j=start; j<limit; j++) {
            if(IS_BIDI_CONTROL_CHAR(pParaBiDi->text[j])) {
                pLineBiDi->controlCount++;
            }
        }
        pLineBiDi->resultLength-=pLineBiDi->controlCount;
    }

    pLineBiDi->dirProps=pParaBiDi->dirProps+start;
    pLineBiDi->levels=pParaBiDi->levels+start;
    pLineBiDi->runCount=-1;

    if(pParaBiDi->direction!=UBIDI_MIXED) {
        
        pLineBiDi->direction=pParaBiDi->direction;

        




        if(pParaBiDi->trailingWSStart<=start) {
            pLineBiDi->trailingWSStart=0;
        } else if(pParaBiDi->trailingWSStart<limit) {
            pLineBiDi->trailingWSStart=pParaBiDi->trailingWSStart-start;
        } else {
            pLineBiDi->trailingWSStart=length;
        }
    } else {
        const UBiDiLevel *levels=pLineBiDi->levels;
        int32_t i, trailingWSStart;
        UBiDiLevel level;

        setTrailingWSStart(pLineBiDi);
        trailingWSStart=pLineBiDi->trailingWSStart;

        
        if(trailingWSStart==0) {
            
            pLineBiDi->direction=(UBiDiDirection)(pLineBiDi->paraLevel&1);
        } else {
            
            level=(UBiDiLevel)(levels[0]&1);

            
            if(trailingWSStart<length && (pLineBiDi->paraLevel&1)!=level) {
                
                pLineBiDi->direction=UBIDI_MIXED;
            } else {
                
                i=1;
                for(;;) {
                    if(i==trailingWSStart) {
                        
                        pLineBiDi->direction=(UBiDiDirection)level;
                        break;
                    } else if((levels[i]&1)!=level) {
                        pLineBiDi->direction=UBIDI_MIXED;
                        break;
                    }
                    ++i;
                }
            }
        }

        switch(pLineBiDi->direction) {
        case UBIDI_LTR:
            
            pLineBiDi->paraLevel=(UBiDiLevel)((pLineBiDi->paraLevel+1)&~1);

            
            pLineBiDi->trailingWSStart=0;
            break;
        case UBIDI_RTL:
            
            pLineBiDi->paraLevel|=1;

            
            pLineBiDi->trailingWSStart=0;
            break;
        default:
            break;
        }
    }
    pLineBiDi->pParaBiDi=pParaBiDi;     
    return;
}

U_CAPI UBiDiLevel U_EXPORT2
ubidi_getLevelAt(const UBiDi *pBiDi, int32_t charIndex) {
    
    if(!IS_VALID_PARA_OR_LINE(pBiDi) || charIndex<0 || pBiDi->length<=charIndex) {
        return 0;
    } else if(pBiDi->direction!=UBIDI_MIXED || charIndex>=pBiDi->trailingWSStart) {
        return GET_PARALEVEL(pBiDi, charIndex);
    } else {
        return pBiDi->levels[charIndex];
    }
}

U_CAPI const UBiDiLevel * U_EXPORT2
ubidi_getLevels(UBiDi *pBiDi, UErrorCode *pErrorCode) {
    int32_t start, length;

    RETURN_IF_NULL_OR_FAILING_ERRCODE(pErrorCode, NULL);
    RETURN_IF_NOT_VALID_PARA_OR_LINE(pBiDi, *pErrorCode, NULL);
    if((length=pBiDi->length)<=0) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }
    if((start=pBiDi->trailingWSStart)==length) {
        
        return pBiDi->levels;
    }

    






    if(getLevelsMemory(pBiDi, length)) {
        UBiDiLevel *levels=pBiDi->levelsMemory;

        if(start>0 && levels!=pBiDi->levels) {
            uprv_memcpy(levels, pBiDi->levels, start);
        }
        

        uprv_memset(levels+start, pBiDi->paraLevel, length-start);

        
        pBiDi->trailingWSStart=length;
        return pBiDi->levels=levels;
    } else {
        
        *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
}

U_CAPI void U_EXPORT2
ubidi_getLogicalRun(const UBiDi *pBiDi, int32_t logicalPosition,
                    int32_t *pLogicalLimit, UBiDiLevel *pLevel) {
    UErrorCode errorCode;
    int32_t runCount, visualStart, logicalLimit, logicalFirst, i;
    Run iRun;

    errorCode=U_ZERO_ERROR;
    RETURN_VOID_IF_BAD_RANGE(logicalPosition, 0, pBiDi->length, errorCode);
    
    runCount=ubidi_countRuns((UBiDi *)pBiDi, &errorCode);
    if(U_FAILURE(errorCode)) {
        return;
    }
    


    visualStart=logicalLimit=0;
    iRun=pBiDi->runs[0];

    for(i=0; i<runCount; i++) {
        iRun = pBiDi->runs[i];
        logicalFirst=GET_INDEX(iRun.logicalStart);
        logicalLimit=logicalFirst+iRun.visualLimit-visualStart;
        if((logicalPosition>=logicalFirst) &&
           (logicalPosition<logicalLimit)) {
            break;
        }
        visualStart = iRun.visualLimit;
    }
    if(pLogicalLimit) {
        *pLogicalLimit=logicalLimit;
    }
    if(pLevel) {
        if(pBiDi->reorderingMode==UBIDI_REORDER_RUNS_ONLY) {
            *pLevel=(UBiDiLevel)GET_ODD_BIT(iRun.logicalStart);
        }
        else if(pBiDi->direction!=UBIDI_MIXED || logicalPosition>=pBiDi->trailingWSStart) {
            *pLevel=GET_PARALEVEL(pBiDi, logicalPosition);
        } else {
        *pLevel=pBiDi->levels[logicalPosition];
        }
    }
}



U_CAPI int32_t U_EXPORT2
ubidi_countRuns(UBiDi *pBiDi, UErrorCode *pErrorCode) {
    RETURN_IF_NULL_OR_FAILING_ERRCODE(pErrorCode, -1);
    RETURN_IF_NOT_VALID_PARA_OR_LINE(pBiDi, *pErrorCode, -1);
    ubidi_getRuns(pBiDi, pErrorCode);
    if(U_FAILURE(*pErrorCode)) {
        return -1;
    }
    return pBiDi->runCount;
}

U_CAPI UBiDiDirection U_EXPORT2
ubidi_getVisualRun(UBiDi *pBiDi, int32_t runIndex,
                   int32_t *pLogicalStart, int32_t *pLength)
{
    int32_t start;
    UErrorCode errorCode = U_ZERO_ERROR;
    RETURN_IF_NOT_VALID_PARA_OR_LINE(pBiDi, errorCode, UBIDI_LTR);
    ubidi_getRuns(pBiDi, &errorCode);
    if(U_FAILURE(errorCode)) {
        return UBIDI_LTR;
    }
    RETURN_IF_BAD_RANGE(runIndex, 0, pBiDi->runCount, errorCode, UBIDI_LTR);

    start=pBiDi->runs[runIndex].logicalStart;
    if(pLogicalStart!=NULL) {
        *pLogicalStart=GET_INDEX(start);
    }
    if(pLength!=NULL) {
        if(runIndex>0) {
            *pLength=pBiDi->runs[runIndex].visualLimit-
                     pBiDi->runs[runIndex-1].visualLimit;
        } else {
            *pLength=pBiDi->runs[0].visualLimit;
        }
    }
    return (UBiDiDirection)GET_ODD_BIT(start);
}


static void
getSingleRun(UBiDi *pBiDi, UBiDiLevel level) {
    
    pBiDi->runs=pBiDi->simpleRuns;
    pBiDi->runCount=1;

    
    pBiDi->runs[0].logicalStart=MAKE_INDEX_ODD_PAIR(0, level);
    pBiDi->runs[0].visualLimit=pBiDi->length;
    pBiDi->runs[0].insertRemove=0;
}


































static void
reorderLine(UBiDi *pBiDi, UBiDiLevel minLevel, UBiDiLevel maxLevel) {
    Run *runs, tempRun;
    UBiDiLevel *levels;
    int32_t firstRun, endRun, limitRun, runCount;

    
    if(maxLevel<=(minLevel|1)) {
        return;
    }

    




    ++minLevel;

    runs=pBiDi->runs;
    levels=pBiDi->levels;
    runCount=pBiDi->runCount;

    
    if(pBiDi->trailingWSStart<pBiDi->length) {
        --runCount;
    }

    while(--maxLevel>=minLevel) {
        firstRun=0;

        
        for(;;) {
            
            
            while(firstRun<runCount && levels[runs[firstRun].logicalStart]<maxLevel) {
                ++firstRun;
            }
            if(firstRun>=runCount) {
                break;  
            }

            
            for(limitRun=firstRun; ++limitRun<runCount && levels[runs[limitRun].logicalStart]>=maxLevel;) {}

            
            endRun=limitRun-1;
            while(firstRun<endRun) {
                tempRun = runs[firstRun];
                runs[firstRun]=runs[endRun];
                runs[endRun]=tempRun;
                ++firstRun;
                --endRun;
            }

            if(limitRun==runCount) {
                break;  
            } else {
                firstRun=limitRun+1;
            }
        }
    }

    
    if(!(minLevel&1)) {
        firstRun=0;

        
        if(pBiDi->trailingWSStart==pBiDi->length) {
            --runCount;
        }

        
        while(firstRun<runCount) {
            tempRun=runs[firstRun];
            runs[firstRun]=runs[runCount];
            runs[runCount]=tempRun;
            ++firstRun;
            --runCount;
        }
    }
}



static int32_t getRunFromLogicalIndex(UBiDi *pBiDi, int32_t logicalIndex, UErrorCode *pErrorCode) {
    Run *runs=pBiDi->runs;
    int32_t runCount=pBiDi->runCount, visualStart=0, i, length, logicalStart;

    for(i=0; i<runCount; i++) {
        length=runs[i].visualLimit-visualStart;
        logicalStart=GET_INDEX(runs[i].logicalStart);
        if((logicalIndex>=logicalStart) && (logicalIndex<(logicalStart+length))) {
            return i;
        }
        visualStart+=length;
    }
    
    U_ASSERT(FALSE);
    *pErrorCode = U_INVALID_STATE_ERROR;
    return 0;
}












U_CFUNC UBool
ubidi_getRuns(UBiDi *pBiDi, UErrorCode *pErrorCode) {
    



    if (pBiDi->runCount>=0) {
        return TRUE;
    }

    if(pBiDi->direction!=UBIDI_MIXED) {
        
        
        getSingleRun(pBiDi, pBiDi->paraLevel);
    } else  {
        
        int32_t length=pBiDi->length, limit;
        UBiDiLevel *levels=pBiDi->levels;
        int32_t i, runCount;
        UBiDiLevel level=UBIDI_DEFAULT_LTR;   
        










        limit=pBiDi->trailingWSStart;
        
        runCount=0;
        for(i=0; i<limit; ++i) {
            
            if(levels[i]!=level) {
                ++runCount;
                level=levels[i];
            }
        }

        



        if(runCount==1 && limit==length) {
            
            getSingleRun(pBiDi, levels[0]);
        } else  {
            
            Run *runs;
            int32_t runIndex, start;
            UBiDiLevel minLevel=UBIDI_MAX_EXPLICIT_LEVEL+1, maxLevel=0;

            
            if(limit<length) {
                ++runCount;
            }

            
            if(getRunsMemory(pBiDi, runCount)) {
                runs=pBiDi->runsMemory;
            } else {
                return FALSE;
            }

            
            




            runIndex=0;

            
            i=0;
            do {
                
                start=i;
                level=levels[i];
                if(level<minLevel) {
                    minLevel=level;
                }
                if(level>maxLevel) {
                    maxLevel=level;
                }

                
                while(++i<limit && levels[i]==level) {}

                
                runs[runIndex].logicalStart=start;
                runs[runIndex].visualLimit=i-start;
                runs[runIndex].insertRemove=0;
                ++runIndex;
            } while(i<limit);

            if(limit<length) {
                
                runs[runIndex].logicalStart=limit;
                runs[runIndex].visualLimit=length-limit;
                

                if(pBiDi->paraLevel<minLevel) {
                    minLevel=pBiDi->paraLevel;
                }
            }

            
            pBiDi->runs=runs;
            pBiDi->runCount=runCount;

            reorderLine(pBiDi, minLevel, maxLevel);

            
            
            limit=0;
            for(i=0; i<runCount; ++i) {
                ADD_ODD_BIT_FROM_LEVEL(runs[i].logicalStart, levels[runs[i].logicalStart]);
                limit+=runs[i].visualLimit;
                runs[i].visualLimit=limit;
            }

            
            
            

            if(runIndex<runCount) {
                int32_t trailingRun = ((pBiDi->paraLevel & 1) != 0)? 0 : runIndex;

                ADD_ODD_BIT_FROM_LEVEL(runs[trailingRun].logicalStart, pBiDi->paraLevel);
            }
        }
    }

    
    if(pBiDi->insertPoints.size>0) {
        Point *point, *start=pBiDi->insertPoints.points,
                      *limit=start+pBiDi->insertPoints.size;
        int32_t runIndex;
        for(point=start; point<limit; point++) {
            runIndex=getRunFromLogicalIndex(pBiDi, point->pos, pErrorCode);
            pBiDi->runs[runIndex].insertRemove|=point->flag;
        }
    }

    
    if(pBiDi->controlCount>0) {
        int32_t runIndex;
        const UChar *start=pBiDi->text, *limit=start+pBiDi->length, *pu;
        for(pu=start; pu<limit; pu++) {
            if(IS_BIDI_CONTROL_CHAR(*pu)) {
                runIndex=getRunFromLogicalIndex(pBiDi, (int32_t)(pu-start), pErrorCode);
                pBiDi->runs[runIndex].insertRemove--;
            }
        }
    }

    return TRUE;
}

static UBool
prepareReorder(const UBiDiLevel *levels, int32_t length,
               int32_t *indexMap,
               UBiDiLevel *pMinLevel, UBiDiLevel *pMaxLevel) {
    int32_t start;
    UBiDiLevel level, minLevel, maxLevel;

    if(levels==NULL || length<=0) {
        return FALSE;
    }

    
    minLevel=UBIDI_MAX_EXPLICIT_LEVEL+1;
    maxLevel=0;
    for(start=length; start>0;) {
        level=levels[--start];
        if(level>UBIDI_MAX_EXPLICIT_LEVEL+1) {
            return FALSE;
        }
        if(level<minLevel) {
            minLevel=level;
        }
        if(level>maxLevel) {
            maxLevel=level;
        }
    }
    *pMinLevel=minLevel;
    *pMaxLevel=maxLevel;

    
    for(start=length; start>0;) {
        --start;
        indexMap[start]=start;
    }

    return TRUE;
}



U_CAPI void U_EXPORT2
ubidi_reorderLogical(const UBiDiLevel *levels, int32_t length, int32_t *indexMap) {
    int32_t start, limit, sumOfSosEos;
    UBiDiLevel minLevel = 0, maxLevel = 0;

    if(indexMap==NULL || !prepareReorder(levels, length, indexMap, &minLevel, &maxLevel)) {
        return;
    }

    
    if(minLevel==maxLevel && (minLevel&1)==0) {
        return;
    }

    
    minLevel|=1;

    
    do {
        start=0;

        
        for(;;) {
            
            
            while(start<length && levels[start]<maxLevel) {
                ++start;
            }
            if(start>=length) {
                break;  
            }

            
            for(limit=start; ++limit<length && levels[limit]>=maxLevel;) {}

            










            sumOfSosEos=start+limit-1;

            
            do {
                indexMap[start]=sumOfSosEos-indexMap[start];
            } while(++start<limit);

            
            if(limit==length) {
                break;  
            } else {
                start=limit+1;
            }
        }
    } while(--maxLevel>=minLevel);
}

U_CAPI void U_EXPORT2
ubidi_reorderVisual(const UBiDiLevel *levels, int32_t length, int32_t *indexMap) {
    int32_t start, end, limit, temp;
    UBiDiLevel minLevel = 0, maxLevel = 0;

    if(indexMap==NULL || !prepareReorder(levels, length, indexMap, &minLevel, &maxLevel)) {
        return;
    }

    
    if(minLevel==maxLevel && (minLevel&1)==0) {
        return;
    }

    
    minLevel|=1;

    
    do {
        start=0;

        
        for(;;) {
            
            
            while(start<length && levels[start]<maxLevel) {
                ++start;
            }
            if(start>=length) {
                break;  
            }

            
            for(limit=start; ++limit<length && levels[limit]>=maxLevel;) {}

            





            end=limit-1;
            while(start<end) {
                temp=indexMap[start];
                indexMap[start]=indexMap[end];
                indexMap[end]=temp;

                ++start;
                --end;
            }

            if(limit==length) {
                break;  
            } else {
                start=limit+1;
            }
        }
    } while(--maxLevel>=minLevel);
}



U_CAPI int32_t U_EXPORT2
ubidi_getVisualIndex(UBiDi *pBiDi, int32_t logicalIndex, UErrorCode *pErrorCode) {
    int32_t visualIndex=UBIDI_MAP_NOWHERE;
    RETURN_IF_NULL_OR_FAILING_ERRCODE(pErrorCode, -1);
    RETURN_IF_NOT_VALID_PARA_OR_LINE(pBiDi, *pErrorCode, -1);
    RETURN_IF_BAD_RANGE(logicalIndex, 0, pBiDi->length, *pErrorCode, -1);

    
    switch(pBiDi->direction) {
    case UBIDI_LTR:
        visualIndex=logicalIndex;
        break;
    case UBIDI_RTL:
        visualIndex=pBiDi->length-logicalIndex-1;
        break;
    default:
        if(!ubidi_getRuns(pBiDi, pErrorCode)) {
            *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
            return -1;
        } else {
            Run *runs=pBiDi->runs;
            int32_t i, visualStart=0, offset, length;

            
            for(i=0; i<pBiDi->runCount; ++i) {
                length=runs[i].visualLimit-visualStart;
                offset=logicalIndex-GET_INDEX(runs[i].logicalStart);
                if(offset>=0 && offset<length) {
                    if(IS_EVEN_RUN(runs[i].logicalStart)) {
                        
                        visualIndex=visualStart+offset;
                    } else {
                        
                        visualIndex=visualStart+length-offset-1;
                    }
                    break;          
                }
                visualStart+=length;
            }
            if(i>=pBiDi->runCount) {
                return UBIDI_MAP_NOWHERE;
            }
        }
    }

    if(pBiDi->insertPoints.size>0) {
        
        Run *runs=pBiDi->runs;
        int32_t i, length, insertRemove;
        int32_t visualStart=0, markFound=0;
        for(i=0; ; i++, visualStart+=length) {
            length=runs[i].visualLimit-visualStart;
            insertRemove=runs[i].insertRemove;
            if(insertRemove & (LRM_BEFORE|RLM_BEFORE)) {
                markFound++;
            }
            
            if(visualIndex<runs[i].visualLimit) {
                return visualIndex+markFound;
            }
            if(insertRemove & (LRM_AFTER|RLM_AFTER)) {
                markFound++;
            }
        }
    }
    else if(pBiDi->controlCount>0) {
        
        Run *runs=pBiDi->runs;
        int32_t i, j, start, limit, length, insertRemove;
        int32_t visualStart=0, controlFound=0;
        UChar uchar=pBiDi->text[logicalIndex];
        
        if(IS_BIDI_CONTROL_CHAR(uchar)) {
            return UBIDI_MAP_NOWHERE;
        }
        
        for(i=0; ; i++, visualStart+=length) {
            length=runs[i].visualLimit-visualStart;
            insertRemove=runs[i].insertRemove;
            
            if(visualIndex>=runs[i].visualLimit) {
                controlFound-=insertRemove;
                continue;
            }
            
            if(insertRemove==0) {
                return visualIndex-controlFound;
            }
            if(IS_EVEN_RUN(runs[i].logicalStart)) {
                
                start=runs[i].logicalStart;
                limit=logicalIndex;
            } else {
                
                start=logicalIndex+1;
                limit=GET_INDEX(runs[i].logicalStart)+length;
            }
            for(j=start; j<limit; j++) {
                uchar=pBiDi->text[j];
                if(IS_BIDI_CONTROL_CHAR(uchar)) {
                    controlFound++;
                }
            }
            return visualIndex-controlFound;
        }
    }

    return visualIndex;
}

U_CAPI int32_t U_EXPORT2
ubidi_getLogicalIndex(UBiDi *pBiDi, int32_t visualIndex, UErrorCode *pErrorCode) {
    Run *runs;
    int32_t i, runCount, start;
    RETURN_IF_NULL_OR_FAILING_ERRCODE(pErrorCode, -1);
    RETURN_IF_NOT_VALID_PARA_OR_LINE(pBiDi, *pErrorCode, -1);
    RETURN_IF_BAD_RANGE(visualIndex, 0, pBiDi->resultLength, *pErrorCode, -1);
    
    if(pBiDi->insertPoints.size==0 && pBiDi->controlCount==0) {
        if(pBiDi->direction==UBIDI_LTR) {
            return visualIndex;
        }
        else if(pBiDi->direction==UBIDI_RTL) {
            return pBiDi->length-visualIndex-1;
        }
    }
    if(!ubidi_getRuns(pBiDi, pErrorCode)) {
        *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
        return -1;
    }

    runs=pBiDi->runs;
    runCount=pBiDi->runCount;
    if(pBiDi->insertPoints.size>0) {
        
        int32_t markFound=0, insertRemove;
        int32_t visualStart=0, length;
        runs=pBiDi->runs;
        
        for(i=0; ; i++, visualStart+=length) {
            length=runs[i].visualLimit-visualStart;
            insertRemove=runs[i].insertRemove;
            if(insertRemove&(LRM_BEFORE|RLM_BEFORE)) {
                if(visualIndex<=(visualStart+markFound)) {
                    return UBIDI_MAP_NOWHERE;
                }
                markFound++;
            }
            
            if(visualIndex<(runs[i].visualLimit+markFound)) {
                visualIndex-=markFound;
                break;
            }
            if(insertRemove&(LRM_AFTER|RLM_AFTER)) {
                if(visualIndex==(visualStart+length+markFound)) {
                    return UBIDI_MAP_NOWHERE;
                }
                markFound++;
            }
        }
    }
    else if(pBiDi->controlCount>0) {
        
        int32_t controlFound=0, insertRemove, length;
        int32_t logicalStart, logicalEnd, visualStart=0, j, k;
        UChar uchar;
        UBool evenRun;
        
        for(i=0; ; i++, visualStart+=length) {
            length=runs[i].visualLimit-visualStart;
            insertRemove=runs[i].insertRemove;
            
            if(visualIndex>=(runs[i].visualLimit-controlFound+insertRemove)) {
                controlFound-=insertRemove;
                continue;
            }
            
            if(insertRemove==0) {
                visualIndex+=controlFound;
                break;
            }
            
            logicalStart=runs[i].logicalStart;
            evenRun=IS_EVEN_RUN(logicalStart);
            REMOVE_ODD_BIT(logicalStart);
            logicalEnd=logicalStart+length-1;
            for(j=0; j<length; j++) {
                k= evenRun ? logicalStart+j : logicalEnd-j;
                uchar=pBiDi->text[k];
                if(IS_BIDI_CONTROL_CHAR(uchar)) {
                    controlFound++;
                }
                if((visualIndex+controlFound)==(visualStart+j)) {
                    break;
                }
            }
            visualIndex+=controlFound;
            break;
        }
    }
    
    if(runCount<=10) {
        
        for(i=0; visualIndex>=runs[i].visualLimit; ++i) {}
    } else {
        
        int32_t begin=0, limit=runCount;

        
        for(;;) {
            i=(begin+limit)/2;
            if(visualIndex>=runs[i].visualLimit) {
                begin=i+1;
            } else if(i==0 || visualIndex>=runs[i-1].visualLimit) {
                break;
            } else {
                limit=i;
            }
        }
    }

    start=runs[i].logicalStart;
    if(IS_EVEN_RUN(start)) {
        
        
        if(i>0) {
            visualIndex-=runs[i-1].visualLimit;
        }
        return start+visualIndex;
    } else {
        
        return GET_INDEX(start)+runs[i].visualLimit-visualIndex-1;
    }
}

U_CAPI void U_EXPORT2
ubidi_getLogicalMap(UBiDi *pBiDi, int32_t *indexMap, UErrorCode *pErrorCode) {
    RETURN_VOID_IF_NULL_OR_FAILING_ERRCODE(pErrorCode);
    
    ubidi_countRuns(pBiDi, pErrorCode);
    if(U_FAILURE(*pErrorCode)) {
        
    } else if(indexMap==NULL) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
    } else {
        
        int32_t visualStart, visualLimit, i, j, k;
        int32_t logicalStart, logicalLimit;
        Run *runs=pBiDi->runs;
        if (pBiDi->length<=0) {
            return;
        }
        if (pBiDi->length>pBiDi->resultLength) {
            uprv_memset(indexMap, 0xFF, pBiDi->length*sizeof(int32_t));
        }

        visualStart=0;
        for(j=0; j<pBiDi->runCount; ++j) {
            logicalStart=GET_INDEX(runs[j].logicalStart);
            visualLimit=runs[j].visualLimit;
            if(IS_EVEN_RUN(runs[j].logicalStart)) {
                do { 
                    indexMap[logicalStart++]=visualStart++;
                } while(visualStart<visualLimit);
            } else {
                logicalStart+=visualLimit-visualStart;  
                do { 
                    indexMap[--logicalStart]=visualStart++;
                } while(visualStart<visualLimit);
            }
            
        }

        if(pBiDi->insertPoints.size>0) {
            int32_t markFound=0, runCount=pBiDi->runCount;
            int32_t length, insertRemove;
            visualStart=0;
            
            for(i=0; i<runCount; i++, visualStart+=length) {
                length=runs[i].visualLimit-visualStart;
                insertRemove=runs[i].insertRemove;
                if(insertRemove&(LRM_BEFORE|RLM_BEFORE)) {
                    markFound++;
                }
                if(markFound>0) {
                    logicalStart=GET_INDEX(runs[i].logicalStart);
                    logicalLimit=logicalStart+length;
                    for(j=logicalStart; j<logicalLimit; j++) {
                        indexMap[j]+=markFound;
                    }
                }
                if(insertRemove&(LRM_AFTER|RLM_AFTER)) {
                    markFound++;
                }
            }
        }
        else if(pBiDi->controlCount>0) {
            int32_t controlFound=0, runCount=pBiDi->runCount;
            int32_t length, insertRemove;
            UBool evenRun;
            UChar uchar;
            visualStart=0;
            
            for(i=0; i<runCount; i++, visualStart+=length) {
                length=runs[i].visualLimit-visualStart;
                insertRemove=runs[i].insertRemove;
                
                if((controlFound-insertRemove)==0) {
                    continue;
                }
                logicalStart=runs[i].logicalStart;
                evenRun=IS_EVEN_RUN(logicalStart);
                REMOVE_ODD_BIT(logicalStart);
                logicalLimit=logicalStart+length;
                
                if(insertRemove==0) {
                    for(j=logicalStart; j<logicalLimit; j++) {
                        indexMap[j]-=controlFound;
                    }
                    continue;
                }
                for(j=0; j<length; j++) {
                    k= evenRun ? logicalStart+j : logicalLimit-j-1;
                    uchar=pBiDi->text[k];
                    if(IS_BIDI_CONTROL_CHAR(uchar)) {
                        controlFound++;
                        indexMap[k]=UBIDI_MAP_NOWHERE;
                        continue;
                    }
                    indexMap[k]-=controlFound;
                }
            }
        }
    }
}

U_CAPI void U_EXPORT2
ubidi_getVisualMap(UBiDi *pBiDi, int32_t *indexMap, UErrorCode *pErrorCode) {
    RETURN_VOID_IF_NULL_OR_FAILING_ERRCODE(pErrorCode);
    if(indexMap==NULL) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    
    ubidi_countRuns(pBiDi, pErrorCode);
    if(U_SUCCESS(*pErrorCode)) {
        
        Run *runs=pBiDi->runs, *runsLimit=runs+pBiDi->runCount;
        int32_t logicalStart, visualStart, visualLimit, *pi=indexMap;

        if (pBiDi->resultLength<=0) {
            return;
        }
        visualStart=0;
        for(; runs<runsLimit; ++runs) {
            logicalStart=runs->logicalStart;
            visualLimit=runs->visualLimit;
            if(IS_EVEN_RUN(logicalStart)) {
                do { 
                    *pi++ = logicalStart++;
                } while(++visualStart<visualLimit);
            } else {
                REMOVE_ODD_BIT(logicalStart);
                logicalStart+=visualLimit-visualStart;  
                do { 
                    *pi++ = --logicalStart;
                } while(++visualStart<visualLimit);
            }
            
        }

        if(pBiDi->insertPoints.size>0) {
            int32_t markFound=0, runCount=pBiDi->runCount;
            int32_t insertRemove, i, j, k;
            runs=pBiDi->runs;
            
            for(i=0; i<runCount; i++) {
                insertRemove=runs[i].insertRemove;
                if(insertRemove&(LRM_BEFORE|RLM_BEFORE)) {
                    markFound++;
                }
                if(insertRemove&(LRM_AFTER|RLM_AFTER)) {
                    markFound++;
                }
            }
            
            k=pBiDi->resultLength;
            for(i=runCount-1; i>=0 && markFound>0; i--) {
                insertRemove=runs[i].insertRemove;
                if(insertRemove&(LRM_AFTER|RLM_AFTER)) {
                    indexMap[--k]= UBIDI_MAP_NOWHERE;
                    markFound--;
                }
                visualStart= i>0 ? runs[i-1].visualLimit : 0;
                for(j=runs[i].visualLimit-1; j>=visualStart && markFound>0; j--) {
                    indexMap[--k]=indexMap[j];
                }
                if(insertRemove&(LRM_BEFORE|RLM_BEFORE)) {
                    indexMap[--k]= UBIDI_MAP_NOWHERE;
                    markFound--;
                }
            }
        }
        else if(pBiDi->controlCount>0) {
            int32_t runCount=pBiDi->runCount, logicalEnd;
            int32_t insertRemove, length, i, j, k, m;
            UChar uchar;
            UBool evenRun;
            runs=pBiDi->runs;
            visualStart=0;
            
            k=0;
            for(i=0; i<runCount; i++, visualStart+=length) {
                length=runs[i].visualLimit-visualStart;
                insertRemove=runs[i].insertRemove;
                
                if((insertRemove==0)&&(k==visualStart)) {
                    k+=length;
                    continue;
                }
                
                if(insertRemove==0) {
                    visualLimit=runs[i].visualLimit;
                    for(j=visualStart; j<visualLimit; j++) {
                        indexMap[k++]=indexMap[j];
                    }
                    continue;
                }
                logicalStart=runs[i].logicalStart;
                evenRun=IS_EVEN_RUN(logicalStart);
                REMOVE_ODD_BIT(logicalStart);
                logicalEnd=logicalStart+length-1;
                for(j=0; j<length; j++) {
                    m= evenRun ? logicalStart+j : logicalEnd-j;
                    uchar=pBiDi->text[m];
                    if(!IS_BIDI_CONTROL_CHAR(uchar)) {
                        indexMap[k++]=m;
                    }
                }
            }
        }
    }
}

U_CAPI void U_EXPORT2
ubidi_invertMap(const int32_t *srcMap, int32_t *destMap, int32_t length) {
    if(srcMap!=NULL && destMap!=NULL && length>0) {
        const int32_t *pi;
        int32_t destLength=-1, count=0;
        
        pi=srcMap+length;
        while(pi>srcMap) {
            if(*--pi>destLength) {
                destLength=*pi;
            }
            if(*pi>=0) {
                count++;
            }
        }
        destLength++;           
        if(count<destLength) {
            
            uprv_memset(destMap, 0xFF, destLength*sizeof(int32_t));
        }
        pi=srcMap+length;
        while(length>0) {
            if(*--pi>=0) {
                destMap[*pi]=--length;
            } else {
                --length;
            }
        }
    }
}
