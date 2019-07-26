

















#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/bytestream.h"
#include "unicode/coleitr.h"
#include "unicode/unorm.h"
#include "unicode/udata.h"
#include "unicode/ustring.h"
#include "unicode/utf8.h"

#include "ucol_imp.h"
#include "bocsu.h"

#include "normalizer2impl.h"
#include "unorm_it.h"
#include "umutex.h"
#include "cmemory.h"
#include "ucln_in.h"
#include "cstring.h"
#include "utracimp.h"
#include "putilimp.h"
#include "uassert.h"
#include "unicode/coll.h"

#ifdef UCOL_DEBUG
#include <stdio.h>
#endif

U_NAMESPACE_USE

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

#define LAST_BYTE_MASK_           0xFF
#define SECOND_LAST_BYTE_SHIFT_   8

#define ZERO_CC_LIMIT_            0xC0





static const Normalizer2 *g_nfd = NULL;
static const Normalizer2Impl *g_nfcImpl = NULL;





static const int32_t maxRegularPrimary  = 0x7A;
static const int32_t minImplicitPrimary = 0xE0;
static const int32_t maxImplicitPrimary = 0xE4;

U_CDECL_BEGIN
static UBool U_CALLCONV
ucol_cleanup(void)
{
    g_nfd = NULL;
    g_nfcImpl = NULL;
    return TRUE;
}

static int32_t U_CALLCONV
_getFoldingOffset(uint32_t data) {
    return (int32_t)(data&0xFFFFFF);
}

U_CDECL_END

static inline
UBool initializeNFD(UErrorCode *status) {
    if (g_nfd != NULL) {
        return TRUE;
    } else {
        
        g_nfd = Normalizer2Factory::getNFDInstance(*status);
        ucln_i18n_registerCleanup(UCLN_I18N_UCOL, ucol_cleanup);
        return U_SUCCESS(*status);
    }
}


static inline
UBool initializeFCD(UErrorCode *status) {
    if (g_nfcImpl != NULL) {
        return TRUE;
    } else {
        
        g_nfcImpl = Normalizer2Factory::getNFCImpl(*status);
        
        
        ucln_i18n_registerCleanup(UCLN_I18N_UCOL, ucol_cleanup);
        return U_SUCCESS(*status);
    }
}

static
inline void IInit_collIterate(const UCollator *collator, const UChar *sourceString,
                              int32_t sourceLen, collIterate *s,
                              UErrorCode *status)
{
    (s)->string = (s)->pos = sourceString;
    (s)->origFlags = 0;
    (s)->flags = 0;
    if (sourceLen >= 0) {
        s->flags |= UCOL_ITER_HASLEN;
        (s)->endp = (UChar *)sourceString+sourceLen;
    }
    else {
        
        (s)->endp = NULL;
    }
    (s)->extendCEs = NULL;
    (s)->extendCEsSize = 0;
    (s)->CEpos = (s)->toReturn = (s)->CEs;
    (s)->offsetBuffer = NULL;
    (s)->offsetBufferSize = 0;
    (s)->offsetReturn = (s)->offsetStore = NULL;
    (s)->offsetRepeatCount = (s)->offsetRepeatValue = 0;
    (s)->coll = (collator);
    if (initializeNFD(status)) {
        (s)->nfd = g_nfd;
    } else {
        return;
    }
    (s)->fcdPosition = 0;
    if(collator->normalizationMode == UCOL_ON) {
        (s)->flags |= UCOL_ITER_NORM;
    }
    if(collator->hiraganaQ == UCOL_ON && collator->strength >= UCOL_QUATERNARY) {
        (s)->flags |= UCOL_HIRAGANA_Q;
    }
    (s)->iterator = NULL;
    
}

U_CAPI void  U_EXPORT2
uprv_init_collIterate(const UCollator *collator, const UChar *sourceString,
                             int32_t sourceLen, collIterate *s,
                             UErrorCode *status) {
    
    IInit_collIterate(collator, sourceString, sourceLen, s, status);
}

U_CAPI collIterate * U_EXPORT2 
uprv_new_collIterate(UErrorCode *status) {
    if(U_FAILURE(*status)) {
        return NULL;
    }
    collIterate *s = new collIterate;
    if(s == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    return s;
}

U_CAPI void U_EXPORT2 
uprv_delete_collIterate(collIterate *s) {
    delete s;
}

U_CAPI UBool U_EXPORT2
uprv_collIterateAtEnd(collIterate *s) {
    return s == NULL || s->pos == s->endp;
}






static
inline void backupState(const collIterate *data, collIterateState *backup)
{
    backup->fcdPosition = data->fcdPosition;
    backup->flags       = data->flags;
    backup->origFlags   = data->origFlags;
    backup->pos         = data->pos;
    backup->bufferaddress = data->writableBuffer.getBuffer();
    backup->buffersize    = data->writableBuffer.length();
    backup->iteratorMove = 0;
    backup->iteratorIndex = 0;
    if(data->iterator != NULL) {
        
        backup->iteratorIndex = data->iterator->getState(data->iterator);
        
        if(backup->iteratorIndex == UITER_NO_STATE) {
            while((backup->iteratorIndex = data->iterator->getState(data->iterator)) == UITER_NO_STATE) {
                backup->iteratorMove++;
                data->iterator->move(data->iterator, -1, UITER_CURRENT);
            }
            data->iterator->move(data->iterator, backup->iteratorMove, UITER_CURRENT);
        }
    }
}








static
inline void loadState(collIterate *data, const collIterateState *backup,
                      UBool        forwards)
{
    UErrorCode status = U_ZERO_ERROR;
    data->flags       = backup->flags;
    data->origFlags   = backup->origFlags;
    if(data->iterator != NULL) {
        
        data->iterator->setState(data->iterator, backup->iteratorIndex, &status);
        if(backup->iteratorMove != 0) {
            data->iterator->move(data->iterator, backup->iteratorMove, UITER_CURRENT);
        }
    }
    data->pos         = backup->pos;

    if ((data->flags & UCOL_ITER_INNORMBUF) &&
        data->writableBuffer.getBuffer() != backup->bufferaddress) {
        




        if (forwards) {
            data->pos = data->writableBuffer.getTerminatedBuffer() +
                                         (data->pos - backup->bufferaddress);
        }
        else {
            
            int32_t temp = backup->buffersize -
                                  (int32_t)(data->pos - backup->bufferaddress);
            data->pos = data->writableBuffer.getTerminatedBuffer() + (data->writableBuffer.length() - temp);
        }
    }
    if ((data->flags & UCOL_ITER_INNORMBUF) == 0) {
        











        data->fcdPosition = backup->fcdPosition;
    }
}

static UBool
reallocCEs(collIterate *data, int32_t newCapacity) {
    uint32_t *oldCEs = data->extendCEs;
    if(oldCEs == NULL) {
        oldCEs = data->CEs;
    }
    int32_t length = data->CEpos - oldCEs;
    uint32_t *newCEs = (uint32_t *)uprv_malloc(newCapacity * 4);
    if(newCEs == NULL) {
        return FALSE;
    }
    uprv_memcpy(newCEs, oldCEs, length * 4);
    uprv_free(data->extendCEs);
    data->extendCEs = newCEs;
    data->extendCEsSize = newCapacity;
    data->CEpos = newCEs + length;
    return TRUE;
}

static UBool
increaseCEsCapacity(collIterate *data) {
    int32_t oldCapacity;
    if(data->extendCEs != NULL) {
        oldCapacity = data->extendCEsSize;
    } else {
        oldCapacity = LENGTHOF(data->CEs);
    }
    return reallocCEs(data, 2 * oldCapacity);
}

static UBool
ensureCEsCapacity(collIterate *data, int32_t minCapacity) {
    int32_t oldCapacity;
    if(data->extendCEs != NULL) {
        oldCapacity = data->extendCEsSize;
    } else {
        oldCapacity = LENGTHOF(data->CEs);
    }
    if(minCapacity <= oldCapacity) {
        return TRUE;
    }
    oldCapacity *= 2;
    return reallocCEs(data, minCapacity > oldCapacity ? minCapacity : oldCapacity);
}

void collIterate::appendOffset(int32_t offset, UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) {
        return;
    }
    int32_t length = offsetStore == NULL ? 0 : (int32_t)(offsetStore - offsetBuffer);
    U_ASSERT(length >= offsetBufferSize || offsetStore != NULL);
    if(length >= offsetBufferSize) {
        int32_t newCapacity = 2 * offsetBufferSize + UCOL_EXPAND_CE_BUFFER_SIZE;
        int32_t *newBuffer = static_cast<int32_t *>(uprv_malloc(newCapacity * 4));
        if(newBuffer == NULL) {
            errorCode = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        if(length > 0) {
            uprv_memcpy(newBuffer, offsetBuffer, length * 4);
        }
        uprv_free(offsetBuffer);
        offsetBuffer = newBuffer;
        offsetStore = offsetBuffer + length;
        offsetBufferSize = newCapacity;
    }
    *offsetStore++ = offset;
}







static
inline UBool collIter_eos(collIterate *s) {
    if(s->flags & UCOL_USE_ITERATOR) {
      return !(s->iterator->hasNext(s->iterator));
    }
    if ((s->flags & UCOL_ITER_HASLEN) == 0 && *s->pos != 0) {
        
        
        return FALSE;
    }

    
    
    if (s->flags & UCOL_ITER_HASLEN) {
        return (s->pos == s->endp);
    }

    
    if ((s->flags & UCOL_ITER_INNORMBUF) == 0) {
        
        return TRUE;
    }

    
    
    if(s->origFlags & UCOL_USE_ITERATOR) {
      return !(s->iterator->hasNext(s->iterator));
    } else if ((s->origFlags & UCOL_ITER_HASLEN) == 0) {
        
        return (*s->fcdPosition == 0);
    }
    else {
        
        return s->fcdPosition == s->endp;
    }
}







static
inline UBool collIter_bos(collIterate *source) {
  
  
  if(source->flags & UCOL_USE_ITERATOR || source->origFlags & UCOL_USE_ITERATOR) {
    return !source->iterator->hasPrevious(source->iterator);
  }
  if (source->pos <= source->string ||
      ((source->flags & UCOL_ITER_INNORMBUF) &&
      *(source->pos - 1) == 0 && source->fcdPosition == NULL)) {
    return TRUE;
  }
  return FALSE;
}













    







static UCollator*
ucol_initFromBinary(const uint8_t *bin, int32_t length,
                const UCollator *base,
                UCollator *fillIn,
                UErrorCode *status)
{
    UCollator *result = fillIn;
    if(U_FAILURE(*status)) {
        return NULL;
    }
    






    
    uprv_uca_initImplicitConstants(status);
    UCATableHeader *colData = (UCATableHeader *)bin;
    
    if((base && (uprv_memcmp(colData->UCAVersion, base->image->UCAVersion, sizeof(UVersionInfo)) != 0 ||
        uprv_memcmp(colData->UCDVersion, base->image->UCDVersion, sizeof(UVersionInfo)) != 0)) ||
        colData->version[0] != UCOL_BUILDER_VERSION)
    {
        *status = U_COLLATOR_VERSION_MISMATCH;
        return NULL;
    }
    else {
        if((uint32_t)length > (paddedsize(sizeof(UCATableHeader)) + paddedsize(sizeof(UColOptionSet)))) {
            result = ucol_initCollator((const UCATableHeader *)bin, result, base, status);
            if(U_FAILURE(*status)){
                return NULL;
            }
            result->hasRealData = TRUE;
        }
        else {
            if(base) {
                result = ucol_initCollator(base->image, result, base, status);
                ucol_setOptionsFromHeader(result, (UColOptionSet *)(bin+((const UCATableHeader *)bin)->options), status);
                if(U_FAILURE(*status)){
                    return NULL;
                }
                result->hasRealData = FALSE;
            }
            else {
                *status = U_USELESS_COLLATOR_ERROR;
                return NULL;
            }
        }
        result->freeImageOnClose = FALSE;
    }
    result->actualLocale = NULL;
    result->validLocale = NULL;
    result->requestedLocale = NULL;
    result->rules = NULL;
    result->rulesLength = 0;
    result->freeRulesOnClose = FALSE;
    result->ucaRules = NULL;
    return result;
}

U_CAPI UCollator* U_EXPORT2
ucol_openBinary(const uint8_t *bin, int32_t length,
                const UCollator *base,
                UErrorCode *status)
{
    return ucol_initFromBinary(bin, length, base, NULL, status);
}

U_CAPI int32_t U_EXPORT2
ucol_cloneBinary(const UCollator *coll,
                 uint8_t *buffer, int32_t capacity,
                 UErrorCode *status)
{
    int32_t length = 0;
    if(U_FAILURE(*status)) {
        return length;
    }
    if(capacity < 0) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return length;
    }
    if(coll->hasRealData == TRUE) {
        length = coll->image->size;
        if(length <= capacity) {
            uprv_memcpy(buffer, coll->image, length);
        } else {
            *status = U_BUFFER_OVERFLOW_ERROR;
        }
    } else {
        length = (int32_t)(paddedsize(sizeof(UCATableHeader))+paddedsize(sizeof(UColOptionSet)));
        if(length <= capacity) {
            
            
            

            
            uprv_memset(buffer, 0, length);

            
            UCATableHeader *myData = (UCATableHeader *)buffer;
            myData->size = length;

            
            myData->options = sizeof(UCATableHeader);

            
            myData->expansion = myData->options + sizeof(UColOptionSet);

            myData->magic = UCOL_HEADER_MAGIC;
            myData->isBigEndian = U_IS_BIG_ENDIAN;
            myData->charSetFamily = U_CHARSET_FAMILY;

            
            uprv_memcpy(myData->version, coll->image->version, sizeof(UVersionInfo));

            uprv_memcpy(myData->UCAVersion, coll->image->UCAVersion, sizeof(UVersionInfo));
            uprv_memcpy(myData->UCDVersion, coll->image->UCDVersion, sizeof(UVersionInfo));
            uprv_memcpy(myData->formatVersion, coll->image->formatVersion, sizeof(UVersionInfo));
            myData->jamoSpecial = coll->image->jamoSpecial;

            
            uprv_memcpy(buffer+paddedsize(sizeof(UCATableHeader)), coll->options, sizeof(UColOptionSet));
        } else {
            *status = U_BUFFER_OVERFLOW_ERROR;
        }
    }
    return length;
}

