
















#include "cmemory.h"
#include "unicode/utypes.h"
#include "unicode/ustring.h"
#include "unicode/uchar.h"
#include "unicode/ubidi.h"
#include "unicode/utf16.h"
#include "ubidi_props.h"
#include "ubidiimp.h"
#include "uassert.h"



















































































static const Flags flagLR[2]={ DIRPROP_FLAG(L), DIRPROP_FLAG(R) };
static const Flags flagE[2]={ DIRPROP_FLAG(LRE), DIRPROP_FLAG(RLE) };
static const Flags flagO[2]={ DIRPROP_FLAG(LRO), DIRPROP_FLAG(RLO) };

#define DIRPROP_FLAG_LR(level) flagLR[(level)&1]
#define DIRPROP_FLAG_E(level)  flagE[(level)&1]
#define DIRPROP_FLAG_O(level)  flagO[(level)&1]

#define DIR_FROM_STRONG(strong) ((strong)==L ? L : R)

#define NO_OVERRIDE(level)  ((level)&~UBIDI_LEVEL_OVERRIDE)



U_CAPI UBiDi * U_EXPORT2
ubidi_open(void)
{
    UErrorCode errorCode=U_ZERO_ERROR;
    return ubidi_openSized(0, 0, &errorCode);
}

U_CAPI UBiDi * U_EXPORT2
ubidi_openSized(int32_t maxLength, int32_t maxRunCount, UErrorCode *pErrorCode) {
    UBiDi *pBiDi;

    
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return NULL;
    } else if(maxLength<0 || maxRunCount<0) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;    
    }

    
    pBiDi=(UBiDi *)uprv_malloc(sizeof(UBiDi));
    if(pBiDi==NULL) {
        *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }

    
    uprv_memset(pBiDi, 0, sizeof(UBiDi));

    
    pBiDi->bdp=ubidi_getSingleton();

    
    if(maxLength>0) {
        if( !getInitialDirPropsMemory(pBiDi, maxLength) ||
            !getInitialLevelsMemory(pBiDi, maxLength)
        ) {
            *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
        }
    } else {
        pBiDi->mayAllocateText=TRUE;
    }

    if(maxRunCount>0) {
        if(maxRunCount==1) {
            
            pBiDi->runsSize=sizeof(Run);
        } else if(!getInitialRunsMemory(pBiDi, maxRunCount)) {
            *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
        }
    } else {
        pBiDi->mayAllocateRuns=TRUE;
    }

    if(U_SUCCESS(*pErrorCode)) {
        return pBiDi;
    } else {
        ubidi_close(pBiDi);
        return NULL;
    }
}














U_CFUNC UBool
ubidi_getMemory(BidiMemoryForAllocation *bidiMem, int32_t *pSize, UBool mayAllocate, int32_t sizeNeeded) {
    void **pMemory = (void **)bidiMem;
    
    if(*pMemory==NULL) {
        
        if(mayAllocate && (*pMemory=uprv_malloc(sizeNeeded))!=NULL) {
            *pSize=sizeNeeded;
            return TRUE;
        } else {
            return FALSE;
        }
    } else {
        if(sizeNeeded<=*pSize) {
            
            return TRUE;
        }
        else if(!mayAllocate) {
            
            return FALSE;
        } else {
            
            void *memory;
            



            if((memory=uprv_realloc(*pMemory, sizeNeeded))!=NULL) {
                *pMemory=memory;
                *pSize=sizeNeeded;
                return TRUE;
            } else {
                
                return FALSE;
            }
        }
    }
}

U_CAPI void U_EXPORT2
ubidi_close(UBiDi *pBiDi) {
    if(pBiDi!=NULL) {
        pBiDi->pParaBiDi=NULL;          
        if(pBiDi->dirPropsMemory!=NULL) {
            uprv_free(pBiDi->dirPropsMemory);
        }
        if(pBiDi->levelsMemory!=NULL) {
            uprv_free(pBiDi->levelsMemory);
        }
        if(pBiDi->openingsMemory!=NULL) {
            uprv_free(pBiDi->openingsMemory);
        }
        if(pBiDi->parasMemory!=NULL) {
            uprv_free(pBiDi->parasMemory);
        }
        if(pBiDi->runsMemory!=NULL) {
            uprv_free(pBiDi->runsMemory);
        }
        if(pBiDi->isolatesMemory!=NULL) {
            uprv_free(pBiDi->isolatesMemory);
        }
        if(pBiDi->insertPoints.points!=NULL) {
            uprv_free(pBiDi->insertPoints.points);
        }

        uprv_free(pBiDi);
    }
}



U_CAPI void U_EXPORT2
ubidi_setInverse(UBiDi *pBiDi, UBool isInverse) {
    if(pBiDi!=NULL) {
        pBiDi->isInverse=isInverse;
        pBiDi->reorderingMode = isInverse ? UBIDI_REORDER_INVERSE_NUMBERS_AS_L
                                          : UBIDI_REORDER_DEFAULT;
    }
}

U_CAPI UBool U_EXPORT2
ubidi_isInverse(UBiDi *pBiDi) {
    if(pBiDi!=NULL) {
        return pBiDi->isInverse;
    } else {
        return FALSE;
    }
}
















U_CAPI void U_EXPORT2
ubidi_setReorderingMode(UBiDi *pBiDi, UBiDiReorderingMode reorderingMode) {
    if ((pBiDi!=NULL) && (reorderingMode >= UBIDI_REORDER_DEFAULT)
                        && (reorderingMode < UBIDI_REORDER_COUNT)) {
        pBiDi->reorderingMode = reorderingMode;
        pBiDi->isInverse = (UBool)(reorderingMode == UBIDI_REORDER_INVERSE_NUMBERS_AS_L);
    }
}

U_CAPI UBiDiReorderingMode U_EXPORT2
ubidi_getReorderingMode(UBiDi *pBiDi) {
    if (pBiDi!=NULL) {
        return pBiDi->reorderingMode;
    } else {
        return UBIDI_REORDER_DEFAULT;
    }
}

U_CAPI void U_EXPORT2
ubidi_setReorderingOptions(UBiDi *pBiDi, uint32_t reorderingOptions) {
    if (reorderingOptions & UBIDI_OPTION_REMOVE_CONTROLS) {
        reorderingOptions&=~UBIDI_OPTION_INSERT_MARKS;
    }
    if (pBiDi!=NULL) {
        pBiDi->reorderingOptions=reorderingOptions;
    }
}

U_CAPI uint32_t U_EXPORT2
ubidi_getReorderingOptions(UBiDi *pBiDi) {
    if (pBiDi!=NULL) {
        return pBiDi->reorderingOptions;
    } else {
        return 0;
    }
}

U_CAPI UBiDiDirection U_EXPORT2
ubidi_getBaseDirection(const UChar *text,
int32_t length){

    int32_t i;
    UChar32 uchar;
    UCharDirection dir;

    if( text==NULL || length<-1 ){
        return UBIDI_NEUTRAL;
    }

    if(length==-1) {
        length=u_strlen(text);
    }

    for( i = 0 ; i < length; ) {
        
        U16_NEXT(text, i, length, uchar);
        dir = u_charDirection(uchar);
        if( dir == U_LEFT_TO_RIGHT )
                return UBIDI_LTR;
        if( dir == U_RIGHT_TO_LEFT || dir ==U_RIGHT_TO_LEFT_ARABIC )
                return UBIDI_RTL;
    }
    return UBIDI_NEUTRAL;
}








static DirProp
firstL_R_AL(UBiDi *pBiDi) {
    const UChar *text=pBiDi->prologue;
    int32_t length=pBiDi->proLength;
    int32_t i;
    UChar32 uchar;
    DirProp dirProp, result=ON;
    for(i=0; i<length; ) {
        
        U16_NEXT(text, i, length, uchar);
        dirProp=(DirProp)ubidi_getCustomizedClass(pBiDi, uchar);
        if(result==ON) {
            if(dirProp==L || dirProp==R || dirProp==AL) {
                result=dirProp;
            }
        } else {
            if(dirProp==B) {
                result=ON;
            }
        }
    }
    return result;
}




static UBool
checkParaCount(UBiDi *pBiDi) {
    int32_t count=pBiDi->paraCount;
    if(pBiDi->paras==pBiDi->simpleParas) {
        if(count<=SIMPLE_PARAS_COUNT)
            return TRUE;
        if(!getInitialParasMemory(pBiDi, SIMPLE_PARAS_COUNT * 2))
            return FALSE;
        pBiDi->paras=pBiDi->parasMemory;
        uprv_memcpy(pBiDi->parasMemory, pBiDi->simpleParas, SIMPLE_PARAS_COUNT * sizeof(Para));
        return TRUE;
    }
    if(!getInitialParasMemory(pBiDi, count * 2))
        return FALSE;
    pBiDi->paras=pBiDi->parasMemory;
    return TRUE;
}









