















#include "unicode/utypes.h"
#include "unicode/ustring.h"
#include "unicode/unistr.h"
#include "unicode/chariter.h"
#include "unicode/utext.h"
#include "unicode/utf.h"
#include "unicode/utf8.h"
#include "unicode/utf16.h"
#include "ustr_imp.h"
#include "cmemory.h"
#include "cstring.h"
#include "uassert.h"
#include "putilimp.h"

U_NAMESPACE_USE

#define I32_FLAG(bitIndex) ((int32_t)1<<(bitIndex))


static UBool
utext_access(UText *ut, int64_t index, UBool forward) {
    return ut->pFuncs->access(ut, index, forward);
}



U_CAPI UBool U_EXPORT2
utext_moveIndex32(UText *ut, int32_t delta) {
    UChar32  c;
    if (delta > 0) {
        do {
            if(ut->chunkOffset>=ut->chunkLength && !utext_access(ut, ut->chunkNativeLimit, TRUE)) {
                return FALSE;
            }
            c = ut->chunkContents[ut->chunkOffset];
            if (U16_IS_SURROGATE(c)) {
                c = utext_next32(ut);
                if (c == U_SENTINEL) {
                    return FALSE;
                }
            } else {
                ut->chunkOffset++;
            }
        } while(--delta>0);

    } else if (delta<0) {
        do {
            if(ut->chunkOffset<=0 && !utext_access(ut, ut->chunkNativeStart, FALSE)) {
                return FALSE;
            }
            c = ut->chunkContents[ut->chunkOffset-1];
            if (U16_IS_SURROGATE(c)) {
                c = utext_previous32(ut);
                if (c == U_SENTINEL) {
                    return FALSE;
                }
            } else {
                ut->chunkOffset--;
            }
        } while(++delta<0);
    }

    return TRUE;
}


U_CAPI int64_t U_EXPORT2
utext_nativeLength(UText *ut) {
    return ut->pFuncs->nativeLength(ut);
}


U_CAPI UBool U_EXPORT2
utext_isLengthExpensive(const UText *ut) {
    UBool r = (ut->providerProperties & I32_FLAG(UTEXT_PROVIDER_LENGTH_IS_EXPENSIVE)) != 0;
    return r;
}


U_CAPI int64_t U_EXPORT2
utext_getNativeIndex(const UText *ut) {
    if(ut->chunkOffset <= ut->nativeIndexingLimit) {
        return ut->chunkNativeStart+ut->chunkOffset;
    } else {
        return ut->pFuncs->mapOffsetToNative(ut);
    }
}


U_CAPI void U_EXPORT2
utext_setNativeIndex(UText *ut, int64_t index) {
    if(index<ut->chunkNativeStart || index>=ut->chunkNativeLimit) {
        
        
        
        
        ut->pFuncs->access(ut, index, TRUE);
    } else if((int32_t)(index - ut->chunkNativeStart) <= ut->nativeIndexingLimit) {
        
        ut->chunkOffset=(int32_t)(index-ut->chunkNativeStart);
    } else {
         ut->chunkOffset=ut->pFuncs->mapNativeIndexToUTF16(ut, index);
    }
    
    
    if (ut->chunkOffset<ut->chunkLength) {
        UChar c= ut->chunkContents[ut->chunkOffset];
        if (U16_IS_TRAIL(c)) {
            if (ut->chunkOffset==0) {
                ut->pFuncs->access(ut, ut->chunkNativeStart, FALSE);
            }
            if (ut->chunkOffset>0) {
                UChar lead = ut->chunkContents[ut->chunkOffset-1];
                if (U16_IS_LEAD(lead)) {
                    ut->chunkOffset--;
                }
            }
        }
    }
}



U_CAPI int64_t U_EXPORT2
utext_getPreviousNativeIndex(UText *ut) {
    
    
    
    
    
    int32_t i = ut->chunkOffset - 1;
    int64_t result;
    if (i >= 0) {
        UChar c = ut->chunkContents[i];
        if (U16_IS_TRAIL(c) == FALSE) {
            if (i <= ut->nativeIndexingLimit) {
                result = ut->chunkNativeStart + i;
            } else {
                ut->chunkOffset = i;
                result = ut->pFuncs->mapOffsetToNative(ut);
                ut->chunkOffset++;
            }
            return result;
        }
    }

    
    if (ut->chunkOffset==0 && ut->chunkNativeStart==0) {
        return 0;
    }

    
    
    
    utext_previous32(ut);
    result = UTEXT_GETNATIVEINDEX(ut);
    utext_next32(ut);
    return result;
}







U_CAPI UChar32 U_EXPORT2
utext_current32(UText *ut) {
    UChar32  c;
    if (ut->chunkOffset==ut->chunkLength) {
        
        if (ut->pFuncs->access(ut, ut->chunkNativeLimit, TRUE) == FALSE) {
            
            return U_SENTINEL;
        }
    }

    c = ut->chunkContents[ut->chunkOffset];
    if (U16_IS_LEAD(c) == FALSE) {
        
        return c;
    }

    
    
    
    UChar32   trail = 0;
    UChar32   supplementaryC = c;
    if ((ut->chunkOffset+1) < ut->chunkLength) {
        
        trail = ut->chunkContents[ut->chunkOffset+1];
    } else {
        
        
        
        
        
        
        
        int64_t  nativePosition = ut->chunkNativeLimit;
        int32_t  originalOffset = ut->chunkOffset;
        if (ut->pFuncs->access(ut, nativePosition, TRUE)) {
            trail = ut->chunkContents[ut->chunkOffset];
        }
        UBool r = ut->pFuncs->access(ut, nativePosition, FALSE);  
        U_ASSERT(r==TRUE);
        ut->chunkOffset = originalOffset;
        if(!r) {
            return U_SENTINEL;
        }
    }

    if (U16_IS_TRAIL(trail)) {
        supplementaryC = U16_GET_SUPPLEMENTARY(c, trail);
    }
    return supplementaryC;

}


U_CAPI UChar32 U_EXPORT2
utext_char32At(UText *ut, int64_t nativeIndex) {
    UChar32 c = U_SENTINEL;

    
    if (nativeIndex>=ut->chunkNativeStart && nativeIndex < ut->chunkNativeStart + ut->nativeIndexingLimit) {
        ut->chunkOffset = (int32_t)(nativeIndex - ut->chunkNativeStart);
        c = ut->chunkContents[ut->chunkOffset];
        if (U16_IS_SURROGATE(c) == FALSE) {
            return c;
        }
    }


    utext_setNativeIndex(ut, nativeIndex);
    if (nativeIndex>=ut->chunkNativeStart && ut->chunkOffset<ut->chunkLength) {
        c = ut->chunkContents[ut->chunkOffset];
        if (U16_IS_SURROGATE(c)) {
            
            
            c = utext_current32(ut);
        }
    }
    return c;
}


U_CAPI UChar32 U_EXPORT2
utext_next32(UText *ut) {
    UChar32       c;

    if (ut->chunkOffset >= ut->chunkLength) {
        if (ut->pFuncs->access(ut, ut->chunkNativeLimit, TRUE) == FALSE) {
            return U_SENTINEL;
        }
    }

    c = ut->chunkContents[ut->chunkOffset++];
    if (U16_IS_LEAD(c) == FALSE) {
        
        
        
        return c;
    }

    if (ut->chunkOffset >= ut->chunkLength) {
        if (ut->pFuncs->access(ut, ut->chunkNativeLimit, TRUE) == FALSE) {
            
            
            return c;
        }
    }
    UChar32 trail = ut->chunkContents[ut->chunkOffset];
    if (U16_IS_TRAIL(trail) == FALSE) {
        
        
        
        
        return c;
    }

    UChar32 supplementary = U16_GET_SUPPLEMENTARY(c, trail);
    ut->chunkOffset++;   
    return supplementary;
    }


U_CAPI UChar32 U_EXPORT2
utext_previous32(UText *ut) {
    UChar32       c;

    if (ut->chunkOffset <= 0) {
        if (ut->pFuncs->access(ut, ut->chunkNativeStart, FALSE) == FALSE) {
            return U_SENTINEL;
        }
    }
    ut->chunkOffset--;
    c = ut->chunkContents[ut->chunkOffset];
    if (U16_IS_TRAIL(c) == FALSE) {
        
        
        
        return c;
    }

    if (ut->chunkOffset <= 0) {
        if (ut->pFuncs->access(ut, ut->chunkNativeStart, FALSE) == FALSE) {
            
            
            return c;
        }
    }

    UChar32 lead = ut->chunkContents[ut->chunkOffset-1];
    if (U16_IS_LEAD(lead) == FALSE) {
        
        
        return c;
    }

    UChar32 supplementary = U16_GET_SUPPLEMENTARY(lead, c);
    ut->chunkOffset--;   
    return supplementary;
}