U_CAPI UCollator* U_EXPORT2
ucol_safeClone(const UCollator *coll, void *stackBuffer, int32_t * pBufferSize, UErrorCode *status)
{
    UCollator * localCollator;
    int32_t bufferSizeNeeded = (int32_t)sizeof(UCollator);
    char *stackBufferChars = (char *)stackBuffer;
    int32_t imageSize = 0;
    int32_t rulesSize = 0;
    int32_t rulesPadding = 0;
    int32_t defaultReorderCodesSize = 0;
    int32_t reorderCodesSize = 0;
    uint8_t *image;
    UChar *rules;
    int32_t* defaultReorderCodes;
    int32_t* reorderCodes;
    uint8_t* leadBytePermutationTable;
    UBool colAllocated = FALSE;
    UBool imageAllocated = FALSE;

    if (status == NULL || U_FAILURE(*status)){
        return 0;
    }
    if ((stackBuffer && !pBufferSize) || !coll){
       *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    if (coll->rules && coll->freeRulesOnClose) {
        rulesSize = (int32_t)(coll->rulesLength + 1)*sizeof(UChar);
        rulesPadding = (int32_t)(bufferSizeNeeded % sizeof(UChar));
        bufferSizeNeeded += rulesSize + rulesPadding;
    }
    
    if (coll->defaultReorderCodes) {
        defaultReorderCodesSize = coll->defaultReorderCodesLength * sizeof(int32_t);
        bufferSizeNeeded += defaultReorderCodesSize;
    }
    if (coll->reorderCodes) {
        reorderCodesSize = coll->reorderCodesLength * sizeof(int32_t);
        bufferSizeNeeded += reorderCodesSize;
    }
    if (coll->leadBytePermutationTable) {
        bufferSizeNeeded += 256 * sizeof(uint8_t);
    }
    
    if (stackBuffer && *pBufferSize <= 0) { 
        *pBufferSize =  bufferSizeNeeded;
        return 0;
    }

    


    if (U_ALIGNMENT_OFFSET(stackBuffer) != 0) {
        int32_t offsetUp = (int32_t)U_ALIGNMENT_OFFSET_UP(stackBufferChars);
        if (*pBufferSize > offsetUp) {
            *pBufferSize -= offsetUp;
            stackBufferChars += offsetUp;
        }
        else {
            
            *pBufferSize = 1;
        }
    }
    stackBuffer = (void *)stackBufferChars;

    if (stackBuffer == NULL || *pBufferSize < bufferSizeNeeded) {
        
        stackBufferChars = (char *)uprv_malloc(bufferSizeNeeded);
        
        if (stackBufferChars == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return NULL;
        }
        colAllocated = TRUE;
        if (U_SUCCESS(*status)) {
            *status = U_SAFECLONE_ALLOCATED_WARNING;
        }
    }
    localCollator = (UCollator *)stackBufferChars;
    rules = (UChar *)(stackBufferChars + sizeof(UCollator) + rulesPadding);
    defaultReorderCodes = (int32_t*)((uint8_t*)rules + rulesSize);
    reorderCodes = (int32_t*)((uint8_t*)defaultReorderCodes + defaultReorderCodesSize);
    leadBytePermutationTable = (uint8_t*)reorderCodes + reorderCodesSize;

    {
        UErrorCode tempStatus = U_ZERO_ERROR;
        imageSize = ucol_cloneBinary(coll, NULL, 0, &tempStatus);
    }
    if (coll->freeImageOnClose) {
        image = (uint8_t *)uprv_malloc(imageSize);
        
        if (image == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return NULL;
        }
        ucol_cloneBinary(coll, image, imageSize, status);
        imageAllocated = TRUE;
    }
    else {
        image = (uint8_t *)coll->image;
    }
    localCollator = ucol_initFromBinary(image, imageSize, coll->UCA, localCollator, status);
    if (U_FAILURE(*status)) {
        return NULL;
    }

    if (coll->rules) {
        if (coll->freeRulesOnClose) {
            localCollator->rules = u_strcpy(rules, coll->rules);
            
        }
        else {
            localCollator->rules = coll->rules;
        }
        localCollator->freeRulesOnClose = FALSE;
        localCollator->rulesLength = coll->rulesLength;
    }
    
    
    if (coll->defaultReorderCodes) {
        localCollator->defaultReorderCodes = 
            (int32_t*) uprv_memcpy(defaultReorderCodes, coll->defaultReorderCodes, coll->defaultReorderCodesLength * sizeof(int32_t));
        localCollator->defaultReorderCodesLength = coll->defaultReorderCodesLength;
        localCollator->freeDefaultReorderCodesOnClose = FALSE;
    }
    if (coll->reorderCodes) {
        localCollator->reorderCodes = 
            (int32_t*)uprv_memcpy(reorderCodes, coll->reorderCodes, coll->reorderCodesLength * sizeof(int32_t));
        localCollator->reorderCodesLength = coll->reorderCodesLength;
        localCollator->freeReorderCodesOnClose = FALSE;
    }
    if (coll->leadBytePermutationTable) {
        localCollator->leadBytePermutationTable = 
            (uint8_t*) uprv_memcpy(leadBytePermutationTable, coll->leadBytePermutationTable, 256);
        localCollator->freeLeadBytePermutationTableOnClose = FALSE;
    }

    int32_t i;
    for(i = 0; i < UCOL_ATTRIBUTE_COUNT; i++) {
        ucol_setAttribute(localCollator, (UColAttribute)i, ucol_getAttribute(coll, (UColAttribute)i, status), status);
    }
    
    localCollator->actualLocale = NULL;
    localCollator->validLocale = NULL;
    localCollator->requestedLocale = NULL;
    localCollator->ucaRules = coll->ucaRules; 
    localCollator->freeOnClose = colAllocated;
    localCollator->freeImageOnClose = imageAllocated;
    return localCollator;
}

U_CAPI void U_EXPORT2
ucol_close(UCollator *coll)
{
    UTRACE_ENTRY_OC(UTRACE_UCOL_CLOSE);
    UTRACE_DATA1(UTRACE_INFO, "coll = %p", coll);
    if(coll != NULL) {
        
        
        if(coll->validLocale != NULL) {
            uprv_free(coll->validLocale);
        }
        if(coll->actualLocale != NULL) {
            uprv_free(coll->actualLocale);
        }
        if(coll->requestedLocale != NULL) {
            uprv_free(coll->requestedLocale);
        }
        if(coll->latinOneCEs != NULL) {
            uprv_free(coll->latinOneCEs);
        }
        if(coll->options != NULL && coll->freeOptionsOnClose) {
            uprv_free(coll->options);
        }
        if(coll->rules != NULL && coll->freeRulesOnClose) {
            uprv_free((UChar *)coll->rules);
        }
        if(coll->image != NULL && coll->freeImageOnClose) {
            uprv_free((UCATableHeader *)coll->image);
        }

        if(coll->leadBytePermutationTable != NULL && coll->freeLeadBytePermutationTableOnClose == TRUE) {
            uprv_free(coll->leadBytePermutationTable);
        }
        if(coll->defaultReorderCodes != NULL && coll->freeDefaultReorderCodesOnClose == TRUE) {
            uprv_free(coll->defaultReorderCodes);
        }
        if(coll->reorderCodes != NULL && coll->freeReorderCodesOnClose == TRUE) {
            uprv_free(coll->reorderCodes);
        }

        if(coll->delegate != NULL) {
          delete (Collator*)coll->delegate;
        }

        
        
        
        UTRACE_DATA1(UTRACE_INFO, "coll->freeOnClose: %d", coll->freeOnClose);
        if(coll->freeOnClose){
            

            uprv_free(coll);
        }
    }
    UTRACE_EXIT();
}



U_CFUNC uint8_t* U_EXPORT2
ucol_cloneRuleData(const UCollator *coll, int32_t *length, UErrorCode *status)
{
    uint8_t *result = NULL;
    if(U_FAILURE(*status)) {
        return NULL;
    }
    if(coll->hasRealData == TRUE) {
        *length = coll->image->size;
        result = (uint8_t *)uprv_malloc(*length);
        
        if (result == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return NULL;
        }
        uprv_memcpy(result, coll->image, *length);
    } else {
        *length = (int32_t)(paddedsize(sizeof(UCATableHeader))+paddedsize(sizeof(UColOptionSet)));
        result = (uint8_t *)uprv_malloc(*length);
        
        if (result == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return NULL;
        }

        
        
        

        
        uprv_memset(result, 0, *length);

        
        UCATableHeader *myData = (UCATableHeader *)result;
        myData->size = *length;

        
        myData->options = sizeof(UCATableHeader);

        
        myData->expansion = myData->options + sizeof(UColOptionSet);

        myData->magic = UCOL_HEADER_MAGIC;
        myData->isBigEndian = U_IS_BIG_ENDIAN;
        myData->charSetFamily = U_CHARSET_FAMILY;

        
        uprv_memcpy(myData->version, coll->image->version, sizeof(UVersionInfo));

        uprv_memcpy(myData->UCAVersion, coll->image->UCAVersion, sizeof(UVersionInfo));
        uprv_memcpy(myData->UCDVersion, coll->image->UCDVersion, sizeof(UVersionInfo));
        uprv_memcpy(myData->formatVersion, coll->image->formatVersion, sizeof(UVersionInfo));
        myData->jamoSpecial = coll->image->jamoSpecial;

        
        uprv_memcpy(result+paddedsize(sizeof(UCATableHeader)), coll->options, sizeof(UColOptionSet));
    }
    return result;
}

void ucol_setOptionsFromHeader(UCollator* result, UColOptionSet * opts, UErrorCode *status) {
    if(U_FAILURE(*status)) {
        return;
    }
    result->caseFirst = (UColAttributeValue)opts->caseFirst;
    result->caseLevel = (UColAttributeValue)opts->caseLevel;
    result->frenchCollation = (UColAttributeValue)opts->frenchCollation;
    result->normalizationMode = (UColAttributeValue)opts->normalizationMode;
    if(result->normalizationMode == UCOL_ON && !initializeFCD(status)) {
        return;
    }
    result->strength = (UColAttributeValue)opts->strength;
    result->variableTopValue = opts->variableTopValue;
    result->alternateHandling = (UColAttributeValue)opts->alternateHandling;
    result->hiraganaQ = (UColAttributeValue)opts->hiraganaQ;
    result->numericCollation = (UColAttributeValue)opts->numericCollation;
    result->caseFirstisDefault = TRUE;
    result->caseLevelisDefault = TRUE;
    result->frenchCollationisDefault = TRUE;
    result->normalizationModeisDefault = TRUE;
    result->strengthisDefault = TRUE;
    result->variableTopValueisDefault = TRUE;
    result->alternateHandlingisDefault = TRUE;
    result->hiraganaQisDefault = TRUE;
    result->numericCollationisDefault = TRUE;

    ucol_updateInternalState(result, status);

    result->options = opts;
}









static
inline UBool ucol_contractionEndCP(UChar c, const UCollator *coll) {
    if (c < coll->minContrEndCP) {
        return FALSE;
    }

    int32_t  hash = c;
    uint8_t  htbyte;
    if (hash >= UCOL_UNSAFECP_TABLE_SIZE*8) {
        if (U16_IS_TRAIL(c)) {
            return TRUE;
        }
        hash = (hash & UCOL_UNSAFECP_TABLE_MASK) + 256;
    }
    htbyte = coll->contrEndCP[hash>>3];
    return (((htbyte >> (hash & 7)) & 1) == 1);
}









static
inline uint8_t i_getCombiningClass(UChar32 c, const UCollator *coll) {
    uint8_t sCC = 0;
    if ((c >= 0x300 && ucol_unsafeCP(c, coll)) || c > 0xFFFF) {
        sCC = u_getCombiningClass(c);
    }
    return sCC;
}

UCollator* ucol_initCollator(const UCATableHeader *image, UCollator *fillIn, const UCollator *UCA, UErrorCode *status) {
    UChar c;
    UCollator *result = fillIn;
    if(U_FAILURE(*status) || image == NULL) {
        return NULL;
    }

    if(result == NULL) {
        result = (UCollator *)uprv_malloc(sizeof(UCollator));
        if(result == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return result;
        }
        result->freeOnClose = TRUE;
    } else {
        result->freeOnClose = FALSE;
    }

    result->delegate = NULL;

    result->image = image;
    result->mapping.getFoldingOffset = _getFoldingOffset;
    const uint8_t *mapping = (uint8_t*)result->image+result->image->mappingPosition;
    utrie_unserialize(&result->mapping, mapping, result->image->endExpansionCE - result->image->mappingPosition, status);
    if(U_FAILURE(*status)) {
        if(result->freeOnClose == TRUE) {
            uprv_free(result);
            result = NULL;
        }
        return result;
    }

    result->latinOneMapping = UTRIE_GET32_LATIN1(&result->mapping);
    result->contractionCEs = (uint32_t*)((uint8_t*)result->image+result->image->contractionCEs);
    result->contractionIndex = (UChar*)((uint8_t*)result->image+result->image->contractionIndex);
    result->expansion = (uint32_t*)((uint8_t*)result->image+result->image->expansion);
    result->rules = NULL;
    result->rulesLength = 0;
    result->freeRulesOnClose = FALSE;
    result->defaultReorderCodes = NULL;
    result->defaultReorderCodesLength = 0;
    result->freeDefaultReorderCodesOnClose = FALSE;
    result->reorderCodes = NULL;
    result->reorderCodesLength = 0;
    result->freeReorderCodesOnClose = FALSE;
    result->leadBytePermutationTable = NULL;
    result->freeLeadBytePermutationTableOnClose = FALSE;

    
    result->dataVersion[0] = result->image->version[0]; 
    result->dataVersion[1] = result->image->version[1]; 
    result->dataVersion[2] = 0;
    result->dataVersion[3] = 0;

    result->unsafeCP = (uint8_t *)result->image + result->image->unsafeCP;
    result->minUnsafeCP = 0;
    for (c=0; c<0x300; c++) {  
        if (ucol_unsafeCP(c, result)) break;
    }
    result->minUnsafeCP = c;

    result->contrEndCP = (uint8_t *)result->image + result->image->contrEndCP;
    result->minContrEndCP = 0;
    for (c=0; c<0x300; c++) {  
        if (ucol_contractionEndCP(c, result)) break;
    }
    result->minContrEndCP = c;

    
    result->endExpansionCE = (uint32_t*)((uint8_t*)result->image +
                                         result->image->endExpansionCE);
    result->lastEndExpansionCE = result->endExpansionCE +
                                 result->image->endExpansionCECount - 1;
    result->expansionCESize = (uint8_t*)result->image +
                                               result->image->expansionCESize;


    

    result->latinOneCEs = NULL;

    result->latinOneRegenTable = FALSE;
    result->latinOneFailed = FALSE;
    result->UCA = UCA;

    
    result->ucaRules = NULL;
    result->actualLocale = NULL;
    result->validLocale = NULL;
    result->requestedLocale = NULL;
    result->hasRealData = FALSE; 
    result->freeImageOnClose = FALSE;

    
    ucol_setOptionsFromHeader(
        result,
        (UColOptionSet*)((uint8_t*)result->image+result->image->options),
        status);
    result->freeOptionsOnClose = FALSE;

    return result;
}
















































static const UChar32
    NON_CJK_OFFSET = 0x110000,
    UCOL_MAX_INPUT = 0x220001; 




static int32_t
    final3Multiplier = 0,
    final4Multiplier = 0,
    final3Count = 0,
    final4Count = 0,
    medialCount = 0,
    min3Primary = 0,
    min4Primary = 0,
    max4Primary = 0,
    minTrail = 0,
    maxTrail = 0,
    max3Trail = 0,
    max4Trail = 0,
    min4Boundary = 0;

static const UChar32
    
    
    CJK_BASE = 0x4E00,
    CJK_LIMIT = 0x9FCC+1,
    
    CJK_COMPAT_USED_BASE = 0xFA0E,
    CJK_COMPAT_USED_LIMIT = 0xFA2F+1,
    
    
    CJK_A_BASE = 0x3400,
    CJK_A_LIMIT = 0x4DB5+1,
    
    
    CJK_B_BASE = 0x20000,
    CJK_B_LIMIT = 0x2A6D6+1,
    
    
    CJK_C_BASE = 0x2A700,
    CJK_C_LIMIT = 0x2B734+1,
    
    
    CJK_D_BASE = 0x2B740,
    CJK_D_LIMIT = 0x2B81D+1;
    
    

static UChar32 swapCJK(UChar32 i) {
    if (i < CJK_A_BASE) {
        
    } else if (i < CJK_A_LIMIT) {
        
        
        return i - CJK_A_BASE
                + (CJK_LIMIT - CJK_BASE)
                + (CJK_COMPAT_USED_LIMIT - CJK_COMPAT_USED_BASE);
    } else if (i < CJK_BASE) {
        
    } else if (i < CJK_LIMIT) {
        return i - CJK_BASE;
    } else if (i < CJK_COMPAT_USED_BASE) {
        
    } else if (i < CJK_COMPAT_USED_LIMIT) {
        return i - CJK_COMPAT_USED_BASE
                + (CJK_LIMIT - CJK_BASE);
    } else if (i < CJK_B_BASE) {
        
    } else if (i < CJK_B_LIMIT) {
        return i; 
    } else if (i < CJK_C_BASE) {
        
    } else if (i < CJK_C_LIMIT) {
        return i; 
    } else if (i < CJK_D_BASE) {
        
    } else if (i < CJK_D_LIMIT) {
        return i; 
    }
    return i + NON_CJK_OFFSET; 
}

U_CAPI UChar32 U_EXPORT2
uprv_uca_getRawFromCodePoint(UChar32 i) {
    return swapCJK(i)+1;
}

U_CAPI UChar32 U_EXPORT2
uprv_uca_getCodePointFromRaw(UChar32 i) {
    i--;
    UChar32 result = 0;
    if(i >= NON_CJK_OFFSET) {
        result = i - NON_CJK_OFFSET;
    } else if(i >= CJK_B_BASE) {
        result = i;
    } else if(i < CJK_A_LIMIT + (CJK_LIMIT - CJK_BASE) + (CJK_COMPAT_USED_LIMIT - CJK_COMPAT_USED_BASE)) { 
        if(i < CJK_LIMIT - CJK_BASE) {
            result = i + CJK_BASE;
        } else if(i < (CJK_LIMIT - CJK_BASE) + (CJK_COMPAT_USED_LIMIT - CJK_COMPAT_USED_BASE)) {
            result = i + CJK_COMPAT_USED_BASE - (CJK_LIMIT - CJK_BASE);
        } else {
            result = i + CJK_A_BASE - (CJK_LIMIT - CJK_BASE) - (CJK_COMPAT_USED_LIMIT - CJK_COMPAT_USED_BASE);
        }
    } else {
        result = -1;
    }
    return result;
}



U_CAPI uint32_t U_EXPORT2
uprv_uca_getImplicitFromRaw(UChar32 cp) {
    




    int32_t last0 = cp - min4Boundary;
    if (last0 < 0) {
        int32_t last1 = cp / final3Count;
        last0 = cp % final3Count;

        int32_t last2 = last1 / medialCount;
        last1 %= medialCount;

        last0 = minTrail + last0*final3Multiplier; 
        last1 = minTrail + last1; 
        last2 = min3Primary + last2; 
        




        return (last2 << 24) + (last1 << 16) + (last0 << 8);
    } else {
        int32_t last1 = last0 / final4Count;
        last0 %= final4Count;

        int32_t last2 = last1 / medialCount;
        last1 %= medialCount;

        int32_t last3 = last2 / medialCount;
        last2 %= medialCount;

        last0 = minTrail + last0*final4Multiplier; 
        last1 = minTrail + last1; 
        last2 = minTrail + last2; 
        last3 = min4Primary + last3; 
        




        return (last3 << 24) + (last2 << 16) + (last1 << 8) + last0;
    }
}

static uint32_t U_EXPORT2
uprv_uca_getImplicitPrimary(UChar32 cp) {
   
    

    cp = swapCJK(cp);
    cp++;
    

    
    

    return uprv_uca_getImplicitFromRaw(cp);
}






U_CAPI UChar32 U_EXPORT2
uprv_uca_getRawFromImplicit(uint32_t implicit) {
    UChar32 result;
    UChar32 b3 = implicit & 0xFF;
    UChar32 b2 = (implicit >> 8) & 0xFF;
    UChar32 b1 = (implicit >> 16) & 0xFF;
    UChar32 b0 = (implicit >> 24) & 0xFF;

    
    if (b0 < min3Primary || b0 > max4Primary
        || b1 < minTrail || b1 > maxTrail)
        return -1;
    
    b1 -= minTrail;

    
    if (b0 < min4Primary) {
        if (b2 < minTrail || b2 > max3Trail || b3 != 0)
            return -1;
        b2 -= minTrail;
        UChar32 remainder = b2 % final3Multiplier;
        if (remainder != 0)
            return -1;
        b0 -= min3Primary;
        b2 /= final3Multiplier;
        result = ((b0 * medialCount) + b1) * final3Count + b2;
    } else {
        if (b2 < minTrail || b2 > maxTrail
            || b3 < minTrail || b3 > max4Trail)
            return -1;
        b2 -= minTrail;
        b3 -= minTrail;
        UChar32 remainder = b3 % final4Multiplier;
        if (remainder != 0)
            return -1;
        b3 /= final4Multiplier;
        b0 -= min4Primary;
        result = (((b0 * medialCount) + b1) * medialCount + b2) * final4Count + b3 + min4Boundary;
    }
    
    if (result < 0 || result > UCOL_MAX_INPUT)
        return -1;
    return result;
}


static inline int32_t divideAndRoundUp(int a, int b) {
    return 1 + (a-1)/b;
}


















static void initImplicitConstants(int minPrimary, int maxPrimary,
                                    int minTrailIn, int maxTrailIn,
                                    int gap3, int primaries3count,
                                    UErrorCode *status) {
    
    if ((minPrimary < 0 || minPrimary >= maxPrimary || maxPrimary > 0xFF)
        || (minTrailIn < 0 || minTrailIn >= maxTrailIn || maxTrailIn > 0xFF)
        || (primaries3count < 1))
    {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    };

    minTrail = minTrailIn;
    maxTrail = maxTrailIn;

    min3Primary = minPrimary;
    max4Primary = maxPrimary;
    
    
    
    
    
    
    final3Multiplier = gap3 + 1;
    final3Count = (maxTrail - minTrail + 1) / final3Multiplier;
    max3Trail = minTrail + (final3Count - 1) * final3Multiplier;

    
    medialCount = (maxTrail - minTrail + 1);
    
    int32_t threeByteCount = medialCount * final3Count;
    
    
    int32_t primariesAvailable = maxPrimary - minPrimary + 1;
    int32_t primaries4count = primariesAvailable - primaries3count;


    int32_t min3ByteCoverage = primaries3count * threeByteCount;
    min4Primary = minPrimary + primaries3count;
    min4Boundary = min3ByteCoverage;
    

    int32_t totalNeeded = UCOL_MAX_INPUT - min4Boundary;
    int32_t neededPerPrimaryByte = divideAndRoundUp(totalNeeded, primaries4count);
    int32_t neededPerFinalByte = divideAndRoundUp(neededPerPrimaryByte, medialCount * medialCount);
    int32_t gap4 = (maxTrail - minTrail - 1) / neededPerFinalByte;
    if (gap4 < 1) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    final4Multiplier = gap4 + 1;
    final4Count = neededPerFinalByte;
    max4Trail = minTrail + (final4Count - 1) * final4Multiplier;
}

    


U_CAPI void U_EXPORT2
uprv_uca_initImplicitConstants(UErrorCode *status) {
    
    
    initImplicitConstants(minImplicitPrimary, maxImplicitPrimary, 0x04, 0xFE, 1, 1, status);
}







static
void collIterNormalize(collIterate *collationSource)
{
    UErrorCode  status = U_ZERO_ERROR;
    const UChar *srcP = collationSource->pos - 1;      
    const UChar *endP = collationSource->fcdPosition;  

    collationSource->nfd->normalize(UnicodeString(FALSE, srcP, (int32_t)(endP - srcP)),
                                    collationSource->writableBuffer,
                                    status);
    if (U_FAILURE(status)) {
#ifdef UCOL_DEBUG
        fprintf(stderr, "collIterNormalize(), NFD failed, status = %s\n", u_errorName(status));
#endif
        return;
    }

    collationSource->pos        = collationSource->writableBuffer.getTerminatedBuffer();
    collationSource->origFlags  = collationSource->flags;
    collationSource->flags     |= UCOL_ITER_INNORMBUF;
    collationSource->flags     &= ~(UCOL_ITER_NORM | UCOL_ITER_HASLEN | UCOL_USE_ITERATOR);
}





















































static
inline UBool collIterFCD(collIterate *collationSource) {
    const UChar *srcP, *endP;
    uint8_t     leadingCC;
    uint8_t     prevTrailingCC = 0;
    uint16_t    fcd;
    UBool       needNormalize = FALSE;

    srcP = collationSource->pos-1;

    if (collationSource->flags & UCOL_ITER_HASLEN) {
        endP = collationSource->endp;
    } else {
        endP = NULL;
    }

    
    fcd = g_nfcImpl->nextFCD16(srcP, endP);
    if (fcd != 0) {
        prevTrailingCC = (uint8_t)(fcd & LAST_BYTE_MASK_);

        if (prevTrailingCC != 0) {
            
            
            while (endP == NULL || srcP != endP)
            {
                const UChar *savedSrcP = srcP;

                fcd = g_nfcImpl->nextFCD16(srcP, endP);
                leadingCC = (uint8_t)(fcd >> SECOND_LAST_BYTE_SHIFT_);
                if (leadingCC == 0) {
                    srcP = savedSrcP;      
                                           
                    break;
                }

                if (leadingCC < prevTrailingCC) {
                    needNormalize = TRUE;
                }

                prevTrailingCC = (uint8_t)(fcd & LAST_BYTE_MASK_);
            }
        }
    }

    collationSource->fcdPosition = (UChar *)srcP;

    return needNormalize;
}






static uint32_t getImplicit(UChar32 cp, collIterate *collationSource);
static uint32_t getPrevImplicit(UChar32 cp, collIterate *collationSource);





static
inline uint32_t ucol_IGetNextCE(const UCollator *coll, collIterate *collationSource, UErrorCode *status) {
    uint32_t order = 0;
    if (collationSource->CEpos > collationSource->toReturn) {       
        order = *(collationSource->toReturn++);                         
        if(collationSource->CEpos == collationSource->toReturn) {
            collationSource->CEpos = collationSource->toReturn = collationSource->extendCEs ? collationSource->extendCEs : collationSource->CEs;
        }
        return order;
    }

    UChar ch = 0;
    collationSource->offsetReturn = NULL;

    do {
        for (;;)                           
        {                                  
            

            if ((collationSource->flags & (UCOL_ITER_HASLEN | UCOL_ITER_INNORMBUF | UCOL_ITER_NORM | UCOL_HIRAGANA_Q | UCOL_USE_ITERATOR)) == 0)
            {
                
                
                
                ch = *collationSource->pos++;
                if (ch != 0) {
                    break;
                }
                else {
                    return UCOL_NO_MORE_CES;
                }
            }

            if (collationSource->flags & UCOL_ITER_HASLEN) {
                
                
                if (collationSource->pos >= collationSource->endp) {
                    
                    return UCOL_NO_MORE_CES;
                }
                ch = *collationSource->pos++;
            }
            else if(collationSource->flags & UCOL_USE_ITERATOR) {
                UChar32 iterCh = collationSource->iterator->next(collationSource->iterator);
                if(iterCh == U_SENTINEL) {
                    return UCOL_NO_MORE_CES;
                }
                ch = (UChar)iterCh;
            }
            else
            {
                
                ch = *collationSource->pos++;
                if (ch == 0) {
                    
                    if ((collationSource->flags & UCOL_ITER_INNORMBUF) == 0) {
                        
                        collationSource->pos--;
                        return UCOL_NO_MORE_CES;
                    }
                    else
                    {
                        
                        
                        
                        
                        if (collationSource->pos == collationSource->writableBuffer.getBuffer()+1) {
                            break;
                        }

                        
                        
                        
                        collationSource->pos   = collationSource->fcdPosition;
                        collationSource->flags = collationSource->origFlags;
                        continue;
                    }
                }
            }

            if(collationSource->flags&UCOL_HIRAGANA_Q) {
                


                if(((ch>=0x3040 && ch<=0x3096) || (ch >= 0x309d && ch <= 0x309f)) ||
                        ((collationSource->flags & UCOL_WAS_HIRAGANA) && (ch >= 0x3099 && ch <= 0x309C))) {
                    collationSource->flags |= UCOL_WAS_HIRAGANA;
                } else {
                    collationSource->flags &= ~UCOL_WAS_HIRAGANA;
                }
            }

            
            
            if ((collationSource->flags & UCOL_ITER_NORM) == 0) {
                break;
            }

            if (collationSource->fcdPosition >= collationSource->pos) {
                
                
                break;
            }

            if (ch < ZERO_CC_LIMIT_ ) {
                
                break;
            }

            if (ch < NFC_ZERO_CC_BLOCK_LIMIT_) {
                
                if ((collationSource->flags & UCOL_ITER_HASLEN) && collationSource->pos >= collationSource->endp) {
                    
                    
                    break;
                }

                
                if (*collationSource->pos < NFC_ZERO_CC_BLOCK_LIMIT_) {
                    break;
                }
            }


            
            if (collIterFCD(collationSource)) {
                collIterNormalize(collationSource);
            }
            if ((collationSource->flags & UCOL_ITER_INNORMBUF) == 0) {
                
                break;
            }

            
            

        }   


        if (ch <= 0xFF) {
            
            
            order = coll->latinOneMapping[ch];
            if (order > UCOL_NOT_FOUND) {
                order = ucol_prv_getSpecialCE(coll, ch, order, collationSource, status);
            }
        }
        else
        {
            
            
            
            if ((collationSource->flags & UCOL_FORCE_HAN_IMPLICIT) != 0 &&
                (ch >= UCOL_FIRST_HAN_A && ch <= UCOL_LAST_HANGUL)) {
                if (ch > UCOL_LAST_HAN && ch < UCOL_FIRST_HANGUL) {
                    
                    
                    
                    
                    
                    order = UTRIE_GET32_FROM_LEAD(&coll->mapping, ch);
                } else {
                    
                    order = UCOL_NOT_FOUND;
                }
            } else {
                order = UTRIE_GET32_FROM_LEAD(&coll->mapping, ch);
            }

            if(order > UCOL_NOT_FOUND) {                                       
                order = ucol_prv_getSpecialCE(coll, ch, order, collationSource, status);    
            }

            if(order == UCOL_NOT_FOUND && coll->UCA) {   
                
                order = UTRIE_GET32_FROM_LEAD(&coll->UCA->mapping, ch);

                if(order > UCOL_NOT_FOUND) { 
                    order = ucol_prv_getSpecialCE(coll->UCA, ch, order, collationSource, status);
                }
            }
        }
    } while ( order == UCOL_IGNORABLE && ch >= UCOL_FIRST_HANGUL && ch <= UCOL_LAST_HANGUL );

    if(order == UCOL_NOT_FOUND) {
        order = getImplicit(ch, collationSource);
    }
    return order; 
}


U_CAPI uint32_t  U_EXPORT2
ucol_getNextCE(const UCollator *coll, collIterate *collationSource, UErrorCode *status) {
    return ucol_IGetNextCE(coll, collationSource, status);
}








static
void collPrevIterNormalize(collIterate *data)
{
    UErrorCode status  = U_ZERO_ERROR;
    const UChar *pEnd   = data->pos;  
    const UChar *pStart;

    
    if (data->fcdPosition == NULL) {
        pStart = data->string;
    }
    else {
        pStart = data->fcdPosition + 1;
    }

    int32_t normLen =
        data->nfd->normalize(UnicodeString(FALSE, pStart, (int32_t)((pEnd - pStart) + 1)),
                             data->writableBuffer,
                             status).
        length();
    if(U_FAILURE(status)) {
        return;
    }
    



    data->writableBuffer.insert(0, (UChar)0);

    














    int32_t firstMarkOffset = 0;
    int32_t trailOffset     = (int32_t)(data->pos - data->string + 1);
    int32_t trailCount      = normLen - 1;

    if (data->fcdPosition != NULL) {
        int32_t baseOffset = (int32_t)(data->fcdPosition - data->string);
        UChar   baseChar   = *data->fcdPosition;

        firstMarkOffset = baseOffset + 1;

        






        if (baseChar >= 0x100) {
            uint32_t baseOrder = UTRIE_GET32_FROM_LEAD(&data->coll->mapping, baseChar);

            if (baseOrder == UCOL_NOT_FOUND && data->coll->UCA) {
                baseOrder = UTRIE_GET32_FROM_LEAD(&data->coll->UCA->mapping, baseChar);
            }

            if (baseOrder > UCOL_NOT_FOUND && getCETag(baseOrder) == CONTRACTION_TAG) {
                firstMarkOffset = trailOffset;
            }
        }

        data->appendOffset(baseOffset, status);
    }

    data->appendOffset(firstMarkOffset, status);

    for (int32_t i = 0; i < trailCount; i += 1) {
        data->appendOffset(trailOffset, status);
    }

    data->offsetRepeatValue = trailOffset;

    data->offsetReturn = data->offsetStore - 1;
    if (data->offsetReturn == data->offsetBuffer) {
        data->offsetStore = data->offsetBuffer;
    }

    data->pos        = data->writableBuffer.getTerminatedBuffer() + 1 + normLen;
    data->origFlags  = data->flags;
    data->flags     |= UCOL_ITER_INNORMBUF;
    data->flags     &= ~(UCOL_ITER_NORM | UCOL_ITER_HASLEN);
}

















static
inline UBool collPrevIterFCD(collIterate *data)
{
    const UChar *src, *start;
    uint8_t     leadingCC;
    uint8_t     trailingCC = 0;
    uint16_t    fcd;
    UBool       result = FALSE;

    start = data->string;
    src = data->pos + 1;

    
    fcd = g_nfcImpl->previousFCD16(start, src);

    leadingCC = (uint8_t)(fcd >> SECOND_LAST_BYTE_SHIFT_);

    if (leadingCC != 0) {
        



        for (;;)
        {
            if (start == src) {
                data->fcdPosition = NULL;
                return result;
            }

            fcd = g_nfcImpl->previousFCD16(start, src);

            trailingCC = (uint8_t)(fcd & LAST_BYTE_MASK_);

            if (trailingCC == 0) {
                break;
            }

            if (leadingCC < trailingCC) {
                result = TRUE;
            }

            leadingCC = (uint8_t)(fcd >> SECOND_LAST_BYTE_SHIFT_);
        }
    }

    data->fcdPosition = (UChar *)src;

    return result;
}





static inline
UChar peekCodeUnit(collIterate *source, int32_t offset) {
    if(source->pos != NULL) {
        return *(source->pos + offset);
    } else if(source->iterator != NULL) {
        UChar32 c;
        if(offset != 0) {
            source->iterator->move(source->iterator, offset, UITER_CURRENT);
            c = source->iterator->next(source->iterator);
            source->iterator->move(source->iterator, -offset-1, UITER_CURRENT);
        } else {
            c = source->iterator->current(source->iterator);
        }
        return c >= 0 ? (UChar)c : 0xfffd;  
    } else {
        return 0xfffd;
    }
}




static inline
UChar32 peekCodePoint(collIterate *source, int32_t offset) {
    UChar32 c;
    if(source->pos != NULL) {
        const UChar *p = source->pos;
        if(offset >= 0) {
            
            while(--offset >= 0) {
                if(U16_IS_LEAD(*p++) && U16_IS_TRAIL(*p)) {
                    ++p;
                }
            }
            
            c = *p++;
            UChar trail;
            if(U16_IS_LEAD(c) && U16_IS_TRAIL(trail = *p)) {
                c = U16_GET_SUPPLEMENTARY(c, trail);
            }
        } else  {
            
            while(++offset < 0) {
                if(U16_IS_TRAIL(*--p) && U16_IS_LEAD(*(p - 1))) {
                    --p;
                }
            }
            
            c = *--p;
            UChar lead;
            if(U16_IS_TRAIL(c) && U16_IS_LEAD(lead = *(p - 1))) {
                c = U16_GET_SUPPLEMENTARY(lead, c);
            }
        }
    } else if(source->iterator != NULL) {
        if(offset >= 0) {
            
            int32_t fwd = offset;
            while(fwd-- > 0) {
                uiter_next32(source->iterator);
            }
            
            c = uiter_current32(source->iterator);
            
            while(offset-- > 0) {
                uiter_previous32(source->iterator);
            }
        } else  {
            
            int32_t back = offset;
            do {
                c = uiter_previous32(source->iterator);
            } while(++back < 0);
            
            do {
                uiter_next32(source->iterator);
            } while(++offset < 0);
        }
    } else {
        c = U_SENTINEL;
    }
    return c;
}







static
inline UBool isAtStartPrevIterate(collIterate *data) {
    if(data->pos == NULL && data->iterator != NULL) {
        return !data->iterator->hasPrevious(data->iterator);
    }
    
    return (data->pos == data->string) ||
              ((data->flags & UCOL_ITER_INNORMBUF) && (data->pos != NULL) &&
              *(data->pos - 1) == 0 && data->fcdPosition == NULL);
}

static
inline void goBackOne(collIterate *data) {
# if 0
    
    
    if(data->pos) {
        data->pos--;
    }
    if(data->iterator) {
        data->iterator->previous(data->iterator);
    }
#endif
    if(data->iterator && (data->flags & UCOL_USE_ITERATOR)) {
        data->iterator->previous(data->iterator);
    }
    if(data->pos) {
        data->pos --;
    }
}












static
inline uint32_t ucol_IGetPrevCE(const UCollator *coll, collIterate *data,
                               UErrorCode *status)
{
    uint32_t result = (uint32_t)UCOL_NULLORDER;

    if (data->offsetReturn != NULL) {
        if (data->offsetRepeatCount > 0) {
                data->offsetRepeatCount -= 1;
        } else {
            if (data->offsetReturn == data->offsetBuffer) {
                data->offsetReturn = NULL;
                data->offsetStore  = data->offsetBuffer;
            } else {
                data->offsetReturn -= 1;
            }
        }
    }

    if ((data->extendCEs && data->toReturn > data->extendCEs) ||
            (!data->extendCEs && data->toReturn > data->CEs))
    {
        data->toReturn -= 1;
        result = *(data->toReturn);
        if (data->CEs == data->toReturn || data->extendCEs == data->toReturn) {
            data->CEpos = data->toReturn;
        }
    }
    else {
        UChar ch = 0;

        do {
            




            for (;;) {
                if (data->flags & UCOL_ITER_HASLEN) {
                    



                    if (data->pos <= data->string) {
                        
                        return UCOL_NO_MORE_CES;
                    }
                    data->pos --;
                    ch = *data->pos;
                }
                
                else if (data->flags & UCOL_USE_ITERATOR) {
                  UChar32 iterCh = data->iterator->previous(data->iterator);
                  if(iterCh == U_SENTINEL) {
                    return UCOL_NO_MORE_CES;
                  } else {
                    ch = (UChar)iterCh;
                  }
                }
                else {
                    data->pos --;
                    ch = *data->pos;
                    
                    if (ch == 0) {
                        





                        data->flags = data->origFlags;
                        data->offsetRepeatValue = 0;

                         if (data->fcdPosition == NULL) {
                            data->pos = data->string;
                            return UCOL_NO_MORE_CES;
                        }
                        else {
                            data->pos   = data->fcdPosition + 1;
                        }

                       continue;
                    }
                }

                if(data->flags&UCOL_HIRAGANA_Q) {
                  if(ch>=0x3040 && ch<=0x309f) {
                    data->flags |= UCOL_WAS_HIRAGANA;
                  } else {
                    data->flags &= ~UCOL_WAS_HIRAGANA;
                  }
                }

                







                if (ch < ZERO_CC_LIMIT_ ||
                  
                    (data->flags & UCOL_ITER_NORM) == 0 ||
                    (data->fcdPosition != NULL && data->fcdPosition <= data->pos)
                    || data->string == data->pos) {
                    break;
                }

                if (ch < NFC_ZERO_CC_BLOCK_LIMIT_) {
                    
                    if (data->pos == data->string) {
                        
                        break;
                    }

                    
                    if (*(data->pos - 1) < NFC_ZERO_CC_BLOCK_LIMIT_) {
                        break;
                    }
                }

                
                if (collPrevIterFCD(data)) {
                    collPrevIterNormalize(data);
                }

                if ((data->flags & UCOL_ITER_INNORMBUF) == 0) {
                    
                    break;
                }

                



            }

            


            if (ucol_contractionEndCP(ch, coll) && !isAtStartPrevIterate(data)) {
                result = ucol_prv_getSpecialPrevCE(coll, ch, UCOL_CONTRACTION, data, status);
            } else {
                if (ch <= 0xFF) {
                    result = coll->latinOneMapping[ch];
                }
                else {
                    
                    
                    if ((data->flags & UCOL_FORCE_HAN_IMPLICIT) != 0 &&
                        (ch >= 0x3400 && ch <= 0xD7AF)) {
                        if (ch > 0x9FFF && ch < 0xAC00) {
                            
                            
                            
                            
                            
                             result = UTRIE_GET32_FROM_LEAD(&coll->mapping, ch);
                        } else {
                            result = UCOL_NOT_FOUND;
                        }
                    } else {
                        result = UTRIE_GET32_FROM_LEAD(&coll->mapping, ch);
                    }
                }
                if (result > UCOL_NOT_FOUND) {
                    result = ucol_prv_getSpecialPrevCE(coll, ch, result, data, status);
                }
                if (result == UCOL_NOT_FOUND) { 
                    if (!isAtStartPrevIterate(data) &&
                        ucol_contractionEndCP(ch, data->coll))
                    {
                        result = UCOL_CONTRACTION;
                    } else {
                        if(coll->UCA) {
                            result = UTRIE_GET32_FROM_LEAD(&coll->UCA->mapping, ch);
                        }
                    }

                    if (result > UCOL_NOT_FOUND) {
                        if(coll->UCA) {
                            result = ucol_prv_getSpecialPrevCE(coll->UCA, ch, result, data, status);
                        }
                    }
                }
            }
        } while ( result == UCOL_IGNORABLE && ch >= UCOL_FIRST_HANGUL && ch <= UCOL_LAST_HANGUL );

        if(result == UCOL_NOT_FOUND) {
            result = getPrevImplicit(ch, data);
        }
    }

    return result;
}



U_CFUNC uint32_t  U_EXPORT2
ucol_getPrevCE(const UCollator *coll, collIterate *data,
                        UErrorCode *status) {
    return ucol_IGetPrevCE(coll, data, status);
}



U_CFUNC uint32_t  U_EXPORT2
ucol_getFirstCE(const UCollator *coll, UChar u, UErrorCode *status) {
    collIterate colIt;
    IInit_collIterate(coll, &u, 1, &colIt, status);
    if(U_FAILURE(*status)) {
        return 0;
    }
    return ucol_IGetNextCE(coll, &colIt, status);
}








static
inline const UChar * insertBufferEnd(collIterate *data, UChar ch)
{
    int32_t oldLength = data->writableBuffer.length();
    return data->writableBuffer.append(ch).getTerminatedBuffer() + oldLength;
}









static
inline const UChar * insertBufferEnd(collIterate *data, const UChar *str, int32_t length)
{
    int32_t oldLength = data->writableBuffer.length();
    return data->writableBuffer.append(str, length).getTerminatedBuffer() + oldLength;
}










static
inline void normalizeNextContraction(collIterate *data)
{
    int32_t     strsize;
    UErrorCode  status     = U_ZERO_ERROR;
    
    const UChar *pStart    = data->pos - 1;
    const UChar *pEnd;

    if ((data->flags & UCOL_ITER_INNORMBUF) == 0) {
        data->writableBuffer.setTo(*(pStart - 1));
        strsize               = 1;
    }
    else {
        strsize = data->writableBuffer.length();
    }

    pEnd = data->fcdPosition;

    data->writableBuffer.append(
        data->nfd->normalize(UnicodeString(FALSE, pStart, (int32_t)(pEnd - pStart)), status));
    if(U_FAILURE(status)) {
        return;
    }

    data->pos        = data->writableBuffer.getTerminatedBuffer() + strsize;
    data->origFlags  = data->flags;
    data->flags     |= UCOL_ITER_INNORMBUF;
    data->flags     &= ~(UCOL_ITER_NORM | UCOL_ITER_HASLEN);
}













static
inline UChar getNextNormalizedChar(collIterate *data)
{
    UChar  nextch;
    UChar  ch;
    
    
    
    
    if ((data->flags & (UCOL_ITER_NORM | UCOL_ITER_INNORMBUF)) == 0 ) {
         
      if(data->flags & UCOL_USE_ITERATOR) {
         return (UChar)data->iterator->next(data->iterator);
      } else {
         return *(data->pos ++);
      }
    }

    
      
    

    UBool  innormbuf = (UBool)(data->flags & UCOL_ITER_INNORMBUF);
    if ((innormbuf && *data->pos != 0) ||
        (data->fcdPosition != NULL && !innormbuf &&
        data->pos < data->fcdPosition)) {
        



        return *(data->pos ++);
    }

    if (data->flags & UCOL_ITER_HASLEN) {
        
        if (data->pos + 1 == data->endp) {
            return *(data->pos ++);
        }
    }
    else {
        if (innormbuf) {
          
          
          
          
          
            
            
            
            
            
          
            



          if(data->fcdPosition) {
            if (*(data->fcdPosition + 1) == 0 ||
                data->fcdPosition + 1 == data->endp) {
                
                data->pos = insertBufferEnd(data, *(data->fcdPosition)) + 1;
                
                if (data->pos == NULL) {
                    return (UChar)-1; 
                }
                return *(data->fcdPosition ++);
            }
            data->pos = data->fcdPosition;
          } else if(data->origFlags & UCOL_USE_ITERATOR) {
            
            
            data->flags = data->origFlags;
            data->pos = NULL;
            return (UChar)data->iterator->next(data->iterator);
          }
          
        }
        else {
            if (*(data->pos + 1) == 0) {
                return *(data->pos ++);
            }
        }
    }

    ch = *data->pos ++;
    nextch = *data->pos;

    



    if ((data->fcdPosition == NULL || data->fcdPosition < data->pos) &&
        (nextch >= NFC_ZERO_CC_BLOCK_LIMIT_ ||
         ch >= NFC_ZERO_CC_BLOCK_LIMIT_)) {
            



        if (collIterFCD(data)) {
            normalizeNextContraction(data);
            return *(data->pos ++);
        }
        else if (innormbuf) {
            


            int32_t length = (int32_t)(data->fcdPosition - data->pos + 1);
            data->pos = insertBufferEnd(data, data->pos - 1, length);
            
            if (data->pos == NULL) {
                return (UChar)-1; 
            }
            return *(data->pos ++);
        }
    }

    if (innormbuf) {
        



        data->pos = insertBufferEnd(data, ch) + 1;
        
        if (data->pos == NULL) {
            return (UChar)-1; 
        }
    }

    
    return ch;
}









static
inline void setDiscontiguosAttribute(collIterate *source, const UnicodeString &buffer)
{
    








    if (source->flags & UCOL_ITER_INNORMBUF) {
        int32_t replaceLength = source->pos - source->writableBuffer.getBuffer();
        source->writableBuffer.replace(0, replaceLength, buffer);
    }
    else {
        source->fcdPosition  = source->pos;
        source->origFlags    = source->flags;
        source->flags       |= UCOL_ITER_INNORMBUF;
        source->flags       &= ~(UCOL_ITER_NORM | UCOL_ITER_HASLEN | UCOL_USE_ITERATOR);
        source->writableBuffer = buffer;
    }

    source->pos = source->writableBuffer.getTerminatedBuffer();
}









static
uint32_t getDiscontiguous(const UCollator *coll, collIterate *source,
                                const UChar *constart)
{
    

          const UChar *temppos      = source->pos;
          UnicodeString buffer;
    const UChar   *tempconstart = constart;
          uint8_t  tempflags    = source->flags;
          UBool    multicontraction = FALSE;
          collIterateState discState;

          backupState(source, &discState);

    buffer.setTo(peekCodePoint(source, -1));
    for (;;) {
        UChar    *UCharOffset;
        UChar     schar,
                  tchar;
        uint32_t  result;

        if (((source->flags & UCOL_ITER_HASLEN) && source->pos >= source->endp)
            || (peekCodeUnit(source, 0) == 0  &&
            
                ((source->flags & UCOL_ITER_INNORMBUF) == 0 ||
                 source->fcdPosition == NULL ||
                 source->fcdPosition == source->endp ||
                 *(source->fcdPosition) == 0 ||
                 u_getCombiningClass(*(source->fcdPosition)) == 0)) ||
                 


                 u_getCombiningClass(peekCodePoint(source, 0)) == 0) {
                 
            
            if (multicontraction) {
                source->pos    = temppos - 1;
                setDiscontiguosAttribute(source, buffer);
                return *(coll->contractionCEs +
                                    (tempconstart - coll->contractionIndex));
            }
            constart = tempconstart;
            break;
        }

        UCharOffset = (UChar *)(tempconstart + 1); 
        schar = getNextNormalizedChar(source);

        while (schar > (tchar = *UCharOffset)) {
            UCharOffset++;
        }

        if (schar != tchar) {
            

            buffer.append(schar);
            continue;
        }
        else {
            if (u_getCombiningClass(schar) ==
                u_getCombiningClass(peekCodePoint(source, -2))) {
                buffer.append(schar);
                continue;
            }
            result = *(coll->contractionCEs +
                                      (UCharOffset - coll->contractionIndex));
        }

        if (result == UCOL_NOT_FOUND) {
          break;
        } else if (isContraction(result)) {
            
            tempconstart = (UChar *)coll->image + getContractOffset(result);
            if (*(coll->contractionCEs + (constart - coll->contractionIndex))
                != UCOL_NOT_FOUND) {
                multicontraction = TRUE;
                temppos       = source->pos + 1;
            }
        } else {
            setDiscontiguosAttribute(source, buffer);
            return result;
        }
    }

    






    loadState(source, &discState, TRUE);
    goBackOne(source);

    
    source->flags = tempflags;
    return *(coll->contractionCEs + (constart - coll->contractionIndex));
}


static
inline uint32_t getImplicit(UChar32 cp, collIterate *collationSource) {
    uint32_t r = uprv_uca_getImplicitPrimary(cp);
    *(collationSource->CEpos++) = ((r & 0x0000FFFF)<<16) | 0x000000C0;
    collationSource->offsetRepeatCount += 1;
    return (r & UCOL_PRIMARYMASK) | 0x00000505; 
}







static
inline void insertBufferFront(collIterate *data, UChar ch)
{
    data->pos = data->writableBuffer.setCharAt(0, ch).insert(0, (UChar)0).getTerminatedBuffer() + 2;
}










static
inline void normalizePrevContraction(collIterate *data, UErrorCode *status)
{
    const UChar *pEnd = data->pos + 1;         
    const UChar *pStart;

    UnicodeString endOfBuffer;
    if (data->flags & UCOL_ITER_HASLEN) {
        



        endOfBuffer.setTo(*pEnd);
    }
    else {
        endOfBuffer.setTo(data->writableBuffer, 1);  
    }

    if (data->fcdPosition == NULL) {
        pStart = data->string;
    }
    else {
        pStart = data->fcdPosition + 1;
    }
    int32_t normLen =
        data->nfd->normalize(UnicodeString(FALSE, pStart, (int32_t)(pEnd - pStart)),
                             data->writableBuffer,
                             *status).
        length();
    if(U_FAILURE(*status)) {
        return;
    }
    



    data->pos =
        data->writableBuffer.insert(0, (UChar)0).append(endOfBuffer).getTerminatedBuffer() +
        1 + normLen;
    data->origFlags  = data->flags;
    data->flags     |= UCOL_ITER_INNORMBUF;
    data->flags     &= ~(UCOL_ITER_NORM | UCOL_ITER_HASLEN);
}














static
inline UChar getPrevNormalizedChar(collIterate *data, UErrorCode *status)
{
    UChar  prevch;
    UChar  ch;
    const UChar *start;
    UBool  innormbuf = (UBool)(data->flags & UCOL_ITER_INNORMBUF);
    if ((data->flags & (UCOL_ITER_NORM | UCOL_ITER_INNORMBUF)) == 0 ||
        (innormbuf && *(data->pos - 1) != 0)) {
        




      if(data->flags & UCOL_USE_ITERATOR) {
        data->iterator->move(data->iterator, -1, UITER_CURRENT);
        return (UChar)data->iterator->next(data->iterator);
      } else {
        return *(data->pos - 1);
      }
    }

    start = data->pos;
    if ((data->fcdPosition==NULL)||(data->flags & UCOL_ITER_HASLEN)) {
        
        if ((start - 1) == data->string) {
            return *(start - 1);
        }
        start --;
        ch     = *start;
        prevch = *(start - 1);
    }
    else {
        



        if (data->fcdPosition == data->string) {
            
            insertBufferFront(data, *(data->fcdPosition));
            data->fcdPosition = NULL;
            return *(data->pos - 1);
        }
        start  = data->fcdPosition;
        ch     = *start;
        prevch = *(start - 1);
    }
    



    if (data->fcdPosition > start &&
       (ch >= NFC_ZERO_CC_BLOCK_LIMIT_ || prevch >= NFC_ZERO_CC_BLOCK_LIMIT_))
    {
        



        const UChar *backuppos = data->pos;
        data->pos = start;
        if (collPrevIterFCD(data)) {
            normalizePrevContraction(data, status);
            return *(data->pos - 1);
        }
        data->pos = backuppos;
        data->fcdPosition ++;
    }

    if (innormbuf) {
    



        insertBufferFront(data, ch);
        data->fcdPosition --;
    }

    return ch;
}





#define UCOL_MAX_DIGITS_FOR_NUMBER 254

uint32_t ucol_prv_getSpecialCE(const UCollator *coll, UChar ch, uint32_t CE, collIterate *source, UErrorCode *status) {
    collIterateState entryState;
    backupState(source, &entryState);
    UChar32 cp = ch;

    for (;;) {
        
        
        
        

        const uint32_t *CEOffset = NULL;
        switch(getCETag(CE)) {
        case NOT_FOUND_TAG:
            
            return CE;
        case SPEC_PROC_TAG:
            {
                
                
                
                
                
                
                const UChar *UCharOffset;
                UChar schar, tchar;
                collIterateState prefixState;
                backupState(source, &prefixState);
                loadState(source, &entryState, TRUE);
                goBackOne(source); 
                

                for(;;) {
                    
                    

                    
                    const UChar *ContractionStart = UCharOffset = (UChar *)coll->image+getContractOffset(CE);
                    if (collIter_bos(source)) {
                        CE = *(coll->contractionCEs + (UCharOffset - coll->contractionIndex));
                        break;
                    }
                    schar = getPrevNormalizedChar(source, status);
                    goBackOne(source);

                    while(schar > (tchar = *UCharOffset)) { 
                        UCharOffset++;
                    }

                    if (schar == tchar) {
                        
                        
                        CE = *(coll->contractionCEs +
                            (UCharOffset - coll->contractionIndex));
                    }
                    else
                    {
                        
                        
                        CE = *(coll->contractionCEs +
                            (ContractionStart - coll->contractionIndex));
                    }

                    if(!isPrefix(CE)) {
                        
                        
                        
                        
                        
                        break;
                    }
                }
                if(CE != UCOL_NOT_FOUND) { 
                    loadState(source, &prefixState, TRUE);
                    if(source->origFlags & UCOL_USE_ITERATOR) {
                        source->flags = source->origFlags;
                    }
                } else { 
                    loadState(source, &entryState, TRUE);
                }
                break;
            }
        case CONTRACTION_TAG:
            {
                
                collIterateState state;
                backupState(source, &state);
                uint32_t firstCE = *(coll->contractionCEs + ((UChar *)coll->image+getContractOffset(CE) - coll->contractionIndex)); 
                const UChar *UCharOffset;
                UChar schar, tchar;

                for (;;) {
                    
                    

                    
                    const UChar *ContractionStart = UCharOffset = (UChar *)coll->image+getContractOffset(CE);

                    if (collIter_eos(source)) {
                        
                        CE = *(coll->contractionCEs + (UCharOffset - coll->contractionIndex));
                        
                        if (CE == UCOL_NOT_FOUND) {
                            
                            CE = firstCE;
                            loadState(source, &state, TRUE);
                            if(source->origFlags & UCOL_USE_ITERATOR) {
                                source->flags = source->origFlags;
                            }
                        }
                        break;
                    }

                    uint8_t maxCC = (uint8_t)(*(UCharOffset)&0xFF);  
                    uint8_t allSame = (uint8_t)(*(UCharOffset++)>>8);

                    schar = getNextNormalizedChar(source);
                    while(schar > (tchar = *UCharOffset)) { 
                        UCharOffset++;
                    }

                    if (schar == tchar) {
                        
                        
                        CE = *(coll->contractionCEs +
                            (UCharOffset - coll->contractionIndex));
                    }
                    else
                    {
                        
                        
                        
                        
                        
                        



                        UChar32 miss = schar;
                        if (source->iterator) {
                            UChar32 surrNextChar; 
                            int32_t prevPos; 
                            if(U16_IS_LEAD(schar) && source->iterator->hasNext(source->iterator)) {
                                prevPos = source->iterator->index;
                                surrNextChar = getNextNormalizedChar(source);
                                if (U16_IS_TRAIL(surrNextChar)) {
                                    miss = U16_GET_SUPPLEMENTARY(schar, surrNextChar);
                                } else if (prevPos < source->iterator->index){
                                    goBackOne(source);
                                }
                            }
                        } else if (U16_IS_LEAD(schar)) {
                            miss = U16_GET_SUPPLEMENTARY(schar, getNextNormalizedChar(source));
                        }

                        uint8_t sCC;
                        if (miss < 0x300 ||
                            maxCC == 0 ||
                            (sCC = i_getCombiningClass(miss, coll)) == 0 ||
                            sCC>maxCC ||
                            (allSame != 0 && sCC == maxCC) ||
                            collIter_eos(source))
                        {
                            
                            goBackOne(source);  
                            
                            
                            if(U_IS_SUPPLEMENTARY(miss)) {
                                goBackOne(source);
                            }
                            CE = *(coll->contractionCEs +
                                (ContractionStart - coll->contractionIndex));
                        } else {
                            
                            
                            
                            
                            UChar tempchar;
                            

                            tempchar = getNextNormalizedChar(source);
                            
                            goBackOne(source);
                            if (i_getCombiningClass(tempchar, coll) == 0) {
                                goBackOne(source);
                                if(U_IS_SUPPLEMENTARY(miss)) {
                                    goBackOne(source);
                                }
                                
                                CE = *(coll->contractionCEs +
                                    (ContractionStart - coll->contractionIndex));
                            } else {
                                CE = getDiscontiguous(coll, source, ContractionStart);
                            }
                        }
                    } 

                    if(CE == UCOL_NOT_FOUND) {
                        
                        
                        
                        loadState(source, &state, TRUE);
                        CE = firstCE;
                        break;
                    }

                    if(!isContraction(CE)) {
                        
                        
                        
                        
                        
                        break;
                    }


                    
                    
                    
                    uint32_t tempCE = *(coll->contractionCEs + (ContractionStart - coll->contractionIndex));
                    if(tempCE != UCOL_NOT_FOUND) {
                        
                        
                        
                        
                        firstCE = tempCE;

                        goBackOne(source);
                        backupState(source, &state);
                        getNextNormalizedChar(source);

                        
                        
                        
                        
                        
                        

                        
                        
                        
                        
                        
                    }
                } 
                break;
            } 
        case LONG_PRIMARY_TAG:
            {
                *(source->CEpos++) = ((CE & 0xFF)<<24)|UCOL_CONTINUATION_MARKER;
                CE = ((CE & 0xFFFF00) << 8) | (UCOL_BYTE_COMMON << 8) | UCOL_BYTE_COMMON;
                source->offsetRepeatCount += 1;
                return CE;
            }
        case EXPANSION_TAG:
            {
                
                
                
                uint32_t size;
                uint32_t i;    

                CEOffset = (uint32_t *)coll->image+getExpansionOffset(CE); 
                size = getExpansionCount(CE);
                CE = *CEOffset++;
              

                if(size != 0) { 
                    for(i = 1; i<size; i++) {
                        *(source->CEpos++) = *CEOffset++;
                        source->offsetRepeatCount += 1;
                    }
                } else { 
                    while(*CEOffset != 0) {
                        *(source->CEpos++) = *CEOffset++;
                        source->offsetRepeatCount += 1;
                    }
                }

                return CE;
            }
        case DIGIT_TAG:
            {
                



                
                uint32_t i;    

                if (source->coll->numericCollation == UCOL_ON){
                    collIterateState digitState = {0,0,0,0,0,0,0,0,0};
                    UChar32 char32 = 0;
                    int32_t digVal = 0;

                    uint32_t digIndx = 0;
                    uint32_t endIndex = 0;
                    uint32_t trailingZeroIndex = 0;

                    uint8_t collateVal = 0;

                    UBool nonZeroValReached = FALSE;

                    uint8_t numTempBuf[UCOL_MAX_DIGITS_FOR_NUMBER/2 + 3]; 
                    




            


















                    digVal = u_charDigitValue(cp); 
                    
                    
                    




                    digIndx++;
                    for(;;){
                        
                        
                        
                        

                        
                        if (digVal != 0) {
                            nonZeroValReached = TRUE;
                        }
                        if (nonZeroValReached) {
                            









                            if (digIndx % 2 == 1){
                                collateVal += (uint8_t)digVal;

                                
                                
                                if (collateVal != 0)
                                    trailingZeroIndex = 0;

                                numTempBuf[(digIndx/2) + 2] = collateVal*2 + 6;
                                collateVal = 0;
                            }
                            else{
                                
                                
                                
                                collateVal = (uint8_t)(digVal * 10);

                                
                                if (collateVal == 0)
                                {
                                    if (!trailingZeroIndex)
                                        trailingZeroIndex = (digIndx/2) + 2;
                                }
                                else
                                    trailingZeroIndex = 0;

                                numTempBuf[(digIndx/2) + 2] = collateVal*2 + 6;
                            }
                            digIndx++;
                        }

                        
                        if (!collIter_eos(source)){
                            ch = getNextNormalizedChar(source);
                            if (U16_IS_LEAD(ch)){
                                if (!collIter_eos(source)) {
                                    backupState(source, &digitState);
                                    UChar trail = getNextNormalizedChar(source);
                                    if(U16_IS_TRAIL(trail)) {
                                        char32 = U16_GET_SUPPLEMENTARY(ch, trail);
                                    } else {
                                        loadState(source, &digitState, TRUE);
                                        char32 = ch;
                                    }
                                }
                            } else {
                                char32 = ch;
                            }

                            if ((digVal = u_charDigitValue(char32)) == -1 || digIndx > UCOL_MAX_DIGITS_FOR_NUMBER){
                                
                                
                                if (char32 > 0xFFFF) { 
                                    loadState(source, &digitState, TRUE);
                                    
                                }
                                goBackOne(source);
                                break;
                            }
                        } else {
                            break;
                        }
                    }

                    if (nonZeroValReached == FALSE){
                        digIndx = 2;
                        numTempBuf[2] = 6;
                    }

                    endIndex = trailingZeroIndex ? trailingZeroIndex : ((digIndx/2) + 2) ;
                    if (digIndx % 2 != 0){
                        






                        for(i = 2; i < endIndex; i++){
                            numTempBuf[i] =     (((((numTempBuf[i] - 6)/2) % 10) * 10) +
                                (((numTempBuf[i+1])-6)/2) / 10) * 2 + 6;
                        }
                        --digIndx;
                    }

                    
                    numTempBuf[endIndex-1] -= 1;

                    




                    numTempBuf[0] = UCOL_CODAN_PLACEHOLDER;
                    numTempBuf[1] = (uint8_t)(0x80 + ((digIndx/2) & 0x7F));

                    
                    
                    
                    CE = (((numTempBuf[0] << 8) | numTempBuf[1]) << UCOL_PRIMARYORDERSHIFT) | 
                        (UCOL_BYTE_COMMON << UCOL_SECONDARYORDERSHIFT) | 
                        UCOL_BYTE_COMMON; 
                    i = 2; 
                    while(i < endIndex)
                    {
                        uint32_t primWeight = numTempBuf[i++] << 8;
                        if ( i < endIndex)
                            primWeight |= numTempBuf[i++];
                        *(source->CEpos++) = (primWeight << UCOL_PRIMARYORDERSHIFT) | UCOL_CONTINUATION_MARKER;
                    }

                } else {
                    
                    CEOffset = (uint32_t *)coll->image+getExpansionOffset(CE); 
                    CE = *CEOffset++;
                    break;
                }
                return CE;
            }
            
        case IMPLICIT_TAG:        
            
            return getImplicit(cp, source);
        case CJK_IMPLICIT_TAG:    
            
            return getImplicit(cp, source);
        case HANGUL_SYLLABLE_TAG: 
            {
                static const uint32_t
                    SBase = 0xAC00, LBase = 0x1100, VBase = 0x1161, TBase = 0x11A7;
                
                static const uint32_t VCount = 21;
                static const uint32_t TCount = 28;
                
                
                uint32_t L = ch - SBase;

                

                uint32_t T = L % TCount; 
                L /= TCount;
                uint32_t V = L % VCount;
                L /= VCount;

                

                L += LBase;
                V += VBase;
                T += TBase;

                
                if (!source->coll->image->jamoSpecial) { 

                    *(source->CEpos++) = UTRIE_GET32_FROM_LEAD(&coll->mapping, V);
                    if (T != TBase) {
                        *(source->CEpos++) = UTRIE_GET32_FROM_LEAD(&coll->mapping, T);
                    }

                    return UTRIE_GET32_FROM_LEAD(&coll->mapping, L);

                } else { 
                    
                    
                    

                    
                    
                    
                    
                    if(source->iterator != NULL && source->flags & UCOL_ITER_INNORMBUF) {
                        source->flags = source->origFlags; 
                        source->pos = NULL;
                    }

                    
                    UChar *buffer = source->writableBuffer.getBuffer(4);
                    int32_t bufferLength;
                    buffer[0] = (UChar)L;
                    buffer[1] = (UChar)V;
                    if (T != TBase) {
                        buffer[2] = (UChar)T;
                        bufferLength = 3;
                    } else {
                        bufferLength = 2;
                    }
                    source->writableBuffer.releaseBuffer(bufferLength);

                    
                    source->fcdPosition       = source->pos;

                    source->pos   = source->writableBuffer.getTerminatedBuffer();
                    source->origFlags   = source->flags;
                    source->flags       |= UCOL_ITER_INNORMBUF;
                    source->flags       &= ~(UCOL_ITER_NORM | UCOL_ITER_HASLEN);

                    return(UCOL_IGNORABLE);
                }
            }
        case SURROGATE_TAG:
            
            
            
            
            {
                UChar trail;
                collIterateState state;
                backupState(source, &state);
                if (collIter_eos(source) || !(U16_IS_TRAIL((trail = getNextNormalizedChar(source))))) {
                    
                    
                    loadState(source, &state, TRUE);
                    return UCOL_NOT_FOUND;
                } else {
                    
                    CE = UTRIE_GET32_FROM_OFFSET_TRAIL(&coll->mapping, CE&0xFFFFFF, trail);
                    if(CE == UCOL_NOT_FOUND) { 
                        
                        loadState(source, &state, TRUE);
                        return CE;
                    }
                    
                    cp = ((((uint32_t)ch)<<10UL)+(trail)-(((uint32_t)0xd800<<10UL)+0xdc00-0x10000));
                }
            }
            break;
        case LEAD_SURROGATE_TAG:  
            UChar nextChar;
            if( source->flags & UCOL_USE_ITERATOR) {
                if(U_IS_TRAIL(nextChar = (UChar)source->iterator->current(source->iterator))) {
                    cp = U16_GET_SUPPLEMENTARY(ch, nextChar);
                    source->iterator->next(source->iterator);
                    return getImplicit(cp, source);
                }
            } else if((((source->flags & UCOL_ITER_HASLEN) == 0 ) || (source->pos<source->endp)) &&
                      U_IS_TRAIL((nextChar=*source->pos))) {
                cp = U16_GET_SUPPLEMENTARY(ch, nextChar);
                source->pos++;
                return getImplicit(cp, source);
            }
            return UCOL_NOT_FOUND;
        case TRAIL_SURROGATE_TAG: 
            return UCOL_NOT_FOUND; 
        case CHARSET_TAG:
            
            
            return UCOL_NOT_FOUND;
        default:
            *status = U_INTERNAL_PROGRAM_ERROR;
            CE=0;
            break;
    }
    if (CE <= UCOL_NOT_FOUND) break;
  }
  return CE;
}



static
inline uint32_t getPrevImplicit(UChar32 cp, collIterate *collationSource) {
    uint32_t r = uprv_uca_getImplicitPrimary(cp);

    *(collationSource->CEpos++) = (r & UCOL_PRIMARYMASK) | 0x00000505;
    collationSource->toReturn = collationSource->CEpos;

    
    if (collationSource->flags & UCOL_ITER_INNORMBUF) {
        collationSource->offsetRepeatCount = 1;
    } else {
        int32_t firstOffset = (int32_t)(collationSource->pos - collationSource->string);

        UErrorCode errorCode = U_ZERO_ERROR;
        collationSource->appendOffset(firstOffset, errorCode);
        collationSource->appendOffset(firstOffset + 1, errorCode);

        collationSource->offsetReturn = collationSource->offsetStore - 1;
        *(collationSource->offsetBuffer) = firstOffset;
        if (collationSource->offsetReturn == collationSource->offsetBuffer) {
            collationSource->offsetStore = collationSource->offsetBuffer;
        }
    }

    return ((r & 0x0000FFFF)<<16) | 0x000000C0;
}






uint32_t ucol_prv_getSpecialPrevCE(const UCollator *coll, UChar ch, uint32_t CE,
                          collIterate *source,
                          UErrorCode *status)
{
    const uint32_t *CEOffset    = NULL;
          UChar    *UCharOffset = NULL;
          UChar    schar;
    const UChar    *constart    = NULL;
          uint32_t size;
          UChar    buffer[UCOL_MAX_BUFFER];
          uint32_t *endCEBuffer;
          UChar   *strbuffer;
          int32_t noChars = 0;
          int32_t CECount = 0;

    for(;;)
    {
        
        switch (getCETag(CE))
        {
        case NOT_FOUND_TAG:  
            return CE;

        case SPEC_PROC_TAG:
            {
                
                
                
                
                
                
                const UChar *UCharOffset;
                UChar schar, tchar;
                collIterateState prefixState;
                backupState(source, &prefixState);
                for(;;) {
                    
                    

                    
                    const UChar *ContractionStart = UCharOffset = (UChar *)coll->image+getContractOffset(CE);

                    if (collIter_bos(source)) {
                        CE = *(coll->contractionCEs + (UCharOffset - coll->contractionIndex));
                        break;
                    }
                    schar = getPrevNormalizedChar(source, status);
                    goBackOne(source);

                    while(schar > (tchar = *UCharOffset)) { 
                        UCharOffset++;
                    }

                    if (schar == tchar) {
                        
                        
                        CE = *(coll->contractionCEs +
                            (UCharOffset - coll->contractionIndex));
                    }
                    else
                    {
                        
                        
                        
                        
                        uint32_t isZeroCE = UTRIE_GET32_FROM_LEAD(&coll->mapping, schar);
                        
                        if(isZeroCE == 0) {
                            continue;
                        } else if(U16_IS_SURROGATE(schar)) {
                            
                            
                            
                            
                            
                            
                            if (!collIter_bos(source)) {
                                UChar lead;
                                if(!U16_IS_SURROGATE_LEAD(schar) && U16_IS_LEAD(lead = getPrevNormalizedChar(source, status))) {
                                    isZeroCE = UTRIE_GET32_FROM_LEAD(&coll->mapping, lead);
                                    if(isSpecial(isZeroCE) && getCETag(isZeroCE) == SURROGATE_TAG) {
                                        uint32_t finalCE = UTRIE_GET32_FROM_OFFSET_TRAIL(&coll->mapping, isZeroCE&0xFFFFFF, schar);
                                        if(finalCE == 0) {
                                            
                                            goBackOne(source);
                                            continue;
                                        }
                                    }
                                } else {
                                    
                                    return UCOL_NOT_FOUND;
                                }
                            } else {
                                
                                return UCOL_NOT_FOUND;
                            }
                        }
                        
                        
                        CE = *(coll->contractionCEs +
                            (ContractionStart - coll->contractionIndex));
                    }

                    if(!isPrefix(CE)) {
                        
                        
                        
                        
                        
                        break;
                    }
                }
                loadState(source, &prefixState, TRUE);
                break;
            }

        case CONTRACTION_TAG: {
            




            schar = peekCodeUnit(source, 0);
            constart = (UChar *)coll->image + getContractOffset(CE);
            if (isAtStartPrevIterate(source)
                
) {
                    
                    CE = *(coll->contractionCEs +
                        (constart - coll->contractionIndex));
                    break;
            }
            strbuffer = buffer;
            UCharOffset = strbuffer + (UCOL_MAX_BUFFER - 1);
            *(UCharOffset --) = 0;
            noChars = 0;
            
            while (ucol_unsafeCP(schar, coll)) {
                *(UCharOffset) = schar;
                noChars++;
                UCharOffset --;
                schar = getPrevNormalizedChar(source, status);
                goBackOne(source);
                
                
                
                
                
                
                if (UCharOffset + 1 == buffer) {
                    
                    int32_t newsize = 0;
                    if(source->pos) { 
                        newsize = (int32_t)(source->pos - source->string + 1);
                    } else { 
                        newsize = 4 * UCOL_MAX_BUFFER;
                    }
                    strbuffer = (UChar *)uprv_malloc(sizeof(UChar) *
                        (newsize + UCOL_MAX_BUFFER));
                    
                    if (strbuffer == NULL) {
                        *status = U_MEMORY_ALLOCATION_ERROR;
                        return UCOL_NO_MORE_CES;
                    }
                    UCharOffset = strbuffer + newsize;
                    uprv_memcpy(UCharOffset, buffer,
                        UCOL_MAX_BUFFER * sizeof(UChar));
                    UCharOffset --;
                }
                if ((source->pos && (source->pos == source->string ||
                    ((source->flags & UCOL_ITER_INNORMBUF) &&
                    *(source->pos - 1) == 0 && source->fcdPosition == NULL)))
                    || (source->iterator && !source->iterator->hasPrevious(source->iterator))) {
                        break;
                }
            }
            
            *(UCharOffset) = schar;
            noChars++;

            int32_t offsetBias;

            
            if (source->flags & UCOL_ITER_INNORMBUF) {
                offsetBias = -1;
            } else {
                offsetBias = (int32_t)(source->pos - source->string);
            }

            


            collIterate temp;
            int32_t rawOffset;

            IInit_collIterate(coll, UCharOffset, noChars, &temp, status);
            if(U_FAILURE(*status)) {
                return (uint32_t)UCOL_NULLORDER;
            }
            temp.flags &= ~UCOL_ITER_NORM;
            temp.flags |= source->flags & UCOL_FORCE_HAN_IMPLICIT;

            rawOffset = (int32_t)(temp.pos - temp.string); 
            CE = ucol_IGetNextCE(coll, &temp, status);

            if (source->extendCEs) {
                endCEBuffer = source->extendCEs + source->extendCEsSize;
                CECount = (int32_t)((source->CEpos - source->extendCEs)/sizeof(uint32_t));
            } else {
                endCEBuffer = source->CEs + UCOL_EXPAND_CE_BUFFER_SIZE;
                CECount = (int32_t)((source->CEpos - source->CEs)/sizeof(uint32_t));
            }

            while (CE != UCOL_NO_MORE_CES) {
                *(source->CEpos ++) = CE;

                if (offsetBias >= 0) {
                    source->appendOffset(rawOffset + offsetBias, *status);
                }

                CECount++;
                if (source->CEpos == endCEBuffer) {
                    



                    if (!increaseCEsCapacity(source)) {
                        *status = U_MEMORY_ALLOCATION_ERROR;
                        break;
                    }

                    endCEBuffer = source->extendCEs + source->extendCEsSize;
                }

                if ((temp.flags & UCOL_ITER_INNORMBUF) != 0) {
                    rawOffset = (int32_t)(temp.fcdPosition - temp.string);
                } else {
                    rawOffset = (int32_t)(temp.pos - temp.string);
                }

                CE = ucol_IGetNextCE(coll, &temp, status);
            }

            if (strbuffer != buffer) {
                uprv_free(strbuffer);
            }
            if (U_FAILURE(*status)) {
                return (uint32_t)UCOL_NULLORDER;
            }

            if (source->offsetRepeatValue != 0) {
                if (CECount > noChars) {
                    source->offsetRepeatCount += temp.offsetRepeatCount;
                } else {
                    
                    source->offsetReturn -= (noChars - CECount);
                }
            }

            if (offsetBias >= 0) {
                source->offsetReturn = source->offsetStore - 1;
                if (source->offsetReturn == source->offsetBuffer) {
                    source->offsetStore = source->offsetBuffer;
                }
            }

            source->toReturn = source->CEpos - 1;
            if (source->toReturn == source->CEs) {
                source->CEpos = source->CEs;
            }

            return *(source->toReturn);
        }
        case LONG_PRIMARY_TAG:
            {
                *(source->CEpos++) = ((CE & 0xFFFF00) << 8) | (UCOL_BYTE_COMMON << 8) | UCOL_BYTE_COMMON;
                *(source->CEpos++) = ((CE & 0xFF)<<24)|UCOL_CONTINUATION_MARKER;
                source->toReturn = source->CEpos - 1;

                if (source->flags & UCOL_ITER_INNORMBUF) {
                    source->offsetRepeatCount = 1;
                } else {
                    int32_t firstOffset = (int32_t)(source->pos - source->string);

                    source->appendOffset(firstOffset, *status);
                    source->appendOffset(firstOffset + 1, *status);

                    source->offsetReturn = source->offsetStore - 1;
                    *(source->offsetBuffer) = firstOffset;
                    if (source->offsetReturn == source->offsetBuffer) {
                        source->offsetStore = source->offsetBuffer;
                    }
                }


                return *(source->toReturn);
            }

        case EXPANSION_TAG: 
            {
            




            int32_t firstOffset = (int32_t)(source->pos - source->string);

            
            if (source->offsetReturn != NULL) {
                if (! (source->flags & UCOL_ITER_INNORMBUF) && source->offsetReturn == source->offsetBuffer) {
                    source->offsetStore = source->offsetBuffer;
                }else {
                  firstOffset = -1;
                }
            }

            
            CEOffset = (uint32_t *)coll->image + getExpansionOffset(CE);
            size     = getExpansionCount(CE);
            if (size != 0) {
                


                uint32_t count;

                for (count = 0; count < size; count++) {
                    *(source->CEpos ++) = *CEOffset++;

                    if (firstOffset >= 0) {
                        source->appendOffset(firstOffset + 1, *status);
                    }
                }
            } else {
                
                while (*CEOffset != 0) {
                    *(source->CEpos ++) = *CEOffset ++;

                    if (firstOffset >= 0) {
                        source->appendOffset(firstOffset + 1, *status);
                    }
                }
            }

            if (firstOffset >= 0) {
                source->offsetReturn = source->offsetStore - 1;
                *(source->offsetBuffer) = firstOffset;
                if (source->offsetReturn == source->offsetBuffer) {
                    source->offsetStore = source->offsetBuffer;
                }
            } else {
                source->offsetRepeatCount += size - 1;
            }

            source->toReturn = source->CEpos - 1;
            
            
            if(source->toReturn == source->CEs) {
                source->CEpos = source->CEs;
            }

            return *(source->toReturn);
            }

        case DIGIT_TAG:
            {
                



                uint32_t i;    

                if (source->coll->numericCollation == UCOL_ON){
                    uint32_t digIndx = 0;
                    uint32_t endIndex = 0;
                    uint32_t leadingZeroIndex = 0;
                    uint32_t trailingZeroCount = 0;

                    uint8_t collateVal = 0;

                    UBool nonZeroValReached = FALSE;

                    uint8_t numTempBuf[UCOL_MAX_DIGITS_FOR_NUMBER/2 + 2]; 
                    




                    








                    uint32_t ceLimit = 0;
                    UChar initial_ch = ch;
                    collIterateState initialState = {0,0,0,0,0,0,0,0,0};
                    backupState(source, &initialState);

                    for(;;) {
                        collIterateState state = {0,0,0,0,0,0,0,0,0};
                        UChar32 char32 = 0;
                        int32_t digVal = 0;

                        if (U16_IS_TRAIL (ch)) {
                            if (!collIter_bos(source)){
                                UChar lead = getPrevNormalizedChar(source, status);
                                if(U16_IS_LEAD(lead)) {
                                    char32 = U16_GET_SUPPLEMENTARY(lead,ch);
                                    goBackOne(source);
                                } else {
                                    char32 = ch;
                                }
                            } else {
                                char32 = ch;
                            }
                        } else {
                            char32 = ch;
                        }
                        digVal = u_charDigitValue(char32);

                        for(;;) {
                            
                            
                            
                            

                            
                            if (digVal != 0)
                                nonZeroValReached = TRUE;

                            if (nonZeroValReached) {
                                












                                if ((digIndx + trailingZeroCount) % 2 == 1) {
                                    
                                    collateVal += (uint8_t)(digVal * 10);

                                    
                                    
                                    
                                    
                                    
                                    if (collateVal != 0)
                                        leadingZeroIndex = 0;

                                    
                                    
                                    if ( digIndx < UCOL_MAX_DIGITS_FOR_NUMBER ) {
                                        numTempBuf[(digIndx/2) + 2] = collateVal*2 + 6;
                                    }
                                    collateVal = 0;
                                } else {
                                    
                                    collateVal = (uint8_t)digVal;

                                    
                                    if (collateVal == 0) {
                                        if (!leadingZeroIndex)
                                            leadingZeroIndex = (digIndx/2) + 2;
                                    } else
                                        leadingZeroIndex = 0;

                                    
                                    
                                }
                                ++digIndx;
                            } else
                                ++trailingZeroCount;

                            if (!collIter_bos(source)) {
                                ch = getPrevNormalizedChar(source, status);
                                
                                if (U16_IS_TRAIL(ch)) {
                                    backupState(source, &state);
                                    if (!collIter_bos(source)) {
                                        goBackOne(source);
                                        UChar lead = getPrevNormalizedChar(source, status);

                                        if(U16_IS_LEAD(lead)) {
                                            char32 = U16_GET_SUPPLEMENTARY(lead,ch);
                                        } else {
                                            loadState(source, &state, FALSE);
                                            char32 = ch;
                                        }
                                    }
                                } else
                                    char32 = ch;

                                if ((digVal = u_charDigitValue(char32)) == -1 || (ceLimit > 0 && (digIndx + trailingZeroCount) >= ceLimit)) {
                                    if (char32 > 0xFFFF) {
                                        loadState(source, &state, FALSE);
                                    }
                                    
                                    
                                    
                                    
                                    break;
                                }

                                goBackOne(source);
                            }else
                                break;
                        }

                        if (digIndx + trailingZeroCount <= UCOL_MAX_DIGITS_FOR_NUMBER) {
                            
                            break;
                        }
                        
                        
                        ceLimit = (digIndx + trailingZeroCount) % UCOL_MAX_DIGITS_FOR_NUMBER;
                        if ( ceLimit == 0 ) {
                            ceLimit = UCOL_MAX_DIGITS_FOR_NUMBER;
                        }
                        ch = initial_ch;
                        loadState(source, &initialState, FALSE);
                        digIndx = endIndex = leadingZeroIndex = trailingZeroCount = 0;
                        collateVal = 0;
                        nonZeroValReached = FALSE;
                    }

                    if (! nonZeroValReached) {
                        digIndx = 2;
                        trailingZeroCount = 0;
                        numTempBuf[2] = 6;
                    }

                    if ((digIndx + trailingZeroCount) % 2 != 0) {
                        numTempBuf[((digIndx)/2) + 2] = collateVal*2 + 6;
                        digIndx += 1;       
                    }
                    if (trailingZeroCount % 2 != 0) {
                        
                        
                        digIndx += 1;       
                        trailingZeroCount -= 1;
                    }

                    endIndex = leadingZeroIndex ? leadingZeroIndex : ((digIndx/2) + 2) ;

                    
                    numTempBuf[2] -= 1;

                    






                    numTempBuf[0] = UCOL_CODAN_PLACEHOLDER;
                    uint32_t exponent = (digIndx+trailingZeroCount)/2;
                    if (leadingZeroIndex)
                        exponent -= ((digIndx/2) + 2 - leadingZeroIndex);
                    numTempBuf[1] = (uint8_t)(0x80 + (exponent & 0x7F));

                    
                    
                    int32_t size = (endIndex+1)/2;
                    if(!ensureCEsCapacity(source, size)) {
                        return (uint32_t)UCOL_NULLORDER;
                    }
                    *(source->CEpos++) = (((numTempBuf[0] << 8) | numTempBuf[1]) << UCOL_PRIMARYORDERSHIFT) | 
                        (UCOL_BYTE_COMMON << UCOL_SECONDARYORDERSHIFT) | 
                        UCOL_BYTE_COMMON; 
                    i = endIndex - 1; 
                    while(i >= 2) {
                        uint32_t primWeight = numTempBuf[i--] << 8;
                        if ( i >= 2)
                            primWeight |= numTempBuf[i--];
                        *(source->CEpos++) = (primWeight << UCOL_PRIMARYORDERSHIFT) | UCOL_CONTINUATION_MARKER;
                    }

                    source->toReturn = source->CEpos -1;
                    return *(source->toReturn);
                } else {
                    CEOffset = (uint32_t *)coll->image + getExpansionOffset(CE);
                    CE = *(CEOffset++);
                    break;
                }
            }

        case HANGUL_SYLLABLE_TAG: 
            {
                static const uint32_t
                    SBase = 0xAC00, LBase = 0x1100, VBase = 0x1161, TBase = 0x11A7;
                
                static const uint32_t VCount = 21;
                static const uint32_t TCount = 28;
                
                

                uint32_t L = ch - SBase;
                




                uint32_t T = L % TCount;
                L /= TCount;
                uint32_t V = L % VCount;
                L /= VCount;

                
                L += LBase;
                V += VBase;
                T += TBase;

                int32_t firstOffset = (int32_t)(source->pos - source->string);
                source->appendOffset(firstOffset, *status);

                


                if (!source->coll->image->jamoSpecial) {
                    *(source->CEpos++) = UTRIE_GET32_FROM_LEAD(&coll->mapping, L);
                    *(source->CEpos++) = UTRIE_GET32_FROM_LEAD(&coll->mapping, V);
                    source->appendOffset(firstOffset + 1, *status);

                    if (T != TBase) {
                        *(source->CEpos++) = UTRIE_GET32_FROM_LEAD(&coll->mapping, T);
                        source->appendOffset(firstOffset + 1, *status);
                    }

                    source->toReturn = source->CEpos - 1;

                    source->offsetReturn = source->offsetStore - 1;
                    if (source->offsetReturn == source->offsetBuffer) {
                        source->offsetStore = source->offsetBuffer;
                    }

                    return *(source->toReturn);
                } else {
                    
                    
                    

                    
                    UChar *tempbuffer = source->writableBuffer.getBuffer(5);
                    int32_t tempbufferLength, jamoOffset;
                    tempbuffer[0] = 0;
                    tempbuffer[1] = (UChar)L;
                    tempbuffer[2] = (UChar)V;
                    if (T != TBase) {
                        tempbuffer[3] = (UChar)T;
                        tempbufferLength = 4;
                    } else {
                        tempbufferLength = 3;
                    }
                    source->writableBuffer.releaseBuffer(tempbufferLength);

                    
                    if (source->pos  == source->string) {
                        jamoOffset = 0;
                        source->fcdPosition = NULL;
                    } else {
                        jamoOffset = source->pos - source->string;
                        source->fcdPosition       = source->pos-1;
                    }
                    
                    
                    
                    int32_t jamoRemaining = tempbufferLength - 2;
                    jamoOffset++; 
                    while (jamoRemaining-- > 0) {
                        source->appendOffset(jamoOffset, *status);
                    }

                    source->offsetRepeatValue = jamoOffset;

                    source->offsetReturn = source->offsetStore - 1;
                    if (source->offsetReturn == source->offsetBuffer) {
                        source->offsetStore = source->offsetBuffer;
                    }

                    source->pos               = source->writableBuffer.getTerminatedBuffer() + tempbufferLength;
                    source->origFlags         = source->flags;
                    source->flags            |= UCOL_ITER_INNORMBUF;
                    source->flags            &= ~(UCOL_ITER_NORM | UCOL_ITER_HASLEN);

                    return(UCOL_IGNORABLE);
                }
            }

        case IMPLICIT_TAG:        
            return getPrevImplicit(ch, source);

            
        case CJK_IMPLICIT_TAG:    
            return getPrevImplicit(ch, source);

        case SURROGATE_TAG:  
            
            
            
            return UCOL_NOT_FOUND;

        case LEAD_SURROGATE_TAG:  
            return UCOL_NOT_FOUND; 

        case TRAIL_SURROGATE_TAG: 
            {
                UChar32 cp = 0;
                UChar  prevChar;
                const UChar *prev;
                if (isAtStartPrevIterate(source)) {
                    
                    return UCOL_NOT_FOUND;
                }
                if (source->pos != source->writableBuffer.getBuffer()) {
                    prev     = source->pos - 1;
                } else {
                    prev     = source->fcdPosition;
                }
                prevChar = *prev;

                
                if (U16_IS_LEAD(prevChar)) {
                    cp = ((((uint32_t)prevChar)<<10UL)+(ch)-(((uint32_t)0xd800<<10UL)+0xdc00-0x10000));
                    source->pos = prev;
                } else {
                    return UCOL_NOT_FOUND; 
                }

                return getPrevImplicit(cp, source);
            }

            
            
        case CHARSET_TAG:  
            
            return UCOL_NOT_FOUND;

        default:           
            *status = U_INTERNAL_PROGRAM_ERROR;
            CE=0;
            break;
        }

        if (CE <= UCOL_NOT_FOUND) {
            break;
        }
    }

    return CE;
}















#define uprv_ucol_reverse_buffer(TYPE, start, end) { \
  TYPE tempA; \
while((start)<(end)) { \
    tempA = *(start); \
    *(start)++ = *(end); \
    *(end)-- = tempA; \
} \
}

