static UBool
getDirProps(UBiDi *pBiDi) {
    const UChar *text=pBiDi->text;
    DirProp *dirProps=pBiDi->dirPropsMemory;    

    int32_t i=0, originalLength=pBiDi->originalLength;
    Flags flags=0;      
    UChar32 uchar;
    DirProp dirProp=0, defaultParaLevel=0;  
    UBool isDefaultLevel=IS_DEFAULT_LEVEL(pBiDi->paraLevel);
    

    UBool isDefaultLevelInverse=isDefaultLevel && (UBool)
            (pBiDi->reorderingMode==UBIDI_REORDER_INVERSE_LIKE_DIRECT ||
             pBiDi->reorderingMode==UBIDI_REORDER_INVERSE_FOR_NUMBERS_SPECIAL);
    int32_t lastArabicPos=-1;
    int32_t controlCount=0;
    UBool removeBiDiControls = (UBool)(pBiDi->reorderingOptions &
                                       UBIDI_OPTION_REMOVE_CONTROLS);

    typedef enum {
         NOT_SEEKING_STRONG,            
         SEEKING_STRONG_FOR_PARA,       
         SEEKING_STRONG_FOR_FSI,        
         LOOKING_FOR_PDI                
    } State;
    State state;
    DirProp lastStrong=ON;              
    





    

    int32_t isolateStartStack[UBIDI_MAX_EXPLICIT_LEVEL+1];
    

    int8_t  previousStateStack[UBIDI_MAX_EXPLICIT_LEVEL+1];
    int32_t stackLast=-1;

    if(pBiDi->reorderingOptions & UBIDI_OPTION_STREAMING)
        pBiDi->length=0;
    defaultParaLevel=pBiDi->paraLevel&1;
    if(isDefaultLevel) {
        pBiDi->paras[0].level=defaultParaLevel;
        lastStrong=defaultParaLevel;
        if(pBiDi->proLength>0 &&                    
           (dirProp=firstL_R_AL(pBiDi))!=ON) {  
            if(dirProp==L)
                pBiDi->paras[0].level=0;    
            else
                pBiDi->paras[0].level=1;    
            state=NOT_SEEKING_STRONG;
        } else {
            state=SEEKING_STRONG_FOR_PARA;
        }
    } else {
        pBiDi->paras[0].level=pBiDi->paraLevel;
        state=NOT_SEEKING_STRONG;
    }
    
    




    for(  ; i<originalLength; ) {
        
        U16_NEXT(text, i, originalLength, uchar);
        flags|=DIRPROP_FLAG(dirProp=(DirProp)ubidi_getCustomizedClass(pBiDi, uchar));
        dirProps[i-1]=dirProp;
        if(uchar>0xffff) {  
            flags|=DIRPROP_FLAG(BN);
            dirProps[i-2]=BN;
        }
        if(removeBiDiControls && IS_BIDI_CONTROL_CHAR(uchar))
            controlCount++;
        if(dirProp==L) {
            if(state==SEEKING_STRONG_FOR_PARA) {
                pBiDi->paras[pBiDi->paraCount-1].level=0;
                state=NOT_SEEKING_STRONG;
            }
            else if(state==SEEKING_STRONG_FOR_FSI) {
                if(stackLast<=UBIDI_MAX_EXPLICIT_LEVEL) {
                    
                    
                    flags|=DIRPROP_FLAG(LRI);
                }
                state=LOOKING_FOR_PDI;
            }
            lastStrong=L;
            continue;
        }
        if(dirProp==R || dirProp==AL) {
            if(state==SEEKING_STRONG_FOR_PARA) {
                pBiDi->paras[pBiDi->paraCount-1].level=1;
                state=NOT_SEEKING_STRONG;
            }
            else if(state==SEEKING_STRONG_FOR_FSI) {
                if(stackLast<=UBIDI_MAX_EXPLICIT_LEVEL) {
                    dirProps[isolateStartStack[stackLast]]=RLI;
                    flags|=DIRPROP_FLAG(RLI);
                }
                state=LOOKING_FOR_PDI;
            }
            lastStrong=R;
            if(dirProp==AL)
                lastArabicPos=i-1;
            continue;
        }
        if(dirProp>=FSI && dirProp<=RLI) {  
            stackLast++;
            if(stackLast<=UBIDI_MAX_EXPLICIT_LEVEL) {
                isolateStartStack[stackLast]=i-1;
                previousStateStack[stackLast]=state;
            }
            if(dirProp==FSI) {
                dirProps[i-1]=LRI;      
                state=SEEKING_STRONG_FOR_FSI;
            }
            else
                state=LOOKING_FOR_PDI;
            continue;
        }
        if(dirProp==PDI) {
            if(state==SEEKING_STRONG_FOR_FSI) {
                if(stackLast<=UBIDI_MAX_EXPLICIT_LEVEL) {
                    
                    
                    flags|=DIRPROP_FLAG(LRI);
                }
            }
            if(stackLast>=0) {
                if(stackLast<=UBIDI_MAX_EXPLICIT_LEVEL)
                    state=previousStateStack[stackLast];
                stackLast--;
            }
            continue;
        }
        if(dirProp==B) {
            if(i<originalLength && uchar==CR && text[i]==LF) 
                continue;
            pBiDi->paras[pBiDi->paraCount-1].limit=i;
            if(isDefaultLevelInverse && lastStrong==R)
                pBiDi->paras[pBiDi->paraCount-1].level=1;
            if(pBiDi->reorderingOptions & UBIDI_OPTION_STREAMING) {
                

                pBiDi->length=i;        
                pBiDi->controlCount=controlCount;
            }
            if(i<originalLength) {              
                pBiDi->paraCount++;
                if(checkParaCount(pBiDi)==FALSE)    
                    return FALSE;
                if(isDefaultLevel) {
                    pBiDi->paras[pBiDi->paraCount-1].level=defaultParaLevel;
                    state=SEEKING_STRONG_FOR_PARA;
                    lastStrong=defaultParaLevel;
                } else {
                    pBiDi->paras[pBiDi->paraCount-1].level=pBiDi->paraLevel;
                    state=NOT_SEEKING_STRONG;
                }
                stackLast=-1;
            }
            continue;
        }
    }
    
    if(stackLast>UBIDI_MAX_EXPLICIT_LEVEL) {
        stackLast=UBIDI_MAX_EXPLICIT_LEVEL;
        state=SEEKING_STRONG_FOR_FSI;   
    }
    
    while(stackLast>=0) {
        if(state==SEEKING_STRONG_FOR_FSI) {
            
            
            flags|=DIRPROP_FLAG(LRI);
            break;
        }
        state=previousStateStack[stackLast];
        stackLast--;
    }
    
    if(pBiDi->reorderingOptions & UBIDI_OPTION_STREAMING) {
        if(pBiDi->length<originalLength)
            pBiDi->paraCount--;
    } else {
        pBiDi->paras[pBiDi->paraCount-1].limit=originalLength;
        pBiDi->controlCount=controlCount;
    }
    

    if(isDefaultLevelInverse && lastStrong==R) {
        pBiDi->paras[pBiDi->paraCount-1].level=1;
    }
    if(isDefaultLevel) {
        pBiDi->paraLevel=pBiDi->paras[0].level;
    }
    

    for(i=0; i<pBiDi->paraCount; i++)
        flags|=DIRPROP_FLAG_LR(pBiDi->paras[i].level);

    if(pBiDi->orderParagraphsLTR && (flags&DIRPROP_FLAG(B))) {
        flags|=DIRPROP_FLAG(L);
    }
    pBiDi->flags=flags;
    pBiDi->lastArabicPos=lastArabicPos;
    return TRUE;
}


U_CFUNC UBiDiLevel
ubidi_getParaLevelAtIndex(const UBiDi *pBiDi, int32_t pindex) {
    int32_t i;
    for(i=0; i<pBiDi->paraCount; i++)
        if(pindex<pBiDi->paras[i].limit)
            break;
    if(i>=pBiDi->paraCount)
        i=pBiDi->paraCount-1;
    return (UBiDiLevel)(pBiDi->paras[i].level);
}

















static void
bracketInit(UBiDi *pBiDi, BracketData *bd) {
    bd->pBiDi=pBiDi;
    bd->isoRunLast=0;
    bd->isoRuns[0].start=0;
    bd->isoRuns[0].limit=0;
    bd->isoRuns[0].level=GET_PARALEVEL(pBiDi, 0);
    bd->isoRuns[0].lastStrong=bd->isoRuns[0].lastBase=bd->isoRuns[0].contextDir=GET_PARALEVEL(pBiDi, 0)&1;
    bd->isoRuns[0].contextPos=0;
    if(pBiDi->openingsMemory) {
        bd->openings=pBiDi->openingsMemory;
        bd->openingsCount=pBiDi->openingsSize / sizeof(Opening);
    } else {
        bd->openings=bd->simpleOpenings;
        bd->openingsCount=SIMPLE_OPENINGS_COUNT;
    }
    bd->isNumbersSpecial=bd->pBiDi->reorderingMode==UBIDI_REORDER_NUMBERS_SPECIAL ||
                         bd->pBiDi->reorderingMode==UBIDI_REORDER_INVERSE_FOR_NUMBERS_SPECIAL;
}


static void
bracketProcessB(BracketData *bd, UBiDiLevel level) {
    bd->isoRunLast=0;
    bd->isoRuns[0].limit=0;
    bd->isoRuns[0].level=level;
    bd->isoRuns[0].lastStrong=bd->isoRuns[0].lastBase=bd->isoRuns[0].contextDir=level&1;
    bd->isoRuns[0].contextPos=0;
}


static void
bracketProcessBoundary(BracketData *bd, int32_t lastCcPos,
                       UBiDiLevel contextLevel, UBiDiLevel embeddingLevel) {
    IsoRun *pLastIsoRun=&bd->isoRuns[bd->isoRunLast];
    DirProp *dirProps=bd->pBiDi->dirProps;
    if(DIRPROP_FLAG(dirProps[lastCcPos])&MASK_ISO)  
        return;
    if(NO_OVERRIDE(embeddingLevel)>NO_OVERRIDE(contextLevel))   
        contextLevel=embeddingLevel;
    pLastIsoRun->limit=pLastIsoRun->start;
    pLastIsoRun->level=embeddingLevel;
    pLastIsoRun->lastStrong=pLastIsoRun->lastBase=pLastIsoRun->contextDir=contextLevel&1;
    pLastIsoRun->contextPos=lastCcPos;
}


static void
bracketProcessLRI_RLI(BracketData *bd, UBiDiLevel level) {
    IsoRun *pLastIsoRun=&bd->isoRuns[bd->isoRunLast];
    int16_t lastLimit;
    pLastIsoRun->lastBase=ON;
    lastLimit=pLastIsoRun->limit;
    bd->isoRunLast++;
    pLastIsoRun++;
    pLastIsoRun->start=pLastIsoRun->limit=lastLimit;
    pLastIsoRun->level=level;
    pLastIsoRun->lastStrong=pLastIsoRun->lastBase=pLastIsoRun->contextDir=level&1;
    pLastIsoRun->contextPos=0;
}


static void
bracketProcessPDI(BracketData *bd) {
    IsoRun *pLastIsoRun;
    bd->isoRunLast--;
    pLastIsoRun=&bd->isoRuns[bd->isoRunLast];
    pLastIsoRun->lastBase=ON;
}


static UBool                            
bracketAddOpening(BracketData *bd, UChar match, int32_t position) {
    IsoRun *pLastIsoRun=&bd->isoRuns[bd->isoRunLast];
    Opening *pOpening;
    if(pLastIsoRun->limit>=bd->openingsCount) {  
        UBiDi *pBiDi=bd->pBiDi;
        if(!getInitialOpeningsMemory(pBiDi, pLastIsoRun->limit * 2))
            return FALSE;
        if(bd->openings==bd->simpleOpenings)
            uprv_memcpy(pBiDi->openingsMemory, bd->simpleOpenings,
                        SIMPLE_OPENINGS_COUNT * sizeof(Opening));
        bd->openings=pBiDi->openingsMemory;     
        bd->openingsCount=pBiDi->openingsSize / sizeof(Opening);
    }
    pOpening=&bd->openings[pLastIsoRun->limit];
    pOpening->position=position;
    pOpening->match=match;
    pOpening->contextDir=pLastIsoRun->contextDir;
    pOpening->contextPos=pLastIsoRun->contextPos;
    pOpening->flags=0;
    pLastIsoRun->limit++;
    return TRUE;
}


static void
fixN0c(BracketData *bd, int32_t openingIndex, int32_t newPropPosition, DirProp newProp) {
    
    IsoRun *pLastIsoRun=&bd->isoRuns[bd->isoRunLast];
    Opening *qOpening;
    DirProp *dirProps=bd->pBiDi->dirProps;
    int32_t k, openingPosition, closingPosition;
    for(k=openingIndex+1, qOpening=&bd->openings[k]; k<pLastIsoRun->limit; k++, qOpening++) {
        if(qOpening->match>=0)      
            continue;
        if(newPropPosition<qOpening->contextPos)
            break;
        if(newPropPosition>=qOpening->position)
            continue;
        if(newProp==qOpening->contextDir)
            break;
        openingPosition=qOpening->position;
        dirProps[openingPosition]=newProp;
        closingPosition=-(qOpening->match);
        dirProps[closingPosition]=newProp;
        qOpening->match=0;                      
        fixN0c(bd, k, openingPosition, newProp);
        fixN0c(bd, k, closingPosition, newProp);
    }
}


static DirProp              
bracketProcessClosing(BracketData *bd, int32_t openIdx, int32_t position) {
    IsoRun *pLastIsoRun=&bd->isoRuns[bd->isoRunLast];
    Opening *pOpening, *qOpening;
    UBiDiDirection direction;
    UBool stable;
    DirProp newProp;
    pOpening=&bd->openings[openIdx];
    direction=pLastIsoRun->level&1;
    stable=TRUE;            

    

















    if((direction==0 && pOpening->flags&FOUND_L) ||
       (direction==1 && pOpening->flags&FOUND_R)) { 
        newProp=direction;
    }
    else if(pOpening->flags&(FOUND_L|FOUND_R)) {    
        

        stable=(openIdx==pLastIsoRun->start);
        if(direction!=pOpening->contextDir)
            newProp=pOpening->contextDir;           
        else
            newProp=direction;                      
    } else {
        
        pLastIsoRun->limit=openIdx;
        return ON;                                  
    }
    bd->pBiDi->dirProps[pOpening->position]=newProp;
    bd->pBiDi->dirProps[position]=newProp;
    
    fixN0c(bd, openIdx, pOpening->position, newProp);
    if(stable) {
        pLastIsoRun->limit=openIdx; 
        
        while(pLastIsoRun->limit>pLastIsoRun->start &&
              bd->openings[pLastIsoRun->limit-1].position==pOpening->position)
            pLastIsoRun->limit--;
    } else {
        int32_t k;
        pOpening->match=-position;
        
        k=openIdx-1;
        while(k>=pLastIsoRun->start &&
              bd->openings[k].position==pOpening->position)
            bd->openings[k--].match=0;
        

        for(k=openIdx+1; k<pLastIsoRun->limit; k++) {
            qOpening=&bd->openings[k];
            if(qOpening->position>=position)
                break;
            if(qOpening->match>0)
                qOpening->match=0;
        }
    }
    return newProp;
}


