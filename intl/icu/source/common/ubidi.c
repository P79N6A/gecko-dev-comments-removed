















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
#define DIRPROP_FLAG_E(level) flagE[(level)&1]
#define DIRPROP_FLAG_O(level) flagO[(level)&1]



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
        if(pBiDi->runsMemory!=NULL) {
            uprv_free(pBiDi->runsMemory);
        }
        if(pBiDi->parasMemory!=NULL) {
            uprv_free(pBiDi->parasMemory);
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






static void
getDirProps(UBiDi *pBiDi) {
    const UChar *text=pBiDi->text;
    DirProp *dirProps=pBiDi->dirPropsMemory;    

    int32_t i=0, i1, length=pBiDi->originalLength;
    Flags flags=0;      
    UChar32 uchar;
    DirProp dirProp=0, paraDirDefault=0;
    UBool isDefaultLevel=IS_DEFAULT_LEVEL(pBiDi->paraLevel);
    

    UBool isDefaultLevelInverse=isDefaultLevel && (UBool)
            (pBiDi->reorderingMode==UBIDI_REORDER_INVERSE_LIKE_DIRECT ||
             pBiDi->reorderingMode==UBIDI_REORDER_INVERSE_FOR_NUMBERS_SPECIAL);
    int32_t lastArabicPos=-1;
    int32_t controlCount=0;
    UBool removeBiDiControls = (UBool)(pBiDi->reorderingOptions &
                                       UBIDI_OPTION_REMOVE_CONTROLS);

    typedef enum {
         NOT_CONTEXTUAL,                
         LOOKING_FOR_STRONG,            
         FOUND_STRONG_CHAR              
    } State;
    State state;
    int32_t paraStart=0;                
    DirProp paraDir;                    

    DirProp lastStrongDir=0;            
    int32_t lastStrongLTR=0;            

    if(pBiDi->reorderingOptions & UBIDI_OPTION_STREAMING) {
        pBiDi->length=0;
        lastStrongLTR=0;
    }
    if(isDefaultLevel) {
        DirProp lastStrong;
        paraDirDefault=pBiDi->paraLevel&1 ? CONTEXT_RTL : 0;
        if(pBiDi->proLength>0 &&
           (lastStrong=firstL_R_AL(pBiDi))!=ON) {
            paraDir=(lastStrong==L) ? 0 : CONTEXT_RTL;
            state=FOUND_STRONG_CHAR;
        } else {
            paraDir=paraDirDefault;
            state=LOOKING_FOR_STRONG;
        }
        lastStrongDir=paraDir;
    } else {
        state=NOT_CONTEXTUAL;
        paraDir=0;
    }
    
    




    for(  ; i<length; ) {
        
        U16_NEXT(text, i, length, uchar);
        flags|=DIRPROP_FLAG(dirProp=(DirProp)ubidi_getCustomizedClass(pBiDi, uchar));
        dirProps[i-1]=dirProp|paraDir;
        if(uchar>0xffff) {  
            flags|=DIRPROP_FLAG(BN);
            dirProps[i-2]=(DirProp)(BN|paraDir);
        }
        if(state==LOOKING_FOR_STRONG) {
            if(dirProp==L) {
                state=FOUND_STRONG_CHAR;
                if(paraDir) {
                    paraDir=0;
                    for(i1=paraStart; i1<i; i1++) {
                        dirProps[i1]&=~CONTEXT_RTL;
                    }
                }
                continue;
            }
            if(dirProp==R || dirProp==AL) {
                state=FOUND_STRONG_CHAR;
                if(paraDir==0) {
                    paraDir=CONTEXT_RTL;
                    for(i1=paraStart; i1<i; i1++) {
                        dirProps[i1]|=CONTEXT_RTL;
                    }
                }
                continue;
            }
        }
        if(dirProp==L) {
            lastStrongDir=0;
            lastStrongLTR=i;            
        }
        else if(dirProp==R) {
            lastStrongDir=CONTEXT_RTL;
        }
        else if(dirProp==AL) {
            lastStrongDir=CONTEXT_RTL;
            lastArabicPos=i-1;
        }
        else if(dirProp==B) {
            if(pBiDi->reorderingOptions & UBIDI_OPTION_STREAMING) {
                pBiDi->length=i;        
            }
            if(isDefaultLevelInverse && (lastStrongDir==CONTEXT_RTL) &&(paraDir!=lastStrongDir)) {
                for( ; paraStart<i; paraStart++) {
                    dirProps[paraStart]|=CONTEXT_RTL;
                }
            }
            if(i<length) {              
                if(!((uchar==CR) && (text[i]==LF))) {
                    pBiDi->paraCount++;
                }
                if(isDefaultLevel) {
                    state=LOOKING_FOR_STRONG;
                    paraStart=i;        
                    paraDir=paraDirDefault;
                    lastStrongDir=paraDirDefault;
                }
            }
        }
        if(removeBiDiControls && IS_BIDI_CONTROL_CHAR(uchar)) {
            controlCount++;
        }
    }
    if(isDefaultLevelInverse && (lastStrongDir==CONTEXT_RTL) &&(paraDir!=lastStrongDir)) {
        for(i1=paraStart; i1<length; i1++) {
            dirProps[i1]|=CONTEXT_RTL;
        }
    }
    if(isDefaultLevel) {
        pBiDi->paraLevel=GET_PARALEVEL(pBiDi, 0);
    }
    if(pBiDi->reorderingOptions & UBIDI_OPTION_STREAMING) {
        if((lastStrongLTR>pBiDi->length) &&
           (GET_PARALEVEL(pBiDi, lastStrongLTR)==0)) {
            pBiDi->length = lastStrongLTR;
        }
        if(pBiDi->length<pBiDi->originalLength) {
            pBiDi->paraCount--;
        }
    }
    

    flags|=DIRPROP_FLAG_LR(pBiDi->paraLevel);

    if(pBiDi->orderParagraphsLTR && (flags&DIRPROP_FLAG(B))) {
        flags|=DIRPROP_FLAG(L);
    }

    pBiDi->controlCount = controlCount;
    pBiDi->flags=flags;
    pBiDi->lastArabicPos=lastArabicPos;
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
resolveExplicitLevels(UBiDi *pBiDi) {
    const DirProp *dirProps=pBiDi->dirProps;
    UBiDiLevel *levels=pBiDi->levels;
    const UChar *text=pBiDi->text;

    int32_t i=0, length=pBiDi->length;
    Flags flags=pBiDi->flags;       
    DirProp dirProp;
    UBiDiLevel level=GET_PARALEVEL(pBiDi, 0);

    UBiDiDirection direction;
    int32_t paraIndex=0;

    
    direction=directionFromFlags(pBiDi);

    

    if((direction!=UBIDI_MIXED) && (pBiDi->paraCount==1)) {
        
    } else if((pBiDi->paraCount==1) &&
              (!(flags&MASK_EXPLICIT) ||
               (pBiDi->reorderingMode > UBIDI_REORDER_LAST_LOGICAL_TO_VISUAL))) {
        
        
        
        
        for(i=0; i<length; ++i) {
            levels[i]=level;
        }
    } else {
        

        
        
        UBiDiLevel embeddingLevel=level, newLevel, stackTop=0;

        UBiDiLevel stack[UBIDI_MAX_EXPLICIT_LEVEL];        
        uint32_t countOver60=0, countOver61=0;  

        
        flags=0;

        for(i=0; i<length; ++i) {
            dirProp=NO_CONTEXT_RTL(dirProps[i]);
            switch(dirProp) {
            case LRE:
            case LRO:
                
                newLevel=(UBiDiLevel)((embeddingLevel+2)&~(UBIDI_LEVEL_OVERRIDE|1)); 
                if(newLevel<=UBIDI_MAX_EXPLICIT_LEVEL) {
                    stack[stackTop]=embeddingLevel;
                    ++stackTop;
                    embeddingLevel=newLevel;
                    if(dirProp==LRO) {
                        embeddingLevel|=UBIDI_LEVEL_OVERRIDE;
                    }
                    



                } else if((embeddingLevel&~UBIDI_LEVEL_OVERRIDE)==UBIDI_MAX_EXPLICIT_LEVEL) {
                    ++countOver61;
                } else  {
                    ++countOver60;
                }
                flags|=DIRPROP_FLAG(BN);
                break;
            case RLE:
            case RLO:
                
                newLevel=(UBiDiLevel)(((embeddingLevel&~UBIDI_LEVEL_OVERRIDE)+1)|1); 
                if(newLevel<=UBIDI_MAX_EXPLICIT_LEVEL) {
                    stack[stackTop]=embeddingLevel;
                    ++stackTop;
                    embeddingLevel=newLevel;
                    if(dirProp==RLO) {
                        embeddingLevel|=UBIDI_LEVEL_OVERRIDE;
                    }
                    



                } else {
                    ++countOver61;
                }
                flags|=DIRPROP_FLAG(BN);
                break;
            case PDF:
                
                
                if(countOver61>0) {
                    --countOver61;
                } else if(countOver60>0 && (embeddingLevel&~UBIDI_LEVEL_OVERRIDE)!=UBIDI_MAX_EXPLICIT_LEVEL) {
                    
                    --countOver60;
                } else if(stackTop>0) {
                    
                    --stackTop;
                    embeddingLevel=stack[stackTop];
                
                }
                flags|=DIRPROP_FLAG(BN);
                break;
            case B:
                stackTop=0;
                countOver60=countOver61=0;
                level=GET_PARALEVEL(pBiDi, i);
                if((i+1)<length) {
                    embeddingLevel=GET_PARALEVEL(pBiDi, i+1);
                    if(!((text[i]==CR) && (text[i+1]==LF))) {
                        pBiDi->paras[paraIndex++]=i+1;
                    }
                }
                flags|=DIRPROP_FLAG(B);
                break;
            case BN:
                
                
                flags|=DIRPROP_FLAG(BN);
                break;
            default:
                
                if(level!=embeddingLevel) {
                    level=embeddingLevel;
                    if(level&UBIDI_LEVEL_OVERRIDE) {
                        flags|=DIRPROP_FLAG_O(level)|DIRPROP_FLAG_MULTI_RUNS;
                    } else {
                        flags|=DIRPROP_FLAG_E(level)|DIRPROP_FLAG_MULTI_RUNS;
                    }
                }
                if(!(level&UBIDI_LEVEL_OVERRIDE)) {
                    flags|=DIRPROP_FLAG(dirProp);
                }
                break;
            }

            



            levels[i]=level;
        }
        if(flags&MASK_EMBEDDING) {
            flags|=DIRPROP_FLAG_LR(pBiDi->paraLevel);
        }
        if(pBiDi->orderParagraphsLTR && (flags&DIRPROP_FLAG(B))) {
            flags|=DIRPROP_FLAG(L);
        }

        

        
        pBiDi->flags=flags;
        direction=directionFromFlags(pBiDi);
    }

    return direction;
}











static UBiDiDirection
checkExplicitLevels(UBiDi *pBiDi, UErrorCode *pErrorCode) {
    const DirProp *dirProps=pBiDi->dirProps;
    DirProp dirProp;
    UBiDiLevel *levels=pBiDi->levels;
    const UChar *text=pBiDi->text;

    int32_t i, length=pBiDi->length;
    Flags flags=0;  
    UBiDiLevel level;
    uint32_t paraIndex=0;

    for(i=0; i<length; ++i) {
        level=levels[i];
        dirProp=NO_CONTEXT_RTL(dirProps[i]);
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
        if((dirProp==B) && ((i+1)<length)) {
            if(!((text[i]==CR) && (text[i+1]==LF))) {
                pBiDi->paras[paraIndex++]=i+1;
            }
        }
    }
    if(flags&MASK_EMBEDDING) {
        flags|=DIRPROP_FLAG_LR(pBiDi->paraLevel);
    }

    
    pBiDi->flags=flags;
    return directionFromFlags(pBiDi);
}


















#define IMPTABPROPS_COLUMNS 14
#define IMPTABPROPS_RES (IMPTABPROPS_COLUMNS - 1)
#define GET_STATEPROPS(cell) ((cell)&0x1f)
#define GET_ACTIONPROPS(cell) ((cell)>>5)
#define s(action, newState) ((uint8_t)(newState+(action<<5)))

static const uint8_t groupProp[] =          
{

    0,  1,  2,  7,  8,  3,  9,  6,  5,  4,  4,  10, 10, 12, 10, 10, 10, 11, 10
};
enum { DirProp_L=0, DirProp_R=1, DirProp_EN=2, DirProp_AN=3, DirProp_ON=4, DirProp_S=5, DirProp_B=6 }; 




































static const uint8_t impTabProps[][IMPTABPROPS_COLUMNS] =
{

 {     1 ,     2 ,     4 ,     5 ,     7 ,    15 ,    17 ,     7 ,     9 ,     7 ,     0 ,     7 ,     3 ,  DirProp_ON },
 {     1 , s(1,2), s(1,4), s(1,5), s(1,7),s(1,15),s(1,17), s(1,7), s(1,9), s(1,7),     1 ,     1 , s(1,3),   DirProp_L },
 { s(1,1),     2 , s(1,4), s(1,5), s(1,7),s(1,15),s(1,17), s(1,7), s(1,9), s(1,7),     2 ,     2 , s(1,3),   DirProp_R },
 { s(1,1), s(1,2), s(1,6), s(1,6), s(1,8),s(1,16),s(1,17), s(1,8), s(1,8), s(1,8),     3 ,     3 ,     3 ,   DirProp_R },
 { s(1,1), s(1,2),     4 , s(1,5), s(1,7),s(1,15),s(1,17),s(2,10),    11 ,s(2,10),     4 ,     4 , s(1,3),  DirProp_EN },
 { s(1,1), s(1,2), s(1,4),     5 , s(1,7),s(1,15),s(1,17), s(1,7), s(1,9),s(2,12),     5 ,     5 , s(1,3),  DirProp_AN },
 { s(1,1), s(1,2),     6 ,     6 , s(1,8),s(1,16),s(1,17), s(1,8), s(1,8),s(2,13),     6 ,     6 , s(1,3),  DirProp_AN },
 { s(1,1), s(1,2), s(1,4), s(1,5),     7 ,s(1,15),s(1,17),     7 ,s(2,14),     7 ,     7 ,     7 , s(1,3),  DirProp_ON },
 { s(1,1), s(1,2), s(1,6), s(1,6),     8 ,s(1,16),s(1,17),     8 ,     8 ,     8 ,     8 ,     8 , s(1,3),  DirProp_ON },
 { s(1,1), s(1,2),     4 , s(1,5),     7 ,s(1,15),s(1,17),     7 ,     9 ,     7 ,     9 ,     9 , s(1,3),  DirProp_ON },
 { s(3,1), s(3,2),     4 , s(3,5), s(4,7),s(3,15),s(3,17), s(4,7),s(4,14), s(4,7),    10 , s(4,7), s(3,3),  DirProp_EN },
 { s(1,1), s(1,2),     4 , s(1,5), s(1,7),s(1,15),s(1,17), s(1,7),    11 , s(1,7),    11 ,    11 , s(1,3),  DirProp_EN },
 { s(3,1), s(3,2), s(3,4),     5 , s(4,7),s(3,15),s(3,17), s(4,7),s(4,14), s(4,7),    12 , s(4,7), s(3,3),  DirProp_AN },
 { s(3,1), s(3,2),     6 ,     6 , s(4,8),s(3,16),s(3,17), s(4,8), s(4,8), s(4,8),    13 , s(4,8), s(3,3),  DirProp_AN },
 { s(1,1), s(1,2), s(4,4), s(1,5),     7 ,s(1,15),s(1,17),     7 ,    14 ,     7 ,    14 ,    14 , s(1,3),  DirProp_ON },
 { s(1,1), s(1,2), s(1,4), s(1,5), s(1,7),    15 ,s(1,17), s(1,7), s(1,9), s(1,7),    15 , s(1,7), s(1,3),   DirProp_S },
 { s(1,1), s(1,2), s(1,6), s(1,6), s(1,8),    16 ,s(1,17), s(1,8), s(1,8), s(1,8),    16 , s(1,8), s(1,3),   DirProp_S },
 { s(1,1), s(1,2), s(1,4), s(1,5), s(1,7),s(1,15),    17 , s(1,7), s(1,9), s(1,7),    17 , s(1,7), s(1,3),   DirProp_B }
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
 { s(2,0),     1 ,     3 ,     3 ,     4 ,     4 , s(2,0),  1 },
 { s(2,0),     1 , s(2,0),     2 ,     5 ,     5 , s(2,0),  1 }
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
static const ImpAct impAct0 = {0,1,2,3,4,5,6};
static const ImpTabPair impTab_DEFAULT = {{&impTabL_DEFAULT,
                                           &impTabR_DEFAULT},
                                          {&impAct0, &impAct0}};

static const ImpTab impTabL_NUMBERS_SPECIAL =   



{

 {     0 ,     2 ,    1 ,      1 ,     0 ,     0 ,     0 ,  0 },
 {     0 ,     2 ,    1 ,      1 ,     0 ,     0 ,     0 ,  2 },
 {     0 ,     2 ,    4 ,      4 , s(1,3),     0 ,     0 ,  1 },
 { s(2,0),     2 ,    4 ,      4 ,     3 ,     3 , s(2,0),  1 },
 {     0 ,     2 ,    4 ,      4 , s(1,3), s(1,3),     0 ,  2 }
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
static const ImpAct impAct1 = {0,1,11,12};


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
static const ImpAct impAct2 = {0,1,7,8,9,10};
static const ImpTabPair impTab_INVERSE_LIKE_DIRECT_WITH_MARKS = {
                        {&impTabL_INVERSE_LIKE_DIRECT_WITH_MARKS,
                         &impTabR_INVERSE_LIKE_DIRECT_WITH_MARKS},
                        {&impAct0, &impAct2}};

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
                        {&impAct0, &impAct2}};

#undef s

typedef struct {
    const ImpTab * pImpTab;             
    const ImpAct * pImpAct;             
    int32_t startON;                    
    int32_t startL2EN;                  
    int32_t lastStrongRTL;              
    int32_t state;                      
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

        case 4:                         
            
            pInsertPoints=&(pBiDi->insertPoints);
            if (pInsertPoints->capacity > 0)
                
                pInsertPoints->size=pInsertPoints->confirmed;
            pLevState->startON=-1;
            pLevState->startL2EN=-1;
            pLevState->lastStrongRTL=limit - 1;
            break;

        case 5:                         
            
            if ((_prop == DirProp_AN) && (NO_CONTEXT_RTL(pBiDi->dirProps[start0]) == AN) &&
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

        case 6:                         
            pLevState->lastStrongRTL=limit - 1;
            pLevState->startON=-1;
            break;

        case 7:                         
            
            for (k=start0-1; k>=0 && !(levels[k]&1); k--);
            if(k>=0) {
                addPoint(pBiDi, k, RLM_BEFORE);             
                pInsertPoints=&(pBiDi->insertPoints);
                pInsertPoints->confirmed=pInsertPoints->size;   
            }
            pLevState->startON=start0;
            break;

        case 8:                         
            
            
            addPoint(pBiDi, start0, LRM_BEFORE);    
            addPoint(pBiDi, start0, LRM_AFTER);     
            break;

        case 9:                         
            
            pInsertPoints=&(pBiDi->insertPoints);
            pInsertPoints->size=pInsertPoints->confirmed;
            if (_prop == DirProp_S)            
            {
                addPoint(pBiDi, start0, RLM_BEFORE);
                pInsertPoints->confirmed=pInsertPoints->size;
            }
            break;

        case 10:                        
            level=pLevState->runLevel + addLevel;
            for(k=pLevState->startON; k<start0; k++) {
                if (levels[k]<level)
                    levels[k]=level;
            }
            pInsertPoints=&(pBiDi->insertPoints);
            pInsertPoints->confirmed=pInsertPoints->size;   
            pLevState->startON=start0;
            break;

        case 11:                        
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

        case 12:                        
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
        for(k=start; k<limit; k++) {
            levels[k]=level;
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

    LevState levState;
    int32_t i, start1, start2;
    uint8_t oldStateImp, stateImp, actionImp;
    uint8_t gprop, resProp, cell;
    UBool inverseRTL;
    DirProp nextStrongProp=R;
    int32_t nextStrongPos=-1;

    levState.startON = -1;  

    
    





    inverseRTL=(UBool)
        ((start<pBiDi->lastArabicPos) && (GET_PARALEVEL(pBiDi, start) & 1) &&
         (pBiDi->reorderingMode==UBIDI_REORDER_INVERSE_LIKE_DIRECT  ||
          pBiDi->reorderingMode==UBIDI_REORDER_INVERSE_FOR_NUMBERS_SPECIAL));
    
    levState.startL2EN=-1;              
    levState.lastStrongRTL=-1;          
    levState.state=0;
    levState.runLevel=pBiDi->levels[start];
    levState.pImpTab=(const ImpTab*)((pBiDi->pImpTabPair)->pImpTab)[levState.runLevel&1];
    levState.pImpAct=(const ImpAct*)((pBiDi->pImpTabPair)->pImpAct)[levState.runLevel&1];
    if(start==0 && pBiDi->proLength>0) {
        DirProp lastStrong=lastL_R_AL(pBiDi);
        if(lastStrong!=DirProp_ON) {
            sor=lastStrong;
        }
    }
    processPropertySeq(pBiDi, &levState, sor, start, start);
    
    if(NO_CONTEXT_RTL(dirProps[start])==NSM) {
        stateImp = 1 + sor;
    } else {
        stateImp=0;
    }
    start1=start;
    start2=start;

    for(i=start; i<=limit; i++) {
        if(i>=limit) {
            gprop=eor;
        } else {
            DirProp prop, prop1;
            prop=NO_CONTEXT_RTL(dirProps[i]);
            if(inverseRTL) {
                if(prop==AL) {
                    
                    prop=R;
                } else if(prop==EN) {
                    if(nextStrongPos<=i) {
                        
                        int32_t j;
                        nextStrongProp=R;   
                        nextStrongPos=limit;
                        for(j=i+1; j<limit; j++) {
                            prop1=NO_CONTEXT_RTL(dirProps[j]);
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
            
            while(i>0 && (flag=DIRPROP_FLAG_NC(dirProps[--i]))&MASK_WS) {
                if(orderParagraphsLTR&&(flag&DIRPROP_FLAG(B))) {
                    levels[i]=0;
                } else {
                    levels[i]=GET_PARALEVEL(pBiDi, i);
                }
            }

            
            
            while(i>0) {
                flag=DIRPROP_FLAG_NC(dirProps[--i]);
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
    void *runsOnlyMemory;
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
    
    uprv_free(runsOnlyMemory);
    if(pBiDi->runCount>1) {
        pBiDi->direction=UBIDI_MIXED;
    }
  cleanup3:
    pBiDi->reorderingMode=UBIDI_REORDER_RUNS_ONLY;
}



U_CAPI void U_EXPORT2
ubidi_setPara(UBiDi *pBiDi, const UChar *text, int32_t length,
              UBiDiLevel paraLevel, UBiDiLevel *embeddingLevels,
              UErrorCode *pErrorCode) {
    UBiDiDirection direction;

    
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
    pBiDi->direction=UBIDI_LTR;
    pBiDi->paraCount=1;

    pBiDi->dirProps=NULL;
    pBiDi->levels=NULL;
    pBiDi->runs=NULL;
    pBiDi->insertPoints.size=0;         
    pBiDi->insertPoints.confirmed=0;    

    


    if(IS_DEFAULT_LEVEL(paraLevel)) {
        pBiDi->defaultParaLevel=paraLevel;
    } else {
        pBiDi->defaultParaLevel=0;
    }

    if(length==0) {
        




        if(IS_DEFAULT_LEVEL(paraLevel)) {
            pBiDi->paraLevel&=1;
            pBiDi->defaultParaLevel=0;
        }
        if(paraLevel&1) {
            pBiDi->flags=DIRPROP_FLAG(R);
            pBiDi->direction=UBIDI_RTL;
        } else {
            pBiDi->flags=DIRPROP_FLAG(L);
            pBiDi->direction=UBIDI_LTR;
        }

        pBiDi->runCount=0;
        pBiDi->paraCount=0;
        setParaSuccess(pBiDi);          
        return;
    }

    pBiDi->runCount=-1;

    




    if(getDirPropsMemory(pBiDi, length)) {
        pBiDi->dirProps=pBiDi->dirPropsMemory;
        getDirProps(pBiDi);
    } else {
        *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    
    length= pBiDi->length;
    pBiDi->trailingWSStart=length;  
    
    if(pBiDi->paraCount>1) {
        if(getInitialParasMemory(pBiDi, pBiDi->paraCount)) {
            pBiDi->paras=pBiDi->parasMemory;
            pBiDi->paras[pBiDi->paraCount-1]=length;
        } else {
            *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
            return;
        }
    } else {
        
        pBiDi->paras=pBiDi->simpleParas;
        pBiDi->simpleParas[0]=length;
    }

    
    if(embeddingLevels==NULL) {
        \
        if(getLevelsMemory(pBiDi, length)) {
            pBiDi->levels=pBiDi->levelsMemory;
            direction=resolveExplicitLevels(pBiDi);
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

    



    pBiDi->direction=direction;
    switch(direction) {
    case UBIDI_LTR:
        
        pBiDi->paraLevel=(UBiDiLevel)((pBiDi->paraLevel+1)&~1);

        
        pBiDi->trailingWSStart=0;
        break;
    case UBIDI_RTL:
        
        pBiDi->paraLevel|=1;

        
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
                if((start>0) && (NO_CONTEXT_RTL(pBiDi->dirProps[start-1])==B)) {
                    
                    sor=GET_LR_FROM_LEVEL(GET_PARALEVEL(pBiDi, start));
                } else {
                    sor=eor;
                }

                
                while(++limit<length && levels[limit]==level) {}

                
                if(limit<length) {
                    nextLevel=levels[limit];
                } else {
                    nextLevel=GET_PARALEVEL(pBiDi, length-1);
                }

                
                if((level&~UBIDI_LEVEL_OVERRIDE)<(nextLevel&~UBIDI_LEVEL_OVERRIDE)) {
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
        DirProp dirProp;
        for(i=0; i<pBiDi->paraCount; i++) {
            last=pBiDi->paras[i]-1;
            if((pBiDi->dirProps[last] & CONTEXT_RTL)==0) {
                continue;           
            }
            start= i==0 ? 0 : pBiDi->paras[i - 1];
            for(j=last; j>=start; j--) {
                dirProp=NO_CONTEXT_RTL(pBiDi->dirProps[j]);
                if(dirProp==L) {
                    if(j<last) {
                        while(NO_CONTEXT_RTL(pBiDi->dirProps[last])==B) {
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
        paraStart=pBiDi->paras[paraIndex-1];
    } else {
        paraStart=0;
    }
    if(pParaStart!=NULL) {
        *pParaStart=paraStart;
    }
    if(pParaLimit!=NULL) {
        *pParaLimit=pBiDi->paras[paraIndex];
    }
    if(pParaLevel!=NULL) {
        *pParaLevel=GET_PARALEVEL(pBiDi, paraStart);
    }
}

U_CAPI int32_t U_EXPORT2
ubidi_getParagraph(const UBiDi *pBiDi, int32_t charIndex,
                          int32_t *pParaStart, int32_t *pParaLimit,
                          UBiDiLevel *pParaLevel, UErrorCode *pErrorCode) {
    uint32_t paraIndex;

    
    
    RETURN_IF_NULL_OR_FAILING_ERRCODE(pErrorCode, -1);
    RETURN_IF_NOT_VALID_PARA_OR_LINE(pBiDi, *pErrorCode, -1);
    pBiDi=pBiDi->pParaBiDi;             
    RETURN_IF_BAD_RANGE(charIndex, 0, pBiDi->length, *pErrorCode, -1);

    for(paraIndex=0; charIndex>=pBiDi->paras[paraIndex]; paraIndex++);
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
        return ubidi_getClass(pBiDi->bdp, c);
    } else {
        return dir;
    }
}