U_CAPI int32_t U_EXPORT2
ucol_mergeSortkeys(const uint8_t *src1, int32_t src1Length,
                   const uint8_t *src2, int32_t src2Length,
                   uint8_t *dest, int32_t destCapacity) {
    int32_t destLength;
    uint8_t b;

    
    if( src1==NULL || src1Length<-2 || src1Length==0 || (src1Length>0 && src1[src1Length-1]!=0) ||
        src2==NULL || src2Length<-2 || src2Length==0 || (src2Length>0 && src2[src2Length-1]!=0) ||
        destCapacity<0 || (destCapacity>0 && dest==NULL)
    ) {
        
        if(dest!=NULL && destCapacity>0) {
            *dest=0;
        }
        return 0;
    }

    
    if(src1Length<0) {
        src1Length=(int32_t)uprv_strlen((const char *)src1)+1;
    }
    if(src2Length<0) {
        src2Length=(int32_t)uprv_strlen((const char *)src2)+1;
    }

    destLength=src1Length+src2Length-1;
    if(destLength>destCapacity) {
        
        return destLength;
    }

    
    while(*src1!=0 && *src2!=0) { 
        
        while((b=*src1)>=2) {
            ++src1;
            *dest++=b;
        }

        
        *dest++=2;

        
        while((b=*src2)>=2) {
            ++src2;
            *dest++=b;
        }

        
        if(*src1==1 && *src2==1) {
            ++src1;
            ++src2;
            *dest++=1;
        }
    }

    




    if(*src1!=0) {
        
        src2=src1;
    }
    
    uprv_strcpy((char *)dest, (const char *)src2);

    
    return destLength;
}