static UBool                            
bracketProcessChar(BracketData *bd, int32_t position) {
    IsoRun *pLastIsoRun=&bd->isoRuns[bd->isoRunLast];
    DirProp *dirProps, dirProp, newProp;
    UBiDiLevel level;
    dirProps=bd->pBiDi->dirProps;
    dirProp=dirProps[position];
    if(dirProp==ON) {
        UChar c, match;
        int32_t idx;
        

        c=bd->pBiDi->text[position];
        for(idx=pLastIsoRun->limit-1; idx>=pLastIsoRun->start; idx--) {
            if(bd->openings[idx].match!=c)
                continue;
            
            newProp=bracketProcessClosing(bd, idx, position);
            if(newProp==ON) {           
                c=0;        
                break;
            }
            pLastIsoRun->lastBase=ON;
            pLastIsoRun->contextDir=newProp;
            pLastIsoRun->contextPos=position;
            level=bd->pBiDi->levels[position];
            if(level&UBIDI_LEVEL_OVERRIDE) {    
                uint16_t flag;
                int32_t i;
                newProp=level&1;
                pLastIsoRun->lastStrong=newProp;
                flag=DIRPROP_FLAG(newProp);
                for(i=pLastIsoRun->start; i<idx; i++)
                    bd->openings[i].flags|=flag;
                
                bd->pBiDi->levels[position]&=~UBIDI_LEVEL_OVERRIDE;
            }
            
            bd->pBiDi->levels[bd->openings[idx].position]&=~UBIDI_LEVEL_OVERRIDE;
            return TRUE;
        }
        

        
        if(c)
            match=u_getBidiPairedBracket(c);    
        else
            match=0;
        if(match!=c &&                  
           ubidi_getPairedBracketType(bd->pBiDi->bdp, c)==U_BPT_OPEN) { 
            

            if(match==0x232A) {     
                if(!bracketAddOpening(bd, 0x3009, position))
                    return FALSE;
            }
            else if(match==0x3009) {         
                if(!bracketAddOpening(bd, 0x232A, position))
                    return FALSE;
            }
            if(!bracketAddOpening(bd, match, position))
                return FALSE;
        }
    }
    level=bd->pBiDi->levels[position];
    if(level&UBIDI_LEVEL_OVERRIDE) {    
        newProp=level&1;
        if(dirProp!=S && dirProp!=WS && dirProp!=ON)
            dirProps[position]=newProp;
        pLastIsoRun->lastBase=newProp;
        pLastIsoRun->lastStrong=newProp;
        pLastIsoRun->contextDir=newProp;
        pLastIsoRun->contextPos=position;
    }
    else if(dirProp<=R || dirProp==AL) {
        newProp=DIR_FROM_STRONG(dirProp);
        pLastIsoRun->lastBase=dirProp;
        pLastIsoRun->lastStrong=dirProp;
        pLastIsoRun->contextDir=newProp;
        pLastIsoRun->contextPos=position;
    }
    else if(dirProp==EN) {
        pLastIsoRun->lastBase=EN;
        if(pLastIsoRun->lastStrong==L) {
            newProp=L;                  
            if(!bd->isNumbersSpecial)
                dirProps[position]=ENL;
            pLastIsoRun->contextDir=L;
            pLastIsoRun->contextPos=position;
        }
        else {
            newProp=R;                  
            if(pLastIsoRun->lastStrong==AL)
                dirProps[position]=AN;  
            else
                dirProps[position]=ENR;
            pLastIsoRun->contextDir=R;
            pLastIsoRun->contextPos=position;
        }
    }
    else if(dirProp==AN) {
        newProp=R;                      
        pLastIsoRun->lastBase=AN;
        pLastIsoRun->contextDir=R;
        pLastIsoRun->contextPos=position;
    }
    else if(dirProp==NSM) {
        


        newProp=pLastIsoRun->lastBase;
        if(newProp==ON)
            dirProps[position]=newProp;
    }
    else {
        newProp=dirProp;
        pLastIsoRun->lastBase=dirProp;
    }
    if(newProp<=R || newProp==AL) {
        int32_t i;
        uint16_t flag=DIRPROP_FLAG(DIR_FROM_STRONG(newProp));
        for(i=pLastIsoRun->start; i<pLastIsoRun->limit; i++)
            if(position>bd->openings[i].position)
                bd->openings[i].flags|=flag;
    }
    return TRUE;
}




static UBiDiDirection
directionFromFlags(UBiDi *pBiDi) {
    Flags flags=pBiDi->flags;
    
    if(!(flags&MASK_RTL || ((flags&DIRPROP_FLAG(AN)) && (flags&MASK_POSSIBLE_N)))) {
        return UBIDI_LTR;
    } else if(!(flags&MASK_LTR)) {
        return UBIDI_RTL;
    } else {
        return UBIDI_MIXED;
    }
}




















































static UBiDiDirection
resolveExplicitLevels(UBiDi *pBiDi, UErrorCode *pErrorCode) {
    DirProp *dirProps=pBiDi->dirProps;
    UBiDiLevel *levels=pBiDi->levels;
    const UChar *text=pBiDi->text;

    int32_t i=0, length=pBiDi->length;
    Flags flags=pBiDi->flags;       
    DirProp dirProp;
    UBiDiLevel level=GET_PARALEVEL(pBiDi, 0);
    UBiDiDirection direction;
    pBiDi->isolateCount=0;

    if(U_FAILURE(*pErrorCode)) { return UBIDI_LTR; }

    
    direction=directionFromFlags(pBiDi);

    
    if((direction!=UBIDI_MIXED)) {
        
        return direction;
    }
    if(pBiDi->reorderingMode > UBIDI_REORDER_LAST_LOGICAL_TO_VISUAL) {
        
        
        int32_t paraIndex, start, limit;
        for(paraIndex=0; paraIndex<pBiDi->paraCount; paraIndex++) {
            if(paraIndex==0)
                start=0;
            else
                start=pBiDi->paras[paraIndex-1].limit;
            limit=pBiDi->paras[paraIndex].limit;
            level=pBiDi->paras[paraIndex].level;
            for(i=start; i<limit; i++)
                levels[i]=level;
        }
        return direction;   
    }
    if(!(flags&(MASK_EXPLICIT|MASK_ISO))) {
        
        
        int32_t paraIndex, start, limit;
        BracketData bracketData;
        bracketInit(pBiDi, &bracketData);
        for(paraIndex=0; paraIndex<pBiDi->paraCount; paraIndex++) {
            if(paraIndex==0)
                start=0;
            else
                start=pBiDi->paras[paraIndex-1].limit;
            limit=pBiDi->paras[paraIndex].limit;
            level=pBiDi->paras[paraIndex].level;
            for(i=start; i<limit; i++) {
                levels[i]=level;
                dirProp=dirProps[i];
                if(dirProp==BN)
                    continue;
                if(dirProp==B) {
                    if((i+1)<length) {
                        if(text[i]==CR && text[i+1]==LF)
                            continue;   
                        bracketProcessB(&bracketData, level);
                    }
                    continue;
                }
                if(!bracketProcessChar(&bracketData, i)) {
                    *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
                    return UBIDI_LTR;
                }
            }
        }
        return direction;
    }
    {
        

        
        
        UBiDiLevel embeddingLevel=level, newLevel;
        UBiDiLevel previousLevel=level;     
        int32_t lastCcPos=0;                

        

        uint16_t stack[UBIDI_MAX_EXPLICIT_LEVEL+2];   

        uint32_t stackLast=0;
        int32_t overflowIsolateCount=0;
        int32_t overflowEmbeddingCount=0;
        int32_t validIsolateCount=0;
        BracketData bracketData;
        bracketInit(pBiDi, &bracketData);
        stack[0]=level;     

        
        flags=0;

        for(i=0; i<length; ++i) {
            dirProp=dirProps[i];
            switch(dirProp) {
            case LRE:
            case RLE:
            case LRO:
            case RLO:
                
                flags|=DIRPROP_FLAG(BN);
                levels[i]=previousLevel;
                if (dirProp==LRE || dirProp==LRO)
                    
                    newLevel=(UBiDiLevel)((embeddingLevel+2)&~(UBIDI_LEVEL_OVERRIDE|1));
                else
                    
                    newLevel=(UBiDiLevel)((NO_OVERRIDE(embeddingLevel)+1)|1);
                if(newLevel<=UBIDI_MAX_EXPLICIT_LEVEL && overflowIsolateCount==0 &&
                                                         overflowEmbeddingCount==0) {
                    lastCcPos=i;
                    embeddingLevel=newLevel;
                    if(dirProp==LRO || dirProp==RLO)
                        embeddingLevel|=UBIDI_LEVEL_OVERRIDE;
                    stackLast++;
                    stack[stackLast]=embeddingLevel;
                    



                } else {
                    if(overflowIsolateCount==0)
                        overflowEmbeddingCount++;
                }
                break;
            case PDF:
                
                flags|=DIRPROP_FLAG(BN);
                levels[i]=previousLevel;
                
                if(overflowIsolateCount) {
                    break;
                }
                if(overflowEmbeddingCount) {
                    overflowEmbeddingCount--;
                    break;
                }
                if(stackLast>0 && stack[stackLast]<ISOLATE) {   
                    lastCcPos=i;
                    stackLast--;
                    embeddingLevel=(UBiDiLevel)stack[stackLast];
                }
                break;
            case LRI:
            case RLI:
                flags|=(DIRPROP_FLAG(ON)|DIRPROP_FLAG_LR(embeddingLevel));
                levels[i]=NO_OVERRIDE(embeddingLevel);
                if(NO_OVERRIDE(embeddingLevel)!=NO_OVERRIDE(previousLevel)) {
                    bracketProcessBoundary(&bracketData, lastCcPos,
                                           previousLevel, embeddingLevel);
                    flags|=DIRPROP_FLAG_MULTI_RUNS;
                }
                previousLevel=embeddingLevel;
                
                if(dirProp==LRI)
                    
                    newLevel=(UBiDiLevel)((embeddingLevel+2)&~(UBIDI_LEVEL_OVERRIDE|1));
                else
                    
                    newLevel=(UBiDiLevel)((NO_OVERRIDE(embeddingLevel)+1)|1);
                if(newLevel<=UBIDI_MAX_EXPLICIT_LEVEL && overflowIsolateCount==0 &&
                                                         overflowEmbeddingCount==0) {
                    flags|=DIRPROP_FLAG(dirProp);
                    lastCcPos=i;
                    validIsolateCount++;
                    if(validIsolateCount>pBiDi->isolateCount)
                        pBiDi->isolateCount=validIsolateCount;
                    embeddingLevel=newLevel;
                    

                    stackLast++;
                    stack[stackLast]=embeddingLevel+ISOLATE;
                    bracketProcessLRI_RLI(&bracketData, embeddingLevel);
                } else {
                    
                    dirProps[i]=WS;
                    overflowIsolateCount++;
                }
                break;
            case PDI:
                if(NO_OVERRIDE(embeddingLevel)!=NO_OVERRIDE(previousLevel)) {
                    bracketProcessBoundary(&bracketData, lastCcPos,
                                           previousLevel, embeddingLevel);
                    flags|=DIRPROP_FLAG_MULTI_RUNS;
                }
                
                if(overflowIsolateCount) {
                    overflowIsolateCount--;
                    
                    dirProps[i]=WS;
                }
                else if(validIsolateCount) {
                    flags|=DIRPROP_FLAG(PDI);
                    lastCcPos=i;
                    overflowEmbeddingCount=0;
                    while(stack[stackLast]<ISOLATE) 
                        stackLast--;                
                    stackLast--;                    
                    validIsolateCount--;
                    bracketProcessPDI(&bracketData);
                } else
                    
                    dirProps[i]=WS;
                embeddingLevel=(UBiDiLevel)stack[stackLast]&~ISOLATE;
                flags|=(DIRPROP_FLAG(ON)|DIRPROP_FLAG_LR(embeddingLevel));
                previousLevel=embeddingLevel;
                levels[i]=NO_OVERRIDE(embeddingLevel);
                break;
            case B:
                flags|=DIRPROP_FLAG(B);
                levels[i]=GET_PARALEVEL(pBiDi, i);
                if((i+1)<length) {
                    if(text[i]==CR && text[i+1]==LF)
                        break;          
                    overflowEmbeddingCount=overflowIsolateCount=0;
                    validIsolateCount=0;
                    stackLast=0;
                    previousLevel=embeddingLevel=GET_PARALEVEL(pBiDi, i+1);
                    stack[0]=embeddingLevel; 
                    bracketProcessB(&bracketData, embeddingLevel);
                }
                break;
            case BN:
                
                
                levels[i]=previousLevel;
                flags|=DIRPROP_FLAG(BN);
                break;
            default:
                
                if(NO_OVERRIDE(embeddingLevel)!=NO_OVERRIDE(previousLevel)) {
                    bracketProcessBoundary(&bracketData, lastCcPos,
                                           previousLevel, embeddingLevel);
                    flags|=DIRPROP_FLAG_MULTI_RUNS;
                    if(embeddingLevel&UBIDI_LEVEL_OVERRIDE)
                        flags|=DIRPROP_FLAG_O(embeddingLevel);
                    else
                        flags|=DIRPROP_FLAG_E(embeddingLevel);
                }
                previousLevel=embeddingLevel;
                levels[i]=embeddingLevel;
                if(!bracketProcessChar(&bracketData, i))
                    return -1;
                
                flags|=DIRPROP_FLAG(dirProps[i]);
                break;
            }
        }
        if(flags&MASK_EMBEDDING)
            flags|=DIRPROP_FLAG_LR(pBiDi->paraLevel);
        if(pBiDi->orderParagraphsLTR && (flags&DIRPROP_FLAG(B)))
            flags|=DIRPROP_FLAG(L);
        
        pBiDi->flags=flags;
        direction=directionFromFlags(pBiDi);
    }
    return direction;
}











