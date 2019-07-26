















#ifndef UBIDIIMP_H
#define UBIDIIMP_H


#ifdef U_COMMON_IMPLEMENTATION

#include "unicode/utypes.h"
#include "unicode/uchar.h"
#include "ubidi_props.h"



typedef uint8_t DirProp;
typedef uint32_t Flags;





enum {
    L=  U_LEFT_TO_RIGHT,
    R=  U_RIGHT_TO_LEFT,
    EN= U_EUROPEAN_NUMBER,
    ES= U_EUROPEAN_NUMBER_SEPARATOR,
    ET= U_EUROPEAN_NUMBER_TERMINATOR,
    AN= U_ARABIC_NUMBER,
    CS= U_COMMON_NUMBER_SEPARATOR,
    B=  U_BLOCK_SEPARATOR,
    S=  U_SEGMENT_SEPARATOR,
    WS= U_WHITE_SPACE_NEUTRAL,
    ON= U_OTHER_NEUTRAL,
    LRE=U_LEFT_TO_RIGHT_EMBEDDING,
    LRO=U_LEFT_TO_RIGHT_OVERRIDE,
    AL= U_RIGHT_TO_LEFT_ARABIC,
    RLE=U_RIGHT_TO_LEFT_EMBEDDING,
    RLO=U_RIGHT_TO_LEFT_OVERRIDE,
    PDF=U_POP_DIRECTIONAL_FORMAT,
    NSM=U_DIR_NON_SPACING_MARK,
    BN= U_BOUNDARY_NEUTRAL,
    dirPropCount
};







#define DIRPROP_FLAG(dir) (1UL<<(dir))


#define DIRPROP_FLAG_MULTI_RUNS (1UL<<31)


#define MASK_LTR (DIRPROP_FLAG(L)|DIRPROP_FLAG(EN)|DIRPROP_FLAG(AN)|DIRPROP_FLAG(LRE)|DIRPROP_FLAG(LRO))
#define MASK_RTL (DIRPROP_FLAG(R)|DIRPROP_FLAG(AL)|DIRPROP_FLAG(RLE)|DIRPROP_FLAG(RLO))
#define MASK_R_AL (DIRPROP_FLAG(R)|DIRPROP_FLAG(AL))


#define MASK_LRX (DIRPROP_FLAG(LRE)|DIRPROP_FLAG(LRO))
#define MASK_RLX (DIRPROP_FLAG(RLE)|DIRPROP_FLAG(RLO))
#define MASK_OVERRIDE (DIRPROP_FLAG(LRO)|DIRPROP_FLAG(RLO))

#define MASK_EXPLICIT (MASK_LRX|MASK_RLX|DIRPROP_FLAG(PDF))
#define MASK_BN_EXPLICIT (DIRPROP_FLAG(BN)|MASK_EXPLICIT)


#define MASK_B_S (DIRPROP_FLAG(B)|DIRPROP_FLAG(S))


#define MASK_WS (MASK_B_S|DIRPROP_FLAG(WS)|MASK_BN_EXPLICIT)
#define MASK_N (DIRPROP_FLAG(ON)|MASK_WS)


#define MASK_ET_NSM_BN (DIRPROP_FLAG(ET)|DIRPROP_FLAG(NSM)|MASK_BN_EXPLICIT)


#define MASK_POSSIBLE_N (DIRPROP_FLAG(CS)|DIRPROP_FLAG(ES)|DIRPROP_FLAG(ET)|MASK_N)






#define MASK_EMBEDDING (DIRPROP_FLAG(NSM)|MASK_POSSIBLE_N)


#define GET_LR_FROM_LEVEL(level) ((DirProp)((level)&1))

#define IS_DEFAULT_LEVEL(level) ((level)>=0xfe)





#define CONTEXT_RTL 0x80
#define NO_CONTEXT_RTL(dir) ((dir)&~CONTEXT_RTL)



#define DIRPROP_FLAG_NC(dir) (1UL<<(NO_CONTEXT_RTL(dir)))

#define GET_PARALEVEL(ubidi, index) \
            (UBiDiLevel)((ubidi)->defaultParaLevel ? (ubidi)->dirProps[index]>>7 \
                                                   : (ubidi)->paraLevel)