U_NAMESPACE_BEGIN

class SortKeyByteSink : public ByteSink {
public:
    SortKeyByteSink(char *dest, int32_t destCapacity)
            : buffer_(dest), capacity_(destCapacity),
              appended_(0) {
        if (buffer_ == NULL) {
            capacity_ = 0;
        } else if(capacity_ < 0) {
            buffer_ = NULL;
            capacity_ = 0;
        }
    }
    virtual ~SortKeyByteSink();

    virtual void Append(const char *bytes, int32_t n);
    void Append(uint32_t b) {
        if (appended_ < capacity_ || Resize(1, appended_)) {
            buffer_[appended_] = (char)b;
        }
        ++appended_;
    }
    void Append(uint32_t b1, uint32_t b2) {
        int32_t a2 = appended_ + 2;
        if (a2 <= capacity_ || Resize(2, appended_)) {
            buffer_[appended_] = (char)b1;
            buffer_[appended_ + 1] = (char)b2;
        } else if(appended_ < capacity_) {
            buffer_[appended_] = (char)b1;
        }
        appended_ = a2;
    }
    virtual char *GetAppendBuffer(int32_t min_capacity,
                                  int32_t desired_capacity_hint,
                                  char *scratch, int32_t scratch_capacity,
                                  int32_t *result_capacity);
    int32_t NumberOfBytesAppended() const { return appended_; }
    
    UBool IsOk() const { return buffer_ != NULL; }

protected:
    virtual void AppendBeyondCapacity(const char *bytes, int32_t n, int32_t length) = 0;
    virtual UBool Resize(int32_t appendCapacity, int32_t length) = 0;

    void SetNotOk() {
        buffer_ = NULL;
        capacity_ = 0;
    }

    char *buffer_;
    int32_t capacity_;
    int32_t appended_;

private:
    SortKeyByteSink(const SortKeyByteSink &); 
    SortKeyByteSink &operator=(const SortKeyByteSink &); 
};

SortKeyByteSink::~SortKeyByteSink() {}

void
SortKeyByteSink::Append(const char *bytes, int32_t n) {
    if (n <= 0 || bytes == NULL) {
        return;
    }
    int32_t length = appended_;
    appended_ += n;
    if ((buffer_ + length) == bytes) {
        return;  
    }
    int32_t available = capacity_ - length;
    if (n <= available) {
        uprv_memcpy(buffer_ + length, bytes, n);
    } else {
        AppendBeyondCapacity(bytes, n, length);
    }
}

char *
SortKeyByteSink::GetAppendBuffer(int32_t min_capacity,
                                 int32_t desired_capacity_hint,
                                 char *scratch,
                                 int32_t scratch_capacity,
                                 int32_t *result_capacity) {
    if (min_capacity < 1 || scratch_capacity < min_capacity) {
        *result_capacity = 0;
        return NULL;
    }
    int32_t available = capacity_ - appended_;
    if (available >= min_capacity) {
        *result_capacity = available;
        return buffer_ + appended_;
    } else if (Resize(desired_capacity_hint, appended_)) {
        *result_capacity = capacity_ - appended_;
        return buffer_ + appended_;
    } else {
        *result_capacity = scratch_capacity;
        return scratch;
    }
}

class FixedSortKeyByteSink : public SortKeyByteSink {
public:
    FixedSortKeyByteSink(char *dest, int32_t destCapacity)
            : SortKeyByteSink(dest, destCapacity) {}
    virtual ~FixedSortKeyByteSink();

private:
    virtual void AppendBeyondCapacity(const char *bytes, int32_t n, int32_t length);
    virtual UBool Resize(int32_t appendCapacity, int32_t length);
};

FixedSortKeyByteSink::~FixedSortKeyByteSink() {}

void
FixedSortKeyByteSink::AppendBeyondCapacity(const char *bytes, int32_t , int32_t length) {
    
    
    int32_t available = capacity_ - length;
    if (available > 0) {
        uprv_memcpy(buffer_ + length, bytes, available);
    }
}

UBool
FixedSortKeyByteSink::Resize(int32_t , int32_t ) {
    return FALSE;
}

class CollationKeyByteSink : public SortKeyByteSink {
public:
    CollationKeyByteSink(CollationKey &key)
            : SortKeyByteSink(reinterpret_cast<char *>(key.getBytes()), key.getCapacity()),
              key_(key) {}
    virtual ~CollationKeyByteSink();

private:
    virtual void AppendBeyondCapacity(const char *bytes, int32_t n, int32_t length);
    virtual UBool Resize(int32_t appendCapacity, int32_t length);

    CollationKey &key_;
};

CollationKeyByteSink::~CollationKeyByteSink() {}

void
CollationKeyByteSink::AppendBeyondCapacity(const char *bytes, int32_t n, int32_t length) {
    
    if (Resize(n, length)) {
        uprv_memcpy(buffer_ + length, bytes, n);
    }
}

UBool
CollationKeyByteSink::Resize(int32_t appendCapacity, int32_t length) {
    if (buffer_ == NULL) {
        return FALSE;  
    }
    int32_t newCapacity = 2 * capacity_;
    int32_t altCapacity = length + 2 * appendCapacity;
    if (newCapacity < altCapacity) {
        newCapacity = altCapacity;
    }
    if (newCapacity < 200) {
        newCapacity = 200;
    }
    uint8_t *newBuffer = key_.reallocate(newCapacity, length);
    if (newBuffer == NULL) {
        SetNotOk();
        return FALSE;
    }
    buffer_ = reinterpret_cast<char *>(newBuffer);
    capacity_ = newCapacity;
    return TRUE;
}




class SortKeyLevel : public UMemory {
public:
    SortKeyLevel() : len(0), ok(TRUE) {}
    ~SortKeyLevel() {}

    
    UBool isOk() const { return ok; }
    UBool isEmpty() const { return len == 0; }
    int32_t length() const { return len; }
    const uint8_t *data() const { return buffer.getAlias(); }
    uint8_t operator[](int32_t index) const { return buffer[index]; }

    void appendByte(uint32_t b);

    void appendTo(ByteSink &sink) const {
        sink.Append(reinterpret_cast<const char *>(buffer.getAlias()), len);
    }

    uint8_t &lastByte() {
        U_ASSERT(len > 0);
        return buffer[len - 1];
    }

    uint8_t *getLastFewBytes(int32_t n) {
        if (ok && len >= n) {
            return buffer.getAlias() + len - n;
        } else {
            return NULL;
        }
    }

private:
    MaybeStackArray<uint8_t, 40> buffer;
    int32_t len;
    UBool ok;

    UBool ensureCapacity(int32_t appendCapacity);

    SortKeyLevel(const SortKeyLevel &other); 
    SortKeyLevel &operator=(const SortKeyLevel &other); 
};

void SortKeyLevel::appendByte(uint32_t b) {
    if(len < buffer.getCapacity() || ensureCapacity(1)) {
        buffer[len++] = (uint8_t)b;
    }
}

UBool SortKeyLevel::ensureCapacity(int32_t appendCapacity) {
    if(!ok) {
        return FALSE;
    }
    int32_t newCapacity = 2 * buffer.getCapacity();
    int32_t altCapacity = len + 2 * appendCapacity;
    if (newCapacity < altCapacity) {
        newCapacity = altCapacity;
    }
    if (newCapacity < 200) {
        newCapacity = 200;
    }
    if(buffer.resize(newCapacity, len)==NULL) {
        return ok = FALSE;
    }
    return TRUE;
}

U_NAMESPACE_END


U_CAPI int32_t U_EXPORT2
ucol_getSortKey(const    UCollator    *coll,
        const    UChar        *source,
        int32_t        sourceLength,
        uint8_t        *result,
        int32_t        resultLength)
{
    UTRACE_ENTRY(UTRACE_UCOL_GET_SORTKEY);
    if (UTRACE_LEVEL(UTRACE_VERBOSE)) {
        UTRACE_DATA3(UTRACE_VERBOSE, "coll=%p, source string = %vh ", coll, source,
            ((sourceLength==-1 && source!=NULL) ? u_strlen(source) : sourceLength));
    }

    if(coll->delegate != NULL) {
      return ((const Collator*)coll->delegate)->getSortKey(source, sourceLength, result, resultLength);
    }

    UErrorCode status = U_ZERO_ERROR;
    int32_t keySize   = 0;

    if(source != NULL) {
        
        
        

        
        
        
        

        uint8_t noDest[1] = { 0 };
        if(result == NULL) {
            
            result = noDest;
            resultLength = 0;
        }
        FixedSortKeyByteSink sink(reinterpret_cast<char *>(result), resultLength);
        coll->sortKeyGen(coll, source, sourceLength, sink, &status);
        if(U_SUCCESS(status)) {
            keySize = sink.NumberOfBytesAppended();
        }
    }
    UTRACE_DATA2(UTRACE_VERBOSE, "Sort Key = %vb", result, keySize);
    UTRACE_EXIT_STATUS(status);
    return keySize;
}

U_CFUNC int32_t
ucol_getCollationKey(const UCollator *coll,
                     const UChar *source, int32_t sourceLength,
                     CollationKey &key,
                     UErrorCode &errorCode) {
    CollationKeyByteSink sink(key);
    coll->sortKeyGen(coll, source, sourceLength, sink, &errorCode);
    return sink.NumberOfBytesAppended();
}




static inline UBool
isCompressible(const UCollator * , uint8_t primary1) {
    return UCOL_BYTE_FIRST_NON_LATIN_PRIMARY <= primary1 && primary1 <= maxRegularPrimary;
}

static
inline void doCaseShift(SortKeyLevel &cases, uint32_t &caseShift) {
    if (caseShift  == 0) {
        cases.appendByte(UCOL_CASE_BYTE_START);
        caseShift = UCOL_CASE_SHIFT_START;
    }
}


static void
packFrench(const uint8_t *secondaries, int32_t secsize, SortKeyByteSink &result) {
    secondaries += secsize;  
    uint8_t secondary;
    int32_t count2 = 0;
    int32_t i = 0;
    
    for(i = 0; i<secsize; i++) {
        secondary = *(secondaries-i-1);
        
        if (secondary == UCOL_COMMON2) {
            ++count2;
        } else {
            if (count2 > 0) {
                if (secondary > UCOL_COMMON2) { 
                    while (count2 > UCOL_TOP_COUNT2) {
                        result.Append(UCOL_COMMON_TOP2 - UCOL_TOP_COUNT2);
                        count2 -= (uint32_t)UCOL_TOP_COUNT2;
                    }
                    result.Append(UCOL_COMMON_TOP2 - (count2-1));
                } else {
                    while (count2 > UCOL_BOT_COUNT2) {
                        result.Append(UCOL_COMMON_BOT2 + UCOL_BOT_COUNT2);
                        count2 -= (uint32_t)UCOL_BOT_COUNT2;
                    }
                    result.Append(UCOL_COMMON_BOT2 + (count2-1));
                }
                count2 = 0;
            }
            result.Append(secondary);
        }
    }
    if (count2 > 0) {
        while (count2 > UCOL_BOT_COUNT2) {
            result.Append(UCOL_COMMON_BOT2 + UCOL_BOT_COUNT2);
            count2 -= (uint32_t)UCOL_BOT_COUNT2;
        }
        result.Append(UCOL_COMMON_BOT2 + (count2-1));
    }
}

#define DEFAULT_ERROR_SIZE_FOR_CALCSORTKEY 0


U_CFUNC void U_CALLCONV
ucol_calcSortKey(const    UCollator    *coll,
        const    UChar        *source,
        int32_t        sourceLength,
        SortKeyByteSink &result,
        UErrorCode *status)
{
    if(U_FAILURE(*status)) {
        return;
    }

    SortKeyByteSink &primaries = result;
    SortKeyLevel secondaries;
    SortKeyLevel tertiaries;
    SortKeyLevel cases;
    SortKeyLevel quads;

    UnicodeString normSource;

    int32_t len = (sourceLength == -1 ? u_strlen(source) : sourceLength);

    UColAttributeValue strength = coll->strength;

    uint8_t compareSec   = (uint8_t)((strength >= UCOL_SECONDARY)?0:0xFF);
    uint8_t compareTer   = (uint8_t)((strength >= UCOL_TERTIARY)?0:0xFF);
    uint8_t compareQuad  = (uint8_t)((strength >= UCOL_QUATERNARY)?0:0xFF);
    UBool  compareIdent = (strength == UCOL_IDENTICAL);
    UBool  doCase = (coll->caseLevel == UCOL_ON);
    UBool  isFrenchSec = (coll->frenchCollation == UCOL_ON) && (compareSec == 0);
    UBool  shifted = (coll->alternateHandling == UCOL_SHIFTED);
    
    UBool  doHiragana = (coll->hiraganaQ == UCOL_ON) && (compareQuad == 0);

    uint32_t variableTopValue = coll->variableTopValue;
    
    
    uint8_t UCOL_COMMON_BOT4 = (uint8_t)((coll->variableTopValue>>8)+1);
    uint8_t UCOL_HIRAGANA_QUAD = 0;
    if(doHiragana) {
        UCOL_HIRAGANA_QUAD=UCOL_COMMON_BOT4++;
        
    }
    uint8_t UCOL_BOT_COUNT4 = (uint8_t)(0xFF - UCOL_COMMON_BOT4);

    
    int32_t lastSecondaryLength = 0;
    uint32_t caseShift = 0;

    
    const Normalizer2 *norm2;
    if(compareIdent) {
        norm2 = Normalizer2Factory::getNFDInstance(*status);
    } else if(coll->normalizationMode != UCOL_OFF) {
        norm2 = Normalizer2Factory::getFCDInstance(*status);
    } else {
        norm2 = NULL;
    }
    if(norm2 != NULL) {
        normSource.setTo(FALSE, source, len);
        int32_t qcYesLength = norm2->spanQuickCheckYes(normSource, *status);
        if(qcYesLength != len) {
            UnicodeString unnormalized = normSource.tempSubString(qcYesLength);
            normSource.truncate(qcYesLength);
            norm2->normalizeSecondAndAppend(normSource, unnormalized, *status);
            source = normSource.getBuffer();
            len = normSource.length();
        }
    }
    collIterate s;
    IInit_collIterate(coll, source, len, &s, status);
    if(U_FAILURE(*status)) {
        return;
    }
    s.flags &= ~UCOL_ITER_NORM;  

    uint32_t order = 0;

    uint8_t primary1 = 0;
    uint8_t primary2 = 0;
    uint8_t secondary = 0;
    uint8_t tertiary = 0;
    uint8_t caseSwitch = coll->caseSwitch;
    uint8_t tertiaryMask = coll->tertiaryMask;
    int8_t tertiaryAddition = coll->tertiaryAddition;
    uint8_t tertiaryTop = coll->tertiaryTop;
    uint8_t tertiaryBottom = coll->tertiaryBottom;
    uint8_t tertiaryCommon = coll->tertiaryCommon;
    uint8_t caseBits = 0;

    UBool wasShifted = FALSE;
    UBool notIsContinuation = FALSE;

    uint32_t count2 = 0, count3 = 0, count4 = 0;
    uint8_t leadPrimary = 0;

    for(;;) {
        order = ucol_IGetNextCE(coll, &s, status);
        if(order == UCOL_NO_MORE_CES) {
            break;
        }

        if(order == 0) {
            continue;
        }

        notIsContinuation = !isContinuation(order);

        if(notIsContinuation) {
            tertiary = (uint8_t)(order & UCOL_BYTE_SIZE_MASK);
        } else {
            tertiary = (uint8_t)((order & UCOL_REMOVE_CONTINUATION));
        }

        secondary = (uint8_t)((order >>= 8) & UCOL_BYTE_SIZE_MASK);
        primary2 = (uint8_t)((order >>= 8) & UCOL_BYTE_SIZE_MASK);
        primary1 = (uint8_t)(order >> 8);

        uint8_t originalPrimary1 = primary1;
        if(notIsContinuation && coll->leadBytePermutationTable != NULL) {
            primary1 = coll->leadBytePermutationTable[primary1];
        }

        if((shifted && ((notIsContinuation && order <= variableTopValue && primary1 > 0)
                        || (!notIsContinuation && wasShifted)))
            || (wasShifted && primary1 == 0)) 
        {
            
            if(primary1 == 0) { 
                
                continue;
            }
            if(compareQuad == 0) {
                if(count4 > 0) {
                    while (count4 > UCOL_BOT_COUNT4) {
                        quads.appendByte(UCOL_COMMON_BOT4 + UCOL_BOT_COUNT4);
                        count4 -= UCOL_BOT_COUNT4;
                    }
                    quads.appendByte(UCOL_COMMON_BOT4 + (count4-1));
                    count4 = 0;
                }
                
                
                if(primary1 != 0) { 
                    quads.appendByte(primary1);
                }
                if(primary2 != 0) {
                    quads.appendByte(primary2);
                }
            }
            wasShifted = TRUE;
        } else {
            wasShifted = FALSE;
            
            
            
            if(primary1 != UCOL_IGNORABLE) {
                if(notIsContinuation) {
                    if(leadPrimary == primary1) {
                        primaries.Append(primary2);
                    } else {
                        if(leadPrimary != 0) {
                            primaries.Append((primary1 > leadPrimary) ? UCOL_BYTE_UNSHIFTED_MAX : UCOL_BYTE_UNSHIFTED_MIN);
                        }
                        if(primary2 == UCOL_IGNORABLE) {
                            
                            primaries.Append(primary1);
                            leadPrimary = 0;
                        } else if(isCompressible(coll, originalPrimary1)) {
                            
                            primaries.Append(leadPrimary = primary1, primary2);
                        } else {
                            leadPrimary = 0;
                            primaries.Append(primary1, primary2);
                        }
                    }
                } else { 
                    if(primary2 == UCOL_IGNORABLE) {
                        primaries.Append(primary1);
                    } else {
                        primaries.Append(primary1, primary2);
                    }
                }
            }

            if(secondary > compareSec) {
                if(!isFrenchSec) {
                    
                    if (secondary == UCOL_COMMON2 && notIsContinuation) {
                        ++count2;
                    } else {
                        if (count2 > 0) {
                            if (secondary > UCOL_COMMON2) { 
                                while (count2 > UCOL_TOP_COUNT2) {
                                    secondaries.appendByte(UCOL_COMMON_TOP2 - UCOL_TOP_COUNT2);
                                    count2 -= (uint32_t)UCOL_TOP_COUNT2;
                                }
                                secondaries.appendByte(UCOL_COMMON_TOP2 - (count2-1));
                            } else {
                                while (count2 > UCOL_BOT_COUNT2) {
                                    secondaries.appendByte(UCOL_COMMON_BOT2 + UCOL_BOT_COUNT2);
                                    count2 -= (uint32_t)UCOL_BOT_COUNT2;
                                }
                                secondaries.appendByte(UCOL_COMMON_BOT2 + (count2-1));
                            }
                            count2 = 0;
                        }
                        secondaries.appendByte(secondary);
                    }
                } else {
                    
                    
                    
                    if(notIsContinuation) {
                        if (lastSecondaryLength > 1) {
                            uint8_t *frenchStartPtr = secondaries.getLastFewBytes(lastSecondaryLength);
                            if (frenchStartPtr != NULL) {
                                
                                uint8_t *frenchEndPtr = frenchStartPtr + lastSecondaryLength - 1;
                                uprv_ucol_reverse_buffer(uint8_t, frenchStartPtr, frenchEndPtr);
                            }
                        }
                        lastSecondaryLength = 1;
                    } else {
                        ++lastSecondaryLength;
                    }
                    secondaries.appendByte(secondary);
                }
            }

            if(doCase && (primary1 > 0 || strength >= UCOL_SECONDARY)) {
                
                
                
                doCaseShift(cases, caseShift);
                if(notIsContinuation) {
                    caseBits = (uint8_t)(tertiary & 0xC0);

                    if(tertiary != 0) {
                        if(coll->caseFirst == UCOL_UPPER_FIRST) {
                            if((caseBits & 0xC0) == 0) {
                                cases.lastByte() |= 1 << (--caseShift);
                            } else {
                                cases.lastByte() |= 0 << (--caseShift);
                                
                                doCaseShift(cases, caseShift);
                                cases.lastByte() |= ((caseBits>>6)&1) << (--caseShift);
                            }
                        } else {
                            if((caseBits & 0xC0) == 0) {
                                cases.lastByte() |= 0 << (--caseShift);
                            } else {
                                cases.lastByte() |= 1 << (--caseShift);
                                
                                doCaseShift(cases, caseShift);
                                cases.lastByte() |= ((caseBits>>7)&1) << (--caseShift);
                            }
                        }
                    }
                }
            } else {
                if(notIsContinuation) {
                    tertiary ^= caseSwitch;
                }
            }

            tertiary &= tertiaryMask;
            if(tertiary > compareTer) {
                
                
                if (tertiary == tertiaryCommon && notIsContinuation) {
                    ++count3;
                } else {
                    if(tertiary > tertiaryCommon && tertiaryCommon == UCOL_COMMON3_NORMAL) {
                        tertiary += tertiaryAddition;
                    } else if(tertiary <= tertiaryCommon && tertiaryCommon == UCOL_COMMON3_UPPERFIRST) {
                        tertiary -= tertiaryAddition;
                    }
                    if (count3 > 0) {
                        if ((tertiary > tertiaryCommon)) {
                            while (count3 > coll->tertiaryTopCount) {
                                tertiaries.appendByte(tertiaryTop - coll->tertiaryTopCount);
                                count3 -= (uint32_t)coll->tertiaryTopCount;
                            }
                            tertiaries.appendByte(tertiaryTop - (count3-1));
                        } else {
                            while (count3 > coll->tertiaryBottomCount) {
                                tertiaries.appendByte(tertiaryBottom + coll->tertiaryBottomCount);
                                count3 -= (uint32_t)coll->tertiaryBottomCount;
                            }
                            tertiaries.appendByte(tertiaryBottom + (count3-1));
                        }
                        count3 = 0;
                    }
                    tertiaries.appendByte(tertiary);
                }
            }

            if((compareQuad==0)  && notIsContinuation) {
                if(s.flags & UCOL_WAS_HIRAGANA) { 
                    if(count4>0) { 
                        while (count4 > UCOL_BOT_COUNT4) {
                            quads.appendByte(UCOL_COMMON_BOT4 + UCOL_BOT_COUNT4);
                            count4 -= UCOL_BOT_COUNT4;
                        }
                        quads.appendByte(UCOL_COMMON_BOT4 + (count4-1));
                        count4 = 0;
                    }
                    quads.appendByte(UCOL_HIRAGANA_QUAD); 
                } else { 
                    count4++;
                }
            }
        }
    }

    
    

    UBool ok = TRUE;
    if(U_SUCCESS(*status)) {
        
        if(compareSec == 0) {
            if (count2 > 0) {
                while (count2 > UCOL_BOT_COUNT2) {
                    secondaries.appendByte(UCOL_COMMON_BOT2 + UCOL_BOT_COUNT2);
                    count2 -= (uint32_t)UCOL_BOT_COUNT2;
                }
                secondaries.appendByte(UCOL_COMMON_BOT2 + (count2-1));
            }
            result.Append(UCOL_LEVELTERMINATOR);
            if(!secondaries.isOk()) {
                ok = FALSE;
            } else if(!isFrenchSec) {
                secondaries.appendTo(result);
            } else {
                
                
                if (lastSecondaryLength > 1) {
                    uint8_t *frenchStartPtr = secondaries.getLastFewBytes(lastSecondaryLength);
                    if (frenchStartPtr != NULL) {
                        
                        uint8_t *frenchEndPtr = frenchStartPtr + lastSecondaryLength - 1;
                        uprv_ucol_reverse_buffer(uint8_t, frenchStartPtr, frenchEndPtr);
                    }
                }
                packFrench(secondaries.data(), secondaries.length(), result);
            }
        }

        if(doCase) {
            ok &= cases.isOk();
            result.Append(UCOL_LEVELTERMINATOR);
            cases.appendTo(result);
        }

        if(compareTer == 0) {
            if (count3 > 0) {
                if (coll->tertiaryCommon != UCOL_COMMON_BOT3) {
                    while (count3 >= coll->tertiaryTopCount) {
                        tertiaries.appendByte(tertiaryTop - coll->tertiaryTopCount);
                        count3 -= (uint32_t)coll->tertiaryTopCount;
                    }
                    tertiaries.appendByte(tertiaryTop - count3);
                } else {
                    while (count3 > coll->tertiaryBottomCount) {
                        tertiaries.appendByte(tertiaryBottom + coll->tertiaryBottomCount);
                        count3 -= (uint32_t)coll->tertiaryBottomCount;
                    }
                    tertiaries.appendByte(tertiaryBottom + (count3-1));
                }
            }
            ok &= tertiaries.isOk();
            result.Append(UCOL_LEVELTERMINATOR);
            tertiaries.appendTo(result);

            if(compareQuad == 0) {
                if(count4 > 0) {
                    while (count4 > UCOL_BOT_COUNT4) {
                        quads.appendByte(UCOL_COMMON_BOT4 + UCOL_BOT_COUNT4);
                        count4 -= UCOL_BOT_COUNT4;
                    }
                    quads.appendByte(UCOL_COMMON_BOT4 + (count4-1));
                }
                ok &= quads.isOk();
                result.Append(UCOL_LEVELTERMINATOR);
                quads.appendTo(result);
            }

            if(compareIdent) {
                result.Append(UCOL_LEVELTERMINATOR);
                u_writeIdenticalLevelRun(s.string, len, result);
            }
        }
        result.Append(0);
    }

    
    ucol_freeOffsetBuffer(&s);

    ok &= result.IsOk();
    if(!ok && U_SUCCESS(*status)) { *status = U_MEMORY_ALLOCATION_ERROR; }
}