static UBiDiDirection
checkExplicitLevels(UBiDi *pBiDi, UErrorCode *pErrorCode) {
    DirProp *dirProps=pBiDi->dirProps;
    DirProp dirProp;
    UBiDiLevel *levels=pBiDi->levels;
    int32_t isolateCount=0;

    int32_t i, length=pBiDi->length;
    Flags flags=0;  
    UBiDiLevel level;
    pBiDi->isolateCount=0;

    for(i=0; i<length; ++i) {
        level=levels[i];
        dirProp=dirProps[i];
        if(dirProp==LRI || dirProp==RLI) {
            isolateCount++;
            if(isolateCount>pBiDi->isolateCount)
                pBiDi->isolateCount=isolateCount;
        }
        else if(dirProp==PDI)
            isolateCount--;
        else if(dirProp==B)
            isolateCount=0;
        if(level&UBIDI_LEVEL_OVERRIDE) {
            
            level&=~UBIDI_LEVEL_OVERRIDE;     
            flags|=DIRPROP_FLAG_O(level);
        } else {
            
            flags|=DIRPROP_FLAG_E(level)|DIRPROP_FLAG(dirProp);
        }
        if((level<GET_PARALEVEL(pBiDi, i) &&
            !((0==level)&&(dirProp==B))) ||
           (UBIDI_MAX_EXPLICIT_LEVEL<level)) {
            
            *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
            return UBIDI_LTR;
        }
    }
    if(flags&MASK_EMBEDDING)
        flags|=DIRPROP_FLAG_LR(pBiDi->paraLevel);
    
    pBiDi->flags=flags;
    return directionFromFlags(pBiDi);
}


















#define IMPTABPROPS_COLUMNS 16
#define IMPTABPROPS_RES (IMPTABPROPS_COLUMNS - 1)
#define GET_STATEPROPS(cell) ((cell)&0x1f)
#define GET_ACTIONPROPS(cell) ((cell)>>5)
#define s(action, newState) ((uint8_t)(newState+(action<<5)))

static const uint8_t groupProp[] =          
{

    0,  1,  2,  7,  8,  3,  9,  6,  5,  4,  4,  10, 10, 12, 10, 10, 10, 11, 10, 4,  4,  4,  4,  13, 14
};
enum { DirProp_L=0, DirProp_R=1, DirProp_EN=2, DirProp_AN=3, DirProp_ON=4, DirProp_S=5, DirProp_B=6 }; 




































static const uint8_t impTabProps[][IMPTABPROPS_COLUMNS] =
{

 {     1 ,     2 ,     4 ,     5 ,     7 ,    15 ,    17 ,     7 ,     9 ,     7 ,     0 ,     7 ,     3 ,    18 ,    21 , DirProp_ON },
 {     1 , s(1,2), s(1,4), s(1,5), s(1,7),s(1,15),s(1,17), s(1,7), s(1,9), s(1,7),     1 ,     1 , s(1,3),s(1,18),s(1,21),  DirProp_L },
 { s(1,1),     2 , s(1,4), s(1,5), s(1,7),s(1,15),s(1,17), s(1,7), s(1,9), s(1,7),     2 ,     2 , s(1,3),s(1,18),s(1,21),  DirProp_R },
 { s(1,1), s(1,2), s(1,6), s(1,6), s(1,8),s(1,16),s(1,17), s(1,8), s(1,8), s(1,8),     3 ,     3 ,     3 ,s(1,18),s(1,21),  DirProp_R },
 { s(1,1), s(1,2),     4 , s(1,5), s(1,7),s(1,15),s(1,17),s(2,10),    11 ,s(2,10),     4 ,     4 , s(1,3),    18 ,    21 , DirProp_EN },
 { s(1,1), s(1,2), s(1,4),     5 , s(1,7),s(1,15),s(1,17), s(1,7), s(1,9),s(2,12),     5 ,     5 , s(1,3),s(1,18),s(1,21), DirProp_AN },
 { s(1,1), s(1,2),     6 ,     6 , s(1,8),s(1,16),s(1,17), s(1,8), s(1,8),s(2,13),     6 ,     6 , s(1,3),    18 ,    21 , DirProp_AN },
 { s(1,1), s(1,2), s(1,4), s(1,5),     7 ,s(1,15),s(1,17),     7 ,s(2,14),     7 ,     7 ,     7 , s(1,3),s(1,18),s(1,21), DirProp_ON },
 { s(1,1), s(1,2), s(1,6), s(1,6),     8 ,s(1,16),s(1,17),     8 ,     8 ,     8 ,     8 ,     8 , s(1,3),s(1,18),s(1,21), DirProp_ON },
 { s(1,1), s(1,2),     4 , s(1,5),     7 ,s(1,15),s(1,17),     7 ,     9 ,     7 ,     9 ,     9 , s(1,3),    18 ,    21 , DirProp_ON },
 { s(3,1), s(3,2),     4 , s(3,5), s(4,7),s(3,15),s(3,17), s(4,7),s(4,14), s(4,7),    10 , s(4,7), s(3,3),    18 ,    21 , DirProp_EN },
 { s(1,1), s(1,2),     4 , s(1,5), s(1,7),s(1,15),s(1,17), s(1,7),    11 , s(1,7),    11 ,    11 , s(1,3),    18 ,    21 , DirProp_EN },
 { s(3,1), s(3,2), s(3,4),     5 , s(4,7),s(3,15),s(3,17), s(4,7),s(4,14), s(4,7),    12 , s(4,7), s(3,3),s(3,18),s(3,21), DirProp_AN },
 { s(3,1), s(3,2),     6 ,     6 , s(4,8),s(3,16),s(3,17), s(4,8), s(4,8), s(4,8),    13 , s(4,8), s(3,3),    18 ,    21 , DirProp_AN },
 { s(1,1), s(1,2), s(4,4), s(1,5),     7 ,s(1,15),s(1,17),     7 ,    14 ,     7 ,    14 ,    14 , s(1,3),s(4,18),s(4,21), DirProp_ON },
 { s(1,1), s(1,2), s(1,4), s(1,5), s(1,7),    15 ,s(1,17), s(1,7), s(1,9), s(1,7),    15 , s(1,7), s(1,3),s(1,18),s(1,21),  DirProp_S },
 { s(1,1), s(1,2), s(1,6), s(1,6), s(1,8),    16 ,s(1,17), s(1,8), s(1,8), s(1,8),    16 , s(1,8), s(1,3),s(1,18),s(1,21),  DirProp_S },
 { s(1,1), s(1,2), s(1,4), s(1,5), s(1,7),s(1,15),    17 , s(1,7), s(1,9), s(1,7),    17 , s(1,7), s(1,3),s(1,18),s(1,21),  DirProp_B },
 { s(1,1), s(1,2),    18 , s(1,5), s(1,7),s(1,15),s(1,17),s(2,19),    20 ,s(2,19),    18 ,    18 , s(1,3),    18 ,    21 ,  DirProp_L },
 { s(3,1), s(3,2),    18 , s(3,5), s(4,7),s(3,15),s(3,17), s(4,7),s(4,14), s(4,7),    19 , s(4,7), s(3,3),    18 ,    21 ,  DirProp_L },
 { s(1,1), s(1,2),    18 , s(1,5), s(1,7),s(1,15),s(1,17), s(1,7),    20 , s(1,7),    20 ,    20 , s(1,3),    18 ,    21 ,  DirProp_L },
 { s(1,1), s(1,2),    21 , s(1,5), s(1,7),s(1,15),s(1,17),s(2,22),    23 ,s(2,22),    21 ,    21 , s(1,3),    18 ,    21 , DirProp_AN },
 { s(3,1), s(3,2),    21 , s(3,5), s(4,7),s(3,15),s(3,17), s(4,7),s(4,14), s(4,7),    22 , s(4,7), s(3,3),    18 ,    21 , DirProp_AN },
 { s(1,1), s(1,2),    21 , s(1,5), s(1,7),s(1,15),s(1,17), s(1,7),    23 , s(1,7),    23 ,    23 , s(1,3),    18 ,    21 , DirProp_AN }
};




#undef s




















#define IMPTABLEVELS_COLUMNS (DirProp_B + 2)
#define IMPTABLEVELS_RES (IMPTABLEVELS_COLUMNS - 1)
#define GET_STATE(cell) ((cell)&0x0f)
#define GET_ACTION(cell) ((cell)>>4)
#define s(action, newState) ((uint8_t)(newState+(action<<4)))

typedef uint8_t ImpTab[][IMPTABLEVELS_COLUMNS];
typedef uint8_t ImpAct[];




typedef struct ImpTabPair {
    const void * pImpTab[2];
    const void * pImpAct[2];
} ImpTabPair;






































static const ImpTab impTabL_DEFAULT =   



{

 {     0 ,     1 ,     0 ,     2 ,     0 ,     0 ,     0 ,  0 },
 {     0 ,     1 ,     3 ,     3 , s(1,4), s(1,4),     0 ,  1 },
 {     0 ,     1 ,     0 ,     2 , s(1,5), s(1,5),     0 ,  2 },
 {     0 ,     1 ,     3 ,     3 , s(1,4), s(1,4),     0 ,  2 },
 {     0 , s(2,1), s(3,3), s(3,3),     4 ,     4 ,     0 ,  0 },
 {     0 , s(2,1),     0 , s(3,2),     5 ,     5 ,     0 ,  0 }
};
static const ImpTab impTabR_DEFAULT =   



{

 {     1 ,     0 ,     2 ,     2 ,     0 ,     0 ,     0 ,  0 },
 {     1 ,     0 ,     1 ,     3 , s(1,4), s(1,4),     0 ,  1 },
 {     1 ,     0 ,     2 ,     2 ,     0 ,     0 ,     0 ,  1 },
 {     1 ,     0 ,     1 ,     3 ,     5 ,     5 ,     0 ,  1 },
 { s(2,1),     0 , s(2,1),     3 ,     4 ,     4 ,     0 ,  0 },
 {     1 ,     0 ,     1 ,     3 ,     5 ,     5 ,     0 ,  0 }
};
static const ImpAct impAct0 = {0,1,2,3,4};
static const ImpTabPair impTab_DEFAULT = {{&impTabL_DEFAULT,
                                           &impTabR_DEFAULT},
                                          {&impAct0, &impAct0}};

static const ImpTab impTabL_NUMBERS_SPECIAL =   