U_CAPI UChar32 U_EXPORT2
utext_next32From(UText *ut, int64_t index) {
    UChar32       c      = U_SENTINEL;

    if(index<ut->chunkNativeStart || index>=ut->chunkNativeLimit) {
        
        if(!ut->pFuncs->access(ut, index, TRUE)) {
            
            return U_SENTINEL;
        }
    } else if (index - ut->chunkNativeStart  <= (int64_t)ut->nativeIndexingLimit) {
        
        ut->chunkOffset = (int32_t)(index - ut->chunkNativeStart);
    } else {
        
        ut->chunkOffset = ut->pFuncs->mapNativeIndexToUTF16(ut, index);
    }

    c = ut->chunkContents[ut->chunkOffset++];
    if (U16_IS_SURROGATE(c)) {
        
        
        utext_setNativeIndex(ut, index);
        c = utext_next32(ut);
    }
    return c;
}


U_CAPI UChar32 U_EXPORT2
utext_previous32From(UText *ut, int64_t index) {
    
    
    
    
    UChar32     cPrev;    

    
    
    
    
    
    
    
    if(index<=ut->chunkNativeStart || index>ut->chunkNativeLimit) {
        
        if(!ut->pFuncs->access(ut, index, FALSE)) {
            
            return U_SENTINEL;
        }
    } else if(index - ut->chunkNativeStart <= (int64_t)ut->nativeIndexingLimit) {
        
        ut->chunkOffset = (int32_t)(index - ut->chunkNativeStart);
    } else {
        ut->chunkOffset=ut->pFuncs->mapNativeIndexToUTF16(ut, index);
        if (ut->chunkOffset==0 && !ut->pFuncs->access(ut, index, FALSE)) {
            
            return U_SENTINEL;
        }
    }

    
    
    
    ut->chunkOffset--;
    cPrev = ut->chunkContents[ut->chunkOffset];

    if (U16_IS_SURROGATE(cPrev)) {
        
        
        utext_setNativeIndex(ut, index);
        cPrev = utext_previous32(ut);
    }
    return cPrev;
}


U_CAPI int32_t U_EXPORT2
utext_extract(UText *ut,
             int64_t start, int64_t limit,
             UChar *dest, int32_t destCapacity,
             UErrorCode *status) {
                 return ut->pFuncs->extract(ut, start, limit, dest, destCapacity, status);
             }



U_CAPI UBool U_EXPORT2
utext_equals(const UText *a, const UText *b) {
    if (a==NULL || b==NULL ||
        a->magic != UTEXT_MAGIC ||
        b->magic != UTEXT_MAGIC) {
            
            return FALSE;
    }

    if (a->pFuncs != b->pFuncs) {
        
        return FALSE;
    }

    if (a->context != b->context) {
        
        return FALSE;
    }
    if (utext_getNativeIndex(a) != utext_getNativeIndex(b)) {
        
        return FALSE;
    }

    return TRUE;
}

U_CAPI UBool U_EXPORT2
utext_isWritable(const UText *ut)
{
    UBool b = (ut->providerProperties & I32_FLAG(UTEXT_PROVIDER_WRITABLE)) != 0;
    return b;
}


U_CAPI void U_EXPORT2
utext_freeze(UText *ut) {
    
    ut->providerProperties &= ~(I32_FLAG(UTEXT_PROVIDER_WRITABLE));
}


U_CAPI UBool U_EXPORT2
utext_hasMetaData(const UText *ut)
{
    UBool b = (ut->providerProperties & I32_FLAG(UTEXT_PROVIDER_HAS_META_DATA)) != 0;
    return b;
}



U_CAPI int32_t U_EXPORT2
utext_replace(UText *ut,
             int64_t nativeStart, int64_t nativeLimit,
             const UChar *replacementText, int32_t replacementLength,
             UErrorCode *status)
{
    if (U_FAILURE(*status)) {
        return 0;
    }
    if ((ut->providerProperties & I32_FLAG(UTEXT_PROVIDER_WRITABLE)) == 0) {
        *status = U_NO_WRITE_PERMISSION;
        return 0;
    }
    int32_t i = ut->pFuncs->replace(ut, nativeStart, nativeLimit, replacementText, replacementLength, status);
    return i;
}

U_CAPI void U_EXPORT2
utext_copy(UText *ut,
          int64_t nativeStart, int64_t nativeLimit,
          int64_t destIndex,
          UBool move,
          UErrorCode *status)
{
    if (U_FAILURE(*status)) {
        return;
    }
    if ((ut->providerProperties & I32_FLAG(UTEXT_PROVIDER_WRITABLE)) == 0) {
        *status = U_NO_WRITE_PERMISSION;
        return;
    }
    ut->pFuncs->copy(ut, nativeStart, nativeLimit, destIndex, move, status);
}



U_CAPI UText * U_EXPORT2
utext_clone(UText *dest, const UText *src, UBool deep, UBool readOnly, UErrorCode *status) {
    UText *result;
    result = src->pFuncs->clone(dest, src, deep, status);
    if (readOnly) {
        utext_freeze(result);
    }
    return result;
}












enum {
    UTEXT_HEAP_ALLOCATED  = 1,      
                                    

    UTEXT_EXTRA_HEAP_ALLOCATED = 2, 
                                    
                                    
                                    
                                    

    UTEXT_OPEN = 4                  
                                    
};






struct ExtendedUText {
    UText          ut;
    UAlignedMemory extension;
};

static const UText emptyText = UTEXT_INITIALIZER;

U_CAPI UText * U_EXPORT2
utext_setup(UText *ut, int32_t extraSpace, UErrorCode *status) {
    if (U_FAILURE(*status)) {
        return ut;
    }

    if (ut == NULL) {
        
        int32_t spaceRequired = sizeof(UText);
        if (extraSpace > 0) {
            spaceRequired = sizeof(ExtendedUText) + extraSpace - sizeof(UAlignedMemory);
        }
        ut = (UText *)uprv_malloc(spaceRequired);
        if (ut == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return NULL;
        } else {
            *ut = emptyText;
            ut->flags |= UTEXT_HEAP_ALLOCATED;
            if (spaceRequired>0) {
                ut->extraSize = extraSpace;
                ut->pExtra    = &((ExtendedUText *)ut)->extension;
            }
        }
    } else {
        
        
        if (ut->magic != UTEXT_MAGIC) {
            *status = U_ILLEGAL_ARGUMENT_ERROR;
            return ut;
        }
        
        
        if ((ut->flags & UTEXT_OPEN) && ut->pFuncs->close != NULL)  {
            ut->pFuncs->close(ut);
        }
        ut->flags &= ~UTEXT_OPEN;

        
        
        if (extraSpace > ut->extraSize) {
            
            
            if (ut->flags & UTEXT_EXTRA_HEAP_ALLOCATED) {
                uprv_free(ut->pExtra);
                ut->extraSize = 0;
            }
            ut->pExtra = uprv_malloc(extraSpace);
            if (ut->pExtra == NULL) {
                *status = U_MEMORY_ALLOCATION_ERROR;
            } else {
                ut->extraSize = extraSpace;
                ut->flags |= UTEXT_EXTRA_HEAP_ALLOCATED;
            }
        }
    }
    if (U_SUCCESS(*status)) {
        ut->flags |= UTEXT_OPEN;

        
        
        ut->context             = NULL;
        ut->chunkContents       = NULL;
        ut->p                   = NULL;
        ut->q                   = NULL;
        ut->r                   = NULL;
        ut->a                   = 0;
        ut->b                   = 0;
        ut->c                   = 0;
        ut->chunkOffset         = 0;
        ut->chunkLength         = 0;
        ut->chunkNativeStart    = 0;
        ut->chunkNativeLimit    = 0;
        ut->nativeIndexingLimit = 0;
        ut->providerProperties  = 0;
        ut->privA               = 0;
        ut->privB               = 0;
        ut->privC               = 0;
        ut->privP               = NULL;
        if (ut->pExtra!=NULL && ut->extraSize>0)
            uprv_memset(ut->pExtra, 0, ut->extraSize);

    }
    return ut;
}


U_CAPI UText * U_EXPORT2
utext_close(UText *ut) {
    if (ut==NULL ||
        ut->magic != UTEXT_MAGIC ||
        (ut->flags & UTEXT_OPEN) == 0)
    {
        
        
        return ut;
    }

    
    
    if (ut->pFuncs->close != NULL) {
        ut->pFuncs->close(ut);
    }
    ut->flags &= ~UTEXT_OPEN;

    
    
    if (ut->flags & UTEXT_EXTRA_HEAP_ALLOCATED) {
        uprv_free(ut->pExtra);
        ut->pExtra = NULL;
        ut->flags &= ~UTEXT_EXTRA_HEAP_ALLOCATED;
        ut->extraSize = 0;
    }

    
    
    
    ut->pFuncs        = NULL;

    if (ut->flags & UTEXT_HEAP_ALLOCATED) {
        
        
        
        ut->magic = 0;
        uprv_free(ut);
        ut = NULL;
    }
    return ut;
}











static void
invalidateChunk(UText *ut) {
    ut->chunkLength = 0;
    ut->chunkNativeLimit = 0;
    ut->chunkNativeStart = 0;
    ut->chunkOffset = 0;
    ut->nativeIndexingLimit = 0;
}