U_CFUNC void U_CALLCONV
ucol_calcSortKeySimpleTertiary(const    UCollator    *coll,
        const    UChar        *source,
        int32_t        sourceLength,
        SortKeyByteSink &result,
        UErrorCode *status)
{
    U_ALIGN_CODE(16);

    if(U_FAILURE(*status)) {
        return;
    }

    SortKeyByteSink &primaries = result;
    SortKeyLevel secondaries;
    SortKeyLevel tertiaries;

    UnicodeString normSource;

    int32_t len =  sourceLength;

    
    if(coll->normalizationMode != UCOL_OFF) {
        normSource.setTo(len < 0, source, len);
        const Normalizer2 *norm2 = Normalizer2Factory::getFCDInstance(*status);
        int32_t qcYesLength = norm2->spanQuickCheckYes(normSource, *status);
        if(qcYesLength != normSource.length()) {
            UnicodeString unnormalized = normSource.tempSubString(qcYesLength);
            normSource.truncate(qcYesLength);
            norm2->normalizeSecondAndAppend(normSource, unnormalized, *status);
            source = normSource.getBuffer();
            len = normSource.length();
        }
    }
    collIterate s;
    IInit_collIterate(coll, (UChar *)source, len, &s, status);
    if(U_FAILURE(*status)) {
        return;
    }
    s.flags &= ~UCOL_ITER_NORM;  

    uint32_t order = 0;

    uint8_t primary1 = 0;
    uint8_t primary2 = 0;
    uint8_t secondary = 0;
    uint8_t tertiary = 0;
    uint8_t caseSwitch = coll->caseSwitch;
    uint8_t tertiaryMask = coll->tertiaryMask;
    int8_t tertiaryAddition = coll->tertiaryAddition;
    uint8_t tertiaryTop = coll->tertiaryTop;
    uint8_t tertiaryBottom = coll->tertiaryBottom;
    uint8_t tertiaryCommon = coll->tertiaryCommon;

    UBool notIsContinuation = FALSE;

    uint32_t count2 = 0, count3 = 0;
    uint8_t leadPrimary = 0;

    for(;;) {
        order = ucol_IGetNextCE(coll, &s, status);

        if(order == 0) {
            continue;
        }

        if(order == UCOL_NO_MORE_CES) {
            break;
        }

        notIsContinuation = !isContinuation(order);

        if(notIsContinuation) {
            tertiary = (uint8_t)((order & tertiaryMask));
        } else {
            tertiary = (uint8_t)((order & UCOL_REMOVE_CONTINUATION));
        }

        secondary = (uint8_t)((order >>= 8) & UCOL_BYTE_SIZE_MASK);
        primary2 = (uint8_t)((order >>= 8) & UCOL_BYTE_SIZE_MASK);
        primary1 = (uint8_t)(order >> 8);

        uint8_t originalPrimary1 = primary1;
        if (coll->leadBytePermutationTable != NULL && notIsContinuation) {
            primary1 = coll->leadBytePermutationTable[primary1];
        }

        
        
        
        
        if(primary1 != UCOL_IGNORABLE) {
            if(notIsContinuation) {
                if(leadPrimary == primary1) {
                    primaries.Append(primary2);
                } else {
                    if(leadPrimary != 0) {
                        primaries.Append((primary1 > leadPrimary) ? UCOL_BYTE_UNSHIFTED_MAX : UCOL_BYTE_UNSHIFTED_MIN);
                    }
                    if(primary2 == UCOL_IGNORABLE) {
                        
                        primaries.Append(primary1);
                        leadPrimary = 0;
                    } else if(isCompressible(coll, originalPrimary1)) {
                        
                        primaries.Append(leadPrimary = primary1, primary2);
                    } else {
                        leadPrimary = 0;
                        primaries.Append(primary1, primary2);
                    }
                }
            } else { 
                if(primary2 == UCOL_IGNORABLE) {
                    primaries.Append(primary1);
                } else {
                    primaries.Append(primary1, primary2);
                }
            }
        }

        if(secondary > 0) { 
            
            if (secondary == UCOL_COMMON2 && notIsContinuation) {
                ++count2;
            } else {
                if (count2 > 0) {
                    if (secondary > UCOL_COMMON2) { 
                        while (count2 > UCOL_TOP_COUNT2) {
                            secondaries.appendByte(UCOL_COMMON_TOP2 - UCOL_TOP_COUNT2);
                            count2 -= (uint32_t)UCOL_TOP_COUNT2;
                        }
                        secondaries.appendByte(UCOL_COMMON_TOP2 - (count2-1));
                    } else {
                        while (count2 > UCOL_BOT_COUNT2) {
                            secondaries.appendByte(UCOL_COMMON_BOT2 + UCOL_BOT_COUNT2);
                            count2 -= (uint32_t)UCOL_BOT_COUNT2;
                        }
                        secondaries.appendByte(UCOL_COMMON_BOT2 + (count2-1));
                    }
                    count2 = 0;
                }
                secondaries.appendByte(secondary);
            }
        }

        if(notIsContinuation) {
            tertiary ^= caseSwitch;
        }

        if(tertiary > 0) {
            
            
            if (tertiary == tertiaryCommon && notIsContinuation) {
                ++count3;
            } else {
                if(tertiary > tertiaryCommon && tertiaryCommon == UCOL_COMMON3_NORMAL) {
                    tertiary += tertiaryAddition;
                } else if (tertiary <= tertiaryCommon && tertiaryCommon == UCOL_COMMON3_UPPERFIRST) {
                    tertiary -= tertiaryAddition;
                }
                if (count3 > 0) {
                    if ((tertiary > tertiaryCommon)) {
                        while (count3 > coll->tertiaryTopCount) {
                            tertiaries.appendByte(tertiaryTop - coll->tertiaryTopCount);
                            count3 -= (uint32_t)coll->tertiaryTopCount;
                        }
                        tertiaries.appendByte(tertiaryTop - (count3-1));
                    } else {
                        while (count3 > coll->tertiaryBottomCount) {
                            tertiaries.appendByte(tertiaryBottom + coll->tertiaryBottomCount);
                            count3 -= (uint32_t)coll->tertiaryBottomCount;
                        }
                        tertiaries.appendByte(tertiaryBottom + (count3-1));
                    }
                    count3 = 0;
                }
                tertiaries.appendByte(tertiary);
            }
        }
    }

    UBool ok = TRUE;
    if(U_SUCCESS(*status)) {
        
        if (count2 > 0) {
            while (count2 > UCOL_BOT_COUNT2) {
                secondaries.appendByte(UCOL_COMMON_BOT2 + UCOL_BOT_COUNT2);
                count2 -= (uint32_t)UCOL_BOT_COUNT2;
            }
            secondaries.appendByte(UCOL_COMMON_BOT2 + (count2-1));
        }
        ok &= secondaries.isOk();
        result.Append(UCOL_LEVELTERMINATOR);
        secondaries.appendTo(result);

        if (count3 > 0) {
            if (coll->tertiaryCommon != UCOL_COMMON3_NORMAL) {
                while (count3 >= coll->tertiaryTopCount) {
                    tertiaries.appendByte(tertiaryTop - coll->tertiaryTopCount);
                    count3 -= (uint32_t)coll->tertiaryTopCount;
                }
                tertiaries.appendByte(tertiaryTop - count3);
            } else {
                while (count3 > coll->tertiaryBottomCount) {
                    tertiaries.appendByte(tertiaryBottom + coll->tertiaryBottomCount);
                    count3 -= (uint32_t)coll->tertiaryBottomCount;
                }
                tertiaries.appendByte(tertiaryBottom + (count3-1));
            }
        }
        ok &= tertiaries.isOk();
        result.Append(UCOL_LEVELTERMINATOR);
        tertiaries.appendTo(result);

        result.Append(0);
    }

    
    ucol_freeOffsetBuffer(&s);

    ok &= result.IsOk();
    if(!ok && U_SUCCESS(*status)) { *status = U_MEMORY_ALLOCATION_ERROR; }
}

static inline
UBool isShiftedCE(uint32_t CE, uint32_t LVT, UBool *wasShifted) {
    UBool notIsContinuation = !isContinuation(CE);
    uint8_t primary1 = (uint8_t)((CE >> 24) & 0xFF);
    if((LVT && ((notIsContinuation && (CE & 0xFFFF0000)<= LVT && primary1 > 0)
               || (!notIsContinuation && *wasShifted)))
        || (*wasShifted && primary1 == 0)) 
    {
        
        if(primary1 != 0) { 
            
            *wasShifted = TRUE;
            
        }
        
        return TRUE;
    } else {
        *wasShifted = FALSE;
        return FALSE;
    }
}
static inline
void terminatePSKLevel(int32_t level, int32_t maxLevel, int32_t &i, uint8_t *dest) {
    if(level < maxLevel) {
        dest[i++] = UCOL_LEVELTERMINATOR;
    } else {
        dest[i++] = 0;
    }
}


enum {
  UCOL_PSK_PRIMARY = 0,
    UCOL_PSK_SECONDARY = 1,
    UCOL_PSK_CASE = 2,
    UCOL_PSK_TERTIARY = 3,
    UCOL_PSK_QUATERNARY = 4,
    UCOL_PSK_QUIN = 5,      
    UCOL_PSK_IDENTICAL = 6,
    UCOL_PSK_NULL = 7,      
    UCOL_PSK_LIMIT
};






enum {
    UCOL_PSK_LEVEL_SHIFT = 0,      
    UCOL_PSK_LEVEL_MASK = 7,       
    UCOL_PSK_BYTE_COUNT_OR_FRENCH_DONE_SHIFT = 3, 
    UCOL_PSK_BYTE_COUNT_OR_FRENCH_DONE_MASK = 1,
    


    UCOL_PSK_WAS_SHIFTED_SHIFT = 4,
    UCOL_PSK_WAS_SHIFTED_MASK = 1, 
    UCOL_PSK_USED_FRENCH_SHIFT = 5,
    UCOL_PSK_USED_FRENCH_MASK = 3, 
    


    UCOL_PSK_BOCSU_BYTES_SHIFT = 7,
    UCOL_PSK_BOCSU_BYTES_MASK = 3,
    UCOL_PSK_CONSUMED_CES_SHIFT = 9,
    UCOL_PSK_CONSUMED_CES_MASK = 0x7FFFF
};


#define uprv_numAvailableExpCEs(s) (s).CEpos - (s).toReturn











































U_CAPI int32_t U_EXPORT2
ucol_nextSortKeyPart(const UCollator *coll,
                     UCharIterator *iter,
                     uint32_t state[2],
                     uint8_t *dest, int32_t count,
                     UErrorCode *status)
{
    
    if(status==NULL || U_FAILURE(*status)) {
        return 0;
    }
    UTRACE_ENTRY(UTRACE_UCOL_NEXTSORTKEYPART);
    if( coll==NULL || iter==NULL ||
        state==NULL ||
        count<0 || (count>0 && dest==NULL)
    ) {
        *status=U_ILLEGAL_ARGUMENT_ERROR;
        UTRACE_EXIT_STATUS(status);
        return 0;
    }

    UTRACE_DATA6(UTRACE_VERBOSE, "coll=%p, iter=%p, state=%d %d, dest=%p, count=%d",
                  coll, iter, state[0], state[1], dest, count);

    if(count==0) {
        
        UTRACE_EXIT_VALUE(0);
        return 0;
    }
    
    
    uint32_t iterState = state[0];
    
    UBool wasShifted = ((state[1] >> UCOL_PSK_WAS_SHIFTED_SHIFT) & UCOL_PSK_WAS_SHIFTED_MASK)?TRUE:FALSE;
    
    int32_t level= (state[1] >> UCOL_PSK_LEVEL_SHIFT) & UCOL_PSK_LEVEL_MASK;
    
    
    int32_t byteCountOrFrenchDone = (state[1] >> UCOL_PSK_BYTE_COUNT_OR_FRENCH_DONE_SHIFT) & UCOL_PSK_BYTE_COUNT_OR_FRENCH_DONE_MASK;
    
    int32_t usedFrench = (state[1] >> UCOL_PSK_USED_FRENCH_SHIFT) & UCOL_PSK_USED_FRENCH_MASK;
    
    
    int32_t bocsuBytesUsed = (state[1] >> UCOL_PSK_BOCSU_BYTES_SHIFT) & UCOL_PSK_BOCSU_BYTES_MASK;
    
    
    
    int32_t cces = (state[1] >> UCOL_PSK_CONSUMED_CES_SHIFT) & UCOL_PSK_CONSUMED_CES_MASK;

    
    
    int32_t strength = ucol_getAttribute(coll, UCOL_STRENGTH, status);
    
    int32_t maxLevel = 0;
    if(strength < UCOL_TERTIARY) {
        if(ucol_getAttribute(coll, UCOL_CASE_LEVEL, status) == UCOL_ON) {
            maxLevel = UCOL_PSK_CASE;
        } else {
            maxLevel = strength;
        }
    } else {
        if(strength == UCOL_TERTIARY) {
            maxLevel = UCOL_PSK_TERTIARY;
        } else if(strength == UCOL_QUATERNARY) {
            maxLevel = UCOL_PSK_QUATERNARY;
        } else { 
            maxLevel = UCOL_IDENTICAL;
        }
    }
    
    uint8_t UCOL_HIRAGANA_QUAD =
      (ucol_getAttribute(coll, UCOL_HIRAGANA_QUATERNARY_MODE, status) == UCOL_ON)?0xFE:0xFF;
    
    uint32_t LVT = (coll->alternateHandling == UCOL_SHIFTED)?(coll->variableTopValue<<16):0;
    
    UBool doingFrench = (ucol_getAttribute(coll, UCOL_FRENCH_COLLATION, status) == UCOL_ON);

    
    UBool notIsContinuation = FALSE;
    uint32_t CE = UCOL_NO_MORE_CES;

    collIterate s;
    IInit_collIterate(coll, NULL, -1, &s, status);
    if(U_FAILURE(*status)) {
        UTRACE_EXIT_STATUS(*status);
        return 0;
    }
    s.iterator = iter;
    s.flags |= UCOL_USE_ITERATOR;
    
    
    
    UBool doingIdenticalFromStart = FALSE;
    
    
    
    
    UAlignedMemory stackNormIter[UNORM_ITER_SIZE/sizeof(UAlignedMemory)];
    UNormIterator *normIter = NULL;
    
    
    if(ucol_getAttribute(coll, UCOL_NORMALIZATION_MODE, status) == UCOL_ON && level < UCOL_PSK_IDENTICAL) {
        normIter = unorm_openIter(stackNormIter, sizeof(stackNormIter), status);
        s.iterator = unorm_setIter(normIter, iter, UNORM_FCD, status);
        s.flags &= ~UCOL_ITER_NORM;
        if(U_FAILURE(*status)) {
            UTRACE_EXIT_STATUS(*status);
            return 0;
        }
    } else if(level == UCOL_PSK_IDENTICAL) {
        
        
        normIter = unorm_openIter(stackNormIter, sizeof(stackNormIter), status);
        s.iterator = unorm_setIter(normIter, iter, UNORM_NFD, status);
        s.flags &= ~UCOL_ITER_NORM;
        if(U_FAILURE(*status)) {
            UTRACE_EXIT_STATUS(*status);
            return 0;
        }
        doingIdenticalFromStart = TRUE;
    }

    
    
    
    
    uint32_t newState = 0;

    
    
    if(iterState == 0) {
        
        if(level == UCOL_PSK_SECONDARY && doingFrench && !byteCountOrFrenchDone) {
            s.iterator->move(s.iterator, 0, UITER_LIMIT);
        } else {
            s.iterator->move(s.iterator, 0, UITER_START);
        }
    } else {
        
        s.iterator->setState(s.iterator, iterState, status);
        if(U_FAILURE(*status)) {
            UTRACE_EXIT_STATUS(*status);
            return 0;
        }
    }



    
    
    
    
    
    UBool canUpdateState = TRUE;

    
    
    
    int32_t counter = cces;
    if(level < UCOL_PSK_IDENTICAL) {
        while(counter-->0) {
            
            
            if(level == UCOL_PSK_SECONDARY && doingFrench) {
                CE = ucol_IGetPrevCE(coll, &s, status);
            } else {
                CE = ucol_IGetNextCE(coll, &s, status);
            }
            if(CE==UCOL_NO_MORE_CES) {
                
                *status=U_INTERNAL_PROGRAM_ERROR;
                UTRACE_EXIT_STATUS(*status);
                return 0;
            }
            if(uprv_numAvailableExpCEs(s)) {
                canUpdateState = FALSE;
            }
        }
    } else {
        while(counter-->0) {
            uiter_next32(s.iterator);
        }
    }

    
    
    UBool wasDoingPrimary = FALSE;
    
    
    int32_t i = 0;
    
    
    int32_t j = 0;


    
    
    
    
    
    
    switch(level) {
    case UCOL_PSK_PRIMARY:
        wasDoingPrimary = TRUE;
        for(;;) {
            if(i==count) {
                goto saveState;
            }
            
            
            
            if(canUpdateState && byteCountOrFrenchDone == 0) {
                newState = s.iterator->getState(s.iterator);
                if(newState != UITER_NO_STATE) {
                    iterState = newState;
                    cces = 0;
                }
            }
            CE = ucol_IGetNextCE(coll, &s, status);
            cces++;
            if(CE==UCOL_NO_MORE_CES) {
                
                terminatePSKLevel(level, maxLevel, i, dest);
                byteCountOrFrenchDone=0;
                
                
                s.iterator->move(s.iterator, 0, UITER_START);
                cces = 0;
                level = UCOL_PSK_SECONDARY;
                break;
            }
            if(!isContinuation(CE)){
                if(coll->leadBytePermutationTable != NULL){
                    CE = (coll->leadBytePermutationTable[CE>>24] << 24) | (CE & 0x00FFFFFF);
                }
            }
            if(!isShiftedCE(CE, LVT, &wasShifted)) {
                CE >>= UCOL_PRIMARYORDERSHIFT; 
                if(CE != 0) {
                    if(byteCountOrFrenchDone == 0) {
                        
                        dest[i++]=(uint8_t)(CE >> 8);
                    } else {
                        byteCountOrFrenchDone = 0;
                    }
                    if((CE &=0xff)!=0) {
                        if(i==count) {
                            
                            byteCountOrFrenchDone = 1;
                            cces--;
                            goto saveState;
                        }
                        dest[i++]=(uint8_t)CE;
                    }
                }
            }
            if(uprv_numAvailableExpCEs(s)) {
                canUpdateState = FALSE;
            } else {
                canUpdateState = TRUE;
            }
        }
        
    case UCOL_PSK_SECONDARY:
        if(strength >= UCOL_SECONDARY) {
            if(!doingFrench) {
                for(;;) {
                    if(i == count) {
                        goto saveState;
                    }
                    
                    
                    
                    if(canUpdateState) {
                        newState = s.iterator->getState(s.iterator);
                        if(newState != UITER_NO_STATE) {
                            iterState = newState;
                            cces = 0;
                        }
                    }
                    CE = ucol_IGetNextCE(coll, &s, status);
                    cces++;
                    if(CE==UCOL_NO_MORE_CES) {
                        
                        terminatePSKLevel(level, maxLevel, i, dest);
                        byteCountOrFrenchDone = 0;
                        
                        
                        s.iterator->move(s.iterator, 0, UITER_START);
                        cces = 0;
                        level = UCOL_PSK_CASE;
                        break;
                    }
                    if(!isShiftedCE(CE, LVT, &wasShifted)) {
                        CE >>= 8; 
                        if(CE != 0) {
                            dest[i++]=(uint8_t)CE;
                        }
                    }
                    if(uprv_numAvailableExpCEs(s)) {
                        canUpdateState = FALSE;
                    } else {
                        canUpdateState = TRUE;
                    }
                }
            } else { 
                uint8_t frenchBuff[UCOL_MAX_BUFFER];
                int32_t frenchIndex = 0;
                
                
                
                if(wasDoingPrimary) {
                    s.iterator->move(s.iterator, 0, UITER_LIMIT);
                    cces = 0;
                }
                for(;;) {
                    if(i == count) {
                        goto saveState;
                    }
                    if(canUpdateState) {
                        newState = s.iterator->getState(s.iterator);
                        if(newState != UITER_NO_STATE) {
                            iterState = newState;
                            cces = 0;
                        }
                    }
                    CE = ucol_IGetPrevCE(coll, &s, status);
                    cces++;
                    if(CE==UCOL_NO_MORE_CES) {
                        
                        terminatePSKLevel(level, maxLevel, i, dest);
                        byteCountOrFrenchDone = 0;
                        
                        s.iterator->move(s.iterator, 0, UITER_START);
                        level = UCOL_PSK_CASE;
                        break;
                    }
                    if(isContinuation(CE)) { 
                        
                        CE >>= 8;
                        frenchBuff[frenchIndex++] = (uint8_t)CE;
                    } else if(!isShiftedCE(CE, LVT, &wasShifted)) {
                        CE >>= 8; 
                        if(!frenchIndex) {
                            if(CE != 0) {
                                dest[i++]=(uint8_t)CE;
                            }
                        } else {
                            frenchBuff[frenchIndex++] = (uint8_t)CE;
                            frenchIndex -= usedFrench;
                            usedFrench = 0;
                            while(i < count && frenchIndex) {
                                dest[i++] = frenchBuff[--frenchIndex];
                                usedFrench++;
                            }
                        }
                    }
                    if(uprv_numAvailableExpCEs(s)) {
                        canUpdateState = FALSE;
                    } else {
                        canUpdateState = TRUE;
                    }
                }
            }
        } else {
            level = UCOL_PSK_CASE;
        }
        
    case UCOL_PSK_CASE:
        if(ucol_getAttribute(coll, UCOL_CASE_LEVEL, status) == UCOL_ON) {
            uint32_t caseShift = UCOL_CASE_SHIFT_START;
            uint8_t caseByte = UCOL_CASE_BYTE_START;
            uint8_t caseBits = 0;

            for(;;) {
                U_ASSERT(caseShift <= UCOL_CASE_SHIFT_START);
                if(i == count) {
                    goto saveState;
                }
                
                
                
                if(canUpdateState) {
                    newState = s.iterator->getState(s.iterator);
                    if(newState != UITER_NO_STATE) {
                        iterState = newState;
                        cces = 0;
                    }
                }
                CE = ucol_IGetNextCE(coll, &s, status);
                cces++;
                if(CE==UCOL_NO_MORE_CES) {
                    
                    
                    if(caseShift != UCOL_CASE_SHIFT_START) {
                        dest[i++] = caseByte;
                    }
                    cces = 0;
                    
                    
                    
                    if(i < count) {
                        
                        terminatePSKLevel(level, maxLevel, i, dest);
                        
                        
                        s.iterator->move(s.iterator, 0, UITER_START);
                        level = UCOL_PSK_TERTIARY;
                    } else {
                        canUpdateState = FALSE;
                    }
                    break;
                }

                if(!isShiftedCE(CE, LVT, &wasShifted)) {
                    if(!isContinuation(CE) && ((CE & UCOL_PRIMARYMASK) != 0 || strength > UCOL_PRIMARY)) {
                        
                        
                        
                        CE = (uint8_t)(CE & UCOL_BYTE_SIZE_MASK);
                        caseBits = (uint8_t)(CE & 0xC0);
                        
                        
                        if(CE != 0) {
                            if (caseShift == 0) {
                                dest[i++] = caseByte;
                                caseShift = UCOL_CASE_SHIFT_START;
                                caseByte = UCOL_CASE_BYTE_START;
                            }
                            if(coll->caseFirst == UCOL_UPPER_FIRST) {
                                if((caseBits & 0xC0) == 0) {
                                    caseByte |= 1 << (--caseShift);
                                } else {
                                    caseByte |= 0 << (--caseShift);
                                    
                                    if(caseShift == 0) {
                                        dest[i++] = caseByte;
                                        caseShift = UCOL_CASE_SHIFT_START;
                                        caseByte = UCOL_CASE_BYTE_START;
                                    }
                                    caseByte |= ((caseBits>>6)&1) << (--caseShift);
                                }
                            } else {
                                if((caseBits & 0xC0) == 0) {
                                    caseByte |= 0 << (--caseShift);
                                } else {
                                    caseByte |= 1 << (--caseShift);
                                    
                                    if(caseShift == 0) {
                                        dest[i++] = caseByte;
                                        caseShift = UCOL_CASE_SHIFT_START;
                                        caseByte = UCOL_CASE_BYTE_START;
                                    }
                                    caseByte |= ((caseBits>>7)&1) << (--caseShift);
                                }
                            }
                        }

                    }
                }
                
                if(uprv_numAvailableExpCEs(s)) {
                    canUpdateState = FALSE;
                } else {
                    canUpdateState = TRUE;
                }
            }
        } else {
            level = UCOL_PSK_TERTIARY;
        }
        
    case UCOL_PSK_TERTIARY:
        if(strength >= UCOL_TERTIARY) {
            for(;;) {
                if(i == count) {
                    goto saveState;
                }
                
                
                
                if(canUpdateState) {
                    newState = s.iterator->getState(s.iterator);
                    if(newState != UITER_NO_STATE) {
                        iterState = newState;
                        cces = 0;
                    }
                }
                CE = ucol_IGetNextCE(coll, &s, status);
                cces++;
                if(CE==UCOL_NO_MORE_CES) {
                    
                    terminatePSKLevel(level, maxLevel, i, dest);
                    byteCountOrFrenchDone = 0;
                    
                    
                    s.iterator->move(s.iterator, 0, UITER_START);
                    cces = 0;
                    level = UCOL_PSK_QUATERNARY;
                    break;
                }
                if(!isShiftedCE(CE, LVT, &wasShifted)) {
                    notIsContinuation = !isContinuation(CE);

                    if(notIsContinuation) {
                        CE = (uint8_t)(CE & UCOL_BYTE_SIZE_MASK);
                        CE ^= coll->caseSwitch;
                        CE &= coll->tertiaryMask;
                    } else {
                        CE = (uint8_t)((CE & UCOL_REMOVE_CONTINUATION));
                    }

                    if(CE != 0) {
                        dest[i++]=(uint8_t)CE;
                    }
                }
                if(uprv_numAvailableExpCEs(s)) {
                    canUpdateState = FALSE;
                } else {
                    canUpdateState = TRUE;
                }
            }
        } else {
            
            
            level = UCOL_PSK_NULL;
        }
        
    case UCOL_PSK_QUATERNARY:
        if(strength >= UCOL_QUATERNARY) {
            for(;;) {
                if(i == count) {
                    goto saveState;
                }
                
                
                
                if(canUpdateState) {
                    newState = s.iterator->getState(s.iterator);
                    if(newState != UITER_NO_STATE) {
                        iterState = newState;
                        cces = 0;
                    }
                }
                CE = ucol_IGetNextCE(coll, &s, status);
                cces++;
                if(CE==UCOL_NO_MORE_CES) {
                    
                    terminatePSKLevel(level, maxLevel, i, dest);
                    
                    byteCountOrFrenchDone = 0;
                    
                    
                    s.iterator->move(s.iterator, 0, UITER_START);
                    cces = 0;
                    level = UCOL_PSK_QUIN;
                    break;
                }
                if(CE==0)
                    continue;
                if(isShiftedCE(CE, LVT, &wasShifted)) {
                    CE >>= 16; 
                    if(CE != 0) {
                        if(byteCountOrFrenchDone == 0) {
                            dest[i++]=(uint8_t)(CE >> 8);
                        } else {
                            byteCountOrFrenchDone = 0;
                        }
                        if((CE &=0xff)!=0) {
                            if(i==count) {
                                
                                byteCountOrFrenchDone = 1;
                                goto saveState;
                            }
                            dest[i++]=(uint8_t)CE;
                        }
                    }
                } else {
                    notIsContinuation = !isContinuation(CE);
                    if(notIsContinuation) {
                        if(s.flags & UCOL_WAS_HIRAGANA) { 
                            dest[i++] = UCOL_HIRAGANA_QUAD;
                        } else {
                            dest[i++] = 0xFF;
                        }
                    }
                }
                if(uprv_numAvailableExpCEs(s)) {
                    canUpdateState = FALSE;
                } else {
                    canUpdateState = TRUE;
                }
            }
        } else {
            
            
            level = UCOL_PSK_NULL;
        }
        
    case UCOL_PSK_QUIN:
        level = UCOL_PSK_IDENTICAL;
        
    case UCOL_PSK_IDENTICAL:
        if(strength >= UCOL_IDENTICAL) {
            UChar32 first, second;
            int32_t bocsuBytesWritten = 0;
            
            
            if(normIter == NULL) {
                
                
                
                normIter = unorm_openIter(stackNormIter, sizeof(stackNormIter), status);
                s.iterator = unorm_setIter(normIter, iter, UNORM_NFD, status);
            } else if(!doingIdenticalFromStart) {
                
                
                
                
                
                iter->move(iter, 0, UITER_START);
                s.iterator = unorm_setIter(normIter, iter, UNORM_NFD, status);
            }
            
            
            if(U_FAILURE(*status)) {
                UTRACE_EXIT_STATUS(*status);
                return 0;
            }
            first = uiter_previous32(s.iterator);
            
            if(first == U_SENTINEL) {
                first = 0;
            } else {
                uiter_next32(s.iterator);
            }

            j = 0;
            for(;;) {
                if(i == count) {
                    if(j+1 < bocsuBytesWritten) {
                        bocsuBytesUsed = j+1;
                    }
                    goto saveState;
                }

                
                
                
                
                
                
                newState = s.iterator->getState(s.iterator);
                if(newState != UITER_NO_STATE) {
                    iterState = newState;
                    cces = 0;
                }

                uint8_t buff[4];
                second = uiter_next32(s.iterator);
                cces++;

                
                if(second == U_SENTINEL) {
                    terminatePSKLevel(level, maxLevel, i, dest);
                    level = UCOL_PSK_NULL;
                    break;
                }
                bocsuBytesWritten = u_writeIdenticalLevelRunTwoChars(first, second, buff);
                first = second;

                j = 0;
                if(bocsuBytesUsed != 0) {
                    while(bocsuBytesUsed-->0) {
                        j++;
                    }
                }

                while(i < count && j < bocsuBytesWritten) {
                    dest[i++] = buff[j++];
                }
            }

        } else {
            level = UCOL_PSK_NULL;
        }
        
    case UCOL_PSK_NULL:
        j = i;
        while(j<count) {
            dest[j++]=0;
        }
        break;
    default:
        *status = U_INTERNAL_PROGRAM_ERROR;
        UTRACE_EXIT_STATUS(*status);
        return 0;
    }

saveState:
    
    
    if(byteCountOrFrenchDone
        || canUpdateState == FALSE
        || (newState = s.iterator->getState(s.iterator)) == UITER_NO_STATE)
    {
        
        
        
        state[0] = iterState;
    } else {
        
        state[0] = s.iterator->getState(s.iterator);
        cces = 0;
    }
    
    if((bocsuBytesUsed & UCOL_PSK_BOCSU_BYTES_MASK) != bocsuBytesUsed) {
        *status = U_INDEX_OUTOFBOUNDS_ERROR;
    }
    state[1] = (bocsuBytesUsed & UCOL_PSK_BOCSU_BYTES_MASK) << UCOL_PSK_BOCSU_BYTES_SHIFT;

    
    state[1] |= ((level & UCOL_PSK_LEVEL_MASK) << UCOL_PSK_LEVEL_SHIFT);

    
    if(level == UCOL_PSK_SECONDARY && doingFrench) {
        state[1] |= (((int32_t)(state[0] == 0) & UCOL_PSK_BYTE_COUNT_OR_FRENCH_DONE_MASK) << UCOL_PSK_BYTE_COUNT_OR_FRENCH_DONE_SHIFT);
    } else {
        state[1] |= ((byteCountOrFrenchDone & UCOL_PSK_BYTE_COUNT_OR_FRENCH_DONE_MASK) << UCOL_PSK_BYTE_COUNT_OR_FRENCH_DONE_SHIFT);
    }

    
    if(wasShifted) {
        state[1] |= 1 << UCOL_PSK_WAS_SHIFTED_SHIFT;
    }
    
    if((cces & UCOL_PSK_CONSUMED_CES_MASK) != cces) {
        *status = U_INDEX_OUTOFBOUNDS_ERROR;
    }
    
    state[1] |= ((cces & UCOL_PSK_CONSUMED_CES_MASK) << UCOL_PSK_CONSUMED_CES_SHIFT);

    
    if((usedFrench & UCOL_PSK_USED_FRENCH_MASK) != usedFrench) {
        *status = U_INDEX_OUTOFBOUNDS_ERROR;
    }
    
    state[1] |= ((usedFrench & UCOL_PSK_USED_FRENCH_MASK) << UCOL_PSK_USED_FRENCH_SHIFT);


    
    if(normIter != NULL) {
        unorm_closeIter(normIter);
    }

    
    ucol_freeOffsetBuffer(&s);
    
    
    UTRACE_DATA4(UTRACE_VERBOSE, "dest = %vb, state=%d %d",
                  dest,i, state[0], state[1]);
    UTRACE_EXIT_VALUE(i);
    return i;
}