{

 {     0 ,     2 , s(1,1), s(1,1),     0 ,     0 ,     0 ,  0 },
 {     0 , s(4,2),     1 ,     1 ,     0 ,     0 ,     0 ,  0 },
 {     0 ,     2 ,     4 ,     4 , s(1,3), s(1,3),     0 ,  1 },
 {     0 , s(2,2), s(3,4), s(3,4),     3 ,     3 ,     0 ,  0 },
 {     0 ,     2 ,     4 ,     4 , s(1,3), s(1,3),     0 ,  2 }
};
static const ImpTabPair impTab_NUMBERS_SPECIAL = {{&impTabL_NUMBERS_SPECIAL,
                                                   &impTabR_DEFAULT},
                                                  {&impAct0, &impAct0}};

static const ImpTab impTabL_GROUP_NUMBERS_WITH_R =



{

 {     0 ,     3 , s(1,1), s(1,1),     0 ,     0 ,     0 ,  0 },
 { s(2,0),     3 ,     1 ,     1 ,     2 , s(2,0), s(2,0),  2 },
 { s(2,0),     3 ,     1 ,     1 ,     2 , s(2,0), s(2,0),  1 },
 {     0 ,     3 ,     5 ,     5 , s(1,4),     0 ,     0 ,  1 },
 { s(2,0),     3 ,     5 ,     5 ,     4 , s(2,0), s(2,0),  1 },
 {     0 ,     3 ,     5 ,     5 , s(1,4),     0 ,     0 ,  2 }
};
static const ImpTab impTabR_GROUP_NUMBERS_WITH_R =



{

 {     2 ,     0 ,     1 ,     1 ,     0 ,     0 ,     0 ,  0 },
 {     2 ,     0 ,     1 ,     1 ,     0 ,     0 ,     0 ,  1 },
 {     2 ,     0 , s(1,4), s(1,4), s(1,3),     0 ,     0 ,  1 },
 { s(2,2),     0 ,     4 ,     4 ,     3 ,     0 ,     0 ,  0 },
 { s(2,2),     0 ,     4 ,     4 ,     3 ,     0 ,     0 ,  1 }
};
static const ImpTabPair impTab_GROUP_NUMBERS_WITH_R = {
                        {&impTabL_GROUP_NUMBERS_WITH_R,
                         &impTabR_GROUP_NUMBERS_WITH_R},
                        {&impAct0, &impAct0}};


static const ImpTab impTabL_INVERSE_NUMBERS_AS_L =



{

 {     0 ,     1 ,     0 ,     0 ,     0 ,     0 ,     0 ,  0 },
 {     0 ,     1 ,     0 ,     0 , s(1,4), s(1,4),     0 ,  1 },
 {     0 ,     1 ,     0 ,     0 , s(1,5), s(1,5),     0 ,  2 },
 {     0 ,     1 ,     0 ,     0 , s(1,4), s(1,4),     0 ,  2 },
 { s(2,0),     1 , s(2,0), s(2,0),     4 ,     4 , s(2,0),  1 },
 { s(2,0),     1 , s(2,0), s(2,0),     5 ,     5 , s(2,0),  1 }
};
static const ImpTab impTabR_INVERSE_NUMBERS_AS_L =



{

 {     1 ,     0 ,     1 ,     1 ,     0 ,     0 ,     0 ,  0 },
 {     1 ,     0 ,     1 ,     1 , s(1,4), s(1,4),     0 ,  1 },
 {     1 ,     0 ,     1 ,     1 ,     0 ,     0 ,     0 ,  1 },
 {     1 ,     0 ,     1 ,     1 ,     5 ,     5 ,     0 ,  1 },
 { s(2,1),     0 , s(2,1), s(2,1),     4 ,     4 ,     0 ,  0 },
 {     1 ,     0 ,     1 ,     1 ,     5 ,     5 ,     0 ,  0 }
};
static const ImpTabPair impTab_INVERSE_NUMBERS_AS_L = {
                        {&impTabL_INVERSE_NUMBERS_AS_L,
                         &impTabR_INVERSE_NUMBERS_AS_L},
                        {&impAct0, &impAct0}};

static const ImpTab impTabR_INVERSE_LIKE_DIRECT =   



{

 {     1 ,     0 ,     2 ,     2 ,     0 ,     0 ,     0 ,  0 },
 {     1 ,     0 ,     1 ,     2 , s(1,3), s(1,3),     0 ,  1 },
 {     1 ,     0 ,     2 ,     2 ,     0 ,     0 ,     0 ,  1 },
 { s(2,1), s(3,0),     6 ,     4 ,     3 ,     3 , s(3,0),  0 },
 { s(2,1), s(3,0),     6 ,     4 ,     5 ,     5 , s(3,0),  3 },
 { s(2,1), s(3,0),     6 ,     4 ,     5 ,     5 , s(3,0),  2 },
 { s(2,1), s(3,0),     6 ,     4 ,     3 ,     3 , s(3,0),  1 }
};
static const ImpAct impAct1 = {0,1,13,14};


static const ImpTabPair impTab_INVERSE_LIKE_DIRECT = {
                        {&impTabL_DEFAULT,
                         &impTabR_INVERSE_LIKE_DIRECT},
                        {&impAct0, &impAct1}};

static const ImpTab impTabL_INVERSE_LIKE_DIRECT_WITH_MARKS =


{

 {     0 , s(6,3),     0 ,     1 ,     0 ,     0 ,     0 ,  0 },
 {     0 , s(6,3),     0 ,     1 , s(1,2), s(3,0),     0 ,  4 },
 { s(2,0), s(6,3), s(2,0),     1 ,     2 , s(3,0), s(2,0),  3 },
 {     0 , s(6,3), s(5,5), s(5,6), s(1,4), s(3,0),     0 ,  3 },
 { s(3,0), s(4,3), s(5,5), s(5,6),     4 , s(3,0), s(3,0),  3 },
 { s(3,0), s(4,3),     5 , s(5,6), s(1,4), s(3,0), s(3,0),  4 },
 { s(3,0), s(4,3), s(5,5),     6 , s(1,4), s(3,0), s(3,0),  4 }
};
static const ImpTab impTabR_INVERSE_LIKE_DIRECT_WITH_MARKS =



{

 { s(1,3),     0 ,     1 ,     1 ,     0 ,     0 ,     0 ,  0 },
 { s(2,3),     0 ,     1 ,     1 ,     2 , s(4,0),     0 ,  1 },
 { s(2,3),     0 ,     1 ,     1 ,     2 , s(4,0),     0 ,  0 },
 {     3 ,     0 ,     3 , s(3,6), s(1,4), s(4,0),     0 ,  1 },
 { s(5,3), s(4,0),     5 , s(3,6),     4 , s(4,0), s(4,0),  0 },
 { s(5,3), s(4,0),     5 , s(3,6),     4 , s(4,0), s(4,0),  1 },
 { s(5,3), s(4,0),     6 ,     6 ,     4 , s(4,0), s(4,0),  3 }
};
static const ImpAct impAct2 = {0,1,2,5,6,7,8};
static const ImpAct impAct3 = {0,1,9,10,11,12};
static const ImpTabPair impTab_INVERSE_LIKE_DIRECT_WITH_MARKS = {
                        {&impTabL_INVERSE_LIKE_DIRECT_WITH_MARKS,
                         &impTabR_INVERSE_LIKE_DIRECT_WITH_MARKS},
                        {&impAct2, &impAct3}};

static const ImpTabPair impTab_INVERSE_FOR_NUMBERS_SPECIAL = {
                        {&impTabL_NUMBERS_SPECIAL,
                         &impTabR_INVERSE_LIKE_DIRECT},
                        {&impAct0, &impAct1}};

static const ImpTab impTabL_INVERSE_FOR_NUMBERS_SPECIAL_WITH_MARKS =


{

 {     0 , s(6,2),     1 ,     1 ,     0 ,     0 ,     0 ,  0 },
 {     0 , s(6,2),     1 ,     1 ,     0 , s(3,0),     0 ,  4 },
 {     0 , s(6,2), s(5,4), s(5,4), s(1,3), s(3,0),     0 ,  3 },
 { s(3,0), s(4,2), s(5,4), s(5,4),     3 , s(3,0), s(3,0),  3 },
 { s(3,0), s(4,2),     4 ,     4 , s(1,3), s(3,0), s(3,0),  4 }
};
static const ImpTabPair impTab_INVERSE_FOR_NUMBERS_SPECIAL_WITH_MARKS = {
                        {&impTabL_INVERSE_FOR_NUMBERS_SPECIAL_WITH_MARKS,
                         &impTabR_INVERSE_LIKE_DIRECT_WITH_MARKS},
                        {&impAct2, &impAct3}};

#undef s

typedef struct {
    const ImpTab * pImpTab;             
    const ImpAct * pImpAct;             
    int32_t startON;                    
    int32_t startL2EN;                  
    int32_t lastStrongRTL;              
    int32_t state;                      
    int32_t runStart;                   
    UBiDiLevel runLevel;                
} LevState;



static void
addPoint(UBiDi *pBiDi, int32_t pos, int32_t flag)
  