static int32_t
pinIndex(int64_t &index, int64_t limit) {
    if (index<0) {
        index = 0;
    } else if (index > limit) {
        index = limit;
    }
    return (int32_t)index;
}


U_CDECL_BEGIN







static void adjustPointer(UText *dest, const void **destPtr, const UText *src) {
    
    char  *dptr = (char *)*destPtr;
    char  *dUText = (char *)dest;
    char  *sUText = (char *)src;

    if (dptr >= (char *)src->pExtra && dptr < ((char*)src->pExtra)+src->extraSize) {
        
        
        *destPtr = ((char *)dest->pExtra) + (dptr - (char *)src->pExtra);
    } else if (dptr>=sUText && dptr < sUText+src->sizeOfStruct) {
        
        
        *destPtr = dUText + (dptr-sUText);
    }
}






static UText * U_CALLCONV
shallowTextClone(UText * dest, const UText * src, UErrorCode * status) {
    if (U_FAILURE(*status)) {
        return NULL;
    }
    int32_t  srcExtraSize = src->extraSize;

    
    
    
    dest = utext_setup(dest, srcExtraSize, status);
    if (U_FAILURE(*status)) {
        return dest;
    }

    
    
    
    
    
    
    void *destExtra = dest->pExtra;
    int32_t flags   = dest->flags;


    
    
    
    
    int sizeToCopy = src->sizeOfStruct;
    if (sizeToCopy > dest->sizeOfStruct) {
        sizeToCopy = dest->sizeOfStruct;
    }
    uprv_memcpy(dest, src, sizeToCopy);
    dest->pExtra = destExtra;
    dest->flags  = flags;
    if (srcExtraSize > 0) {
        uprv_memcpy(dest->pExtra, src->pExtra, srcExtraSize);
    }

    
    
    
    
    adjustPointer(dest, &dest->context, src);
    adjustPointer(dest, &dest->p, src);
    adjustPointer(dest, &dest->q, src);
    adjustPointer(dest, &dest->r, src);
    adjustPointer(dest, (const void **)&dest->chunkContents, src);

    return dest;
}


U_CDECL_END
























enum { UTF8_TEXT_CHUNK_SIZE=32 };















struct UTF8Buf {
    int32_t   bufNativeStart;                        
    int32_t   bufNativeLimit;                        
    int32_t   bufStartIdx;                           
    int32_t   bufLimitIdx;                           
    int32_t   bufNILimit;                            
    int32_t   toUCharsMapStart;                      
                                                     
                                                     
                                                     

    UChar     buf[UTF8_TEXT_CHUNK_SIZE+4];           
                                                     
                                                     
                                                     
                                                     
                                                     
    uint8_t   mapToNative[UTF8_TEXT_CHUNK_SIZE+4];   
                                                     
                                                     
                                                     
                                                     
    uint8_t   mapToUChars[UTF8_TEXT_CHUNK_SIZE*3+6]; 
                                                     
    int32_t   align;
};

U_CDECL_BEGIN







static int64_t U_CALLCONV
utf8TextLength(UText *ut) {
    if (ut->b < 0) {
        
        
        const char *r = (const char *)ut->context + ut->c;
        while (*r != 0) {
            r++;
        }
        if ((r - (const char *)ut->context) < 0x7fffffff) {
            ut->b = (int32_t)(r - (const char *)ut->context);
        } else {
            
            
            ut->b = 0x7fffffff;
        }
        ut->providerProperties &= ~I32_FLAG(UTEXT_PROVIDER_LENGTH_IS_EXPENSIVE);
    }
    return ut->b;
}