U_CAPI int32_t U_EXPORT2
ucol_getBound(const uint8_t       *source,
        int32_t             sourceLength,
        UColBoundMode       boundType,
        uint32_t            noOfLevels,
        uint8_t             *result,
        int32_t             resultLength,
        UErrorCode          *status)
{
    
    if(status == NULL || U_FAILURE(*status)) {
        return 0;
    }
    if(source == NULL) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    int32_t sourceIndex = 0;
    
    do {
        sourceIndex++;
        if(source[sourceIndex] == UCOL_LEVELTERMINATOR) {
            noOfLevels--;
        }
    } while (noOfLevels > 0
        && (source[sourceIndex] != 0 || sourceIndex < sourceLength));

    if((source[sourceIndex] == 0 || sourceIndex == sourceLength)
        && noOfLevels > 0) {
            *status = U_SORT_KEY_TOO_SHORT_WARNING;
    }


    
    
    
    
    if(result != NULL && resultLength >= sourceIndex+boundType) {
        uprv_memcpy(result, source, sourceIndex);
        switch(boundType) {
            
        case UCOL_BOUND_LOWER: 
            break;
            
        case UCOL_BOUND_UPPER: 
            result[sourceIndex++] = 2;
            break;
            
        case UCOL_BOUND_UPPER_LONG: 
            result[sourceIndex++] = 0xFF;
            result[sourceIndex++] = 0xFF;
            break;
        default:
            *status = U_ILLEGAL_ARGUMENT_ERROR;
            return 0;
        }
        result[sourceIndex++] = 0;

        return sourceIndex;
    } else {
        return sourceIndex+boundType+1;
    }
}






static inline void
ucol_addLatinOneEntry(UCollator *coll, UChar ch, uint32_t CE,
                    int32_t *primShift, int32_t *secShift, int32_t *terShift)
{
    uint8_t primary1 = 0, primary2 = 0, secondary = 0, tertiary = 0;
    UBool reverseSecondary = FALSE;
    UBool continuation = isContinuation(CE);
    if(!continuation) {
        tertiary = (uint8_t)((CE & coll->tertiaryMask));
        tertiary ^= coll->caseSwitch;
        reverseSecondary = TRUE;
    } else {
        tertiary = (uint8_t)((CE & UCOL_REMOVE_CONTINUATION));
        tertiary &= UCOL_REMOVE_CASE;
        reverseSecondary = FALSE;
    }

    secondary = (uint8_t)((CE >>= 8) & UCOL_BYTE_SIZE_MASK);
    primary2 = (uint8_t)((CE >>= 8) & UCOL_BYTE_SIZE_MASK);
    primary1 = (uint8_t)(CE >> 8);

    if(primary1 != 0) {
        if (coll->leadBytePermutationTable != NULL && !continuation) {
            primary1 = coll->leadBytePermutationTable[primary1];
        }

        coll->latinOneCEs[ch] |= (primary1 << *primShift);
        *primShift -= 8;
    }
    if(primary2 != 0) {
        if(*primShift < 0) {
            coll->latinOneCEs[ch] = UCOL_BAIL_OUT_CE;
            coll->latinOneCEs[coll->latinOneTableLen+ch] = UCOL_BAIL_OUT_CE;
            coll->latinOneCEs[2*coll->latinOneTableLen+ch] = UCOL_BAIL_OUT_CE;
            return;
        }
        coll->latinOneCEs[ch] |= (primary2 << *primShift);
        *primShift -= 8;
    }
    if(secondary != 0) {
        if(reverseSecondary && coll->frenchCollation == UCOL_ON) { 
            coll->latinOneCEs[coll->latinOneTableLen+ch] >>= 8; 
            coll->latinOneCEs[coll->latinOneTableLen+ch] |= (secondary << 24);
        } else { 
            coll->latinOneCEs[coll->latinOneTableLen+ch] |= (secondary << *secShift);
        }
        *secShift -= 8;
    }
    if(tertiary != 0) {
        coll->latinOneCEs[2*coll->latinOneTableLen+ch] |= (tertiary << *terShift);
        *terShift -= 8;
    }
}

static inline UBool
ucol_resizeLatinOneTable(UCollator *coll, int32_t size, UErrorCode *status) {
    uint32_t *newTable = (uint32_t *)uprv_malloc(size*sizeof(uint32_t)*3);
    if(newTable == NULL) {
      *status = U_MEMORY_ALLOCATION_ERROR;
      coll->latinOneFailed = TRUE;
      return FALSE;
    }
    int32_t sizeToCopy = ((size<coll->latinOneTableLen)?size:coll->latinOneTableLen)*sizeof(uint32_t);
    uprv_memset(newTable, 0, size*sizeof(uint32_t)*3);
    uprv_memcpy(newTable, coll->latinOneCEs, sizeToCopy);
    uprv_memcpy(newTable+size, coll->latinOneCEs+coll->latinOneTableLen, sizeToCopy);
    uprv_memcpy(newTable+2*size, coll->latinOneCEs+2*coll->latinOneTableLen, sizeToCopy);
    coll->latinOneTableLen = size;
    uprv_free(coll->latinOneCEs);
    coll->latinOneCEs = newTable;
    return TRUE;
}

static UBool
ucol_setUpLatinOne(UCollator *coll, UErrorCode *status) {
    UBool result = TRUE;
    if(coll->latinOneCEs == NULL) {
        coll->latinOneCEs = (uint32_t *)uprv_malloc(sizeof(uint32_t)*UCOL_LATINONETABLELEN*3);
        if(coll->latinOneCEs == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return FALSE;
        }
        coll->latinOneTableLen = UCOL_LATINONETABLELEN;
    }
    UChar ch = 0;
    UCollationElements *it = ucol_openElements(coll, &ch, 1, status);
    
    if (U_FAILURE(*status)) {
        ucol_closeElements(it);
        return FALSE;
    }
    uprv_memset(coll->latinOneCEs, 0, sizeof(uint32_t)*coll->latinOneTableLen*3);

    int32_t primShift = 24, secShift = 24, terShift = 24;
    uint32_t CE = 0;
    int32_t contractionOffset = UCOL_ENDOFLATINONERANGE+1;

    
    for(ch = 0; ch <= UCOL_ENDOFLATINONERANGE; ch++) {
        primShift = 24; secShift = 24; terShift = 24;
        if(ch < 0x100) {
            CE = coll->latinOneMapping[ch];
        } else {
            CE = UTRIE_GET32_FROM_LEAD(&coll->mapping, ch);
            if(CE == UCOL_NOT_FOUND && coll->UCA) {
                CE = UTRIE_GET32_FROM_LEAD(&coll->UCA->mapping, ch);
            }
        }
        if(CE < UCOL_NOT_FOUND) {
            ucol_addLatinOneEntry(coll, ch, CE, &primShift, &secShift, &terShift);
        } else {
            switch (getCETag(CE)) {
            case EXPANSION_TAG:
            case DIGIT_TAG:
                ucol_setText(it, &ch, 1, status);
                while((int32_t)(CE = ucol_next(it, status)) != UCOL_NULLORDER) {
                    if(primShift < 0 || secShift < 0 || terShift < 0) {
                        coll->latinOneCEs[ch] = UCOL_BAIL_OUT_CE;
                        coll->latinOneCEs[coll->latinOneTableLen+ch] = UCOL_BAIL_OUT_CE;
                        coll->latinOneCEs[2*coll->latinOneTableLen+ch] = UCOL_BAIL_OUT_CE;
                        break;
                    }
                    ucol_addLatinOneEntry(coll, ch, CE, &primShift, &secShift, &terShift);
                }
                break;
            case CONTRACTION_TAG:
                
                
                
                
                
                {
                    if((CE & 0x00FFF000) != 0) {
                        *status = U_UNSUPPORTED_ERROR;
                        goto cleanup_after_failure;
                    }

                    const UChar *UCharOffset = (UChar *)coll->image+getContractOffset(CE);

                    CE |= (contractionOffset & 0xFFF) << 12; 

                    coll->latinOneCEs[ch] = CE;
                    coll->latinOneCEs[coll->latinOneTableLen+ch] = CE;
                    coll->latinOneCEs[2*coll->latinOneTableLen+ch] = CE;

                    
                    
                    do {
                        CE = *(coll->contractionCEs +
                            (UCharOffset - coll->contractionIndex));
                        if(CE > UCOL_NOT_FOUND && getCETag(CE) == EXPANSION_TAG) {
                            uint32_t size;
                            uint32_t i;    
                            uint32_t *CEOffset = (uint32_t *)coll->image+getExpansionOffset(CE); 
                            size = getExpansionCount(CE);
                            
                            if(size != 0) { 
                                for(i = 0; i<size; i++) {
                                    if(primShift < 0 || secShift < 0 || terShift < 0) {
                                        coll->latinOneCEs[(UChar)contractionOffset] = UCOL_BAIL_OUT_CE;
                                        coll->latinOneCEs[coll->latinOneTableLen+(UChar)contractionOffset] = UCOL_BAIL_OUT_CE;
                                        coll->latinOneCEs[2*coll->latinOneTableLen+(UChar)contractionOffset] = UCOL_BAIL_OUT_CE;
                                        break;
                                    }
                                    ucol_addLatinOneEntry(coll, (UChar)contractionOffset, *CEOffset++, &primShift, &secShift, &terShift);
                                }
                            } else { 
                                while(*CEOffset != 0) {
                                    if(primShift < 0 || secShift < 0 || terShift < 0) {
                                        coll->latinOneCEs[(UChar)contractionOffset] = UCOL_BAIL_OUT_CE;
                                        coll->latinOneCEs[coll->latinOneTableLen+(UChar)contractionOffset] = UCOL_BAIL_OUT_CE;
                                        coll->latinOneCEs[2*coll->latinOneTableLen+(UChar)contractionOffset] = UCOL_BAIL_OUT_CE;
                                        break;
                                    }
                                    ucol_addLatinOneEntry(coll, (UChar)contractionOffset, *CEOffset++, &primShift, &secShift, &terShift);
                                }
                            }
                            contractionOffset++;
                        } else if(CE < UCOL_NOT_FOUND) {
                            ucol_addLatinOneEntry(coll, (UChar)contractionOffset++, CE, &primShift, &secShift, &terShift);
                        } else {
                            coll->latinOneCEs[(UChar)contractionOffset] = UCOL_BAIL_OUT_CE;
                            coll->latinOneCEs[coll->latinOneTableLen+(UChar)contractionOffset] = UCOL_BAIL_OUT_CE;
                            coll->latinOneCEs[2*coll->latinOneTableLen+(UChar)contractionOffset] = UCOL_BAIL_OUT_CE;
                            contractionOffset++;
                        }
                        UCharOffset++;
                        primShift = 24; secShift = 24; terShift = 24;
                        if(contractionOffset == coll->latinOneTableLen) { 
                            if(!ucol_resizeLatinOneTable(coll, 2*coll->latinOneTableLen, status)) {
                                goto cleanup_after_failure;
                            }
                        }
                    } while(*UCharOffset != 0xFFFF);
                }
                break;;
            case SPEC_PROC_TAG:
                {
                    
                    
                    
                    if (ch==0xb7) {
                        ucol_addLatinOneEntry(coll, ch, CE, &primShift, &secShift, &terShift);
                    }
                    else {
                        goto cleanup_after_failure;
                    }
                }
                break;
            default:
                goto cleanup_after_failure;
            }
        }
    }
    
    if(contractionOffset < coll->latinOneTableLen) {
        if(!ucol_resizeLatinOneTable(coll, contractionOffset, status)) {
            goto cleanup_after_failure;
        }
    }
    ucol_closeElements(it);
    return result;

cleanup_after_failure:
    
    coll->latinOneFailed = TRUE;
    ucol_closeElements(it);
    return FALSE;
}

void ucol_updateInternalState(UCollator *coll, UErrorCode *status) {
    if(U_SUCCESS(*status)) {
        if(coll->caseFirst == UCOL_UPPER_FIRST) {
            coll->caseSwitch = UCOL_CASE_SWITCH;
        } else {
            coll->caseSwitch = UCOL_NO_CASE_SWITCH;
        }

        if(coll->caseLevel == UCOL_ON || coll->caseFirst == UCOL_OFF) {
            coll->tertiaryMask = UCOL_REMOVE_CASE;
            coll->tertiaryCommon = UCOL_COMMON3_NORMAL;
            coll->tertiaryAddition = (int8_t)UCOL_FLAG_BIT_MASK_CASE_SW_OFF; 
            coll->tertiaryTop = UCOL_COMMON_TOP3_CASE_SW_OFF;
            coll->tertiaryBottom = UCOL_COMMON_BOT3;
        } else {
            coll->tertiaryMask = UCOL_KEEP_CASE;
            coll->tertiaryAddition = UCOL_FLAG_BIT_MASK_CASE_SW_ON;
            if(coll->caseFirst == UCOL_UPPER_FIRST) {
                coll->tertiaryCommon = UCOL_COMMON3_UPPERFIRST;
                coll->tertiaryTop = UCOL_COMMON_TOP3_CASE_SW_UPPER;
                coll->tertiaryBottom = UCOL_COMMON_BOTTOM3_CASE_SW_UPPER;
            } else {
                coll->tertiaryCommon = UCOL_COMMON3_NORMAL;
                coll->tertiaryTop = UCOL_COMMON_TOP3_CASE_SW_LOWER;
                coll->tertiaryBottom = UCOL_COMMON_BOTTOM3_CASE_SW_LOWER;
            }
        }

        
        uint8_t tertiaryTotal = (uint8_t)(coll->tertiaryTop - coll->tertiaryBottom - 1);
        coll->tertiaryTopCount = (uint8_t)(UCOL_PROPORTION3*tertiaryTotal); 
        coll->tertiaryBottomCount = (uint8_t)(tertiaryTotal - coll->tertiaryTopCount);

        if(coll->caseLevel == UCOL_OFF && coll->strength == UCOL_TERTIARY
            && coll->frenchCollation == UCOL_OFF && coll->alternateHandling == UCOL_NON_IGNORABLE)
        {
            coll->sortKeyGen = ucol_calcSortKeySimpleTertiary;
        } else {
            coll->sortKeyGen = ucol_calcSortKey;
        }
        if(coll->caseLevel == UCOL_OFF && coll->strength <= UCOL_TERTIARY && coll->numericCollation == UCOL_OFF
            && coll->alternateHandling == UCOL_NON_IGNORABLE && !coll->latinOneFailed)
        {
            if(coll->latinOneCEs == NULL || coll->latinOneRegenTable) {
                if(ucol_setUpLatinOne(coll, status)) { 
                    
                    coll->latinOneUse = TRUE;
                } else {
                    coll->latinOneUse = FALSE;
                }
                if(*status == U_UNSUPPORTED_ERROR) {
                    *status = U_ZERO_ERROR;
                }
            } else { 
                coll->latinOneUse = TRUE;
            }
        } else {
            coll->latinOneUse = FALSE;
        }
    }
}

U_CAPI uint32_t  U_EXPORT2
ucol_setVariableTop(UCollator *coll, const UChar *varTop, int32_t len, UErrorCode *status) {
    if(U_FAILURE(*status) || coll == NULL) {
        return 0;
    }
    if(len == -1) {
        len = u_strlen(varTop);
    }
    if(len == 0) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    if(coll->delegate!=NULL) {
      return ((Collator*)coll->delegate)->setVariableTop(varTop, len, *status);
    }


    collIterate s;
    IInit_collIterate(coll, varTop, len, &s, status);
    if(U_FAILURE(*status)) {
        return 0;
    }

    uint32_t CE = ucol_IGetNextCE(coll, &s, status);

    
    
    
    if(s.pos != s.endp || CE == UCOL_NO_MORE_CES) {
        *status = U_CE_NOT_FOUND_ERROR;
        return 0;
    }

    uint32_t nextCE = ucol_IGetNextCE(coll, &s, status);

    if(isContinuation(nextCE) && (nextCE & UCOL_PRIMARYMASK) != 0) {
        *status = U_PRIMARY_TOO_LONG_ERROR;
        return 0;
    }
    if(coll->variableTopValue != (CE & UCOL_PRIMARYMASK)>>16) {
        coll->variableTopValueisDefault = FALSE;
        coll->variableTopValue = (CE & UCOL_PRIMARYMASK)>>16;
    }
    
    
    ucol_freeOffsetBuffer(&s);

    return CE & UCOL_PRIMARYMASK;
}

U_CAPI uint32_t U_EXPORT2 ucol_getVariableTop(const UCollator *coll, UErrorCode *status) {
    if(U_FAILURE(*status) || coll == NULL) {
        return 0;
    }
    if(coll->delegate!=NULL) {
      return ((const Collator*)coll->delegate)->getVariableTop(*status);
    }
    return coll->variableTopValue<<16;
}

U_CAPI void  U_EXPORT2
ucol_restoreVariableTop(UCollator *coll, const uint32_t varTop, UErrorCode *status) {
    if(U_FAILURE(*status) || coll == NULL) {
        return;
    }

    if(coll->variableTopValue != (varTop & UCOL_PRIMARYMASK)>>16) {
        coll->variableTopValueisDefault = FALSE;
        coll->variableTopValue = (varTop & UCOL_PRIMARYMASK)>>16;
    }
}

U_CAPI void  U_EXPORT2
ucol_setAttribute(UCollator *coll, UColAttribute attr, UColAttributeValue value, UErrorCode *status) {
    if(U_FAILURE(*status) || coll == NULL) {
      return;
    }

    if(coll->delegate != NULL) {
      ((Collator*)coll->delegate)->setAttribute(attr,value,*status);
      return;
    }

    UColAttributeValue oldFrench = coll->frenchCollation;
    UColAttributeValue oldCaseFirst = coll->caseFirst;
    switch(attr) {
    case UCOL_NUMERIC_COLLATION: 
        if(value == UCOL_ON) {
            coll->numericCollation = UCOL_ON;
            coll->numericCollationisDefault = FALSE;
        } else if (value == UCOL_OFF) {
            coll->numericCollation = UCOL_OFF;
            coll->numericCollationisDefault = FALSE;
        } else if (value == UCOL_DEFAULT) {
            coll->numericCollationisDefault = TRUE;
            coll->numericCollation = (UColAttributeValue)coll->options->numericCollation;
        } else {
            *status = U_ILLEGAL_ARGUMENT_ERROR;
        }
        break;
    case UCOL_HIRAGANA_QUATERNARY_MODE: 
        if(value == UCOL_ON || value == UCOL_OFF || value == UCOL_DEFAULT) {
            
            
            
            
        } else {
            *status = U_ILLEGAL_ARGUMENT_ERROR;
        }
        break;
    case UCOL_FRENCH_COLLATION: 
        if(value == UCOL_ON) {
            coll->frenchCollation = UCOL_ON;
            coll->frenchCollationisDefault = FALSE;
        } else if (value == UCOL_OFF) {
            coll->frenchCollation = UCOL_OFF;
            coll->frenchCollationisDefault = FALSE;
        } else if (value == UCOL_DEFAULT) {
            coll->frenchCollationisDefault = TRUE;
            coll->frenchCollation = (UColAttributeValue)coll->options->frenchCollation;
        } else {
            *status = U_ILLEGAL_ARGUMENT_ERROR  ;
        }
        break;
    case UCOL_ALTERNATE_HANDLING: 
        if(value == UCOL_SHIFTED) {
            coll->alternateHandling = UCOL_SHIFTED;
            coll->alternateHandlingisDefault = FALSE;
        } else if (value == UCOL_NON_IGNORABLE) {
            coll->alternateHandling = UCOL_NON_IGNORABLE;
            coll->alternateHandlingisDefault = FALSE;
        } else if (value == UCOL_DEFAULT) {
            coll->alternateHandlingisDefault = TRUE;
            coll->alternateHandling = (UColAttributeValue)coll->options->alternateHandling ;
        } else {
            *status = U_ILLEGAL_ARGUMENT_ERROR  ;
        }
        break;
    case UCOL_CASE_FIRST: 
        if(value == UCOL_LOWER_FIRST) {
            coll->caseFirst = UCOL_LOWER_FIRST;
            coll->caseFirstisDefault = FALSE;
        } else if (value == UCOL_UPPER_FIRST) {
            coll->caseFirst = UCOL_UPPER_FIRST;
            coll->caseFirstisDefault = FALSE;
        } else if (value == UCOL_OFF) {
            coll->caseFirst = UCOL_OFF;
            coll->caseFirstisDefault = FALSE;
        } else if (value == UCOL_DEFAULT) {
            coll->caseFirst = (UColAttributeValue)coll->options->caseFirst;
            coll->caseFirstisDefault = TRUE;
        } else {
            *status = U_ILLEGAL_ARGUMENT_ERROR  ;
        }
        break;
    case UCOL_CASE_LEVEL: 
        if(value == UCOL_ON) {
            coll->caseLevel = UCOL_ON;
            coll->caseLevelisDefault = FALSE;
        } else if (value == UCOL_OFF) {
            coll->caseLevel = UCOL_OFF;
            coll->caseLevelisDefault = FALSE;
        } else if (value == UCOL_DEFAULT) {
            coll->caseLevel = (UColAttributeValue)coll->options->caseLevel;
            coll->caseLevelisDefault = TRUE;
        } else {
            *status = U_ILLEGAL_ARGUMENT_ERROR  ;
        }
        break;
    case UCOL_NORMALIZATION_MODE: 
        if(value == UCOL_ON) {
            coll->normalizationMode = UCOL_ON;
            coll->normalizationModeisDefault = FALSE;
            initializeFCD(status);
        } else if (value == UCOL_OFF) {
            coll->normalizationMode = UCOL_OFF;
            coll->normalizationModeisDefault = FALSE;
        } else if (value == UCOL_DEFAULT) {
            coll->normalizationModeisDefault = TRUE;
            coll->normalizationMode = (UColAttributeValue)coll->options->normalizationMode;
            if(coll->normalizationMode == UCOL_ON) {
                initializeFCD(status);
            }
        } else {
            *status = U_ILLEGAL_ARGUMENT_ERROR  ;
        }
        break;
    case UCOL_STRENGTH:         
        if (value == UCOL_DEFAULT) {
            coll->strengthisDefault = TRUE;
            coll->strength = (UColAttributeValue)coll->options->strength;
        } else if (value <= UCOL_IDENTICAL) {
            coll->strengthisDefault = FALSE;
            coll->strength = value;
        } else {
            *status = U_ILLEGAL_ARGUMENT_ERROR  ;
        }
        break;
    case UCOL_ATTRIBUTE_COUNT:
    default:
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        break;
    }
    if(oldFrench != coll->frenchCollation || oldCaseFirst != coll->caseFirst) {
        coll->latinOneRegenTable = TRUE;
    } else {
        coll->latinOneRegenTable = FALSE;
    }
    ucol_updateInternalState(coll, status);
}

U_CAPI UColAttributeValue  U_EXPORT2
ucol_getAttribute(const UCollator *coll, UColAttribute attr, UErrorCode *status) {
    if(U_FAILURE(*status) || coll == NULL) {
      return UCOL_DEFAULT;
    }

    if(coll->delegate != NULL) {
      return ((Collator*)coll->delegate)->getAttribute(attr,*status);
    }

    switch(attr) {
    case UCOL_NUMERIC_COLLATION:
      return coll->numericCollation;
    case UCOL_HIRAGANA_QUATERNARY_MODE:
      return coll->hiraganaQ;
    case UCOL_FRENCH_COLLATION: 
        return coll->frenchCollation;
    case UCOL_ALTERNATE_HANDLING: 
        return coll->alternateHandling;
    case UCOL_CASE_FIRST: 
        return coll->caseFirst;
    case UCOL_CASE_LEVEL: 
        return coll->caseLevel;
    case UCOL_NORMALIZATION_MODE: 
        return coll->normalizationMode;
    case UCOL_STRENGTH:         
        return coll->strength;
    case UCOL_ATTRIBUTE_COUNT:
    default:
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        break;
    }
    return UCOL_DEFAULT;
}

U_CAPI void U_EXPORT2
ucol_setStrength(    UCollator                *coll,
            UCollationStrength        strength)
{
    UErrorCode status = U_ZERO_ERROR;
    ucol_setAttribute(coll, UCOL_STRENGTH, strength, &status);
}

U_CAPI UCollationStrength U_EXPORT2
ucol_getStrength(const UCollator *coll)
{
    UErrorCode status = U_ZERO_ERROR;
    return ucol_getAttribute(coll, UCOL_STRENGTH, &status);
}

U_CAPI int32_t U_EXPORT2 
ucol_getReorderCodes(const UCollator *coll,
                    int32_t *dest,
                    int32_t destCapacity,
                    UErrorCode *status) {
    if (U_FAILURE(*status)) {
        return 0;
    }

    if(coll->delegate!=NULL) {
      return ((const Collator*)coll->delegate)->getReorderCodes(dest, destCapacity, *status);
    }

    if (destCapacity < 0 || (destCapacity > 0 && dest == NULL)) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

#ifdef UCOL_DEBUG
    printf("coll->reorderCodesLength = %d\n", coll->reorderCodesLength);
    printf("coll->defaultReorderCodesLength = %d\n", coll->defaultReorderCodesLength);
#endif

    if (coll->reorderCodesLength > destCapacity) {
        *status = U_BUFFER_OVERFLOW_ERROR;
        return coll->reorderCodesLength;
    }
    for (int32_t i = 0; i < coll->reorderCodesLength; i++) {
        dest[i] = coll->reorderCodes[i];
    }
    return coll->reorderCodesLength;
}

U_CAPI void U_EXPORT2 
ucol_setReorderCodes(UCollator* coll,
                    const int32_t* reorderCodes,
                    int32_t reorderCodesLength,
                    UErrorCode *status) {
    if (U_FAILURE(*status)) {
        return;
    }

    if (reorderCodesLength < 0 || (reorderCodesLength > 0 && reorderCodes == NULL)) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }

    if(coll->delegate!=NULL) {
      ((Collator*)coll->delegate)->setReorderCodes(reorderCodes, reorderCodesLength, *status);
      return;
    }
    
    if (coll->reorderCodes != NULL && coll->freeReorderCodesOnClose == TRUE) {
        uprv_free(coll->reorderCodes);
    }
    coll->reorderCodes = NULL;
    coll->reorderCodesLength = 0;
    if (reorderCodesLength == 0) {
        if (coll->leadBytePermutationTable != NULL && coll->freeLeadBytePermutationTableOnClose == TRUE) {
            uprv_free(coll->leadBytePermutationTable);
        }
        coll->leadBytePermutationTable = NULL;
        return;
    }
    coll->reorderCodes = (int32_t*) uprv_malloc(reorderCodesLength * sizeof(int32_t));
    if (coll->reorderCodes == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    coll->freeReorderCodesOnClose = TRUE;
    for (int32_t i = 0; i < reorderCodesLength; i++) {
        coll->reorderCodes[i] = reorderCodes[i];
    }
    coll->reorderCodesLength = reorderCodesLength;
    ucol_buildPermutationTable(coll, status);
}