typedef int32_t Para;

#define CR  0x000D
#define LF  0x000A


enum {
    LRM_BEFORE=1,
    LRM_AFTER=2,
    RLM_BEFORE=4,
    RLM_AFTER=8
};

typedef struct Run {
    int32_t logicalStart,   
            visualLimit,    
            insertRemove;   

} Run;


#define INDEX_ODD_BIT (1UL<<31)

#define MAKE_INDEX_ODD_PAIR(index, level) ((index)|((int32_t)(level)<<31))
#define ADD_ODD_BIT_FROM_LEVEL(x, level)  ((x)|=((int32_t)(level)<<31))
#define REMOVE_ODD_BIT(x)                 ((x)&=~INDEX_ODD_BIT)

#define GET_INDEX(x)   ((x)&~INDEX_ODD_BIT)
#define GET_ODD_BIT(x) ((uint32_t)(x)>>31)
#define IS_ODD_RUN(x)  ((UBool)(((x)&INDEX_ODD_BIT)!=0))
#define IS_EVEN_RUN(x) ((UBool)(((x)&INDEX_ODD_BIT)==0))

U_CFUNC UBool
ubidi_getRuns(UBiDi *pBiDi, UErrorCode *pErrorCode);


enum {
    ZWNJ_CHAR=0x200c,
    ZWJ_CHAR,
    LRM_CHAR,
    RLM_CHAR,
    LRE_CHAR=0x202a,
    RLE_CHAR,
    PDF_CHAR,
    LRO_CHAR,
    RLO_CHAR
};

#define IS_BIDI_CONTROL_CHAR(c) (((uint32_t)(c)&0xfffffffc)==ZWNJ_CHAR || (uint32_t)((c)-LRE_CHAR)<5)



typedef struct Point {
    int32_t pos;            
    int32_t flag;           
} Point;

typedef struct InsertPoints {
    int32_t capacity;       
    int32_t size;           
    int32_t confirmed;      
    UErrorCode errorCode;   
    Point *points;          
} InsertPoints;




struct UBiDi {
    



    const UBiDi * pParaBiDi;

    const UBiDiProps *bdp;

    
    const UChar *text;

    
    int32_t originalLength;

    




    int32_t length;

    



    int32_t resultLength;

    
    int32_t dirPropsSize, levelsSize, parasSize, runsSize;

    
    DirProp *dirPropsMemory;
    UBiDiLevel *levelsMemory;
    Para *parasMemory;
    Run *runsMemory;

    
    UBool mayAllocateText, mayAllocateRuns;

    
    const DirProp *dirProps;
    UBiDiLevel *levels;

    
    UBool isInverse;

    
    UBiDiReorderingMode reorderingMode;

    



    #define UBIDI_REORDER_LAST_LOGICAL_TO_VISUAL    UBIDI_REORDER_NUMBERS_SPECIAL

    
    uint32_t reorderingOptions;

    
    UBool orderParagraphsLTR;

    
    UBiDiLevel paraLevel;
    
    
    UBiDiLevel defaultParaLevel;

    
    const UChar *prologue;
    int32_t proLength;
    const UChar *epilogue;
    int32_t epiLength;

    
    const struct ImpTabPair * pImpTabPair;  

    
    UBiDiDirection direction;

    
    Flags flags;

    
    int32_t lastArabicPos;

    
    
    int32_t trailingWSStart;

    
    int32_t paraCount;                  
    Para *paras;                        


    
    Para simpleParas[1];

    
    int32_t runCount;     
    Run *runs;

    
    Run simpleRuns[1];

    
    InsertPoints insertPoints;

    
    int32_t controlCount;

    
    UBiDiClassCallback *fnClassCallback;    
    const void *coClassCallback;            
};

#define IS_VALID_PARA(x) ((x) && ((x)->pParaBiDi==(x)))
#define IS_VALID_PARA_OR_LINE(x) ((x) && ((x)->pParaBiDi==(x) || (((x)->pParaBiDi) && (x)->pParaBiDi->pParaBiDi==(x)->pParaBiDi)))