static UBool U_CALLCONV
utf8TextAccess(UText *ut, int64_t index, UBool forward) {
    
    
    
    
    
    
    const uint8_t *s8=(const uint8_t *)ut->context;
    UTF8Buf *u8b = NULL;
    int32_t  length = ut->b;         
    int32_t  ix= (int32_t)index;     
    int32_t  mapIndex = 0;
    if (index<0) {
        ix=0;
    } else if (index > 0x7fffffff) {
        
        ix = 0x7fffffff;
    }

    
    if (ix>length) {
        if (length>=0) {
            ix=length;
        } else if (ix>=ut->c) {
            
            
            
            
            while (ut->c<ix && s8[ut->c]!=0) {
                ut->c++;
            }
            
            if (s8[ut->c] == 0) {
                
                
                ix     = ut->c;
                ut->b  = ut->c;
                length = ut->c;
                ut->providerProperties &= ~I32_FLAG(UTEXT_PROVIDER_LENGTH_IS_EXPENSIVE);
            }
        }
    }

    
    
    
    if (forward) {
        if (ix==ut->chunkNativeLimit) {
            
            if (ix==length) {
                
                
                
                ut->chunkOffset = ut->chunkLength;
                return FALSE;
            } else {
                
                
                UTF8Buf *altB = (UTF8Buf *)ut->q;
                if (ix>=altB->bufNativeStart && ix<altB->bufNativeLimit) {
                    goto swapBuffers;
                }
            }
        }

        
        
        
        
        {
            u8b = (UTF8Buf *)ut->q;   
            if (ix>=u8b->bufNativeStart && ix<u8b->bufNativeLimit) {
                
                goto swapBuffers;
            }
            if (ix == length) {
                
                
                
                if (ix == ut->chunkNativeLimit) {
                    
                    
                    ut->chunkOffset = ut->chunkLength;
                    return FALSE;
                }
                if (ix == u8b->bufNativeLimit) {
                    
                    
                    goto swapBuffersAndFail;
                }

                
                goto makeStubBuffer;
            }

            if (ix<ut->chunkNativeStart || ix>=ut->chunkNativeLimit) {
                
                goto fillForward;
            }

            
            u8b = (UTF8Buf *)ut->p;   
            mapIndex = ix - u8b->toUCharsMapStart;
            ut->chunkOffset = u8b->mapToUChars[mapIndex] - u8b->bufStartIdx;
            return TRUE;

        }
    }


    
    
    
    
    if (ix==ut->chunkNativeStart) {
        
        if (ix==0) {
            
            
            
            ut->chunkOffset = 0;
            return FALSE;
        } else {
            
            
            UTF8Buf *altB = (UTF8Buf *)ut->q;
            if (ix>altB->bufNativeStart && ix<=altB->bufNativeLimit) {
                goto swapBuffers;
            }
        }
    }

    
    
    
    
    
    u8b = (UTF8Buf *)ut->q;   
    if (ix>u8b->bufNativeStart && ix<=u8b->bufNativeLimit) {
        
        goto swapBuffers;
    }
    
    
    
    if (ix==0) {
        if (u8b->bufNativeStart==0) {
            
            
            goto swapBuffersAndFail;
        } else {
            
            
            
            goto makeStubBuffer;
        }
    }

    if (ix<=ut->chunkNativeStart || ix>ut->chunkNativeLimit) {
        
        goto fillReverse;
    }

    
    
    u8b = (UTF8Buf *)ut->p;
    mapIndex = ix - u8b->toUCharsMapStart;
    ut->chunkOffset = u8b->mapToUChars[mapIndex] - u8b->bufStartIdx;
    if (ut->chunkOffset==0) {
        
        
        
        
        
        return FALSE;
    } else {
        return TRUE;
    }



swapBuffers:
    
    
    
    {
        u8b   = (UTF8Buf *)ut->q;
        ut->q = ut->p;
        ut->p = u8b;
        ut->chunkContents       = &u8b->buf[u8b->bufStartIdx];
        ut->chunkLength         = u8b->bufLimitIdx - u8b->bufStartIdx;
        ut->chunkNativeStart    = u8b->bufNativeStart;
        ut->chunkNativeLimit    = u8b->bufNativeLimit;
        ut->nativeIndexingLimit = u8b->bufNILimit;

        
        
        
        U_ASSERT(ix>=u8b->bufNativeStart);
        U_ASSERT(ix<=u8b->bufNativeLimit);
        mapIndex = ix - u8b->toUCharsMapStart;
        U_ASSERT(mapIndex>=0);
        U_ASSERT(mapIndex<(int32_t)sizeof(u8b->mapToUChars));
        ut->chunkOffset = u8b->mapToUChars[mapIndex] - u8b->bufStartIdx;

        return TRUE;
    }


 swapBuffersAndFail:
    
    
    
    
    
    
    
    
    u8b   = (UTF8Buf *)ut->q;
    ut->q = ut->p;
    ut->p = u8b;
    ut->chunkContents       = &u8b->buf[u8b->bufStartIdx];
    ut->chunkLength         = u8b->bufLimitIdx - u8b->bufStartIdx;
    ut->chunkNativeStart    = u8b->bufNativeStart;
    ut->chunkNativeLimit    = u8b->bufNativeLimit;
    ut->nativeIndexingLimit = u8b->bufNILimit;

    
    
    
    if (ix==u8b->bufNativeLimit) {
        ut->chunkOffset = ut->chunkLength;
    } else  {
        ut->chunkOffset = 0;
        U_ASSERT(ix == u8b->bufNativeStart);
    }
    return FALSE;

makeStubBuffer:
    
    
    
    
    u8b = (UTF8Buf *)ut->q;
    u8b->bufNativeStart   = ix;
    u8b->bufNativeLimit   = ix;
    u8b->bufStartIdx      = 0;
    u8b->bufLimitIdx      = 0;
    u8b->bufNILimit       = 0;
    u8b->toUCharsMapStart = ix;
    u8b->mapToNative[0]   = 0;
    u8b->mapToUChars[0]   = 0;
    goto swapBuffersAndFail;



fillForward:
    {
        
        U8_SET_CP_START(s8, 0, ix);

        
        
        
        UTF8Buf *u8b = (UTF8Buf *)ut->q;
        ut->q = ut->p;
        ut->p = u8b;

        int32_t strLen = ut->b;
        UBool   nulTerminated = FALSE;
        if (strLen < 0) {
            strLen = 0x7fffffff;
            nulTerminated = TRUE;
        }

        UChar   *buf = u8b->buf;
        uint8_t *mapToNative  = u8b->mapToNative;
        uint8_t *mapToUChars  = u8b->mapToUChars;
        int32_t  destIx       = 0;
        int32_t  srcIx        = ix;
        UBool    seenNonAscii = FALSE;
        UChar32  c = 0;

        
        while (destIx<UTF8_TEXT_CHUNK_SIZE) {
            c = s8[srcIx];
            if (c>0 && c<0x80) {
                
                
                buf[destIx] = (UChar)c;
                mapToNative[destIx]    = (uint8_t)(srcIx - ix);
                mapToUChars[srcIx-ix]  = (uint8_t)destIx;
                srcIx++;
                destIx++;
            } else {
                
                if (seenNonAscii == FALSE) {
                    seenNonAscii = TRUE;
                    u8b->bufNILimit = destIx;
                }

                int32_t  cIx      = srcIx;
                int32_t  dIx      = destIx;
                int32_t  dIxSaved = destIx;
                U8_NEXT(s8, srcIx, strLen, c);
                if (c==0 && nulTerminated) {
                    srcIx--;
                    break;
                }
                if (c<0) {
                    
                    c = 0x0fffd;
                }

                U16_APPEND_UNSAFE(buf, destIx, c);
                do {
                    mapToNative[dIx++] = (uint8_t)(cIx - ix);
                } while (dIx < destIx);

                do {
                    mapToUChars[cIx++ - ix] = (uint8_t)dIxSaved;
                } while (cIx < srcIx);
            }
            if (srcIx>=strLen) {
                break;
            }

        }

        
        
        mapToNative[destIx]     = (uint8_t)(srcIx - ix);
        mapToUChars[srcIx - ix] = (uint8_t)destIx;

        
        u8b->bufNativeStart     = ix;
        u8b->bufNativeLimit     = srcIx;
        u8b->bufStartIdx        = 0;
        u8b->bufLimitIdx        = destIx;
        if (seenNonAscii == FALSE) {
            u8b->bufNILimit     = destIx;
        }
        u8b->toUCharsMapStart   = u8b->bufNativeStart;

        
        ut->chunkContents       = buf;
        ut->chunkOffset         = 0;
        ut->chunkLength         = u8b->bufLimitIdx;
        ut->chunkNativeStart    = u8b->bufNativeStart;
        ut->chunkNativeLimit    = u8b->bufNativeLimit;
        ut->nativeIndexingLimit = u8b->bufNILimit;

        
        
        if (nulTerminated && srcIx>ut->c) {
            ut->c = srcIx;
            if (c==0) {
                
                
                ut->b = srcIx;
                ut->providerProperties &= ~I32_FLAG(UTEXT_PROVIDER_LENGTH_IS_EXPENSIVE);
            }
        }
        return TRUE;
    }


fillReverse:
    {
        
        
        
        if (ix != ut->b) {
            U8_SET_CP_START(s8, 0, ix);
        }

        
        
        
        UTF8Buf *u8b = (UTF8Buf *)ut->q;
        ut->q = ut->p;
        ut->p = u8b;

        UChar   *buf = u8b->buf;
        uint8_t *mapToNative = u8b->mapToNative;
        uint8_t *mapToUChars = u8b->mapToUChars;
        int32_t  toUCharsMapStart = ix - (UTF8_TEXT_CHUNK_SIZE*3 + 1);
        int32_t  destIx = UTF8_TEXT_CHUNK_SIZE+2;   
                                                    
                                                    
                                                    
        int32_t  srcIx  = ix;
        int32_t  bufNILimit = destIx;
        UChar32   c;

        
        
        
        mapToNative[destIx] = (uint8_t)(srcIx - toUCharsMapStart);
        mapToUChars[srcIx - toUCharsMapStart] = (uint8_t)destIx;

        
        
        
        while (destIx>2 && (srcIx - toUCharsMapStart > 5) && (srcIx > 0)) {
            srcIx--;
            destIx--;

            
            c = s8[srcIx];
            if (c<0x80) {
                
                buf[destIx] = (UChar)c;
                mapToUChars[srcIx - toUCharsMapStart] = (uint8_t)destIx;
                mapToNative[destIx] = (uint8_t)(srcIx - toUCharsMapStart);
            } else {
                

                int32_t  sIx      = srcIx;  

                
                
                
                
                if (c<=0xbf) {
                    c=utf8_prevCharSafeBody(s8, 0, &srcIx, c, -1);
                    
                } else {
                    c=0x0fffd;
                }

                
                if (c<0x10000) {
                    buf[destIx] = (UChar)c;
                    mapToNative[destIx] = (uint8_t)(srcIx - toUCharsMapStart);
                } else {
                    buf[destIx]         = U16_TRAIL(c);
                    mapToNative[destIx] = (uint8_t)(srcIx - toUCharsMapStart);
                    buf[--destIx]       = U16_LEAD(c);
                    mapToNative[destIx] = (uint8_t)(srcIx - toUCharsMapStart);
                }

                
                do {
                    mapToUChars[sIx-- - toUCharsMapStart] = (uint8_t)destIx;
                } while (sIx >= srcIx);

                
                
                
                
                bufNILimit = destIx;
            }
        }
        u8b->bufNativeStart     = srcIx;
        u8b->bufNativeLimit     = ix;
        u8b->bufStartIdx        = destIx;
        u8b->bufLimitIdx        = UTF8_TEXT_CHUNK_SIZE+2;
        u8b->bufNILimit         = bufNILimit - u8b->bufStartIdx;
        u8b->toUCharsMapStart   = toUCharsMapStart;

        ut->chunkContents       = &buf[u8b->bufStartIdx];
        ut->chunkLength         = u8b->bufLimitIdx - u8b->bufStartIdx;
        ut->chunkOffset         = ut->chunkLength;
        ut->chunkNativeStart    = u8b->bufNativeStart;
        ut->chunkNativeLimit    = u8b->bufNativeLimit;
        ut->nativeIndexingLimit = u8b->bufNILimit;
        return TRUE;
    }

}








static UChar*
utext_strFromUTF8(UChar *dest,
              int32_t destCapacity,
              int32_t *pDestLength,
              const char* src,
              int32_t srcLength,        
              UErrorCode *pErrorCode
              )
{

    UChar *pDest = dest;
    UChar *pDestLimit = (dest!=NULL)?(dest+destCapacity):NULL;
    UChar32 ch=0;
    int32_t index = 0;
    int32_t reqLength = 0;
    uint8_t* pSrc = (uint8_t*) src;


    while((index < srcLength)&&(pDest<pDestLimit)){
        ch = pSrc[index++];
        if(ch <=0x7f){
            *pDest++=(UChar)ch;
        }else{
            ch=utf8_nextCharSafeBody(pSrc, &index, srcLength, ch, -1);
            if(ch<0){
                ch = 0xfffd;
            }
            if(U_IS_BMP(ch)){
                *(pDest++)=(UChar)ch;
            }else{
                *(pDest++)=U16_LEAD(ch);
                if(pDest<pDestLimit){
                    *(pDest++)=U16_TRAIL(ch);
                }else{
                    reqLength++;
                    break;
                }
            }
        }
    }
    
    while(index < srcLength){
        ch = pSrc[index++];
        if(ch <= 0x7f){
            reqLength++;
        }else{
            ch=utf8_nextCharSafeBody(pSrc, &index, srcLength, ch, -1);
            if(ch<0){
                ch = 0xfffd;
            }
            reqLength+=U16_LENGTH(ch);
        }
    }

    reqLength+=(int32_t)(pDest - dest);

    if(pDestLength){
        *pDestLength = reqLength;
    }

    
    u_terminateUChars(dest,destCapacity,reqLength,pErrorCode);

    return dest;
}



