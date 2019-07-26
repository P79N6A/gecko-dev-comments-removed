



















#ifndef U_COMMON_IMPLEMENTATION
#   define U_COMMON_IMPLEMENTATION
#endif

#include "unicode/utypes.h"
#include "unicode/ustring.h"
#include "unicode/uchar.h"
#include "unicode/ubidi.h"
#include "unicode/utf16.h"
#include "cmemory.h"
#include "ustr_imp.h"
#include "ubidiimp.h"













#if UTF_SIZE==8
# error reimplement ubidi_writeReordered() for UTF-8, see comment above
#endif

#define IS_COMBINING(type) ((1UL<<(type))&(1UL<<U_NON_SPACING_MARK|1UL<<U_COMBINING_SPACING_MARK|1UL<<U_ENCLOSING_MARK))










static int32_t
doWriteForward(const UChar *src, int32_t srcLength,
               UChar *dest, int32_t destSize,
               uint16_t options,
               UErrorCode *pErrorCode) {
    
    switch(options&(UBIDI_REMOVE_BIDI_CONTROLS|UBIDI_DO_MIRRORING)) {
    case 0: {
        
        int32_t length=srcLength;
        if(destSize<length) {
            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
            return srcLength;
        }
        do {
            *dest++=*src++;
        } while(--length>0);
        return srcLength;
    }
    case UBIDI_DO_MIRRORING: {
        
        int32_t i=0, j=0;
        UChar32 c;

        if(destSize<srcLength) {
            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
            return srcLength;
        }
        do {
            U16_NEXT(src, i, srcLength, c);
            c=u_charMirror(c);
            U16_APPEND_UNSAFE(dest, j, c);
        } while(i<srcLength);
        return srcLength;
    }
    case UBIDI_REMOVE_BIDI_CONTROLS: {
        
        int32_t remaining=destSize;
        UChar c;
        do {
            c=*src++;
            if(!IS_BIDI_CONTROL_CHAR(c)) {
                if(--remaining<0) {
                    *pErrorCode=U_BUFFER_OVERFLOW_ERROR;

                    
                    while(--srcLength>0) {
                        c=*src++;
                        if(!IS_BIDI_CONTROL_CHAR(c)) {
                            --remaining;
                        }
                    }
                    return destSize-remaining;
                }
                *dest++=c;
            }
        } while(--srcLength>0);
        return destSize-remaining;
    }
    default: {
        
        int32_t remaining=destSize;
        int32_t i, j=0;
        UChar32 c;
        do {
            i=0;
            U16_NEXT(src, i, srcLength, c);
            src+=i;
            srcLength-=i;
            if(!IS_BIDI_CONTROL_CHAR(c)) {
                remaining-=i;
                if(remaining<0) {
                    *pErrorCode=U_BUFFER_OVERFLOW_ERROR;

                    
                    while(srcLength>0) {
                        c=*src++;
                        if(!IS_BIDI_CONTROL_CHAR(c)) {
                            --remaining;
                        }
                        --srcLength;
                    }
                    return destSize-remaining;
                }
                c=u_charMirror(c);
                U16_APPEND_UNSAFE(dest, j, c);
            }
        } while(srcLength>0);
        return j;
    }
    } 
}

static int32_t
doWriteReverse(const UChar *src, int32_t srcLength,
               UChar *dest, int32_t destSize,
               uint16_t options,
               UErrorCode *pErrorCode) {
    

















    int32_t i, j;
    UChar32 c;

    
    switch(options&(UBIDI_REMOVE_BIDI_CONTROLS|UBIDI_DO_MIRRORING|UBIDI_KEEP_BASE_COMBINING)) {
    case 0:
        





        if(destSize<srcLength) {
            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
            return srcLength;
        }
        destSize=srcLength;

        
        do {
            
            i=srcLength;

            
            U16_BACK_1(src, 0, srcLength);

            
            j=srcLength;
            do {
                *dest++=src[j++];
            } while(j<i);
        } while(srcLength>0);
        break;
    case UBIDI_KEEP_BASE_COMBINING:
        





        if(destSize<srcLength) {
            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
            return srcLength;
        }
        destSize=srcLength;

        
        do {
            
            i=srcLength;

            
            do {
                U16_PREV(src, 0, srcLength, c);
            } while(srcLength>0 && IS_COMBINING(u_charType(c)));

            
            j=srcLength;
            do {
                *dest++=src[j++];
            } while(j<i);
        } while(srcLength>0);
        break;
    default:
        






        if(!(options&UBIDI_REMOVE_BIDI_CONTROLS)) {
            i=srcLength;
        } else {
            

            int32_t length=srcLength;
            UChar ch;

            i=0;
            do {
                ch=*src++;
                if(!IS_BIDI_CONTROL_CHAR(ch)) {
                    ++i;
                }
            } while(--length>0);
            src-=srcLength;
        }

        if(destSize<i) {
            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
            return i;
        }
        destSize=i;

        
        do {
            
            i=srcLength;

            
            U16_PREV(src, 0, srcLength, c);
            if(options&UBIDI_KEEP_BASE_COMBINING) {
                
                while(srcLength>0 && IS_COMBINING(u_charType(c))) {
                    U16_PREV(src, 0, srcLength, c);
                }
            }

            if(options&UBIDI_REMOVE_BIDI_CONTROLS && IS_BIDI_CONTROL_CHAR(c)) {
                
                continue;
            }

            
            j=srcLength;
            if(options&UBIDI_DO_MIRRORING) {
                
                int32_t k=0;
                c=u_charMirror(c);
                U16_APPEND_UNSAFE(dest, k, c);
                dest+=k;
                j+=k;
            }
            while(j<i) {
                *dest++=src[j++];
            }
        } while(srcLength>0);
        break;
    } 

    return destSize;
}