U_CAPI int32_t U_EXPORT2 
ucol_getEquivalentReorderCodes(int32_t reorderCode,
                    int32_t* dest,
                    int32_t destCapacity,
                    UErrorCode *pErrorCode) {
    bool equivalentCodesSet[USCRIPT_CODE_LIMIT];
    uint16_t leadBytes[256];
    int leadBytesCount;
    int leadByteIndex;
    int16_t reorderCodesForLeadByte[USCRIPT_CODE_LIMIT];
    int reorderCodesForLeadByteCount;
    int reorderCodeIndex;
    
    int32_t equivalentCodesCount = 0;
    int setIndex;
    
    if (U_FAILURE(*pErrorCode)) {
        return 0;
    }

    if (destCapacity < 0 || (destCapacity > 0 && dest == NULL)) {
        *pErrorCode = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    uprv_memset(equivalentCodesSet, 0, USCRIPT_CODE_LIMIT * sizeof(bool));

    const UCollator* uca = ucol_initUCA(pErrorCode);
    if (U_FAILURE(*pErrorCode)) {
	return 0;
    }
    leadBytesCount = ucol_getLeadBytesForReorderCode(uca, reorderCode, leadBytes, 256);
    for (leadByteIndex = 0; leadByteIndex < leadBytesCount; leadByteIndex++) {
        reorderCodesForLeadByteCount = ucol_getReorderCodesForLeadByte(
            uca, leadBytes[leadByteIndex], reorderCodesForLeadByte, USCRIPT_CODE_LIMIT);
        for (reorderCodeIndex = 0; reorderCodeIndex < reorderCodesForLeadByteCount; reorderCodeIndex++) {
            equivalentCodesSet[reorderCodesForLeadByte[reorderCodeIndex]] = true;
        }
    }
    
    for (setIndex = 0; setIndex < USCRIPT_CODE_LIMIT; setIndex++) {
        if (equivalentCodesSet[setIndex] == true) {
            equivalentCodesCount++;
        }
    }
    
    if (destCapacity == 0) {
        return equivalentCodesCount;
    }
    
    equivalentCodesCount = 0;
    for (setIndex = 0; setIndex < USCRIPT_CODE_LIMIT; setIndex++) {
        if (equivalentCodesSet[setIndex] == true) {
            dest[equivalentCodesCount++] = setIndex;
            if (equivalentCodesCount >= destCapacity) {
                break;
            }
        }        
    }
    return equivalentCodesCount;
}







U_CAPI void U_EXPORT2
ucol_getVersion(const UCollator* coll,
                UVersionInfo versionInfo)
{
    if(coll->delegate!=NULL) {
      ((const Collator*)coll->delegate)->getVersion(versionInfo);
      return;
    }
    
    uint8_t rtVersion = UCOL_RUNTIME_VERSION;
    
    uint8_t bdVersion = coll->image->version[0];

    



    uint8_t csVersion = 0;

    
    uint16_t cmbVersion = (uint16_t)((rtVersion<<11) | (bdVersion<<6) | (csVersion));

    
    versionInfo[0] = (uint8_t)(cmbVersion>>8);
    versionInfo[1] = (uint8_t)cmbVersion;
    versionInfo[2] = coll->image->version[1];
    if(coll->UCA) {
        
        versionInfo[3] = (coll->UCA->image->UCAVersion[0] & 0x1f) << 3 | (coll->UCA->image->UCAVersion[1] & 0x07);
    } else {
        versionInfo[3] = 0;
    }
}



U_CAPI UBool  U_EXPORT2
ucol_isTailored(const UCollator *coll, const UChar u, UErrorCode *status) {
    if(U_FAILURE(*status) || coll == NULL || coll == coll->UCA) {
        return FALSE;
    }

    uint32_t CE = UCOL_NOT_FOUND;
    const UChar *ContractionStart = NULL;
    if(u < 0x100) { 
        CE = coll->latinOneMapping[u];
        if(coll->UCA && CE == coll->UCA->latinOneMapping[u]) {
            return FALSE;
        }
    } else { 
        CE = UTRIE_GET32_FROM_LEAD(&coll->mapping, u);
    }

    if(isContraction(CE)) {
        ContractionStart = (UChar *)coll->image+getContractOffset(CE);
        CE = *(coll->contractionCEs + (ContractionStart- coll->contractionIndex));
    }

    return (UBool)(CE != UCOL_NOT_FOUND);
}















static
UCollationResult    ucol_checkIdent(collIterate *sColl, collIterate *tColl, UBool normalize, UErrorCode *status)
{
    
    
    int32_t            comparison;

    if (sColl->flags & UCOL_USE_ITERATOR) {
        
        
        
        UAlignedMemory stackNormIter1[UNORM_ITER_SIZE/sizeof(UAlignedMemory)];
        UAlignedMemory stackNormIter2[UNORM_ITER_SIZE/sizeof(UAlignedMemory)];
        UNormIterator *sNIt = NULL, *tNIt = NULL;
        sNIt = unorm_openIter(stackNormIter1, sizeof(stackNormIter1), status);
        tNIt = unorm_openIter(stackNormIter2, sizeof(stackNormIter2), status);
        sColl->iterator->move(sColl->iterator, 0, UITER_START);
        tColl->iterator->move(tColl->iterator, 0, UITER_START);
        UCharIterator *sIt = unorm_setIter(sNIt, sColl->iterator, UNORM_NFD, status);
        UCharIterator *tIt = unorm_setIter(tNIt, tColl->iterator, UNORM_NFD, status);
        comparison = u_strCompareIter(sIt, tIt, TRUE);
        unorm_closeIter(sNIt);
        unorm_closeIter(tNIt);
    } else {
        int32_t sLen      = (sColl->flags & UCOL_ITER_HASLEN) ? (int32_t)(sColl->endp - sColl->string) : -1;
        const UChar *sBuf = sColl->string;
        int32_t tLen      = (tColl->flags & UCOL_ITER_HASLEN) ? (int32_t)(tColl->endp - tColl->string) : -1;
        const UChar *tBuf = tColl->string;

        if (normalize) {
            *status = U_ZERO_ERROR;
            
            
            
            
            
            sColl->nfd->normalize(UnicodeString((sColl->flags & UCOL_ITER_HASLEN) == 0, sBuf, sLen),
                                  sColl->writableBuffer,
                                  *status);
            tColl->nfd->normalize(UnicodeString((tColl->flags & UCOL_ITER_HASLEN) == 0, tBuf, tLen),
                                  tColl->writableBuffer,
                                  *status);
            if(U_FAILURE(*status)) {
                return UCOL_LESS;
            }
            comparison = sColl->writableBuffer.compareCodePointOrder(tColl->writableBuffer);
        } else {
            comparison = u_strCompare(sBuf, sLen, tBuf, tLen, TRUE);
        }
    }

    if (comparison < 0) {
        return UCOL_LESS;
    } else if (comparison == 0) {
        return UCOL_EQUAL;
    } else  {
        return UCOL_GREATER;
    }
}




#define UCOL_CEBUF_SIZE 512
typedef struct ucol_CEBuf {
    uint32_t    *buf;
    uint32_t    *endp;
    uint32_t    *pos;
    uint32_t     localArray[UCOL_CEBUF_SIZE];
} ucol_CEBuf;


static
inline void UCOL_INIT_CEBUF(ucol_CEBuf *b) {
    (b)->buf = (b)->pos = (b)->localArray;
    (b)->endp = (b)->buf + UCOL_CEBUF_SIZE;
}

static
void ucol_CEBuf_Expand(ucol_CEBuf *b, collIterate *ci, UErrorCode *status) {
    uint32_t  oldSize;
    uint32_t  newSize;
    uint32_t  *newBuf;

    ci->flags |= UCOL_ITER_ALLOCATED;
    oldSize = (uint32_t)(b->pos - b->buf);
    newSize = oldSize * 2;
    newBuf = (uint32_t *)uprv_malloc(newSize * sizeof(uint32_t));
    if(newBuf == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
    }
    else {
        uprv_memcpy(newBuf, b->buf, oldSize * sizeof(uint32_t));
        if (b->buf != b->localArray) {
            uprv_free(b->buf);
        }
        b->buf = newBuf;
        b->endp = b->buf + newSize;
        b->pos  = b->buf + oldSize;
    }
}

static
inline void UCOL_CEBUF_PUT(ucol_CEBuf *b, uint32_t ce, collIterate *ci, UErrorCode *status) {
    if (b->pos == b->endp) {
        ucol_CEBuf_Expand(b, ci, status);
    }
    if (U_SUCCESS(*status)) {
        *(b)->pos++ = ce;
    }
}



static UCollationResult ucol_compareUsingSortKeys(collIterate *sColl,
                                                  collIterate *tColl,
                                                  UErrorCode *status)
{
    uint8_t sourceKey[UCOL_MAX_BUFFER], targetKey[UCOL_MAX_BUFFER];
    uint8_t *sourceKeyP = sourceKey;
    uint8_t *targetKeyP = targetKey;
    int32_t sourceKeyLen = UCOL_MAX_BUFFER, targetKeyLen = UCOL_MAX_BUFFER;
    const UCollator *coll = sColl->coll;
    const UChar *source = NULL;
    const UChar *target = NULL;
    int32_t result = UCOL_EQUAL;
    UnicodeString sourceString, targetString;
    int32_t sourceLength;
    int32_t targetLength;

    if(sColl->flags & UCOL_USE_ITERATOR) {
        sColl->iterator->move(sColl->iterator, 0, UITER_START);
        tColl->iterator->move(tColl->iterator, 0, UITER_START);
        UChar32 c;
        while((c=sColl->iterator->next(sColl->iterator))>=0) {
            sourceString.append((UChar)c);
        }
        while((c=tColl->iterator->next(tColl->iterator))>=0) {
            targetString.append((UChar)c);
        }
        source = sourceString.getBuffer();
        sourceLength = sourceString.length();
        target = targetString.getBuffer();
        targetLength = targetString.length();
    } else { 
        sourceLength = (sColl->flags&UCOL_ITER_HASLEN)?(int32_t)(sColl->endp-sColl->string):-1;
        targetLength = (tColl->flags&UCOL_ITER_HASLEN)?(int32_t)(tColl->endp-tColl->string):-1;
        source = sColl->string;
        target = tColl->string;
    }



    sourceKeyLen = ucol_getSortKey(coll, source, sourceLength, sourceKeyP, sourceKeyLen);
    if(sourceKeyLen > UCOL_MAX_BUFFER) {
        sourceKeyP = (uint8_t*)uprv_malloc(sourceKeyLen*sizeof(uint8_t));
        if(sourceKeyP == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            goto cleanup_and_do_compare;
        }
        sourceKeyLen = ucol_getSortKey(coll, source, sourceLength, sourceKeyP, sourceKeyLen);
    }

    targetKeyLen = ucol_getSortKey(coll, target, targetLength, targetKeyP, targetKeyLen);
    if(targetKeyLen > UCOL_MAX_BUFFER) {
        targetKeyP = (uint8_t*)uprv_malloc(targetKeyLen*sizeof(uint8_t));
        if(targetKeyP == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            goto cleanup_and_do_compare;
        }
        targetKeyLen = ucol_getSortKey(coll, target, targetLength, targetKeyP, targetKeyLen);
    }

    result = uprv_strcmp((const char*)sourceKeyP, (const char*)targetKeyP);

cleanup_and_do_compare:
    if(sourceKeyP != NULL && sourceKeyP != sourceKey) {
        uprv_free(sourceKeyP);
    }

    if(targetKeyP != NULL && targetKeyP != targetKey) {
        uprv_free(targetKeyP);
    }

    if(result<0) {
        return UCOL_LESS;
    } else if(result>0) {
        return UCOL_GREATER;
    } else {
        return UCOL_EQUAL;
    }
}


static UCollationResult
ucol_strcollRegular(collIterate *sColl, collIterate *tColl, UErrorCode *status)
{
    U_ALIGN_CODE(16);

    const UCollator *coll = sColl->coll;


    
    UColAttributeValue strength = coll->strength;
    UBool initialCheckSecTer = (strength  >= UCOL_SECONDARY);

    UBool checkSecTer = initialCheckSecTer;
    UBool checkTertiary = (strength  >= UCOL_TERTIARY);
    UBool checkQuad = (strength  >= UCOL_QUATERNARY);
    UBool checkIdent = (strength == UCOL_IDENTICAL);
    UBool checkCase = (coll->caseLevel == UCOL_ON);
    UBool isFrenchSec = (coll->frenchCollation == UCOL_ON) && checkSecTer;
    UBool shifted = (coll->alternateHandling == UCOL_SHIFTED);
    UBool qShifted = shifted && checkQuad;
    UBool doHiragana = (coll->hiraganaQ == UCOL_ON) && checkQuad;

    if(doHiragana && shifted) {
        return (ucol_compareUsingSortKeys(sColl, tColl, status));
    }
    uint8_t caseSwitch = coll->caseSwitch;
    uint8_t tertiaryMask = coll->tertiaryMask;

    
    uint32_t LVT = (shifted)?(coll->variableTopValue<<16):0;

    UCollationResult result = UCOL_EQUAL;
    UCollationResult hirResult = UCOL_EQUAL;

    
    ucol_CEBuf   sCEs;
    ucol_CEBuf   tCEs;
    UCOL_INIT_CEBUF(&sCEs);
    UCOL_INIT_CEBUF(&tCEs);

    uint32_t secS = 0, secT = 0;
    uint32_t sOrder=0, tOrder=0;

    
    if(!shifted) {
        for(;;) {

            
            do {
                
                sOrder = ucol_IGetNextCE(coll, sColl, status);
                
                UCOL_CEBUF_PUT(&sCEs, sOrder, sColl, status);
                
                sOrder &= UCOL_PRIMARYMASK;
            } while(sOrder == 0);

            
            do {
                tOrder = ucol_IGetNextCE(coll, tColl, status);
                UCOL_CEBUF_PUT(&tCEs, tOrder, tColl, status);
                tOrder &= UCOL_PRIMARYMASK;
            } while(tOrder == 0);

            
            if(sOrder == tOrder) {
                
                if(sOrder == UCOL_NO_MORE_CES_PRIMARY) {
                    break;
                }
                if(doHiragana && hirResult == UCOL_EQUAL) {
                    if((sColl->flags & UCOL_WAS_HIRAGANA) != (tColl->flags & UCOL_WAS_HIRAGANA)) {
                        hirResult = ((sColl->flags & UCOL_WAS_HIRAGANA) > (tColl->flags & UCOL_WAS_HIRAGANA))
                            ? UCOL_LESS:UCOL_GREATER;
                    }
                }
            } else {
                
                
                if (coll->leadBytePermutationTable != NULL && !isContinuation(sOrder)) {
                    sOrder = (coll->leadBytePermutationTable[sOrder>>24] << 24) | (sOrder & 0x00FFFFFF);
                    tOrder = (coll->leadBytePermutationTable[tOrder>>24] << 24) | (tOrder & 0x00FFFFFF);
                }
                
                result = (sOrder < tOrder) ?  UCOL_LESS: UCOL_GREATER;
                goto commonReturn;
            }
        } 
    } else { 
        for(;;) {
            UBool sInShifted = FALSE;
            UBool tInShifted = FALSE;
            
            
            for(;;) {
                sOrder = ucol_IGetNextCE(coll, sColl, status);
                if(sOrder == UCOL_NO_MORE_CES) {
                    UCOL_CEBUF_PUT(&sCEs, sOrder, sColl, status);
                    break;
                } else if(sOrder == 0 || (sInShifted && (sOrder & UCOL_PRIMARYMASK) == 0)) {
                    
                    continue;
                } else if(isContinuation(sOrder)) {
                    if((sOrder & UCOL_PRIMARYMASK) > 0) { 
                        if(sInShifted) {
                            sOrder = (sOrder & UCOL_PRIMARYMASK) | 0xC0; 
                            UCOL_CEBUF_PUT(&sCEs, sOrder, sColl, status);
                            continue;
                        } else {
                            UCOL_CEBUF_PUT(&sCEs, sOrder, sColl, status);
                            break;
                        }
                    } else { 
                        if(sInShifted) {
                            continue;
                        } else {
                            UCOL_CEBUF_PUT(&sCEs, sOrder, sColl, status);
                            continue;
                        }
                    }
                } else { 
                    if(coll->leadBytePermutationTable != NULL){
                        sOrder = (coll->leadBytePermutationTable[sOrder>>24] << 24) | (sOrder & 0x00FFFFFF);
                    }
                    if((sOrder & UCOL_PRIMARYMASK) > LVT) {
                        UCOL_CEBUF_PUT(&sCEs, sOrder, sColl, status);
                        break;
                    } else {
                        if((sOrder & UCOL_PRIMARYMASK) > 0) {
                            sInShifted = TRUE;
                            sOrder &= UCOL_PRIMARYMASK;
                            UCOL_CEBUF_PUT(&sCEs, sOrder, sColl, status);
                            continue;
                        } else {
                            UCOL_CEBUF_PUT(&sCEs, sOrder, sColl, status);
                            sInShifted = FALSE;
                            continue;
                        }
                    }
                }
            }
            sOrder &= UCOL_PRIMARYMASK;
            sInShifted = FALSE;

            for(;;) {
                tOrder = ucol_IGetNextCE(coll, tColl, status);
                if(tOrder == UCOL_NO_MORE_CES) {
                    UCOL_CEBUF_PUT(&tCEs, tOrder, tColl, status);
                    break;
                } else if(tOrder == 0 || (tInShifted && (tOrder & UCOL_PRIMARYMASK) == 0)) {
                    
                    continue;
                } else if(isContinuation(tOrder)) {
                    if((tOrder & UCOL_PRIMARYMASK) > 0) { 
                        if(tInShifted) {
                            tOrder = (tOrder & UCOL_PRIMARYMASK) | 0xC0; 
                            UCOL_CEBUF_PUT(&tCEs, tOrder, tColl, status);
                            continue;
                        } else {
                            UCOL_CEBUF_PUT(&tCEs, tOrder, tColl, status);
                            break;
                        }
                    } else { 
                        if(tInShifted) {
                            continue;
                        } else {
                            UCOL_CEBUF_PUT(&tCEs, tOrder, tColl, status);
                            continue;
                        }
                    }
                } else { 
                    if(coll->leadBytePermutationTable != NULL){
                        tOrder = (coll->leadBytePermutationTable[tOrder>>24] << 24) | (tOrder & 0x00FFFFFF);
                    }
                    if((tOrder & UCOL_PRIMARYMASK) > LVT) {
                        UCOL_CEBUF_PUT(&tCEs, tOrder, tColl, status);
                        break;
                    } else {
                        if((tOrder & UCOL_PRIMARYMASK) > 0) {
                            tInShifted = TRUE;
                            tOrder &= UCOL_PRIMARYMASK;
                            UCOL_CEBUF_PUT(&tCEs, tOrder, tColl, status);
                            continue;
                        } else {
                            UCOL_CEBUF_PUT(&tCEs, tOrder, tColl, status);
                            tInShifted = FALSE;
                            continue;
                        }
                    }
                }
            }
            tOrder &= UCOL_PRIMARYMASK;
            tInShifted = FALSE;

            if(sOrder == tOrder) {
                







                if(sOrder == UCOL_NO_MORE_CES_PRIMARY) {
                    break;
                } else {
                    sOrder = 0;
                    tOrder = 0;
                    continue;
                }
            } else {
                result = (sOrder < tOrder) ? UCOL_LESS : UCOL_GREATER;
                goto commonReturn;
            }
        } 
    }

    
    uint32_t    *sCE;
    uint32_t    *tCE;

    
    if(checkSecTer) {
        if(!isFrenchSec) { 
            sCE = sCEs.buf;
            tCE = tCEs.buf;
            for(;;) {
                while (secS == 0) {
                    secS = *(sCE++) & UCOL_SECONDARYMASK;
                }

                while(secT == 0) {
                    secT = *(tCE++) & UCOL_SECONDARYMASK;
                }

                if(secS == secT) {
                    if(secS == UCOL_NO_MORE_CES_SECONDARY) {
                        break;
                    } else {
                        secS = 0; secT = 0;
                        continue;
                    }
                } else {
                    result = (secS < secT) ? UCOL_LESS : UCOL_GREATER;
                    goto commonReturn;
                }
            }
        } else { 
            uint32_t *sCESave = NULL;
            uint32_t *tCESave = NULL;
            sCE = sCEs.pos-2; 
            tCE = tCEs.pos-2;
            for(;;) {
                while (secS == 0 && sCE >= sCEs.buf) {
                    if(sCESave == NULL) {
                        secS = *(sCE--);
                        if(isContinuation(secS)) {
                            while(isContinuation(secS = *(sCE--)))
                                ;
                            
                            sCESave = sCE; 
                            sCE+=2;  
                            
                        }
                    } else {
                        secS = *(sCE++);
                        if(!isContinuation(secS)) { 
                            sCE = sCESave;            
                            sCESave = NULL;
                            secS = 0;  
                            continue;
                        }
                    }
                    secS &= UCOL_SECONDARYMASK; 
                }

                while(secT == 0 && tCE >= tCEs.buf) {
                    if(tCESave == NULL) {
                        secT = *(tCE--);
                        if(isContinuation(secT)) {
                            while(isContinuation(secT = *(tCE--)))
                                ;
                            
                            tCESave = tCE; 
                            tCE+=2;  
                            
                        }
                    } else {
                        secT = *(tCE++);
                        if(!isContinuation(secT)) { 
                            tCE = tCESave;          
                            tCESave = NULL;
                            secT = 0;  
                            continue;
                        }
                    }
                    secT &= UCOL_SECONDARYMASK; 
                }

                if(secS == secT) {
                    if(secS == UCOL_NO_MORE_CES_SECONDARY || (sCE < sCEs.buf && tCE < tCEs.buf)) {
                        break;
                    } else {
                        secS = 0; secT = 0;
                        continue;
                    }
                } else {
                    result = (secS < secT) ? UCOL_LESS : UCOL_GREATER;
                    goto commonReturn;
                }
            }
        }
    }

    
    if(checkCase) {
        sCE = sCEs.buf;
        tCE = tCEs.buf;
        for(;;) {
            while((secS & UCOL_REMOVE_CASE) == 0) {
                if(!isContinuation(*sCE++)) {
                    secS =*(sCE-1);
                    if(((secS & UCOL_PRIMARYMASK) != 0) || strength > UCOL_PRIMARY) {
                        
                        
                        secS &= UCOL_TERT_CASE_MASK;
                        secS ^= caseSwitch;
                    } else {
                        secS = 0;
                    }
                } else {
                    secS = 0;
                }
            }

            while((secT & UCOL_REMOVE_CASE) == 0) {
                if(!isContinuation(*tCE++)) {
                    secT = *(tCE-1);
                    if(((secT & UCOL_PRIMARYMASK) != 0) || strength > UCOL_PRIMARY) {
                        
                        
                        secT &= UCOL_TERT_CASE_MASK;
                        secT ^= caseSwitch;
                    } else {
                        secT = 0;
                    }
                } else {
                    secT = 0;
                }
            }

            if((secS & UCOL_CASE_BIT_MASK) < (secT & UCOL_CASE_BIT_MASK)) {
                result = UCOL_LESS;
                goto commonReturn;
            } else if((secS & UCOL_CASE_BIT_MASK) > (secT & UCOL_CASE_BIT_MASK)) {
                result = UCOL_GREATER;
                goto commonReturn;
            }

            if((secS & UCOL_REMOVE_CASE) == UCOL_NO_MORE_CES_TERTIARY || (secT & UCOL_REMOVE_CASE) == UCOL_NO_MORE_CES_TERTIARY ) {
                break;
            } else {
                secS = 0;
                secT = 0;
            }
        }
    }

    
    if(checkTertiary) {
        secS = 0;
        secT = 0;
        sCE = sCEs.buf;
        tCE = tCEs.buf;
        for(;;) {
            while((secS & UCOL_REMOVE_CASE) == 0) {
                secS = *(sCE++) & tertiaryMask;
                if(!isContinuation(secS)) {
                    secS ^= caseSwitch;
                } else {
                    secS &= UCOL_REMOVE_CASE;
                }
            }

            while((secT & UCOL_REMOVE_CASE)  == 0) {
                secT = *(tCE++) & tertiaryMask;
                if(!isContinuation(secT)) {
                    secT ^= caseSwitch;
                } else {
                    secT &= UCOL_REMOVE_CASE;
                }
            }

            if(secS == secT) {
                if((secS & UCOL_REMOVE_CASE) == 1) {
                    break;
                } else {
                    secS = 0; secT = 0;
                    continue;
                }
            } else {
                result = (secS < secT) ? UCOL_LESS : UCOL_GREATER;
                goto commonReturn;
            }
        }
    }


    if(qShifted ) {
        UBool sInShifted = TRUE;
        UBool tInShifted = TRUE;
        secS = 0;
        secT = 0;
        sCE = sCEs.buf;
        tCE = tCEs.buf;
        for(;;) {
            while((secS == 0 && secS != UCOL_NO_MORE_CES) || (isContinuation(secS) && !sInShifted)) {
                secS = *(sCE++);
                if(isContinuation(secS)) {
                    if(!sInShifted) {
                        continue;
                    }
                } else if(secS > LVT || (secS & UCOL_PRIMARYMASK) == 0) { 
                    secS = UCOL_PRIMARYMASK;
                    sInShifted = FALSE;
                } else {
                    sInShifted = TRUE;
                }
            }
            secS &= UCOL_PRIMARYMASK;


            while((secT == 0 && secT != UCOL_NO_MORE_CES) || (isContinuation(secT) && !tInShifted)) {
                secT = *(tCE++);
                if(isContinuation(secT)) {
                    if(!tInShifted) {
                        continue;
                    }
                } else if(secT > LVT || (secT & UCOL_PRIMARYMASK) == 0) {
                    secT = UCOL_PRIMARYMASK;
                    tInShifted = FALSE;
                } else {
                    tInShifted = TRUE;
                }
            }
            secT &= UCOL_PRIMARYMASK;

            if(secS == secT) {
                if(secS == UCOL_NO_MORE_CES_PRIMARY) {
                    break;
                } else {
                    secS = 0; secT = 0;
                    continue;
                }
            } else {
                result = (secS < secT) ? UCOL_LESS : UCOL_GREATER;
                goto commonReturn;
            }
        }
    } else if(doHiragana && hirResult != UCOL_EQUAL) {
        
        
        result = hirResult;
        goto commonReturn;
    }

    
    
    
    
    if(checkIdent)
    {
        
        result = ucol_checkIdent(sColl, tColl, TRUE, status);
    }

commonReturn:
    if ((sColl->flags | tColl->flags) & UCOL_ITER_ALLOCATED) {
        if (sCEs.buf != sCEs.localArray ) {
            uprv_free(sCEs.buf);
        }
        if (tCEs.buf != tCEs.localArray ) {
            uprv_free(tCEs.buf);
        }
    }

    return result;
}

static UCollationResult
ucol_strcollRegular(const UCollator *coll,
                    const UChar *source, int32_t sourceLength,
                    const UChar *target, int32_t targetLength,
                    UErrorCode *status) {
    collIterate sColl, tColl;
    
    IInit_collIterate(coll, source, sourceLength, &sColl, status);
    IInit_collIterate(coll, target, targetLength, &tColl, status);
    if(U_FAILURE(*status)) {
        return UCOL_LESS;
    }
    return ucol_strcollRegular(&sColl, &tColl, status);
}

static inline uint32_t
ucol_getLatinOneContraction(const UCollator *coll, int32_t strength,
                          uint32_t CE, const UChar *s, int32_t *index, int32_t len)
{
    const UChar *UCharOffset = (UChar *)coll->image+getContractOffset(CE&0xFFF);
    int32_t latinOneOffset = (CE & 0x00FFF000) >> 12;
    int32_t offset = 1;
    UChar schar = 0, tchar = 0;

    for(;;) {
        if(len == -1) {
            if(s[*index] == 0) { 
                return(coll->latinOneCEs[strength*coll->latinOneTableLen+latinOneOffset]);
            } else {
                schar = s[*index];
            }
        } else {
            if(*index == len) {
                return(coll->latinOneCEs[strength*coll->latinOneTableLen+latinOneOffset]);
            } else {
                schar = s[*index];
            }
        }

        while(schar > (tchar = *(UCharOffset+offset))) { 
            offset++;
        }

        if (schar == tchar) {
            (*index)++;
            return(coll->latinOneCEs[strength*coll->latinOneTableLen+latinOneOffset+offset]);
        }
        else
        {
            if(schar & 0xFF00 ) {
                return UCOL_BAIL_OUT_CE;
            }
            
            uint32_t isZeroCE = UTRIE_GET32_FROM_LEAD(&coll->mapping, schar);
            if(isZeroCE == 0) { 
                (*index)++;
                continue;
            }

            return(coll->latinOneCEs[strength*coll->latinOneTableLen+latinOneOffset]);
        }
    }
}











static UCollationResult
ucol_strcollUseLatin1( const UCollator    *coll,
              const UChar        *source,
              int32_t            sLen,
              const UChar        *target,
              int32_t            tLen,
              UErrorCode *status)
{
    U_ALIGN_CODE(16);
    int32_t strength = coll->strength;

    int32_t sIndex = 0, tIndex = 0;
    UChar sChar = 0, tChar = 0;
    uint32_t sOrder=0, tOrder=0;

    UBool endOfSource = FALSE;

    uint32_t *elements = coll->latinOneCEs;

    UBool haveContractions = FALSE; 
                                    

    
    for(;;) {
        while(sOrder==0) { 
            
            if(sLen==-1) {   
                sChar=source[sIndex++];
                if(sChar==0) {
                    endOfSource = TRUE;
                    break;
                }
            } else {        
                if(sIndex==sLen) {
                    endOfSource = TRUE;
                    break;
                }
                sChar=source[sIndex++];
            }
            if(sChar&0xFF00) { 
                
                return ucol_strcollRegular(coll, source, sLen, target, tLen, status);
            }
            sOrder = elements[sChar];
            if(sOrder >= UCOL_NOT_FOUND) { 
                
                
                if(getCETag(sOrder) == CONTRACTION_TAG) {
                    sOrder = ucol_getLatinOneContraction(coll, UCOL_PRIMARY, sOrder, source, &sIndex, sLen);
                    haveContractions = TRUE; 
                    
                    
                }
                if(sOrder >= UCOL_NOT_FOUND ) {
                    
                    return ucol_strcollRegular(coll, source, sLen, target, tLen, status);
                }
            }
        }

        while(tOrder==0) {  
            
            if(tLen==-1) {    
                tChar=target[tIndex++];
                if(tChar==0) {
                    if(endOfSource) { 
                        
                        
                        
                        
                        goto endOfPrimLoop;
                    } else {
                        return UCOL_GREATER;
                    }
                }
            } else {          
                if(tIndex==tLen) {
                    if(endOfSource) {
                        goto endOfPrimLoop;
                    } else {
                        return UCOL_GREATER;
                    }
                }
                tChar=target[tIndex++];
            }
            if(tChar&0xFF00) { 
                
                return ucol_strcollRegular(coll, source, sLen, target, tLen, status);
            }
            tOrder = elements[tChar];
            if(tOrder >= UCOL_NOT_FOUND) {
                
                if(getCETag(tOrder) == CONTRACTION_TAG) {
                    tOrder = ucol_getLatinOneContraction(coll, UCOL_PRIMARY, tOrder, target, &tIndex, tLen);
                    haveContractions = TRUE;
                }
                if(tOrder >= UCOL_NOT_FOUND ) {
                    
                    return ucol_strcollRegular(coll, source, sLen, target, tLen, status);
                }
            }
        }
        if(endOfSource) { 
            return UCOL_LESS;
        }

        if(sOrder == tOrder) { 
            sOrder = 0; tOrder = 0;
            continue;
        } else {
            
            if(((sOrder^tOrder)&0xFF000000)!=0) {
                
                if(sOrder < tOrder) {
                    return UCOL_LESS;
                } else if(sOrder > tOrder) {
                    return UCOL_GREATER;
                }
                
                
            }

            
            sOrder<<=8;
            tOrder<<=8;
        }
    }

endOfPrimLoop:
    
    
    sLen = sIndex; tLen = tIndex;
    if(strength >= UCOL_SECONDARY) {
        
        elements += coll->latinOneTableLen;
        endOfSource = FALSE;

        if(coll->frenchCollation == UCOL_OFF) { 
            
            
            
            
            sIndex = 0; tIndex = 0;
            for(;;) {
                while(sOrder==0) {
                    if(sIndex==sLen) {
                        endOfSource = TRUE;
                        break;
                    }
                    sChar=source[sIndex++];
                    sOrder = elements[sChar];
                    if(sOrder > UCOL_NOT_FOUND) {
                        sOrder = ucol_getLatinOneContraction(coll, UCOL_SECONDARY, sOrder, source, &sIndex, sLen);
                    }
                }

                while(tOrder==0) {
                    if(tIndex==tLen) {
                        if(endOfSource) {
                            goto endOfSecLoop;
                        } else {
                            return UCOL_GREATER;
                        }
                    }
                    tChar=target[tIndex++];
                    tOrder = elements[tChar];
                    if(tOrder > UCOL_NOT_FOUND) {
                        tOrder = ucol_getLatinOneContraction(coll, UCOL_SECONDARY, tOrder, target, &tIndex, tLen);
                    }
                }
                if(endOfSource) {
                    return UCOL_LESS;
                }

                if(sOrder == tOrder) {
                    sOrder = 0; tOrder = 0;
                    continue;
                } else {
                    
                    if(((sOrder^tOrder)&0xFF000000)!=0) {
                        if(sOrder < tOrder) {
                            return UCOL_LESS;
                        } else if(sOrder > tOrder) {
                            return UCOL_GREATER;
                        }
                    }
                    sOrder<<=8;
                    tOrder<<=8;
                }
            }
        } else { 
            if(haveContractions) { 
                
                return ucol_strcollRegular(coll, source, sLen, target, tLen, status);
            }
            
            sIndex = sLen; tIndex = tLen;
            for(;;) {
                while(sOrder==0) {
                    if(sIndex==0) {
                        endOfSource = TRUE;
                        break;
                    }
                    sChar=source[--sIndex];
                    sOrder = elements[sChar];
                    
                }

                while(tOrder==0) {
                    if(tIndex==0) {
                        if(endOfSource) {
                            goto endOfSecLoop;
                        } else {
                            return UCOL_GREATER;
                        }
                    }
                    tChar=target[--tIndex];
                    tOrder = elements[tChar];
                    
                }
                if(endOfSource) {
                    return UCOL_LESS;
                }

                if(sOrder == tOrder) {
                    sOrder = 0; tOrder = 0;
                    continue;
                } else {
                    
                    if(((sOrder^tOrder)&0xFF000000)!=0) {
                        if(sOrder < tOrder) {
                            return UCOL_LESS;
                        } else if(sOrder > tOrder) {
                            return UCOL_GREATER;
                        }
                    }
                    sOrder<<=8;
                    tOrder<<=8;
                }
            }
        }
    }

endOfSecLoop:
    if(strength >= UCOL_TERTIARY) {
        
        elements += coll->latinOneTableLen;
        sIndex = 0; tIndex = 0;
        endOfSource = FALSE;
        for(;;) {
            while(sOrder==0) {
                if(sIndex==sLen) {
                    endOfSource = TRUE;
                    break;
                }
                sChar=source[sIndex++];
                sOrder = elements[sChar];
                if(sOrder > UCOL_NOT_FOUND) {
                    sOrder = ucol_getLatinOneContraction(coll, UCOL_TERTIARY, sOrder, source, &sIndex, sLen);
                }
            }
            while(tOrder==0) {
                if(tIndex==tLen) {
                    if(endOfSource) {
                        return UCOL_EQUAL; 
                    } else {
                        return UCOL_GREATER;
                    }
                }
                tChar=target[tIndex++];
                tOrder = elements[tChar];
                if(tOrder > UCOL_NOT_FOUND) {
                    tOrder = ucol_getLatinOneContraction(coll, UCOL_TERTIARY, tOrder, target, &tIndex, tLen);
                }
            }
            if(endOfSource) {
                return UCOL_LESS;
            }
            if(sOrder == tOrder) {
                sOrder = 0; tOrder = 0;
                continue;
            } else {
                if(((sOrder^tOrder)&0xff000000)!=0) {
                    if(sOrder < tOrder) {
                        return UCOL_LESS;
                    } else if(sOrder > tOrder) {
                        return UCOL_GREATER;
                    }
                }
                sOrder<<=8;
                tOrder<<=8;
            }
        }
    }
    return UCOL_EQUAL;
}









static const UChar32
utf8_minLegal[4]={ 0, 0x80, 0x800, 0x10000 };

#define UTF8_ERROR_VALUE_1 0x15
#define UTF8_ERROR_VALUE_2 0x9f
#define UTF_ERROR_VALUE 0xffff

static const UChar32
utf8_errorValue[6]={
    UTF8_ERROR_VALUE_1, UTF8_ERROR_VALUE_2, UTF_ERROR_VALUE, 0x10ffff,
    0x3ffffff, 0x7fffffff
};

static
UChar32 utf8_nextCharSafeBodyNullTerm(const uint8_t *s, int32_t *pi, UChar32 c, UBool strict) {
    int32_t i=*pi;
    uint8_t count=U8_COUNT_TRAIL_BYTES(c);
    U_ASSERT(count <= 5); 

    if (c) {
        uint8_t trail, illegal=0;

        U8_MASK_LEAD_BYTE((c), count);
        
        switch(count) {
        
        case 5:
        case 4:
            
            illegal=1;
            break;
        case 3:
            trail=s[(i)];
            if (trail==0) {
                illegal=1;
                break;
            }
            (c)=((c)<<6)|(trail&0x3f);
            if(c<0x110) {
                illegal|=(trail&0xc0)^0x80;
            } else {
                
                illegal=1;
                break;
            }
            ++(i);
        case 2:
            trail=s[(i)];
            if (trail==0) {
                illegal=1;
                break;
            }
            (c)=((c)<<6)|(trail&0x3f);
            illegal|=(trail&0xc0)^0x80;
            ++(i);
        case 1:
            trail=s[(i)];
            if (trail==0) {
                illegal=1;
                break;
            }
            (c)=((c)<<6)|(trail&0x3f);
            illegal|=(trail&0xc0)^0x80;
            ++(i);
            break;
        case 0:
            if(strict>=0) {
                return UTF8_ERROR_VALUE_1;
            } else {
                return U_SENTINEL;
            }
        
        }

        










        
        
        if(illegal || (c)<utf8_minLegal[count] || (U_IS_SURROGATE(c) && strict!=-2)) {
            
            uint8_t errorCount=count;
            
            i=*pi;
            while(count>0 && U8_IS_TRAIL(s[i])) {
                ++(i);
                --count;
            }
            if(strict>=0) {
                c=utf8_errorValue[errorCount-count];
            } else {
                c=U_SENTINEL;
            }
        } else if((strict)>0 && U_IS_UNICODE_NONCHAR(c)) {
            
            c=utf8_errorValue[count];
        }
    }
    *pi=i;
    return c;
}

#define U8_NEXT_NULLTERM(s, i, c) { \
    (c)=(uint8_t)(s)[(i)]; \
    if((c)>=0x80) { \
        uint8_t __t1, __t2; \
        if( /* handle U+1000..U+CFFF inline */ \
            (0xe0<(c) && (c)<=0xec) && \
            (__t1=(uint8_t)((s)[(i)+1]-0x80))<=0x3f && __t1 != 0 && \
            (__t2=(uint8_t)((s)[(i)+2]-0x80))<= 0x3f && __t2 != 0 \
        ) { \
            /* no need for (c&0xf) because the upper bits are truncated after <<12 in the cast to (UChar) */ \
            (c)=(UChar)(((c)<<12)|(__t1<<6)|__t2); \
            (i)+=3; \
        } else if( /* handle U+0080..U+07FF inline */ \
            ((c)<0xe0 && (c)>=0xc2) && \
            (__t1=(uint8_t)((s)[(i)+1]-0x80))<=0x3f && __t1 != 0 \
        ) { \
            (c)=(UChar)((((c)&0x1f)<<6)|__t1); \
            (i)+=2; \
        } else if(U8_IS_LEAD(c)) { \
            /* function call for "complicated" and error cases */ \
            ++(i); \
            (c)=utf8_nextCharSafeBodyNullTerm((const uint8_t *)s, &(i), c, -1); \
        } else { \
            (c)=U_SENTINEL; \
            ++(i); \
        } \
    } else { \
        if ((c)) { \
            ++(i); \
        } \
    } \
}