static int32_t U_CALLCONV
utf8TextExtract(UText *ut,
                int64_t start, int64_t limit,
                UChar *dest, int32_t destCapacity,
                UErrorCode *pErrorCode) {
    if(U_FAILURE(*pErrorCode)) {
        return 0;
    }
    if(destCapacity<0 || (dest==NULL && destCapacity>0)) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }
    int32_t  length  = ut->b;
    int32_t  start32 = pinIndex(start, length);
    int32_t  limit32 = pinIndex(limit, length);

    if(start32>limit32) {
        *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
        return 0;
    }


    
    
    
    const uint8_t *buf = (const uint8_t *)ut->context;
    int i;
    if (start32 < ut->chunkNativeLimit) {
        for (i=0; i<3; i++) {
            if (U8_IS_SINGLE(buf[start32]) || U8_IS_LEAD(buf[start32]) || start32==0) {
                break;
            }
            start32--;
        }
    }

    if (limit32 < ut->chunkNativeLimit) {
        for (i=0; i<3; i++) {
            if (U8_IS_SINGLE(buf[limit32]) || U8_IS_LEAD(buf[limit32]) || limit32==0) {
                break;
            }
            limit32--;
        }
    }

    
    int32_t destLength=0;
    utext_strFromUTF8(dest, destCapacity, &destLength,
                    (const char *)ut->context+start32, limit32-start32,
                    pErrorCode);
    utf8TextAccess(ut, limit32, TRUE);
    return destLength;
}





static int64_t U_CALLCONV
utf8TextMapOffsetToNative(const UText *ut) {
    
    UTF8Buf *u8b = (UTF8Buf *)ut->p;
    U_ASSERT(ut->chunkOffset>ut->nativeIndexingLimit && ut->chunkOffset<=ut->chunkLength);
    int32_t nativeOffset = u8b->mapToNative[ut->chunkOffset + u8b->bufStartIdx] + u8b->toUCharsMapStart;
    U_ASSERT(nativeOffset >= ut->chunkNativeStart && nativeOffset <= ut->chunkNativeLimit);
    return nativeOffset;
}




static int32_t U_CALLCONV
utf8TextMapIndexToUTF16(const UText *ut, int64_t index64) {
    U_ASSERT(index64 <= 0x7fffffff);
    int32_t index = (int32_t)index64;
    UTF8Buf *u8b = (UTF8Buf *)ut->p;
    U_ASSERT(index>=ut->chunkNativeStart+ut->nativeIndexingLimit);
    U_ASSERT(index<=ut->chunkNativeLimit);
    int32_t mapIndex = index - u8b->toUCharsMapStart;
    int32_t offset = u8b->mapToUChars[mapIndex] - u8b->bufStartIdx;
    U_ASSERT(offset>=0 && offset<=ut->chunkLength);
    return offset;
}

static UText * U_CALLCONV
utf8TextClone(UText *dest, const UText *src, UBool deep, UErrorCode *status)
{
    
    dest = shallowTextClone(dest, src, status);

    
    
    
    
    
    
    
    
    
    if (deep && U_SUCCESS(*status)) {
        int32_t  len = (int32_t)utext_nativeLength((UText *)src);
        char *copyStr = (char *)uprv_malloc(len+1);
        if (copyStr == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
        } else {
            uprv_memcpy(copyStr, src->context, len+1);
            dest->context = copyStr;
            dest->providerProperties |= I32_FLAG(UTEXT_PROVIDER_OWNS_TEXT);
        }
    }
    return dest;
}


static void U_CALLCONV
utf8TextClose(UText *ut) {
    
    
    
    if (ut->providerProperties & I32_FLAG(UTEXT_PROVIDER_OWNS_TEXT)) {
        char *s = (char *)ut->context;
        uprv_free(s);
        ut->context = NULL;
    }
}

U_CDECL_END


static const struct UTextFuncs utf8Funcs = 
{
    sizeof(UTextFuncs),
    0, 0, 0,             
    utf8TextClone,
    utf8TextLength,
    utf8TextAccess,
    utf8TextExtract,
    NULL,                
    NULL,                
    utf8TextMapOffsetToNative,
    utf8TextMapIndexToUTF16,
    utf8TextClose,
    NULL,                
    NULL,                
    NULL                 
};


static const char gEmptyString[] = {0};

U_CAPI UText * U_EXPORT2
utext_openUTF8(UText *ut, const char *s, int64_t length, UErrorCode *status) {
    if(U_FAILURE(*status)) {
        return NULL;
    }
    if(s==NULL && length==0) {
        s = gEmptyString;
    }

    if(s==NULL || length<-1 || length>INT32_MAX) {
        *status=U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }

    ut = utext_setup(ut, sizeof(UTF8Buf) * 2, status);
    if (U_FAILURE(*status)) {
        return ut;
    }

    ut->pFuncs  = &utf8Funcs;
    ut->context = s;
    ut->b       = (int32_t)length;
    ut->c       = (int32_t)length;
    if (ut->c < 0) {
        ut->c = 0;
        ut->providerProperties |= I32_FLAG(UTEXT_PROVIDER_LENGTH_IS_EXPENSIVE);
    }
    ut->p = ut->pExtra;
    ut->q = (char *)ut->pExtra + sizeof(UTF8Buf);
    return ut;

}






















enum { REP_TEXT_CHUNK_SIZE=10 };

struct ReplExtra {
    



    UChar s[REP_TEXT_CHUNK_SIZE+1];
};


U_CDECL_BEGIN

static UText * U_CALLCONV
repTextClone(UText *dest, const UText *src, UBool deep, UErrorCode *status) {
    
    dest = shallowTextClone(dest, src, status);

    
    
    
    
    
    if (deep && U_SUCCESS(*status)) {
        const Replaceable *replSrc = (const Replaceable *)src->context;
        dest->context = replSrc->clone();
        dest->providerProperties |= I32_FLAG(UTEXT_PROVIDER_OWNS_TEXT);

        
        dest->providerProperties |= I32_FLAG(UTEXT_PROVIDER_WRITABLE);
    }
    return dest;
}


static void U_CALLCONV
repTextClose(UText *ut) {
    
    
    
    if (ut->providerProperties & I32_FLAG(UTEXT_PROVIDER_OWNS_TEXT)) {
        Replaceable *rep = (Replaceable *)ut->context;
        delete rep;
        ut->context = NULL;
    }
}


static int64_t U_CALLCONV
repTextLength(UText *ut) {
    const Replaceable *replSrc = (const Replaceable *)ut->context;
    int32_t  len = replSrc->length();
    return len;
}


static UBool U_CALLCONV
repTextAccess(UText *ut, int64_t index, UBool forward) {
    const Replaceable *rep=(const Replaceable *)ut->context;
    int32_t length=rep->length();   

    
    int32_t index32 = pinIndex(index, length);
    U_ASSERT(index<=INT32_MAX);


    







    if(forward) {

        if (index32>=ut->chunkNativeStart && index32<ut->chunkNativeLimit) {
            
            ut->chunkOffset = (int32_t)(index - ut->chunkNativeStart);
            return TRUE;
        }
        if (index32>=length && ut->chunkNativeLimit==length) {
            
            
            ut->chunkOffset = length - (int32_t)ut->chunkNativeStart;
            return FALSE;
        }

        ut->chunkNativeLimit = index + REP_TEXT_CHUNK_SIZE - 1;
        
        
        
        
        if(ut->chunkNativeLimit > length) {
            ut->chunkNativeLimit = length;
        }
        
        ut->chunkNativeStart = ut->chunkNativeLimit - REP_TEXT_CHUNK_SIZE;
        if(ut->chunkNativeStart < 0) {
            ut->chunkNativeStart = 0;
        }
    } else {
        
        if (index32>ut->chunkNativeStart && index32<=ut->chunkNativeLimit) {
            
            ut->chunkOffset = index32 - (int32_t)ut->chunkNativeStart;
            return TRUE;
        }
        if (index32==0 && ut->chunkNativeStart==0) {
            
            
            ut->chunkOffset = 0;
            return FALSE;
        }

        
        
        
        
        
        
        ut->chunkNativeStart = index32 + 1 - REP_TEXT_CHUNK_SIZE;
        if (ut->chunkNativeStart < 0) {
            ut->chunkNativeStart = 0;
        }

        ut->chunkNativeLimit = index32 + 1;
        if (ut->chunkNativeLimit > length) {
            ut->chunkNativeLimit = length;
        }
    }

    
    ReplExtra *ex = (ReplExtra *)ut->pExtra;
    
    UnicodeString buffer(ex->s, 0 , REP_TEXT_CHUNK_SIZE );
    rep->extractBetween((int32_t)ut->chunkNativeStart, (int32_t)ut->chunkNativeLimit, buffer);

    ut->chunkContents  = ex->s;
    ut->chunkLength    = (int32_t)(ut->chunkNativeLimit - ut->chunkNativeStart);
    ut->chunkOffset    = (int32_t)(index32 - ut->chunkNativeStart);

    
    
    if (ut->chunkNativeLimit < length &&
        U16_IS_LEAD(ex->s[ut->chunkLength-1])) {
            ut->chunkLength--;
            ut->chunkNativeLimit--;
            if (ut->chunkOffset > ut->chunkLength) {
                ut->chunkOffset = ut->chunkLength;
            }
        }

    
    
    if(ut->chunkNativeStart>0 && U16_IS_TRAIL(ex->s[0])) {
        ++(ut->chunkContents);
        ++(ut->chunkNativeStart);
        --(ut->chunkLength);
        --(ut->chunkOffset);
    }

    
    U16_SET_CP_START(ut->chunkContents, 0, ut->chunkOffset);

    
    ut->nativeIndexingLimit = ut->chunkLength;

    return TRUE;
}