typedef union {
    DirProp *dirPropsMemory;
    UBiDiLevel *levelsMemory;
    Para *parasMemory;
    Run *runsMemory;
} BidiMemoryForAllocation;


#define RETURN_IF_NULL_OR_FAILING_ERRCODE(pErrcode, retvalue)   \
        if((pErrcode)==NULL || U_FAILURE(*pErrcode)) return retvalue
#define RETURN_IF_NOT_VALID_PARA(bidi, errcode, retvalue)   \
        if(!IS_VALID_PARA(bidi)) {  \
            errcode=U_INVALID_STATE_ERROR;  \
            return retvalue;                \
        }
#define RETURN_IF_NOT_VALID_PARA_OR_LINE(bidi, errcode, retvalue)   \
        if(!IS_VALID_PARA_OR_LINE(bidi)) {  \
            errcode=U_INVALID_STATE_ERROR;  \
            return retvalue;                \
        }
#define RETURN_IF_BAD_RANGE(arg, start, limit, errcode, retvalue)   \
        if((arg)<(start) || (arg)>=(limit)) {       \
            (errcode)=U_ILLEGAL_ARGUMENT_ERROR;     \
            return retvalue;                        \
        }

#define RETURN_VOID_IF_NULL_OR_FAILING_ERRCODE(pErrcode)   \
        if((pErrcode)==NULL || U_FAILURE(*pErrcode)) return
#define RETURN_VOID_IF_NOT_VALID_PARA(bidi, errcode)   \
        if(!IS_VALID_PARA(bidi)) {  \
            errcode=U_INVALID_STATE_ERROR;  \
            return;                \
        }
#define RETURN_VOID_IF_NOT_VALID_PARA_OR_LINE(bidi, errcode)   \
        if(!IS_VALID_PARA_OR_LINE(bidi)) {  \
            errcode=U_INVALID_STATE_ERROR;  \
            return;                \
        }
#define RETURN_VOID_IF_BAD_RANGE(arg, start, limit, errcode)   \
        if((arg)<(start) || (arg)>=(limit)) {       \
            (errcode)=U_ILLEGAL_ARGUMENT_ERROR;     \
            return;                        \
        }


U_CFUNC UBool
ubidi_getMemory(BidiMemoryForAllocation *pMemory, int32_t *pSize, UBool mayAllocate, int32_t sizeNeeded);


#define getDirPropsMemory(pBiDi, length) \
        ubidi_getMemory((BidiMemoryForAllocation *)&(pBiDi)->dirPropsMemory, &(pBiDi)->dirPropsSize, \
                        (pBiDi)->mayAllocateText, (length))

#define getLevelsMemory(pBiDi, length) \
        ubidi_getMemory((BidiMemoryForAllocation *)&(pBiDi)->levelsMemory, &(pBiDi)->levelsSize, \
                        (pBiDi)->mayAllocateText, (length))

#define getRunsMemory(pBiDi, length) \
        ubidi_getMemory((BidiMemoryForAllocation *)&(pBiDi)->runsMemory, &(pBiDi)->runsSize, \
                        (pBiDi)->mayAllocateRuns, (length)*sizeof(Run))


#define getInitialDirPropsMemory(pBiDi, length) \
        ubidi_getMemory((BidiMemoryForAllocation *)&(pBiDi)->dirPropsMemory, &(pBiDi)->dirPropsSize, \
                        TRUE, (length))

#define getInitialLevelsMemory(pBiDi, length) \
        ubidi_getMemory((BidiMemoryForAllocation *)&(pBiDi)->levelsMemory, &(pBiDi)->levelsSize, \
                        TRUE, (length))

#define getInitialParasMemory(pBiDi, length) \
        ubidi_getMemory((BidiMemoryForAllocation *)&(pBiDi)->parasMemory, &(pBiDi)->parasSize, \
                        TRUE, (length)*sizeof(Para))

#define getInitialRunsMemory(pBiDi, length) \
        ubidi_getMemory((BidiMemoryForAllocation *)&(pBiDi)->runsMemory, &(pBiDi)->runsSize, \
                        TRUE, (length)*sizeof(Run))

#endif

#endif