#define U8_GET_NULLTERM(s, start, i, c) { \
    int32_t _u8_get_index=(int32_t)(i); \
    U8_SET_CP_START(s, start, _u8_get_index); \
    U8_NEXT_NULLTERM(s, _u8_get_index, c); \
}


static UCollationResult
ucol_strcollRegularUTF8(
                    const UCollator *coll,
                    const char      *source,
                    int32_t         sourceLength,
                    const char      *target,
                    int32_t         targetLength,
                    UErrorCode      *status)
{
    UCharIterator src;
    UCharIterator tgt;

    uiter_setUTF8(&src, source, sourceLength);
    uiter_setUTF8(&tgt, target, targetLength);

    
    collIterate sColl, tColl;
    IInit_collIterate(coll, NULL, -1, &sColl, status);
    IInit_collIterate(coll, NULL, -1, &tColl, status);
    if(U_FAILURE(*status)) {
        UTRACE_EXIT_VALUE_STATUS(UCOL_EQUAL, *status)
        return UCOL_EQUAL;
    }
    
    
    
    UAlignedMemory stackNormIter1[UNORM_ITER_SIZE/sizeof(UAlignedMemory)];
    UAlignedMemory stackNormIter2[UNORM_ITER_SIZE/sizeof(UAlignedMemory)];
    UNormIterator *sNormIter = NULL, *tNormIter = NULL;

    sColl.iterator = &src;
    sColl.flags |= UCOL_USE_ITERATOR;
    tColl.flags |= UCOL_USE_ITERATOR;
    tColl.iterator = &tgt;

    if(ucol_getAttribute(coll, UCOL_NORMALIZATION_MODE, status) == UCOL_ON) {
        sNormIter = unorm_openIter(stackNormIter1, sizeof(stackNormIter1), status);
        sColl.iterator = unorm_setIter(sNormIter, &src, UNORM_FCD, status);
        sColl.flags &= ~UCOL_ITER_NORM;

        tNormIter = unorm_openIter(stackNormIter2, sizeof(stackNormIter2), status);
        tColl.iterator = unorm_setIter(tNormIter, &tgt, UNORM_FCD, status);
        tColl.flags &= ~UCOL_ITER_NORM;
    }

    return ucol_strcollRegular(&sColl, &tColl, status);
}

static inline uint32_t
ucol_getLatinOneContractionUTF8(const UCollator *coll, int32_t strength,
                          uint32_t CE, const char *s, int32_t *index, int32_t len)
{
    const UChar *UCharOffset = (UChar *)coll->image+getContractOffset(CE&0xFFF);
    int32_t latinOneOffset = (CE & 0x00FFF000) >> 12;
    int32_t offset = 1;
    UChar32 schar = 0, tchar = 0;

    for(;;) {
        if (len == -1) {
            U8_GET_NULLTERM((const uint8_t*)s, 0, *index, schar);
            if (schar == 0) {
                return(coll->latinOneCEs[strength*coll->latinOneTableLen+latinOneOffset]);
            }
        } else {
            if (*index == len) {
                return(coll->latinOneCEs[strength*coll->latinOneTableLen+latinOneOffset]);
            }
            U8_GET((const uint8_t*)s, 0, *index, len, schar);
        }
        if (schar == -1) {
            schar = 0xfffd;
        }

        while(schar > (tchar = *(UCharOffset+offset))) { 
            offset++;
        }

        if (schar == tchar) {
            U8_FWD_1(s, *index, len);
            return(coll->latinOneCEs[strength*coll->latinOneTableLen+latinOneOffset+offset]);
        }
        else
        {
            if(schar & 0xFF00 ) {
                return UCOL_BAIL_OUT_CE;
            }
            
            uint32_t isZeroCE = UTRIE_GET32_FROM_LEAD(&coll->mapping, schar);
            if(isZeroCE == 0) { 
                U8_FWD_1(s, *index, len);
                continue;
            }

            return(coll->latinOneCEs[strength*coll->latinOneTableLen+latinOneOffset]);
        }
    }
}

static inline UCollationResult
ucol_strcollUseLatin1UTF8(
                const UCollator *coll,
                const char      *source,
                int32_t         sLen,
                const char      *target,
                int32_t         tLen,
                UErrorCode      *status)
{
    U_ALIGN_CODE(16);
    int32_t strength = coll->strength;

    int32_t sIndex = 0, tIndex = 0;
    UChar32 sChar = 0, tChar = 0;
    uint32_t sOrder=0, tOrder=0;

    UBool endOfSource = FALSE;

    uint32_t *elements = coll->latinOneCEs;

    UBool haveContractions = FALSE; 
                                    

    
    for(;;) {
        while(sOrder==0) { 
            
            if (sLen==-1) {
                U8_NEXT_NULLTERM(source, sIndex, sChar);
                if (sChar == 0) {
                    endOfSource = TRUE;
                    sLen = sIndex;
                    break;
                }
            } else {
                if (sIndex == sLen) {
                    endOfSource = TRUE;
                    break;
                }
                U8_NEXT(source, sIndex, sLen ,sChar);
            }
            if (sChar == -1) {
                sChar = 0xfffd; 
            }
            if(sChar&0xFFFFFF00) { 
                
                return ucol_strcollRegularUTF8(coll, source, sLen, target, tLen, status);
            }
            sOrder = elements[sChar];
            if(sOrder >= UCOL_NOT_FOUND) { 
                
                
                if(getCETag(sOrder) == CONTRACTION_TAG) {
                    sOrder = ucol_getLatinOneContractionUTF8(coll, UCOL_PRIMARY, sOrder, source, &sIndex, sLen);
                    haveContractions = TRUE; 
                    
                    
                }
                if(sOrder >= UCOL_NOT_FOUND ) {
                    
                    return ucol_strcollRegularUTF8(coll, source, sLen, target, tLen, status);
                }
            }
        }

        while(tOrder==0) {  
            
            if (tLen == -1) {
                U8_NEXT_NULLTERM(target, tIndex, tChar);
                if (tChar == 0) {
                    if(endOfSource) {
                        tLen = tIndex;
                        goto endOfPrimLoopU8;
                    } else {
                        return UCOL_GREATER;
                    }
                }
            } else {
                if (tIndex == tLen) {
                    if(endOfSource) {
                        goto endOfPrimLoopU8;
                    } else {
                        return UCOL_GREATER;
                    }
                }
                U8_NEXT(target, tIndex, tLen, tChar);
            }
            if (tChar == -1) {
                tChar = 0xfffd;
            }
            if(tChar&0xFFFFFF00) { 
                
                return ucol_strcollRegularUTF8(coll, source, sLen, target, tLen, status);
            }
            tOrder = elements[tChar];
            if(tOrder >= UCOL_NOT_FOUND) {
                
                if(getCETag(tOrder) == CONTRACTION_TAG) {
                    tOrder = ucol_getLatinOneContractionUTF8(coll, UCOL_PRIMARY, tOrder, target, &tIndex, tLen);
                    haveContractions = TRUE;
                }
                if(tOrder >= UCOL_NOT_FOUND ) {
                    
                    return ucol_strcollRegularUTF8(coll, source, sLen, target, tLen, status);
                }
            }
        }
        if(endOfSource) { 
            return UCOL_LESS;
        }

        if(sOrder == tOrder) { 
            sOrder = 0; tOrder = 0;
            continue;
        } else {
            
            if(((sOrder^tOrder)&0xFF000000)!=0) {
                
                if(sOrder < tOrder) {
                    return UCOL_LESS;
                } else if(sOrder > tOrder) {
                    return UCOL_GREATER;
                }
                
                
            }

            
            sOrder<<=8;
            tOrder<<=8;
        }
    }

endOfPrimLoopU8:
    
    
    sLen = sIndex; tLen = tIndex;
    if(strength >= UCOL_SECONDARY) {
        
        elements += coll->latinOneTableLen;
        endOfSource = FALSE;

        if(coll->frenchCollation == UCOL_OFF) { 
            
            
            
            
            sIndex = 0; tIndex = 0;
            for(;;) {
                while(sOrder==0) {
                    if(sIndex==sLen) {
                        endOfSource = TRUE;
                        break;
                    }
                    U_ASSERT(sLen >= 0);
                    U8_NEXT(source, sIndex, sLen, sChar);
                    U_ASSERT(sChar >= 0 && sChar <= 0xFF);
                    sOrder = elements[sChar];
                    if(sOrder > UCOL_NOT_FOUND) {
                        sOrder = ucol_getLatinOneContractionUTF8(coll, UCOL_SECONDARY, sOrder, source, &sIndex, sLen);
                    }
                }

                while(tOrder==0) {
                    if(tIndex==tLen) {
                        if(endOfSource) {
                            goto endOfSecLoopU8;
                        } else {
                            return UCOL_GREATER;
                        }
                    }
                    U_ASSERT(tLen >= 0);
                    U8_NEXT(target, tIndex, tLen, tChar);
                    U_ASSERT(tChar >= 0 && tChar <= 0xFF);
                    tOrder = elements[tChar];
                    if(tOrder > UCOL_NOT_FOUND) {
                        tOrder = ucol_getLatinOneContractionUTF8(coll, UCOL_SECONDARY, tOrder, target, &tIndex, tLen);
                    }
                }
                if(endOfSource) {
                    return UCOL_LESS;
                }

                if(sOrder == tOrder) {
                    sOrder = 0; tOrder = 0;
                    continue;
                } else {
                    
                    if(((sOrder^tOrder)&0xFF000000)!=0) {
                        if(sOrder < tOrder) {
                            return UCOL_LESS;
                        } else if(sOrder > tOrder) {
                            return UCOL_GREATER;
                        }
                    }
                    sOrder<<=8;
                    tOrder<<=8;
                }
            }
        } else { 
            if(haveContractions) { 
                
                return ucol_strcollRegularUTF8(coll, source, sLen, target, tLen, status);
            }
            
            sIndex = sLen; tIndex = tLen;
            for(;;) {
                while(sOrder==0) {
                    if(sIndex==0) {
                        endOfSource = TRUE;
                        break;
                    }
                    U8_PREV(source, 0, sIndex, sChar);
                    U_ASSERT(sChar >= 0 && sChar <= 0xFF);
                    sOrder = elements[sChar];
                    
                }

                while(tOrder==0) {
                    if(tIndex==0) {
                        if(endOfSource) {
                            goto endOfSecLoopU8;
                        } else {
                            return UCOL_GREATER;
                        }
                    }
                    U8_PREV(target, 0, tIndex, tChar);
                    U_ASSERT(tChar >= 0 && tChar <= 0xFF);
                    tOrder = elements[tChar];
                    
                }
                if(endOfSource) {
                    return UCOL_LESS;
                }

                if(sOrder == tOrder) {
                    sOrder = 0; tOrder = 0;
                    continue;
                } else {
                    
                    if(((sOrder^tOrder)&0xFF000000)!=0) {
                        if(sOrder < tOrder) {
                            return UCOL_LESS;
                        } else if(sOrder > tOrder) {
                            return UCOL_GREATER;
                        }
                    }
                    sOrder<<=8;
                    tOrder<<=8;
                }
            }
        }
    }

endOfSecLoopU8:
    if(strength >= UCOL_TERTIARY) {
        
        elements += coll->latinOneTableLen;
        sIndex = 0; tIndex = 0;
        endOfSource = FALSE;
        for(;;) {
            while(sOrder==0) {
                if(sIndex==sLen) {
                    endOfSource = TRUE;
                    break;
                }
                U_ASSERT(sLen >= 0);
                U8_NEXT(source, sIndex, sLen, sChar);
                U_ASSERT(sChar >= 0 && sChar <= 0xFF);
                sOrder = elements[sChar];
                if(sOrder > UCOL_NOT_FOUND) {
                    sOrder = ucol_getLatinOneContractionUTF8(coll, UCOL_TERTIARY, sOrder, source, &sIndex, sLen);
                }
            }
            while(tOrder==0) {
                if(tIndex==tLen) {
                    if(endOfSource) {
                        return UCOL_EQUAL; 
                    } else {
                        return UCOL_GREATER;
                    }
                }
                U_ASSERT(tLen >= 0);
                U8_NEXT(target, tIndex, tLen, tChar);
                U_ASSERT(tChar >= 0 && tChar <= 0xFF);
                tOrder = elements[tChar];
                if(tOrder > UCOL_NOT_FOUND) {
                    tOrder = ucol_getLatinOneContractionUTF8(coll, UCOL_TERTIARY, tOrder, target, &tIndex, tLen);
                }
            }
            if(endOfSource) {
                return UCOL_LESS;
            }
            if(sOrder == tOrder) {
                sOrder = 0; tOrder = 0;
                continue;
            } else {
                if(((sOrder^tOrder)&0xff000000)!=0) {
                    if(sOrder < tOrder) {
                        return UCOL_LESS;
                    } else if(sOrder > tOrder) {
                        return UCOL_GREATER;
                    }
                }
                sOrder<<=8;
                tOrder<<=8;
            }
        }
    }
    return UCOL_EQUAL;
}

U_CAPI UCollationResult U_EXPORT2
ucol_strcollIter( const UCollator    *coll,
                 UCharIterator *sIter,
                 UCharIterator *tIter,
                 UErrorCode         *status)
{
    if(!status || U_FAILURE(*status)) {
        return UCOL_EQUAL;
    }

    UTRACE_ENTRY(UTRACE_UCOL_STRCOLLITER);
    UTRACE_DATA3(UTRACE_VERBOSE, "coll=%p, sIter=%p, tIter=%p", coll, sIter, tIter);

    if (sIter == tIter) {
        UTRACE_EXIT_VALUE_STATUS(UCOL_EQUAL, *status)
        return UCOL_EQUAL;
    }
    if(sIter == NULL || tIter == NULL || coll == NULL) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        UTRACE_EXIT_VALUE_STATUS(UCOL_EQUAL, *status)
        return UCOL_EQUAL;
    }

    UCollationResult result = UCOL_EQUAL;

    
    collIterate sColl, tColl;
    IInit_collIterate(coll, NULL, -1, &sColl, status);
    IInit_collIterate(coll, NULL, -1, &tColl, status);
    if(U_FAILURE(*status)) {
        UTRACE_EXIT_VALUE_STATUS(UCOL_EQUAL, *status)
        return UCOL_EQUAL;
    }
    
    
    
    UAlignedMemory stackNormIter1[UNORM_ITER_SIZE/sizeof(UAlignedMemory)];
    UAlignedMemory stackNormIter2[UNORM_ITER_SIZE/sizeof(UAlignedMemory)];
    UNormIterator *sNormIter = NULL, *tNormIter = NULL;

    sColl.iterator = sIter;
    sColl.flags |= UCOL_USE_ITERATOR;
    tColl.flags |= UCOL_USE_ITERATOR;
    tColl.iterator = tIter;

    if(ucol_getAttribute(coll, UCOL_NORMALIZATION_MODE, status) == UCOL_ON) {
        sNormIter = unorm_openIter(stackNormIter1, sizeof(stackNormIter1), status);
        sColl.iterator = unorm_setIter(sNormIter, sIter, UNORM_FCD, status);
        sColl.flags &= ~UCOL_ITER_NORM;

        tNormIter = unorm_openIter(stackNormIter2, sizeof(stackNormIter2), status);
        tColl.iterator = unorm_setIter(tNormIter, tIter, UNORM_FCD, status);
        tColl.flags &= ~UCOL_ITER_NORM;
    }

    UChar32 sChar = U_SENTINEL, tChar = U_SENTINEL;

    while((sChar = sColl.iterator->next(sColl.iterator)) ==
        (tChar = tColl.iterator->next(tColl.iterator))) {
            if(sChar == U_SENTINEL) {
                result = UCOL_EQUAL;
                goto end_compare;
            }
    }

    if(sChar == U_SENTINEL) {
        tChar = tColl.iterator->previous(tColl.iterator);
    }

    if(tChar == U_SENTINEL) {
        sChar = sColl.iterator->previous(sColl.iterator);
    }

    sChar = sColl.iterator->previous(sColl.iterator);
    tChar = tColl.iterator->previous(tColl.iterator);

    if (ucol_unsafeCP((UChar)sChar, coll) || ucol_unsafeCP((UChar)tChar, coll))
    {
        
        
        
        do
        {
            sChar = sColl.iterator->previous(sColl.iterator);
            tChar = tColl.iterator->previous(tColl.iterator);
        }
        while (sChar != U_SENTINEL && ucol_unsafeCP((UChar)sChar, coll));
    }


    if(U_SUCCESS(*status)) {
        result = ucol_strcollRegular(&sColl, &tColl, status);
    }

end_compare:
    if(sNormIter || tNormIter) {
        unorm_closeIter(sNormIter);
        unorm_closeIter(tNormIter);
    }

    UTRACE_EXIT_VALUE_STATUS(result, *status)
    return result;
}





U_CAPI UCollationResult U_EXPORT2
ucol_strcoll( const UCollator    *coll,
              const UChar        *source,
              int32_t            sourceLength,
              const UChar        *target,
              int32_t            targetLength)
{
    U_ALIGN_CODE(16);

    UTRACE_ENTRY(UTRACE_UCOL_STRCOLL);
    if (UTRACE_LEVEL(UTRACE_VERBOSE)) {
        UTRACE_DATA3(UTRACE_VERBOSE, "coll=%p, source=%p, target=%p", coll, source, target);
        UTRACE_DATA2(UTRACE_VERBOSE, "source string = %vh ", source, sourceLength);
        UTRACE_DATA2(UTRACE_VERBOSE, "target string = %vh ", target, targetLength);
    }

    if(source == NULL || target == NULL) {
        
        
        UTRACE_EXIT_VALUE(UCOL_EQUAL);
        return UCOL_EQUAL;
    }

    
    
    if (source==target && sourceLength==targetLength) {
        UTRACE_EXIT_VALUE(UCOL_EQUAL);
        return UCOL_EQUAL;
    }

    if(coll->delegate != NULL) {
      UErrorCode status = U_ZERO_ERROR;
      return ((const Collator*)coll->delegate)->compare(source,sourceLength,target,targetLength, status);
    }

    
    
    
    const UChar    *pSrc    = source;
    const UChar    *pTarg   = target;
    int32_t        equalLength;

    if (sourceLength == -1 && targetLength == -1) {
        
        
        while (*pSrc == *pTarg && *pSrc != 0) {
            pSrc++;
            pTarg++;
        }
        if (*pSrc == 0 && *pTarg == 0) {
            UTRACE_EXIT_VALUE(UCOL_EQUAL);
            return UCOL_EQUAL;
        }
        equalLength = (int32_t)(pSrc - source);
    }
    else
    {
        
        const UChar    *pSrcEnd = source + sourceLength;
        const UChar    *pTargEnd = target + targetLength;

        
        for (;;) {
            if (pSrc == pSrcEnd || pTarg == pTargEnd) {
                break;
            }
            if ((*pSrc == 0 && sourceLength == -1) || (*pTarg == 0 && targetLength == -1)) {
                break;
            }
            if (*pSrc != *pTarg) {
                break;
            }
            pSrc++;
            pTarg++;
        }
        equalLength = (int32_t)(pSrc - source);

        
        if ((pSrc ==pSrcEnd  || (pSrcEnd <pSrc  && *pSrc==0))  &&   
            (pTarg==pTargEnd || (pTargEnd<pTarg && *pTarg==0)))     
        {
            UTRACE_EXIT_VALUE(UCOL_EQUAL);
            return UCOL_EQUAL;
        }
    }
    if (equalLength > 0) {
        
        
        
        
        
        
        
        if ((pSrc  != source+sourceLength && ucol_unsafeCP(*pSrc, coll)) ||
            (pTarg != target+targetLength && ucol_unsafeCP(*pTarg, coll)))
        {
            
            
            
            do
            {
                equalLength--;
                pSrc--;
            }
            while (equalLength>0 && ucol_unsafeCP(*pSrc, coll));
        }

        source += equalLength;
        target += equalLength;
        if (sourceLength > 0) {
            sourceLength -= equalLength;
        }
        if (targetLength > 0) {
            targetLength -= equalLength;
        }
    }

    UErrorCode status = U_ZERO_ERROR;
    UCollationResult returnVal;
    if(!coll->latinOneUse || (sourceLength > 0 && *source&0xff00) || (targetLength > 0 && *target&0xff00)) {
        returnVal = ucol_strcollRegular(coll, source, sourceLength, target, targetLength, &status);
    } else {
        returnVal = ucol_strcollUseLatin1(coll, source, sourceLength, target, targetLength, &status);
    }
    UTRACE_EXIT_VALUE(returnVal);
    return returnVal;
}

U_CAPI UCollationResult U_EXPORT2
ucol_strcollUTF8(
        const UCollator *coll,
        const char      *source,
        int32_t         sourceLength,
        const char      *target,
        int32_t         targetLength,
        UErrorCode      *status)
{
    U_ALIGN_CODE(16);

    UTRACE_ENTRY(UTRACE_UCOL_STRCOLLUTF8);
    if (UTRACE_LEVEL(UTRACE_VERBOSE)) {
        UTRACE_DATA3(UTRACE_VERBOSE, "coll=%p, source=%p, target=%p", coll, source, target);
        UTRACE_DATA2(UTRACE_VERBOSE, "source string = %vb ", source, sourceLength);
        UTRACE_DATA2(UTRACE_VERBOSE, "target string = %vb ", target, targetLength);
    }

    if (U_FAILURE(*status)) {
        
        UTRACE_EXIT_VALUE_STATUS(UCOL_EQUAL, *status);
        return UCOL_EQUAL;
    }

    if(source == NULL || target == NULL) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        UTRACE_EXIT_VALUE_STATUS(UCOL_EQUAL, *status);
        return UCOL_EQUAL;
    }

    
    
    if (source==target && sourceLength==targetLength) {
        UTRACE_EXIT_VALUE_STATUS(UCOL_EQUAL, *status);
        return UCOL_EQUAL;
    }

    if(coll->delegate != NULL) {
        return ((const Collator*)coll->delegate)->compareUTF8(
            StringPiece(source, (sourceLength < 0) ? uprv_strlen(source) : sourceLength),
            StringPiece(target, (targetLength < 0) ? uprv_strlen(target) : targetLength),
            *status);
    }

    
    
    
    const char  *pSrc = source;
    const char  *pTarg = target;
    UBool       bSrcLimit = FALSE;
    UBool       bTargLimit = FALSE;

    if (sourceLength == -1 && targetLength == -1) {
        
        
        while (*pSrc == *pTarg && *pSrc != 0) {
            pSrc++;
            pTarg++;
        }
        if (*pSrc == 0 && *pTarg == 0) {
            UTRACE_EXIT_VALUE_STATUS(UCOL_EQUAL, *status);
            return UCOL_EQUAL;
        }
        bSrcLimit = (*pSrc == 0);
        bTargLimit = (*pTarg == 0);
    }
    else
    {
        
        const char *pSrcEnd = source + sourceLength;
        const char *pTargEnd = target + targetLength;

        
        for (;;) {
            if (pSrc == pSrcEnd || pTarg == pTargEnd) {
                break;
            }
            if ((*pSrc == 0 && sourceLength == -1) || (*pTarg == 0 && targetLength == -1)) {
                break;
            }
            if (*pSrc != *pTarg) {
                break;
            }
            pSrc++;
            pTarg++;
        }
        bSrcLimit = (pSrc ==pSrcEnd  || (pSrcEnd <pSrc  && *pSrc==0));
        bTargLimit = (pTarg==pTargEnd || (pTargEnd<pTarg && *pTarg==0));

        
        if (bSrcLimit &&    
            bTargLimit)     
        {
            UTRACE_EXIT_VALUE_STATUS(UCOL_EQUAL, *status);
            return UCOL_EQUAL;
        }
    }

    U_ASSERT(!(bSrcLimit && bTargLimit));

    int32_t    equalLength = pSrc - source;
    UBool       bSawNonLatin1 = FALSE;

    if (equalLength > 0) {
        
        if (bTargLimit) {
            U8_SET_CP_START((const uint8_t*)source, 0, equalLength);
        } else {
            U8_SET_CP_START((const uint8_t*)target, 0, equalLength);
        }
        pSrc = source + equalLength;
        pTarg = target + equalLength;
    }

    if (equalLength > 0) {
        
        
        
        UBool bUnsafeCP = FALSE;
        UChar32 uc32 = -1;

        if (!bSrcLimit) {
            if (sourceLength >= 0) {
                U8_GET((uint8_t*)source, 0, equalLength, sourceLength, uc32);
            } else {
                U8_GET_NULLTERM((uint8_t*)source, 0, equalLength, uc32);
            }
            if (uc32 == -1) {
                uc32 = 0xfffd;
                bSawNonLatin1 |= TRUE;
            } else {
                if (uc32 >= 0x10000 || ucol_unsafeCP((UChar)uc32, coll)) {
                    bUnsafeCP = TRUE;
                }
                bSawNonLatin1 |= (uc32 > 0xff);
            }
        }
        if (!bTargLimit) {
            if (targetLength >= 0) {
                U8_GET((uint8_t*)target, 0, equalLength, targetLength, uc32);
            } else {
                U8_GET_NULLTERM((uint8_t*)target, 0, equalLength, uc32);
            }
            if (uc32 == -1) {
                uc32 = 0xfffd;
                bSawNonLatin1 |= TRUE;
            } else {
                if (uc32 >= 0x10000 || ucol_unsafeCP((UChar)uc32, coll)) {
                    bUnsafeCP = TRUE;
                }
                bSawNonLatin1 |= (uc32 > 0xff);
            }
        }

        if (bUnsafeCP) {
            while (equalLength > 0) {
                
                
                
                U8_PREV((uint8_t*)source, 0, equalLength, uc32);
                bSawNonLatin1 |= (uc32 > 0xff);
                if (uc32 < 0x10000 && !ucol_unsafeCP((UChar)uc32, coll)) {
                    break;
                }
            }
        }
        source += equalLength;
        target += equalLength;
        if (sourceLength > 0) {
            sourceLength -= equalLength;
        }
        if (targetLength > 0) {
            targetLength -= equalLength;
        }
    } else {
        
        bSawNonLatin1 = (source && (sourceLength != 0) && (uint8_t)*source > 0xc3);
        bSawNonLatin1 |= (target && (targetLength != 0) && (uint8_t)*target > 0xc3);
    }

    UCollationResult returnVal;

    if(!coll->latinOneUse || bSawNonLatin1) {
        returnVal = ucol_strcollRegularUTF8(coll, source, sourceLength, target, targetLength, status);
    } else {
        returnVal = ucol_strcollUseLatin1UTF8(coll, source, sourceLength, target, targetLength, status);
    }
    UTRACE_EXIT_VALUE_STATUS(returnVal, *status);
    return returnVal;
}



U_CAPI UBool U_EXPORT2
ucol_greater(    const    UCollator        *coll,
        const    UChar            *source,
        int32_t            sourceLength,
        const    UChar            *target,
        int32_t            targetLength)
{
    return (ucol_strcoll(coll, source, sourceLength, target, targetLength)
        == UCOL_GREATER);
}


U_CAPI UBool U_EXPORT2
ucol_greaterOrEqual(    const    UCollator    *coll,
            const    UChar        *source,
            int32_t        sourceLength,
            const    UChar        *target,
            int32_t        targetLength)
{
    return (ucol_strcoll(coll, source, sourceLength, target, targetLength)
        != UCOL_LESS);
}


U_CAPI UBool U_EXPORT2
ucol_equal(        const    UCollator        *coll,
            const    UChar            *source,
            int32_t            sourceLength,
            const    UChar            *target,
            int32_t            targetLength)
{
    return (ucol_strcoll(coll, source, sourceLength, target, targetLength)
        == UCOL_EQUAL);
}

U_CAPI void U_EXPORT2
ucol_getUCAVersion(const UCollator* coll, UVersionInfo info) {
    if(coll && coll->UCA) {
        uprv_memcpy(info, coll->UCA->image->UCAVersion, sizeof(UVersionInfo));
    }
}

#endif 