static int32_t U_CALLCONV
repTextExtract(UText *ut,
               int64_t start, int64_t limit,
               UChar *dest, int32_t destCapacity,
               UErrorCode *status) {
    const Replaceable *rep=(const Replaceable *)ut->context;
    int32_t  length=rep->length();

    if(U_FAILURE(*status)) {
        return 0;
    }
    if(destCapacity<0 || (dest==NULL && destCapacity>0)) {
        *status=U_ILLEGAL_ARGUMENT_ERROR;
    }
    if(start>limit) {
        *status=U_INDEX_OUTOFBOUNDS_ERROR;
        return 0;
    }

    int32_t  start32 = pinIndex(start, length);
    int32_t  limit32 = pinIndex(limit, length);

    
    if (start32<length && U16_IS_TRAIL(rep->charAt(start32)) &&
        U_IS_SUPPLEMENTARY(rep->char32At(start32))){
            start32--;
    }
    if (limit32<length && U16_IS_TRAIL(rep->charAt(limit32)) &&
        U_IS_SUPPLEMENTARY(rep->char32At(limit32))){
            limit32--;
    }

    length=limit32-start32;
    if(length>destCapacity) {
        limit32 = start32 + destCapacity;
    }
    UnicodeString buffer(dest, 0, destCapacity); 
    rep->extractBetween(start32, limit32, buffer);
    repTextAccess(ut, limit32, TRUE);
    
    return u_terminateUChars(dest, destCapacity, length, status);
}

static int32_t U_CALLCONV
repTextReplace(UText *ut,
               int64_t start, int64_t limit,
               const UChar *src, int32_t length,
               UErrorCode *status) {
    Replaceable *rep=(Replaceable *)ut->context;
    int32_t oldLength;

    if(U_FAILURE(*status)) {
        return 0;
    }
    if(src==NULL && length!=0) {
        *status=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }
    oldLength=rep->length(); 
    if(start>limit ) {
        *status=U_INDEX_OUTOFBOUNDS_ERROR;
        return 0;
    }

    int32_t start32 = pinIndex(start, oldLength);
    int32_t limit32 = pinIndex(limit, oldLength);

    
    if (start32<oldLength && U16_IS_TRAIL(rep->charAt(start32)) &&
        start32>0 && U16_IS_LEAD(rep->charAt(start32-1)))
    {
            start32--;
    }
    if (limit32<oldLength && U16_IS_LEAD(rep->charAt(limit32-1)) &&
        U16_IS_TRAIL(rep->charAt(limit32)))
    {
            limit32++;
    }

    
    UnicodeString replStr((UBool)(length<0), src, length); 
    rep->handleReplaceBetween(start32, limit32, replStr);
    int32_t newLength = rep->length();
    int32_t lengthDelta = newLength - oldLength;

    
    if (ut->chunkNativeLimit > start32) {
        
        
        invalidateChunk(ut);
    }

    
    int32_t newIndexPos = limit32 + lengthDelta;
    repTextAccess(ut, newIndexPos, TRUE);

    return lengthDelta;
}


static void U_CALLCONV
repTextCopy(UText *ut,
                int64_t start, int64_t limit,
                int64_t destIndex,
                UBool move,
                UErrorCode *status)
{
    Replaceable *rep=(Replaceable *)ut->context;
    int32_t length=rep->length();

    if(U_FAILURE(*status)) {
        return;
    }
    if (start>limit || (start<destIndex && destIndex<limit))
    {
        *status=U_INDEX_OUTOFBOUNDS_ERROR;
        return;
    }

    int32_t start32     = pinIndex(start, length);
    int32_t limit32     = pinIndex(limit, length);
    int32_t destIndex32 = pinIndex(destIndex, length);

    

    if(move) {
        
        int32_t segLength=limit32-start32;
        rep->copy(start32, limit32, destIndex32);
        if(destIndex32<start32) {
            start32+=segLength;
            limit32+=segLength;
        }
        rep->handleReplaceBetween(start32, limit32, UnicodeString());
    } else {
        
        rep->copy(start32, limit32, destIndex32);
    }

    
    
    int32_t firstAffectedIndex = destIndex32;
    if (move && start32<firstAffectedIndex) {
        firstAffectedIndex = start32;
    }
    if (firstAffectedIndex < ut->chunkNativeLimit) {
        
        invalidateChunk(ut);
    }

    
    int32_t  nativeIterIndex = destIndex32 + limit32 - start32;
    if (move && destIndex32>start32) {
        
        nativeIterIndex = destIndex32;
    }

    
    repTextAccess(ut, nativeIterIndex, TRUE);
}

static const struct UTextFuncs repFuncs = 
{
    sizeof(UTextFuncs),
    0, 0, 0,           
    repTextClone,
    repTextLength,
    repTextAccess,
    repTextExtract,
    repTextReplace,   
    repTextCopy,   
    NULL,              
    NULL,              
    repTextClose,
    NULL,              
    NULL,              
    NULL               
};


U_CAPI UText * U_EXPORT2
utext_openReplaceable(UText *ut, Replaceable *rep, UErrorCode *status)
{
    if(U_FAILURE(*status)) {
        return NULL;
    }
    if(rep==NULL) {
        *status=U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }
    ut = utext_setup(ut, sizeof(ReplExtra), status);

    ut->providerProperties = I32_FLAG(UTEXT_PROVIDER_WRITABLE);
    if(rep->hasMetaData()) {
        ut->providerProperties |=I32_FLAG(UTEXT_PROVIDER_HAS_META_DATA);
    }

    ut->pFuncs  = &repFuncs;
    ut->context =  rep;
    return ut;
}

U_CDECL_END





















U_CDECL_BEGIN


static UText * U_CALLCONV
unistrTextClone(UText *dest, const UText *src, UBool deep, UErrorCode *status) {
    
    dest = shallowTextClone(dest, src, status);

    
    
    
    
    
    if (deep && U_SUCCESS(*status)) {
        const UnicodeString *srcString = (const UnicodeString *)src->context;
        dest->context = new UnicodeString(*srcString);
        dest->providerProperties |= I32_FLAG(UTEXT_PROVIDER_OWNS_TEXT);

        
        dest->providerProperties |= I32_FLAG(UTEXT_PROVIDER_WRITABLE);
    }
    return dest;
}

static void U_CALLCONV
unistrTextClose(UText *ut) {
    
    
    
    if (ut->providerProperties & I32_FLAG(UTEXT_PROVIDER_OWNS_TEXT)) {
        UnicodeString *str = (UnicodeString *)ut->context;
        delete str;
        ut->context = NULL;
    }
}


static int64_t U_CALLCONV
unistrTextLength(UText *t) {
    return ((const UnicodeString *)t->context)->length();
}


static UBool U_CALLCONV
unistrTextAccess(UText *ut, int64_t index, UBool  forward) {
    int32_t length  = ut->chunkLength;
    ut->chunkOffset = pinIndex(index, length);

    
    UBool retVal = (forward && index<length) || (!forward && index>0);
    return retVal;
}



static int32_t U_CALLCONV
unistrTextExtract(UText *t,
                  int64_t start, int64_t limit,
                  UChar *dest, int32_t destCapacity,
                  UErrorCode *pErrorCode) {
    const UnicodeString *us=(const UnicodeString *)t->context;
    int32_t length=us->length();

    if(U_FAILURE(*pErrorCode)) {
        return 0;
    }
    if(destCapacity<0 || (dest==NULL && destCapacity>0)) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
    }
    if(start<0 || start>limit) {
        *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
        return 0;
    }

    int32_t start32 = start<length ? us->getChar32Start((int32_t)start) : length;
    int32_t limit32 = limit<length ? us->getChar32Start((int32_t)limit) : length;

    length=limit32-start32;
    if (destCapacity>0 && dest!=NULL) {
        int32_t trimmedLength = length;
        if(trimmedLength>destCapacity) {
            trimmedLength=destCapacity;
        }
        us->extract(start32, trimmedLength, dest);
        t->chunkOffset = start32+trimmedLength;
    } else {
        t->chunkOffset = start32;
    }
    u_terminateUChars(dest, destCapacity, length, pErrorCode);
    return length;
}