{
#define FIRSTALLOC  10
    Point point;
    InsertPoints * pInsertPoints=&(pBiDi->insertPoints);

    if (pInsertPoints->capacity == 0)
    {
        pInsertPoints->points=uprv_malloc(sizeof(Point)*FIRSTALLOC);
        if (pInsertPoints->points == NULL)
        {
            pInsertPoints->errorCode=U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        pInsertPoints->capacity=FIRSTALLOC;
    }
    if (pInsertPoints->size >= pInsertPoints->capacity) 
    {
        void * savePoints=pInsertPoints->points;
        pInsertPoints->points=uprv_realloc(pInsertPoints->points,
                                           pInsertPoints->capacity*2*sizeof(Point));
        if (pInsertPoints->points == NULL)
        {
            pInsertPoints->points=savePoints;
            pInsertPoints->errorCode=U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        else  pInsertPoints->capacity*=2;
    }
    point.pos=pos;
    point.flag=flag;
    pInsertPoints->points[pInsertPoints->size]=point;
    pInsertPoints->size++;
#undef FIRSTALLOC
}

static void
setLevelsOutsideIsolates(UBiDi *pBiDi, int32_t start, int32_t limit, UBiDiLevel level)
{
    DirProp *dirProps=pBiDi->dirProps, dirProp;
    UBiDiLevel *levels=pBiDi->levels;
    int32_t isolateCount=0, k;
    for(k=start; k<limit; k++) {
        dirProp=dirProps[k];
        if(dirProp==PDI)
            isolateCount--;
        if(isolateCount==0)
            levels[k]=level;
        if(dirProp==LRI || dirProp==RLI)
            isolateCount++;
    }
}
















static void
processPropertySeq(UBiDi *pBiDi, LevState *pLevState, uint8_t _prop,
                   int32_t start, int32_t limit) {
    uint8_t cell, oldStateSeq, actionSeq;
    const ImpTab * pImpTab=pLevState->pImpTab;
    const ImpAct * pImpAct=pLevState->pImpAct;
    UBiDiLevel * levels=pBiDi->levels;
    UBiDiLevel level, addLevel;
    InsertPoints * pInsertPoints;
    int32_t start0, k;

    start0=start;                           
    oldStateSeq=(uint8_t)pLevState->state;
    cell=(*pImpTab)[oldStateSeq][_prop];
    pLevState->state=GET_STATE(cell);       
    actionSeq=(*pImpAct)[GET_ACTION(cell)]; 
    addLevel=(*pImpTab)[pLevState->state][IMPTABLEVELS_RES];

    if(actionSeq) {
        switch(actionSeq) {
        case 1:                         
            pLevState->startON=start0;
            break;

        case 2:                         
            start=pLevState->startON;
            break;

        case 3:                         
            level=pLevState->runLevel+1;
            setLevelsOutsideIsolates(pBiDi, pLevState->startON, start0, level);
            break;

        case 4:                         
            level=pLevState->runLevel+2;
            setLevelsOutsideIsolates(pBiDi, pLevState->startON, start0, level);
            break;

        case 5:                         
            
            if (pLevState->startL2EN >= 0) {
                addPoint(pBiDi, pLevState->startL2EN, LRM_BEFORE);
            }
            pLevState->startL2EN=-1;  
            
            pInsertPoints=&(pBiDi->insertPoints);
            if ((pInsertPoints->capacity == 0) ||
                (pInsertPoints->size <= pInsertPoints->confirmed))
            {
                
                pLevState->lastStrongRTL=-1;
                
                level=(*pImpTab)[oldStateSeq][IMPTABLEVELS_RES];
                if ((level & 1) && (pLevState->startON > 0)) {  
                    start=pLevState->startON;   
                }
                if (_prop == DirProp_S)                
                {
                    addPoint(pBiDi, start0, LRM_BEFORE);
                    pInsertPoints->confirmed=pInsertPoints->size;
                }
                break;
            }
            
            for (k=pLevState->lastStrongRTL+1; k<start0; k++)
            {
                
                levels[k]=(levels[k] - 2) & ~1;
            }
            
            pInsertPoints->confirmed=pInsertPoints->size;
            pLevState->lastStrongRTL=-1;
            if (_prop == DirProp_S)            
            {
                addPoint(pBiDi, start0, LRM_BEFORE);
                pInsertPoints->confirmed=pInsertPoints->size;
            }
            break;

        case 6:                         
            
            pInsertPoints=&(pBiDi->insertPoints);
            if (pInsertPoints->capacity > 0)
                
                pInsertPoints->size=pInsertPoints->confirmed;
            pLevState->startON=-1;
            pLevState->startL2EN=-1;
            pLevState->lastStrongRTL=limit - 1;
            break;

        case 7:                         
            
            if ((_prop == DirProp_AN) && (pBiDi->dirProps[start0] == AN) &&
                (pBiDi->reorderingMode!=UBIDI_REORDER_INVERSE_FOR_NUMBERS_SPECIAL))
            {
                
                if (pLevState->startL2EN == -1) 
                {
                    
                    pLevState->lastStrongRTL=limit - 1;
                    break;
                }
                if (pLevState->startL2EN >= 0)  
                {
                    addPoint(pBiDi, pLevState->startL2EN, LRM_BEFORE);
                    pLevState->startL2EN=-2;
                }
                
                addPoint(pBiDi, start0, LRM_BEFORE);
                break;
            }
            
            if (pLevState->startL2EN == -1) {
                pLevState->startL2EN=start0;
            }
            break;

        case 8:                         
            pLevState->lastStrongRTL=limit - 1;
            pLevState->startON=-1;
            break;

        case 9:                         
            
            for (k=start0-1; k>=0 && !(levels[k]&1); k--);
            if(k>=0) {
                addPoint(pBiDi, k, RLM_BEFORE);             
                pInsertPoints=&(pBiDi->insertPoints);
                pInsertPoints->confirmed=pInsertPoints->size;   
            }
            pLevState->startON=start0;
            break;

        case 10:                        
            
            
            addPoint(pBiDi, start0, LRM_BEFORE);    
            addPoint(pBiDi, start0, LRM_AFTER);     
            break;

        case 11:                        
            
            pInsertPoints=&(pBiDi->insertPoints);
            pInsertPoints->size=pInsertPoints->confirmed;
            if (_prop == DirProp_S)            
            {
                addPoint(pBiDi, start0, RLM_BEFORE);
                pInsertPoints->confirmed=pInsertPoints->size;
            }
            break;

        case 12:                        
            level=pLevState->runLevel + addLevel;
            for(k=pLevState->startON; k<start0; k++) {
                if (levels[k]<level)
                    levels[k]=level;
            }
            pInsertPoints=&(pBiDi->insertPoints);
            pInsertPoints->confirmed=pInsertPoints->size;   
            pLevState->startON=start0;
            break;

        case 13:                        
            level=pLevState->runLevel;
            for(k=start0-1; k>=pLevState->startON; k--) {
                if(levels[k]==level+3) {
                    while(levels[k]==level+3) {
                        levels[k--]-=2;
                    }
                    while(levels[k]==level) {
                        k--;
                    }
                }
                if(levels[k]==level+2) {
                    levels[k]=level;
                    continue;
                }
                levels[k]=level+1;
            }
            break;

        case 14:                        
            level=pLevState->runLevel+1;
            for(k=start0-1; k>=pLevState->startON; k--) {
                if(levels[k]>level) {
                    levels[k]-=2;
                }
            }
            break;

        default:                        
            U_ASSERT(FALSE);
            break;
        }
    }
    if((addLevel) || (start < start0)) {
        level=pLevState->runLevel + addLevel;
        if(start>=pLevState->runStart) {
            for(k=start; k<limit; k++) {
                levels[k]=level;
            }
        } else {
            setLevelsOutsideIsolates(pBiDi, start, limit, level);
        }
    }
}





static DirProp
lastL_R_AL(UBiDi *pBiDi) {
    const UChar *text=pBiDi->prologue;
    int32_t length=pBiDi->proLength;
    int32_t i;
    UChar32 uchar;
    DirProp dirProp;
    for(i=length; i>0; ) {
        
        U16_PREV(text, 0, i, uchar);
        dirProp=(DirProp)ubidi_getCustomizedClass(pBiDi, uchar);
        if(dirProp==L) {
            return DirProp_L;
        }
        if(dirProp==R || dirProp==AL) {
            return DirProp_R;
        }
        if(dirProp==B) {
            return DirProp_ON;
        }
    }
    return DirProp_ON;
}





static DirProp
firstL_R_AL_EN_AN(UBiDi *pBiDi) {
    const UChar *text=pBiDi->epilogue;
    int32_t length=pBiDi->epiLength;
    int32_t i;
    UChar32 uchar;
    DirProp dirProp;
    for(i=0; i<length; ) {
        
        U16_NEXT(text, i, length, uchar);
        dirProp=(DirProp)ubidi_getCustomizedClass(pBiDi, uchar);
        if(dirProp==L) {
            return DirProp_L;
        }
        if(dirProp==R || dirProp==AL) {
            return DirProp_R;
        }
        if(dirProp==EN) {
            return DirProp_EN;
        }
        if(dirProp==AN) {
            return DirProp_AN;
        }
    }
    return DirProp_ON;
}

static void
resolveImplicitLevels(UBiDi *pBiDi,
                      int32_t start, int32_t limit,
                      DirProp sor, DirProp eor) {
    const DirProp *dirProps=pBiDi->dirProps;
    DirProp dirProp;
    LevState levState;
    int32_t i, start1, start2;
    uint16_t oldStateImp, stateImp, actionImp;
    uint8_t gprop, resProp, cell;
    UBool inverseRTL;
    DirProp nextStrongProp=R;
    int32_t nextStrongPos=-1;

    
    





    inverseRTL=(UBool)
        ((start<pBiDi->lastArabicPos) && (GET_PARALEVEL(pBiDi, start) & 1) &&
         (pBiDi->reorderingMode==UBIDI_REORDER_INVERSE_LIKE_DIRECT  ||
          pBiDi->reorderingMode==UBIDI_REORDER_INVERSE_FOR_NUMBERS_SPECIAL));

    
    levState.startL2EN=-1;              
    levState.lastStrongRTL=-1;          
    levState.runStart=start;
    levState.runLevel=pBiDi->levels[start];
    levState.pImpTab=(const ImpTab*)((pBiDi->pImpTabPair)->pImpTab)[levState.runLevel&1];
    levState.pImpAct=(const ImpAct*)((pBiDi->pImpTabPair)->pImpAct)[levState.runLevel&1];
    if(start==0 && pBiDi->proLength>0) {
        DirProp lastStrong=lastL_R_AL(pBiDi);
        if(lastStrong!=DirProp_ON) {
            sor=lastStrong;
        }
    }
    


    if(dirProps[start]==PDI  && pBiDi->isolateCount >= 0) {
        levState.startON=pBiDi->isolates[pBiDi->isolateCount].startON;
        start1=pBiDi->isolates[pBiDi->isolateCount].start1;
        stateImp=pBiDi->isolates[pBiDi->isolateCount].stateImp;
        levState.state=pBiDi->isolates[pBiDi->isolateCount].state;
        pBiDi->isolateCount--;
    } else {
        levState.startON=-1;
        start1=start;
        if(dirProps[start]==NSM)
            stateImp = 1 + sor;
        else
            stateImp=0;
        levState.state=0;
        processPropertySeq(pBiDi, &levState, sor, start, start);
    }
    start2=start;                       

    for(i=start; i<=limit; i++) {
        if(i>=limit) {
            int32_t k;
            for(k=limit-1; k>start&&(DIRPROP_FLAG(dirProps[k])&MASK_BN_EXPLICIT); k--);
            dirProp=dirProps[k];
            if(dirProp==LRI || dirProp==RLI)
                break;      
            gprop=eor;
        } else {
            DirProp prop, prop1;
            prop=dirProps[i];
            if(prop==B) {
                pBiDi->isolateCount=-1; 
            }
            if(inverseRTL) {
                if(prop==AL) {
                    
                    prop=R;
                } else if(prop==EN) {
                    if(nextStrongPos<=i) {
                        
                        int32_t j;
                        nextStrongProp=R;   
                        nextStrongPos=limit;
                        for(j=i+1; j<limit; j++) {
                            prop1=dirProps[j];
                            if(prop1==L || prop1==R || prop1==AL) {
                                nextStrongProp=prop1;
                                nextStrongPos=j;
                                break;
                            }
                        }
                    }
                    if(nextStrongProp==AL) {
                        prop=AN;
                    }
                }
            }
            gprop=groupProp[prop];
        }
        oldStateImp=stateImp;
        cell=impTabProps[oldStateImp][gprop];
        stateImp=GET_STATEPROPS(cell);      
        actionImp=GET_ACTIONPROPS(cell);    
        if((i==limit) && (actionImp==0)) {
            
            actionImp=1;                    
        }
        if(actionImp) {
            resProp=impTabProps[oldStateImp][IMPTABPROPS_RES];
            switch(actionImp) {
            case 1:             
                processPropertySeq(pBiDi, &levState, resProp, start1, i);
                start1=i;
                break;
            case 2:             
                start2=i;
                break;
            case 3:             
                processPropertySeq(pBiDi, &levState, resProp, start1, start2);
                processPropertySeq(pBiDi, &levState, DirProp_ON, start2, i);
                start1=i;
                break;
            case 4:             
                processPropertySeq(pBiDi, &levState, resProp, start1, start2);
                start1=start2;
                start2=i;
                break;
            default:            
                U_ASSERT(FALSE);
                break;
            }
        }
    }

    
    if(limit==pBiDi->length && pBiDi->epiLength>0) {
        DirProp firstStrong=firstL_R_AL_EN_AN(pBiDi);
        if(firstStrong!=DirProp_ON) {
            eor=firstStrong;
        }
    }

    
    for(i=limit-1; i>start&&(DIRPROP_FLAG(dirProps[i])&MASK_BN_EXPLICIT); i--);
    dirProp=dirProps[i];
    if((dirProp==LRI || dirProp==RLI) && limit<pBiDi->length) {
        pBiDi->isolateCount++;
        pBiDi->isolates[pBiDi->isolateCount].stateImp=stateImp;
        pBiDi->isolates[pBiDi->isolateCount].state=levState.state;
        pBiDi->isolates[pBiDi->isolateCount].start1=start1;
        pBiDi->isolates[pBiDi->isolateCount].startON=levState.startON;
    }
    else
        processPropertySeq(pBiDi, &levState, eor, limit, limit);
}









static void
adjustWSLevels(UBiDi *pBiDi) {
    const DirProp *dirProps=pBiDi->dirProps;
    UBiDiLevel *levels=pBiDi->levels;
    int32_t i;

    if(pBiDi->flags&MASK_WS) {
        UBool orderParagraphsLTR=pBiDi->orderParagraphsLTR;
        Flags flag;

        i=pBiDi->trailingWSStart;
        while(i>0) {
            
            while(i>0 && (flag=DIRPROP_FLAG(dirProps[--i]))&MASK_WS) {
                if(orderParagraphsLTR&&(flag&DIRPROP_FLAG(B))) {
                    levels[i]=0;
                } else {
                    levels[i]=GET_PARALEVEL(pBiDi, i);
                }
            }

            
            
            while(i>0) {
                flag=DIRPROP_FLAG(dirProps[--i]);
                if(flag&MASK_BN_EXPLICIT) {
                    levels[i]=levels[i+1];
                } else if(orderParagraphsLTR&&(flag&DIRPROP_FLAG(B))) {
                    levels[i]=0;
                    break;
                } else if(flag&MASK_B_S) {
                    levels[i]=GET_PARALEVEL(pBiDi, i);
                    break;
                }
            }
        }
    }
}

U_CAPI void U_EXPORT2
ubidi_setContext(UBiDi *pBiDi,
                 const UChar *prologue, int32_t proLength,
                 const UChar *epilogue, int32_t epiLength,
                 UErrorCode *pErrorCode) {
    
    RETURN_VOID_IF_NULL_OR_FAILING_ERRCODE(pErrorCode);
    if(pBiDi==NULL || proLength<-1 || epiLength<-1 ||
       (prologue==NULL && proLength!=0) || (epilogue==NULL && epiLength!=0)) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }

    if(proLength==-1) {
        pBiDi->proLength=u_strlen(prologue);
    } else {
        pBiDi->proLength=proLength;
    }
    if(epiLength==-1) {
        pBiDi->epiLength=u_strlen(epilogue);
    } else {
        pBiDi->epiLength=epiLength;
    }
    pBiDi->prologue=prologue;
    pBiDi->epilogue=epilogue;
}

static void
setParaSuccess(UBiDi *pBiDi) {
    pBiDi->proLength=0;                 
    pBiDi->epiLength=0;
    pBiDi->pParaBiDi=pBiDi;             
}

#define BIDI_MIN(x, y)   ((x)<(y) ? (x) : (y))
#define BIDI_ABS(x)      ((x)>=0  ? (x) : (-(x)))

static void
setParaRunsOnly(UBiDi *pBiDi, const UChar *text, int32_t length,
                UBiDiLevel paraLevel, UErrorCode *pErrorCode) {
    void *runsOnlyMemory = NULL;
    int32_t *visualMap;
    UChar *visualText;
    int32_t saveLength, saveTrailingWSStart;
    const UBiDiLevel *levels;
    UBiDiLevel *saveLevels;
    UBiDiDirection saveDirection;
    UBool saveMayAllocateText;
    Run *runs;
    int32_t visualLength, i, j, visualStart, logicalStart,
            runCount, runLength, addedRuns, insertRemove,
            start, limit, step, indexOddBit, logicalPos,
            index0, index1;
    uint32_t saveOptions;

    pBiDi->reorderingMode=UBIDI_REORDER_DEFAULT;
    if(length==0) {
        ubidi_setPara(pBiDi, text, length, paraLevel, NULL, pErrorCode);
        goto cleanup3;
    }
    
    runsOnlyMemory=uprv_malloc(length*(sizeof(int32_t)+sizeof(UChar)+sizeof(UBiDiLevel)));
    if(runsOnlyMemory==NULL) {
        *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
        goto cleanup3;
    }
    visualMap=runsOnlyMemory;
    visualText=(UChar *)&visualMap[length];
    saveLevels=(UBiDiLevel *)&visualText[length];
    saveOptions=pBiDi->reorderingOptions;
    if(saveOptions & UBIDI_OPTION_INSERT_MARKS) {
        pBiDi->reorderingOptions&=~UBIDI_OPTION_INSERT_MARKS;
        pBiDi->reorderingOptions|=UBIDI_OPTION_REMOVE_CONTROLS;
    }
    paraLevel&=1;                       
    ubidi_setPara(pBiDi, text, length, paraLevel, NULL, pErrorCode);
    if(U_FAILURE(*pErrorCode)) {
        goto cleanup3;
    }
    


    levels=ubidi_getLevels(pBiDi, pErrorCode);
    uprv_memcpy(saveLevels, levels, pBiDi->length*sizeof(UBiDiLevel));
    saveTrailingWSStart=pBiDi->trailingWSStart;
    saveLength=pBiDi->length;
    saveDirection=pBiDi->direction;

    





    visualLength=ubidi_writeReordered(pBiDi, visualText, length,
                                      UBIDI_DO_MIRRORING, pErrorCode);
    ubidi_getVisualMap(pBiDi, visualMap, pErrorCode);
    if(U_FAILURE(*pErrorCode)) {
        goto cleanup2;
    }
    pBiDi->reorderingOptions=saveOptions;

    pBiDi->reorderingMode=UBIDI_REORDER_INVERSE_LIKE_DIRECT;
    paraLevel^=1;
    






    saveMayAllocateText=pBiDi->mayAllocateText;
    pBiDi->mayAllocateText=FALSE;
    ubidi_setPara(pBiDi, visualText, visualLength, paraLevel, NULL, pErrorCode);
    pBiDi->mayAllocateText=saveMayAllocateText;
    ubidi_getRuns(pBiDi, pErrorCode);
    if(U_FAILURE(*pErrorCode)) {
        goto cleanup1;
    }
    
    addedRuns=0;
    runCount=pBiDi->runCount;
    runs=pBiDi->runs;
    visualStart=0;
    for(i=0; i<runCount; i++, visualStart+=runLength) {
        runLength=runs[i].visualLimit-visualStart;
        if(runLength<2) {
            continue;
        }
        logicalStart=GET_INDEX(runs[i].logicalStart);
        for(j=logicalStart+1; j<logicalStart+runLength; j++) {
            index0=visualMap[j];
            index1=visualMap[j-1];
            if((BIDI_ABS(index0-index1)!=1) || (saveLevels[index0]!=saveLevels[index1])) {
                addedRuns++;
            }
        }
    }
    if(addedRuns) {
        if(getRunsMemory(pBiDi, runCount+addedRuns)) {
            if(runCount==1) {
                
                pBiDi->runsMemory[0]=runs[0];
            }
            runs=pBiDi->runs=pBiDi->runsMemory;
            pBiDi->runCount+=addedRuns;
        } else {
            goto cleanup1;
        }
    }
    
    for(i=runCount-1; i>=0; i--) {
        runLength= i==0 ? runs[0].visualLimit :
                          runs[i].visualLimit-runs[i-1].visualLimit;
        logicalStart=runs[i].logicalStart;
        indexOddBit=GET_ODD_BIT(logicalStart);
        logicalStart=GET_INDEX(logicalStart);
        if(runLength<2) {
            if(addedRuns) {
                runs[i+addedRuns]=runs[i];
            }
            logicalPos=visualMap[logicalStart];
            runs[i+addedRuns].logicalStart=MAKE_INDEX_ODD_PAIR(logicalPos,
                                            saveLevels[logicalPos]^indexOddBit);
            continue;
        }
        if(indexOddBit) {
            start=logicalStart;
            limit=logicalStart+runLength-1;
            step=1;
        } else {
            start=logicalStart+runLength-1;
            limit=logicalStart;
            step=-1;
        }
        for(j=start; j!=limit; j+=step) {
            index0=visualMap[j];
            index1=visualMap[j+step];
            if((BIDI_ABS(index0-index1)!=1) || (saveLevels[index0]!=saveLevels[index1])) {
                logicalPos=BIDI_MIN(visualMap[start], index0);
                runs[i+addedRuns].logicalStart=MAKE_INDEX_ODD_PAIR(logicalPos,
                                            saveLevels[logicalPos]^indexOddBit);
                runs[i+addedRuns].visualLimit=runs[i].visualLimit;
                runs[i].visualLimit-=BIDI_ABS(j-start)+1;
                insertRemove=runs[i].insertRemove&(LRM_AFTER|RLM_AFTER);
                runs[i+addedRuns].insertRemove=insertRemove;
                runs[i].insertRemove&=~insertRemove;
                start=j+step;
                addedRuns--;
            }
        }
        if(addedRuns) {
            runs[i+addedRuns]=runs[i];
        }
        logicalPos=BIDI_MIN(visualMap[start], visualMap[limit]);
        runs[i+addedRuns].logicalStart=MAKE_INDEX_ODD_PAIR(logicalPos,
                                            saveLevels[logicalPos]^indexOddBit);
    }

  cleanup1:
    
    pBiDi->paraLevel^=1;
  cleanup2:
    
    pBiDi->text=text;
    pBiDi->length=saveLength;
    pBiDi->originalLength=length;
    pBiDi->direction=saveDirection;
    
    if(saveLength>pBiDi->levelsSize) {
        saveLength=pBiDi->levelsSize;
    }
    uprv_memcpy(pBiDi->levels, saveLevels, saveLength*sizeof(UBiDiLevel));
    pBiDi->trailingWSStart=saveTrailingWSStart;
    if(pBiDi->runCount>1) {
        pBiDi->direction=UBIDI_MIXED;
    }
  cleanup3:
    
    uprv_free(runsOnlyMemory);

    pBiDi->reorderingMode=UBIDI_REORDER_RUNS_ONLY;
}



U_CAPI void U_EXPORT2
ubidi_setPara(UBiDi *pBiDi, const UChar *text, int32_t length,
              UBiDiLevel paraLevel, UBiDiLevel *embeddingLevels,
              UErrorCode *pErrorCode) {
    UBiDiDirection direction;
    DirProp *dirProps;

    
    RETURN_VOID_IF_NULL_OR_FAILING_ERRCODE(pErrorCode);
    if(pBiDi==NULL || text==NULL || length<-1 ||
       (paraLevel>UBIDI_MAX_EXPLICIT_LEVEL && paraLevel<UBIDI_DEFAULT_LTR)) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }

    if(length==-1) {
        length=u_strlen(text);
    }

    
    if(pBiDi->reorderingMode==UBIDI_REORDER_RUNS_ONLY) {
        setParaRunsOnly(pBiDi, text, length, paraLevel, pErrorCode);
        return;
    }

    
    pBiDi->pParaBiDi=NULL;          
    pBiDi->text=text;
    pBiDi->length=pBiDi->originalLength=pBiDi->resultLength=length;
    pBiDi->paraLevel=paraLevel;
    pBiDi->direction=paraLevel&1;
    pBiDi->paraCount=1;

    pBiDi->dirProps=NULL;
    pBiDi->levels=NULL;
    pBiDi->runs=NULL;
    pBiDi->insertPoints.size=0;         
    pBiDi->insertPoints.confirmed=0;    

    


    pBiDi->defaultParaLevel=IS_DEFAULT_LEVEL(paraLevel);

    if(length==0) {
        




        if(IS_DEFAULT_LEVEL(paraLevel)) {
            pBiDi->paraLevel&=1;
            pBiDi->defaultParaLevel=0;
        }
        pBiDi->flags=DIRPROP_FLAG_LR(paraLevel);
        pBiDi->runCount=0;
        pBiDi->paraCount=0;
        setParaSuccess(pBiDi);          
        return;
    }

    pBiDi->runCount=-1;

    
    if(pBiDi->parasMemory)
        pBiDi->paras=pBiDi->parasMemory;
    else
        pBiDi->paras=pBiDi->simpleParas;

    




    if(getDirPropsMemory(pBiDi, length)) {
        pBiDi->dirProps=pBiDi->dirPropsMemory;
        if(!getDirProps(pBiDi)) {
            *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
            return;
        }
    } else {
        *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    dirProps=pBiDi->dirProps;
    
    length= pBiDi->length;
    pBiDi->trailingWSStart=length;  

    
    if(embeddingLevels==NULL) {
        \
        if(getLevelsMemory(pBiDi, length)) {
            pBiDi->levels=pBiDi->levelsMemory;
            direction=resolveExplicitLevels(pBiDi, pErrorCode);
            if(U_FAILURE(*pErrorCode)) {
                return;
            }
        } else {
            *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
            return;
        }
    } else {
        
        pBiDi->levels=embeddingLevels;
        direction=checkExplicitLevels(pBiDi, pErrorCode);
        if(U_FAILURE(*pErrorCode)) {
            return;
        }
    }

    
    if(pBiDi->isolateCount<=SIMPLE_ISOLATES_COUNT)
        pBiDi->isolates=pBiDi->simpleIsolates;
    else
        if((int32_t)(pBiDi->isolateCount*sizeof(Isolate))<=pBiDi->isolatesSize)
            pBiDi->isolates=pBiDi->isolatesMemory;
        else {
            if(getInitialIsolatesMemory(pBiDi, pBiDi->isolateCount)) {
                pBiDi->isolates=pBiDi->isolatesMemory;
            } else {
                *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
                return;
            }
        }
    pBiDi->isolateCount=-1;             

    



    pBiDi->direction=direction;
    switch(direction) {
    case UBIDI_LTR:
        
        pBiDi->trailingWSStart=0;
        break;
    case UBIDI_RTL:
        
        pBiDi->trailingWSStart=0;
        break;
    default:
        


        switch(pBiDi->reorderingMode) {
        case UBIDI_REORDER_DEFAULT:
            pBiDi->pImpTabPair=&impTab_DEFAULT;
            break;
        case UBIDI_REORDER_NUMBERS_SPECIAL:
            pBiDi->pImpTabPair=&impTab_NUMBERS_SPECIAL;
            break;
        case UBIDI_REORDER_GROUP_NUMBERS_WITH_R:
            pBiDi->pImpTabPair=&impTab_GROUP_NUMBERS_WITH_R;
            break;
        case UBIDI_REORDER_INVERSE_NUMBERS_AS_L:
            pBiDi->pImpTabPair=&impTab_INVERSE_NUMBERS_AS_L;
            break;
        case UBIDI_REORDER_INVERSE_LIKE_DIRECT:
            if (pBiDi->reorderingOptions & UBIDI_OPTION_INSERT_MARKS) {
                pBiDi->pImpTabPair=&impTab_INVERSE_LIKE_DIRECT_WITH_MARKS;
            } else {
                pBiDi->pImpTabPair=&impTab_INVERSE_LIKE_DIRECT;
            }
            break;
        case UBIDI_REORDER_INVERSE_FOR_NUMBERS_SPECIAL:
            if (pBiDi->reorderingOptions & UBIDI_OPTION_INSERT_MARKS) {
                pBiDi->pImpTabPair=&impTab_INVERSE_FOR_NUMBERS_SPECIAL_WITH_MARKS;
            } else {
                pBiDi->pImpTabPair=&impTab_INVERSE_FOR_NUMBERS_SPECIAL;
            }
            break;
        default:
            
            U_ASSERT(FALSE);
            break;
        }
        










        if(embeddingLevels==NULL && pBiDi->paraCount<=1 &&
                                   !(pBiDi->flags&DIRPROP_FLAG_MULTI_RUNS)) {
            resolveImplicitLevels(pBiDi, 0, length,
                                    GET_LR_FROM_LEVEL(GET_PARALEVEL(pBiDi, 0)),
                                    GET_LR_FROM_LEVEL(GET_PARALEVEL(pBiDi, length-1)));
        } else {
            
            UBiDiLevel *levels=pBiDi->levels;
            int32_t start, limit=0;
            UBiDiLevel level, nextLevel;
            DirProp sor, eor;

            
            level=GET_PARALEVEL(pBiDi, 0);
            nextLevel=levels[0];
            if(level<nextLevel) {
                eor=GET_LR_FROM_LEVEL(nextLevel);
            } else {
                eor=GET_LR_FROM_LEVEL(level);
            }

            do {
                

                
                start=limit;
                level=nextLevel;
                if((start>0) && (dirProps[start-1]==B)) {
                    
                    sor=GET_LR_FROM_LEVEL(GET_PARALEVEL(pBiDi, start));
                } else {
                    sor=eor;
                }

                
                while((++limit<length) &&
                      ((levels[limit]==level) ||
                       (DIRPROP_FLAG(dirProps[limit])&MASK_BN_EXPLICIT))) {}

                
                if(limit<length) {
                    nextLevel=levels[limit];
                } else {
                    nextLevel=GET_PARALEVEL(pBiDi, length-1);
                }

                
                if(NO_OVERRIDE(level)<NO_OVERRIDE(nextLevel)) {
                    eor=GET_LR_FROM_LEVEL(nextLevel);
                } else {
                    eor=GET_LR_FROM_LEVEL(level);
                }

                

                if(!(level&UBIDI_LEVEL_OVERRIDE)) {
                    resolveImplicitLevels(pBiDi, start, limit, sor, eor);
                } else {
                    
                    do {
                        levels[start++]&=~UBIDI_LEVEL_OVERRIDE;
                    } while(start<limit);
                }
            } while(limit<length);
        }
        
        if (U_FAILURE(pBiDi->insertPoints.errorCode))
        {
            *pErrorCode=pBiDi->insertPoints.errorCode;
            return;
        }
        
        adjustWSLevels(pBiDi);
        break;
    }
    


    if((pBiDi->defaultParaLevel>0) &&
       (pBiDi->reorderingOptions & UBIDI_OPTION_INSERT_MARKS) &&
       ((pBiDi->reorderingMode==UBIDI_REORDER_INVERSE_LIKE_DIRECT) ||
        (pBiDi->reorderingMode==UBIDI_REORDER_INVERSE_FOR_NUMBERS_SPECIAL))) {
        int32_t i, j, start, last;
        UBiDiLevel level;
        DirProp dirProp;
        for(i=0; i<pBiDi->paraCount; i++) {
            last=(pBiDi->paras[i].limit)-1;
            level=pBiDi->paras[i].level;
            if(level==0)
                continue;           
            start= i==0 ? 0 : pBiDi->paras[i-1].limit;
            for(j=last; j>=start; j--) {
                dirProp=dirProps[j];
                if(dirProp==L) {
                    if(j<last) {
                        while(dirProps[last]==B) {
                            last--;
                        }
                    }
                    addPoint(pBiDi, last, RLM_BEFORE);
                    break;
                }
                if(DIRPROP_FLAG(dirProp) & MASK_R_AL) {
                    break;
                }
            }
        }
    }

    if(pBiDi->reorderingOptions & UBIDI_OPTION_REMOVE_CONTROLS) {
        pBiDi->resultLength -= pBiDi->controlCount;
    } else {
        pBiDi->resultLength += pBiDi->insertPoints.size;
    }
    setParaSuccess(pBiDi);              
}

U_CAPI void U_EXPORT2
ubidi_orderParagraphsLTR(UBiDi *pBiDi, UBool orderParagraphsLTR) {
    if(pBiDi!=NULL) {
        pBiDi->orderParagraphsLTR=orderParagraphsLTR;
    }
}

U_CAPI UBool U_EXPORT2
ubidi_isOrderParagraphsLTR(UBiDi *pBiDi) {
    if(pBiDi!=NULL) {
        return pBiDi->orderParagraphsLTR;
    } else {
        return FALSE;
    }
}

U_CAPI UBiDiDirection U_EXPORT2
ubidi_getDirection(const UBiDi *pBiDi) {
    if(IS_VALID_PARA_OR_LINE(pBiDi)) {
        return pBiDi->direction;
    } else {
        return UBIDI_LTR;
    }
}

U_CAPI const UChar * U_EXPORT2
ubidi_getText(const UBiDi *pBiDi) {
    if(IS_VALID_PARA_OR_LINE(pBiDi)) {
        return pBiDi->text;
    } else {
        return NULL;
    }
}

U_CAPI int32_t U_EXPORT2
ubidi_getLength(const UBiDi *pBiDi) {
    if(IS_VALID_PARA_OR_LINE(pBiDi)) {
        return pBiDi->originalLength;
    } else {
        return 0;
    }
}

U_CAPI int32_t U_EXPORT2
ubidi_getProcessedLength(const UBiDi *pBiDi) {
    if(IS_VALID_PARA_OR_LINE(pBiDi)) {
        return pBiDi->length;
    } else {
        return 0;
    }
}

U_CAPI int32_t U_EXPORT2
ubidi_getResultLength(const UBiDi *pBiDi) {
    if(IS_VALID_PARA_OR_LINE(pBiDi)) {
        return pBiDi->resultLength;
    } else {
        return 0;
    }
}



U_CAPI UBiDiLevel U_EXPORT2
ubidi_getParaLevel(const UBiDi *pBiDi) {
    if(IS_VALID_PARA_OR_LINE(pBiDi)) {
        return pBiDi->paraLevel;
    } else {
        return 0;
    }
}

U_CAPI int32_t U_EXPORT2
ubidi_countParagraphs(UBiDi *pBiDi) {
    if(!IS_VALID_PARA_OR_LINE(pBiDi)) {
        return 0;
    } else {
        return pBiDi->paraCount;
    }
}

U_CAPI void U_EXPORT2
ubidi_getParagraphByIndex(const UBiDi *pBiDi, int32_t paraIndex,
                          int32_t *pParaStart, int32_t *pParaLimit,
                          UBiDiLevel *pParaLevel, UErrorCode *pErrorCode) {
    int32_t paraStart;

    
    RETURN_VOID_IF_NULL_OR_FAILING_ERRCODE(pErrorCode);
    RETURN_VOID_IF_NOT_VALID_PARA_OR_LINE(pBiDi, *pErrorCode);
    RETURN_VOID_IF_BAD_RANGE(paraIndex, 0, pBiDi->paraCount, *pErrorCode);

    pBiDi=pBiDi->pParaBiDi;             
    if(paraIndex) {
        paraStart=pBiDi->paras[paraIndex-1].limit;
    } else {
        paraStart=0;
    }
    if(pParaStart!=NULL) {
        *pParaStart=paraStart;
    }
    if(pParaLimit!=NULL) {
        *pParaLimit=pBiDi->paras[paraIndex].limit;
    }
    if(pParaLevel!=NULL) {
        *pParaLevel=GET_PARALEVEL(pBiDi, paraStart);
    }
}

U_CAPI int32_t U_EXPORT2
ubidi_getParagraph(const UBiDi *pBiDi, int32_t charIndex,
                          int32_t *pParaStart, int32_t *pParaLimit,
                          UBiDiLevel *pParaLevel, UErrorCode *pErrorCode) {
    int32_t paraIndex;

    
    
    RETURN_IF_NULL_OR_FAILING_ERRCODE(pErrorCode, -1);
    RETURN_IF_NOT_VALID_PARA_OR_LINE(pBiDi, *pErrorCode, -1);
    pBiDi=pBiDi->pParaBiDi;             
    RETURN_IF_BAD_RANGE(charIndex, 0, pBiDi->length, *pErrorCode, -1);

    for(paraIndex=0; charIndex>=pBiDi->paras[paraIndex].limit; paraIndex++);
    ubidi_getParagraphByIndex(pBiDi, paraIndex, pParaStart, pParaLimit, pParaLevel, pErrorCode);
    return paraIndex;
}

U_CAPI void U_EXPORT2
ubidi_setClassCallback(UBiDi *pBiDi, UBiDiClassCallback *newFn,
                       const void *newContext, UBiDiClassCallback **oldFn,
                       const void **oldContext, UErrorCode *pErrorCode)
{
    RETURN_VOID_IF_NULL_OR_FAILING_ERRCODE(pErrorCode);
    if(pBiDi==NULL) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    if( oldFn )
    {
        *oldFn = pBiDi->fnClassCallback;
    }
    if( oldContext )
    {
        *oldContext = pBiDi->coClassCallback;
    }
    pBiDi->fnClassCallback = newFn;
    pBiDi->coClassCallback = newContext;
}

U_CAPI void U_EXPORT2
ubidi_getClassCallback(UBiDi *pBiDi, UBiDiClassCallback **fn, const void **context)
{
    if(pBiDi==NULL) {
        return;
    }
    if( fn )
    {
        *fn = pBiDi->fnClassCallback;
    }
    if( context )
    {
        *context = pBiDi->coClassCallback;
    }
}

U_CAPI UCharDirection U_EXPORT2
ubidi_getCustomizedClass(UBiDi *pBiDi, UChar32 c)
{
    UCharDirection dir;

    if( pBiDi->fnClassCallback == NULL ||
        (dir = (*pBiDi->fnClassCallback)(pBiDi->coClassCallback, c)) == U_BIDI_CLASS_DEFAULT )
    {
        dir = ubidi_getClass(pBiDi->bdp, c);
    }
    if(dir >= U_CHAR_DIRECTION_COUNT) {
        dir = ON;
    }
    return dir;
}