U_CAPI int32_t U_EXPORT2
ubidi_writeReverse(const UChar *src, int32_t srcLength,
                   UChar *dest, int32_t destSize,
                   uint16_t options,
                   UErrorCode *pErrorCode) {
    int32_t destLength;

    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    
    if( src==NULL || srcLength<-1 ||
        destSize<0 || (destSize>0 && dest==NULL))
    {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    
    if( dest!=NULL &&
        ((src>=dest && src<dest+destSize) ||
         (dest>=src && dest<src+srcLength)))
    {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    if(srcLength==-1) {
        srcLength=u_strlen(src);
    }
    if(srcLength>0) {
        destLength=doWriteReverse(src, srcLength, dest, destSize, options, pErrorCode);
    } else {
        
        destLength=0;
    }

    return u_terminateUChars(dest, destSize, destLength, pErrorCode);
}

U_CAPI int32_t U_EXPORT2
ubidi_writeReordered(UBiDi *pBiDi,
                     UChar *dest, int32_t destSize,
                     uint16_t options,
                     UErrorCode *pErrorCode) {
    const UChar *text;
    UChar *saveDest;
    int32_t length, destCapacity;
    int32_t run, runCount, logicalStart, runLength;

    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    
    if( pBiDi==NULL ||
        (text=pBiDi->text)==NULL || (length=pBiDi->length)<0 ||
        destSize<0 || (destSize>0 && dest==NULL))
    {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    
    if( dest!=NULL &&
        ((text>=dest && text<dest+destSize) ||
         (dest>=text && dest<text+pBiDi->originalLength)))
    {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    if(length==0) {
        
        return u_terminateUChars(dest, destSize, 0, pErrorCode);
    }

    runCount=ubidi_countRuns(pBiDi, pErrorCode);
    if(U_FAILURE(*pErrorCode)) {
        return 0;
    }

    
    saveDest=dest;
    destCapacity=destSize;

    



    if(pBiDi->reorderingOptions & UBIDI_OPTION_INSERT_MARKS) {
        options|=UBIDI_INSERT_LRM_FOR_NUMERIC;
        options&=~UBIDI_REMOVE_BIDI_CONTROLS;
    }
    



    if(pBiDi->reorderingOptions & UBIDI_OPTION_REMOVE_CONTROLS) {
        options|=UBIDI_REMOVE_BIDI_CONTROLS;
        options&=~UBIDI_INSERT_LRM_FOR_NUMERIC;
    }
    



    if((pBiDi->reorderingMode != UBIDI_REORDER_INVERSE_NUMBERS_AS_L) &&
       (pBiDi->reorderingMode != UBIDI_REORDER_INVERSE_LIKE_DIRECT)  &&
       (pBiDi->reorderingMode != UBIDI_REORDER_INVERSE_FOR_NUMBERS_SPECIAL) &&
       (pBiDi->reorderingMode != UBIDI_REORDER_RUNS_ONLY)) {
        options&=~UBIDI_INSERT_LRM_FOR_NUMERIC;
    }
    











    if(!(options&UBIDI_OUTPUT_REVERSE)) {
        
        if(!(options&UBIDI_INSERT_LRM_FOR_NUMERIC)) {
            
            for(run=0; run<runCount; ++run) {
                if(UBIDI_LTR==ubidi_getVisualRun(pBiDi, run, &logicalStart, &runLength)) {
                    runLength=doWriteForward(text+logicalStart, runLength,
                                             dest, destSize,
                                             (uint16_t)(options&~UBIDI_DO_MIRRORING), pErrorCode);
                } else {
                    runLength=doWriteReverse(text+logicalStart, runLength,
                                             dest, destSize,
                                             options, pErrorCode);
                }
                if(dest!=NULL) {
                  dest+=runLength;
                }
                destSize-=runLength;
            }
        } else {
            
            const DirProp *dirProps=pBiDi->dirProps;
            const UChar *src;
            UChar uc;
            UBiDiDirection dir;
            int32_t markFlag;

            for(run=0; run<runCount; ++run) {
                dir=ubidi_getVisualRun(pBiDi, run, &logicalStart, &runLength);
                src=text+logicalStart;
                
                markFlag=pBiDi->runs[run].insertRemove;
                if(markFlag<0) {        
                    markFlag=0;
                }

                if(UBIDI_LTR==dir) {
                    if((pBiDi->isInverse) &&
                       ( dirProps[logicalStart]!=L)) {
                        markFlag |= LRM_BEFORE;
                    }
                    if (markFlag & LRM_BEFORE) {
                        uc=LRM_CHAR;
                    }
                    else if (markFlag & RLM_BEFORE) {
                        uc=RLM_CHAR;
                    }
                    else  uc=0;
                    if(uc) {
                        if(destSize>0) {
                            *dest++=uc;
                        }
                        --destSize;
                    }

                    runLength=doWriteForward(src, runLength,
                                             dest, destSize,
                                             (uint16_t)(options&~UBIDI_DO_MIRRORING), pErrorCode);
                    if(dest!=NULL) {
                      dest+=runLength;
                    }
                    destSize-=runLength;

                    if((pBiDi->isInverse) &&
                       ( dirProps[logicalStart+runLength-1]!=L)) {
                        markFlag |= LRM_AFTER;
                    }
                    if (markFlag & LRM_AFTER) {
                        uc=LRM_CHAR;
                    }
                    else if (markFlag & RLM_AFTER) {
                        uc=RLM_CHAR;
                    }
                    else  uc=0;
                    if(uc) {
                        if(destSize>0) {
                            *dest++=uc;
                        }
                        --destSize;
                    }
                } else {                
                    if((pBiDi->isInverse) &&
                       ( !(MASK_R_AL&DIRPROP_FLAG(dirProps[logicalStart+runLength-1])))) {
                        markFlag |= RLM_BEFORE;
                    }
                    if (markFlag & LRM_BEFORE) {
                        uc=LRM_CHAR;
                    }
                    else if (markFlag & RLM_BEFORE) {
                        uc=RLM_CHAR;
                    }
                    else  uc=0;
                    if(uc) {
                        if(destSize>0) {
                            *dest++=uc;
                        }
                        --destSize;
                    }

                    runLength=doWriteReverse(src, runLength,
                                             dest, destSize,
                                             options, pErrorCode);
                    if(dest!=NULL) {
                      dest+=runLength;
                    }
                    destSize-=runLength;

                    if((pBiDi->isInverse) &&
                       ( !(MASK_R_AL&DIRPROP_FLAG(dirProps[logicalStart])))) {
                        markFlag |= RLM_AFTER;
                    }
                    if (markFlag & LRM_AFTER) {
                        uc=LRM_CHAR;
                    }
                    else if (markFlag & RLM_AFTER) {
                        uc=RLM_CHAR;
                    }
                    else  uc=0;
                    if(uc) {
                        if(destSize>0) {
                            *dest++=uc;
                        }
                        --destSize;
                    }
                }
            }
        }
    } else {
        
        if(!(options&UBIDI_INSERT_LRM_FOR_NUMERIC)) {
            
            for(run=runCount; --run>=0;) {
                if(UBIDI_LTR==ubidi_getVisualRun(pBiDi, run, &logicalStart, &runLength)) {
                    runLength=doWriteReverse(text+logicalStart, runLength,
                                             dest, destSize,
                                             (uint16_t)(options&~UBIDI_DO_MIRRORING), pErrorCode);
                } else {
                    runLength=doWriteForward(text+logicalStart, runLength,
                                             dest, destSize,
                                             options, pErrorCode);
                }
                if(dest!=NULL) {
                  dest+=runLength;
                }
                destSize-=runLength;
            }
        } else {
            
            const DirProp *dirProps=pBiDi->dirProps;
            const UChar *src;
            UBiDiDirection dir;

            for(run=runCount; --run>=0;) {
                
                dir=ubidi_getVisualRun(pBiDi, run, &logicalStart, &runLength);
                src=text+logicalStart;

                if(UBIDI_LTR==dir) {
                    if( dirProps[logicalStart+runLength-1]!=L) {
                        if(destSize>0) {
                            *dest++=LRM_CHAR;
                        }
                        --destSize;
                    }

                    runLength=doWriteReverse(src, runLength,
                                             dest, destSize,
                                             (uint16_t)(options&~UBIDI_DO_MIRRORING), pErrorCode);
                    if(dest!=NULL) {
                      dest+=runLength;
                    }
                    destSize-=runLength;

                    if( dirProps[logicalStart]!=L) {
                        if(destSize>0) {
                            *dest++=LRM_CHAR;
                        }
                        --destSize;
                    }
                } else {
                    if( !(MASK_R_AL&DIRPROP_FLAG(dirProps[logicalStart]))) {
                        if(destSize>0) {
                            *dest++=RLM_CHAR;
                        }
                        --destSize;
                    }

                    runLength=doWriteForward(src, runLength,
                                             dest, destSize,
                                             options, pErrorCode);
                    if(dest!=NULL) {
                      dest+=runLength;
                    }
                    destSize-=runLength;

                    if( !(MASK_R_AL&DIRPROP_FLAG(dirProps[logicalStart+runLength-1]))) {
                        if(destSize>0) {
                            *dest++=RLM_CHAR;
                        }
                        --destSize;
                    }
                }
            }
        }
    }

    return u_terminateUChars(saveDest, destCapacity, destCapacity-destSize, pErrorCode);
}