static int32_t U_CALLCONV
unistrTextReplace(UText *ut,
                  int64_t start, int64_t limit,
                  const UChar *src, int32_t length,
                  UErrorCode *pErrorCode) {
    UnicodeString *us=(UnicodeString *)ut->context;
    int32_t oldLength;

    if(U_FAILURE(*pErrorCode)) {
        return 0;
    }
    if(src==NULL && length!=0) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
    }
    if(start>limit) {
        *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
        return 0;
    }
    oldLength=us->length();
    int32_t start32 = pinIndex(start, oldLength);
    int32_t limit32 = pinIndex(limit, oldLength);
    if (start32 < oldLength) {
        start32 = us->getChar32Start(start32);
    }
    if (limit32 < oldLength) {
        limit32 = us->getChar32Start(limit32);
    }

    
    us->replace(start32, limit32-start32, src, length);
    int32_t newLength = us->length();

    
    ut->chunkContents    = us->getBuffer();
    ut->chunkLength      = newLength;
    ut->chunkNativeLimit = newLength;
    ut->nativeIndexingLimit = newLength;

    
    int32_t lengthDelta = newLength - oldLength;
    ut->chunkOffset = limit32 + lengthDelta;

    return lengthDelta;
}

static void U_CALLCONV
unistrTextCopy(UText *ut,
               int64_t start, int64_t limit,
               int64_t destIndex,
               UBool move,
               UErrorCode *pErrorCode) {
    UnicodeString *us=(UnicodeString *)ut->context;
    int32_t length=us->length();

    if(U_FAILURE(*pErrorCode)) {
        return;
    }
    int32_t start32 = pinIndex(start, length);
    int32_t limit32 = pinIndex(limit, length);
    int32_t destIndex32 = pinIndex(destIndex, length);

    if( start32>limit32 || (start32<destIndex32 && destIndex32<limit32)) {
        *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
        return;
    }

    if(move) {
        
        int32_t segLength=limit32-start32;
        us->copy(start32, limit32, destIndex32);
        if(destIndex32<start32) {
            start32+=segLength;
        }
        us->replace(start32, segLength, NULL, 0);
    } else {
        
        us->copy(start32, limit32, destIndex32);
    }

    
    ut->chunkContents = us->getBuffer();
    if (move==FALSE) {
        
        ut->chunkLength += limit32-start32;
        ut->chunkNativeLimit = ut->chunkLength;
        ut->nativeIndexingLimit = ut->chunkLength;
    }

    
    ut->chunkOffset = destIndex32+limit32-start32;
    if (move && destIndex32>start32) {
        ut->chunkOffset = destIndex32;
    }

}

static const struct UTextFuncs unistrFuncs = 
{
    sizeof(UTextFuncs),
    0, 0, 0,             
    unistrTextClone,
    unistrTextLength,
    unistrTextAccess,
    unistrTextExtract,
    unistrTextReplace,   
    unistrTextCopy,   
    NULL,                
    NULL,                
    unistrTextClose,
    NULL,                
    NULL,                
    NULL                 
};



U_CDECL_END


U_CAPI UText * U_EXPORT2
utext_openUnicodeString(UText *ut, UnicodeString *s, UErrorCode *status) {
    ut = utext_openConstUnicodeString(ut, s, status);
    if (U_SUCCESS(*status)) {
        ut->providerProperties |= I32_FLAG(UTEXT_PROVIDER_WRITABLE);
    }
    return ut;
}



U_CAPI UText * U_EXPORT2
utext_openConstUnicodeString(UText *ut, const UnicodeString *s, UErrorCode *status) {
    if (U_SUCCESS(*status) && s->isBogus()) {
        
        
        utext_openUChars(ut, NULL, 0, status);
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return ut;
    }
    ut = utext_setup(ut, 0, status);
    
    
    
    if (U_SUCCESS(*status)) {
        ut->pFuncs              = &unistrFuncs;
        ut->context             = s;
        ut->providerProperties  = I32_FLAG(UTEXT_PROVIDER_STABLE_CHUNKS);
        ut->chunkContents       = s->getBuffer();
        ut->chunkLength         = s->length();
        ut->chunkNativeStart    = 0;
        ut->chunkNativeLimit    = ut->chunkLength;
        ut->nativeIndexingLimit = ut->chunkLength;
    }
    return ut;
}













U_CDECL_BEGIN


static UText * U_CALLCONV
ucstrTextClone(UText *dest, const UText * src, UBool deep, UErrorCode * status) {
    
    dest = shallowTextClone(dest, src, status);

    
    
    
    
    
    if (deep && U_SUCCESS(*status)) {
        U_ASSERT(utext_nativeLength(dest) < INT32_MAX);
        int32_t  len = (int32_t)utext_nativeLength(dest);

        
        const UChar *srcStr = (const UChar *)src->context;
        UChar *copyStr = (UChar *)uprv_malloc((len+1) * sizeof(UChar));
        if (copyStr == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
        } else {
            int64_t i;
            for (i=0; i<len; i++) {
                copyStr[i] = srcStr[i];
            }
            copyStr[len] = 0;
            dest->context = copyStr;
            dest->providerProperties |= I32_FLAG(UTEXT_PROVIDER_OWNS_TEXT);
        }
    }
    return dest;
}


static void U_CALLCONV
ucstrTextClose(UText *ut) {
    
    
    
    if (ut->providerProperties & I32_FLAG(UTEXT_PROVIDER_OWNS_TEXT)) {
        UChar *s = (UChar *)ut->context;
        uprv_free(s);
        ut->context = NULL;
    }
}



static int64_t U_CALLCONV
ucstrTextLength(UText *ut) {
    if (ut->a < 0) {
        
        
        
        const UChar  *str = (const UChar *)ut->context;
        for (;;) {
            if (str[ut->chunkNativeLimit] == 0) {
                break;
            }
            ut->chunkNativeLimit++;
        }
        ut->a = ut->chunkNativeLimit;
        ut->chunkLength = (int32_t)ut->chunkNativeLimit;
        ut->nativeIndexingLimit = ut->chunkLength;
        ut->providerProperties &= ~I32_FLAG(UTEXT_PROVIDER_LENGTH_IS_EXPENSIVE);
    }
    return ut->a;
}


static UBool U_CALLCONV
ucstrTextAccess(UText *ut, int64_t index, UBool  forward) {
    const UChar *str   = (const UChar *)ut->context;

    
    
    if (index<0) {
        index = 0;
    } else if (index < ut->chunkNativeLimit) {
        
        
        U16_SET_CP_START(str, 0, index);
    } else if (ut->a >= 0) {
        
        
        index = ut->a;
    } else {
        
        
        
        
        
        int32_t scanLimit = (int32_t)index + 32;
        if ((index + 32)>INT32_MAX || (index + 32)<0 ) {   
            scanLimit = INT32_MAX;
        }

        int32_t chunkLimit = (int32_t)ut->chunkNativeLimit;
        for (; chunkLimit<scanLimit; chunkLimit++) {
            if (str[chunkLimit] == 0) {
                
                
                ut->a = chunkLimit;
                ut->chunkLength = chunkLimit;
                ut->nativeIndexingLimit = chunkLimit;
                if (index >= chunkLimit) {
                    index = chunkLimit;
                } else {
                    U16_SET_CP_START(str, 0, index);
                }

                ut->chunkNativeLimit = chunkLimit;
                ut->providerProperties &= ~I32_FLAG(UTEXT_PROVIDER_LENGTH_IS_EXPENSIVE);
                goto breakout;
            }
        }
        
        U16_SET_CP_START(str, 0, index);
        if (chunkLimit == INT32_MAX) {
            
            
            
            ut->a = chunkLimit;
            ut->chunkLength = chunkLimit;
            ut->nativeIndexingLimit = chunkLimit;
            if (index > chunkLimit) {
                index = chunkLimit;
            }
            ut->chunkNativeLimit = chunkLimit;
            ut->providerProperties &= ~I32_FLAG(UTEXT_PROVIDER_LENGTH_IS_EXPENSIVE);
        } else {
            
            
            
            
            if (U16_IS_LEAD(str[chunkLimit-1])) {
                --chunkLimit;
            }
            
            
            
            
            ut->chunkNativeLimit = chunkLimit;
            ut->nativeIndexingLimit = chunkLimit;
            ut->chunkLength = chunkLimit;
        }

    }
breakout:
    U_ASSERT(index<=INT32_MAX);
    ut->chunkOffset = (int32_t)index;

    
    UBool retVal = (forward && index<ut->chunkNativeLimit) || (!forward && index>0);
    return retVal;
}



static int32_t U_CALLCONV
ucstrTextExtract(UText *ut,
                  int64_t start, int64_t limit,
                  UChar *dest, int32_t destCapacity,
                  UErrorCode *pErrorCode)
{
    if(U_FAILURE(*pErrorCode)) {
        return 0;
    }
    if(destCapacity<0 || (dest==NULL && destCapacity>0) || start>limit) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    
    int32_t si, di;

    int32_t start32;
    int32_t limit32;

    
    
    
    ucstrTextAccess(ut, start, TRUE);
    const UChar *s=ut->chunkContents;
    start32 = ut->chunkOffset;

    int32_t strLength=(int32_t)ut->a;
    if (strLength >= 0) {
        limit32 = pinIndex(limit, strLength);
    } else {
        limit32 = pinIndex(limit, INT32_MAX);
    }
    di = 0;
    for (si=start32; si<limit32; si++) {
        if (strLength<0 && s[si]==0) {
            
            ut->a = si;               
            ut->chunkNativeLimit    = si;
            ut->chunkLength         = si;
            ut->nativeIndexingLimit = si;
            strLength               = si;
            break;
        }
        U_ASSERT(di>=0); 
        if (di<destCapacity) {
            
            dest[di] = s[si];
        } else {
            if (strLength>=0) {
                
                
                di = limit32 - start32;
                si = limit32;
                break;
            }
        }
        di++;
    }

    
    
    if (si>0 && U16_IS_LEAD(s[si-1]) &&
        ((si<strLength || strLength<0)  && U16_IS_TRAIL(s[si])))
    {
        if (di<destCapacity) {
            
            dest[di++] = s[si++];
        }
    }

    
    ut->chunkOffset = uprv_min(strLength, start32 + destCapacity);

    
    
    u_terminateUChars(dest, destCapacity, di, pErrorCode);
    return di;
}

static const struct UTextFuncs ucstrFuncs = 
{
    sizeof(UTextFuncs),
    0, 0, 0,           
    ucstrTextClone,
    ucstrTextLength,
    ucstrTextAccess,
    ucstrTextExtract,
    NULL,              
    NULL,              
    NULL,              
    NULL,              
    ucstrTextClose,
    NULL,              
    NULL,              
    NULL,              
};

U_CDECL_END

static const UChar gEmptyUString[] = {0};

U_CAPI UText * U_EXPORT2
utext_openUChars(UText *ut, const UChar *s, int64_t length, UErrorCode *status) {
    if (U_FAILURE(*status)) {
        return NULL;
    }
    if(s==NULL && length==0) {
        s = gEmptyUString;
    }
    if (s==NULL || length < -1 || length>INT32_MAX) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }
    ut = utext_setup(ut, 0, status);
    if (U_SUCCESS(*status)) {
        ut->pFuncs               = &ucstrFuncs;
        ut->context              = s;
        ut->providerProperties   = I32_FLAG(UTEXT_PROVIDER_STABLE_CHUNKS);
        if (length==-1) {
            ut->providerProperties |= I32_FLAG(UTEXT_PROVIDER_LENGTH_IS_EXPENSIVE);
        }
        ut->a                    = length;
        ut->chunkContents        = s;
        ut->chunkNativeStart     = 0;
        ut->chunkNativeLimit     = length>=0? length : 0;
        ut->chunkLength          = (int32_t)ut->chunkNativeLimit;
        ut->chunkOffset          = 0;
        ut->nativeIndexingLimit  = ut->chunkLength;
    }
    return ut;
}

















#define CIBufSize 16

U_CDECL_BEGIN
static void U_CALLCONV
charIterTextClose(UText *ut) {
    
    
    
    CharacterIterator *ci = (CharacterIterator *)ut->r;
    delete ci;
    ut->r = NULL;
}

static int64_t U_CALLCONV
charIterTextLength(UText *ut) {
    return (int32_t)ut->a;
}

static UBool U_CALLCONV
charIterTextAccess(UText *ut, int64_t index, UBool  forward) {
    CharacterIterator *ci   = (CharacterIterator *)ut->context;

    int32_t clippedIndex = (int32_t)index;
    if (clippedIndex<0) {
        clippedIndex=0;
    } else if (clippedIndex>=ut->a) {
        clippedIndex=(int32_t)ut->a;
    }
    int32_t neededIndex = clippedIndex;
    if (!forward && neededIndex>0) {
        
        neededIndex--;
    } else if (forward && neededIndex==ut->a && neededIndex>0) {
        
        neededIndex--;
    }

    
    neededIndex -= neededIndex % CIBufSize;

    UChar *buf = NULL;
    UBool  needChunkSetup = TRUE;
    int    i;
    if (ut->chunkNativeStart == neededIndex) {
        
        needChunkSetup = FALSE;
    } else if (ut->b == neededIndex) {
        
        buf = (UChar *)ut->p;
    } else if (ut->c == neededIndex) {
        
        buf = (UChar *)ut->q;
    } else {
        
        
        
        buf = (UChar *)ut->p;
        if (ut->p == ut->chunkContents) {
            buf = (UChar *)ut->q;
        }
        ci->setIndex(neededIndex);
        for (i=0; i<CIBufSize; i++) {
            buf[i] = ci->nextPostInc();
            if (i+neededIndex > ut->a) {
                break;
            }
        }
    }

    
    
    if (needChunkSetup) {
        ut->chunkContents = buf;
        ut->chunkLength   = CIBufSize;
        ut->chunkNativeStart = neededIndex;
        ut->chunkNativeLimit = neededIndex + CIBufSize;
        if (ut->chunkNativeLimit > ut->a) {
            ut->chunkNativeLimit = ut->a;
            ut->chunkLength  = (int32_t)(ut->chunkNativeLimit)-(int32_t)(ut->chunkNativeStart);
        }
        ut->nativeIndexingLimit = ut->chunkLength;
        U_ASSERT(ut->chunkOffset>=0 && ut->chunkOffset<=CIBufSize);
    }
    ut->chunkOffset = clippedIndex - (int32_t)ut->chunkNativeStart;
    UBool success = (forward? ut->chunkOffset<ut->chunkLength : ut->chunkOffset>0);
    return success;
}

static UText * U_CALLCONV
charIterTextClone(UText *dest, const UText *src, UBool deep, UErrorCode * status) {
    if (U_FAILURE(*status)) {
        return NULL;
    }

    if (deep) {
        
        *status = U_UNSUPPORTED_ERROR;
        return NULL;
    } else {
        CharacterIterator *srcCI =(CharacterIterator *)src->context;
        srcCI = srcCI->clone();
        dest = utext_openCharacterIterator(dest, srcCI, status);
        
        
        int64_t  ix = utext_getNativeIndex((UText *)src);
        utext_setNativeIndex(dest, ix);
        dest->r = srcCI;    
    }
    return dest;
}

static int32_t U_CALLCONV
charIterTextExtract(UText *ut,
                  int64_t start, int64_t limit,
                  UChar *dest, int32_t destCapacity,
                  UErrorCode *status)
{
    if(U_FAILURE(*status)) {
        return 0;
    }
    if(destCapacity<0 || (dest==NULL && destCapacity>0) || start>limit) {
        *status=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }
    int32_t  length  = (int32_t)ut->a;
    int32_t  start32 = pinIndex(start, length);
    int32_t  limit32 = pinIndex(limit, length);
    int32_t  desti   = 0;
    int32_t  srci;
    int32_t  copyLimit;

    CharacterIterator *ci = (CharacterIterator *)ut->context;
    ci->setIndex32(start32);   
    srci = ci->getIndex();
    copyLimit = srci;
    while (srci<limit32) {
        UChar32 c = ci->next32PostInc();
        int32_t  len = U16_LENGTH(c);
        U_ASSERT(desti+len>0); 
        if (desti+len <= destCapacity) {
            U16_APPEND_UNSAFE(dest, desti, c);
            copyLimit = srci+len;
        } else {
            desti += len;
            *status = U_BUFFER_OVERFLOW_ERROR;
        }
        srci += len;
    }
    
    charIterTextAccess(ut, copyLimit, TRUE);

    u_terminateUChars(dest, destCapacity, desti, status);
    return desti;
}

static const struct UTextFuncs charIterFuncs = 
{
    sizeof(UTextFuncs),
    0, 0, 0,             
    charIterTextClone,
    charIterTextLength,
    charIterTextAccess,
    charIterTextExtract,
    NULL,                
    NULL,                
    NULL,                
    NULL,                
    charIterTextClose,
    NULL,                
    NULL,                
    NULL                 
};
U_CDECL_END


U_CAPI UText * U_EXPORT2
utext_openCharacterIterator(UText *ut, CharacterIterator *ci, UErrorCode *status) {
    if (U_FAILURE(*status)) {
        return NULL;
    }

    if (ci->startIndex() > 0) {
        
        *status = U_UNSUPPORTED_ERROR;
        return NULL;
    }

    
    int32_t  extraSpace = 2 * CIBufSize * sizeof(UChar);
    ut = utext_setup(ut, extraSpace, status);
    if (U_SUCCESS(*status)) {
        ut->pFuncs                = &charIterFuncs;
        ut->context              = ci;
        ut->providerProperties   = 0;
        ut->a                    = ci->endIndex();        
        ut->p                    = ut->pExtra;            
        ut->b                    = -1;                    
        ut->q                    = (UChar*)ut->pExtra+CIBufSize;  
        ut->c                    = -1;                    

        
        
        
        
        
        
        ut->chunkContents        = (UChar *)ut->p;
        ut->chunkNativeStart     = -1;
        ut->chunkOffset          = 1;
        ut->chunkNativeLimit     = 0;
        ut->chunkLength          = 0;
        ut->nativeIndexingLimit  = ut->chunkOffset;  
    }
    return ut;
}



