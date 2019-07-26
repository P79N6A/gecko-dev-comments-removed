

























#include "unicode/ucnvsel.h"

#if !UCONFIG_NO_CONVERSION

#include <string.h>

#include "unicode/uchar.h"
#include "unicode/uniset.h"
#include "unicode/ucnv.h"
#include "unicode/ustring.h"
#include "unicode/uchriter.h"
#include "utrie2.h"
#include "propsvec.h"
#include "uassert.h"
#include "ucmndata.h"
#include "uenumimp.h"
#include "cmemory.h"
#include "cstring.h"

U_NAMESPACE_USE

struct UConverterSelector {
  UTrie2 *trie;              
  uint32_t* pv;              
  int32_t pvCount;
  char** encodings;          
  int32_t encodingsCount;
  int32_t encodingStrLength;
  uint8_t* swapped;
  UBool ownPv, ownEncodingStrings;
};

static void generateSelectorData(UConverterSelector* result,
                                 UPropsVectors *upvec,
                                 const USet* excludedCodePoints,
                                 const UConverterUnicodeSet whichSet,
                                 UErrorCode* status) {
  if (U_FAILURE(*status)) {
    return;
  }

  int32_t columns = (result->encodingsCount+31)/32;

  
  for (int32_t col = 0; col < columns; col++) {
    upvec_setValue(upvec, UPVEC_ERROR_VALUE_CP, UPVEC_ERROR_VALUE_CP,
                   col, ~0, ~0, status);
  }

  for (int32_t i = 0; i < result->encodingsCount; ++i) {
    uint32_t mask;
    uint32_t column;
    int32_t item_count;
    int32_t j;
    UConverter* test_converter = ucnv_open(result->encodings[i], status);
    if (U_FAILURE(*status)) {
      return;
    }
    USet* unicode_point_set;
    unicode_point_set = uset_open(1, 0);  

    ucnv_getUnicodeSet(test_converter, unicode_point_set,
                       whichSet, status);
    if (U_FAILURE(*status)) {
      ucnv_close(test_converter);
      return;
    }

    column = i / 32;
    mask = 1 << (i%32);
    
    item_count = uset_getItemCount(unicode_point_set);

    for (j = 0; j < item_count; ++j) {
      UChar32 start_char;
      UChar32 end_char;
      UErrorCode smallStatus = U_ZERO_ERROR;
      uset_getItem(unicode_point_set, j, &start_char, &end_char, NULL, 0,
                   &smallStatus);
      if (U_FAILURE(smallStatus)) {
        
        
      } else {
        upvec_setValue(upvec, start_char, end_char, column, ~0, mask,
                       status);
      }
    }
    ucnv_close(test_converter);
    uset_close(unicode_point_set);
    if (U_FAILURE(*status)) {
      return;
    }
  }

  
  if (excludedCodePoints) {
    int32_t item_count = uset_getItemCount(excludedCodePoints);
    for (int32_t j = 0; j < item_count; ++j) {
      UChar32 start_char;
      UChar32 end_char;

      uset_getItem(excludedCodePoints, j, &start_char, &end_char, NULL, 0,
                   status);
      for (int32_t col = 0; col < columns; col++) {
        upvec_setValue(upvec, start_char, end_char, col, ~0, ~0,
                      status);
      }
    }
  }

  
  
  result->trie = upvec_compactToUTrie2WithRowIndexes(upvec, status);
  result->pv = upvec_cloneArray(upvec, &result->pvCount, NULL, status);
  result->pvCount *= columns;  
  result->ownPv = TRUE;
}



U_CAPI UConverterSelector* U_EXPORT2
ucnvsel_open(const char* const*  converterList, int32_t converterListSize,
             const USet* excludedCodePoints,
             const UConverterUnicodeSet whichSet, UErrorCode* status) {
  
  if (U_FAILURE(*status)) {
    return NULL;
  }
  
  if (converterListSize < 0 || (converterList == NULL && converterListSize != 0)) {
    *status = U_ILLEGAL_ARGUMENT_ERROR;
    return NULL;
  }

  
  LocalUConverterSelectorPointer newSelector(
    (UConverterSelector*)uprv_malloc(sizeof(UConverterSelector)));
  if (newSelector.isNull()) {
    *status = U_MEMORY_ALLOCATION_ERROR;
    return NULL;
  }
  uprv_memset(newSelector.getAlias(), 0, sizeof(UConverterSelector));

  if (converterListSize == 0) {
    converterList = NULL;
    converterListSize = ucnv_countAvailable();
  }
  newSelector->encodings =
    (char**)uprv_malloc(converterListSize * sizeof(char*));
  if (!newSelector->encodings) {
    *status = U_MEMORY_ALLOCATION_ERROR;
    return NULL;
  }
  newSelector->encodings[0] = NULL;  

  
  int32_t totalSize = 0;
  int32_t i;
  for (i = 0; i < converterListSize; i++) {
    totalSize +=
      (int32_t)uprv_strlen(converterList != NULL ? converterList[i] : ucnv_getAvailableName(i)) + 1;
  }
  
  int32_t encodingStrPadding = totalSize & 3;
  if (encodingStrPadding != 0) {
    encodingStrPadding = 4 - encodingStrPadding;
  }
  newSelector->encodingStrLength = totalSize += encodingStrPadding;
  char* allStrings = (char*) uprv_malloc(totalSize);
  if (!allStrings) {
    *status = U_MEMORY_ALLOCATION_ERROR;
    return NULL;
  }

  for (i = 0; i < converterListSize; i++) {
    newSelector->encodings[i] = allStrings;
    uprv_strcpy(newSelector->encodings[i],
                converterList != NULL ? converterList[i] : ucnv_getAvailableName(i));
    allStrings += uprv_strlen(newSelector->encodings[i]) + 1;
  }
  while (encodingStrPadding > 0) {
    *allStrings++ = 0;
    --encodingStrPadding;
  }

  newSelector->ownEncodingStrings = TRUE;
  newSelector->encodingsCount = converterListSize;
  UPropsVectors *upvec = upvec_open((converterListSize+31)/32, status);
  generateSelectorData(newSelector.getAlias(), upvec, excludedCodePoints, whichSet, status);
  upvec_close(upvec);

  if (U_FAILURE(*status)) {
    return NULL;
  }

  return newSelector.orphan();
}


U_CAPI void U_EXPORT2
ucnvsel_close(UConverterSelector *sel) {
  if (!sel) {
    return;
  }
  if (sel->ownEncodingStrings) {
    uprv_free(sel->encodings[0]);
  }
  uprv_free(sel->encodings);
  if (sel->ownPv) {
    uprv_free(sel->pv);
  }
  utrie2_close(sel->trie);
  uprv_free(sel->swapped);
  uprv_free(sel);
}

static const UDataInfo dataInfo = {
  sizeof(UDataInfo),
  0,

  U_IS_BIG_ENDIAN,
  U_CHARSET_FAMILY,
  U_SIZEOF_UCHAR,
  0,

  { 0x43, 0x53, 0x65, 0x6c },   
  { 1, 0, 0, 0 },               
  { 0, 0, 0, 0 }                
};

enum {
  UCNVSEL_INDEX_TRIE_SIZE,      
  UCNVSEL_INDEX_PV_COUNT,       
  UCNVSEL_INDEX_NAMES_COUNT,    
  UCNVSEL_INDEX_NAMES_LENGTH,   
  UCNVSEL_INDEX_SIZE = 15,      
  UCNVSEL_INDEX_COUNT = 16
};














U_CAPI int32_t U_EXPORT2
ucnvsel_serialize(const UConverterSelector* sel,
                  void* buffer, int32_t bufferCapacity, UErrorCode* status) {
  
  if (U_FAILURE(*status)) {
    return 0;
  }
  
  uint8_t *p = (uint8_t *)buffer;
  if (bufferCapacity < 0 ||
      (bufferCapacity > 0 && (p == NULL || (U_POINTER_MASK_LSB(p, 3) != 0)))
  ) {
    *status = U_ILLEGAL_ARGUMENT_ERROR;
    return 0;
  }
  
  int32_t serializedTrieSize = utrie2_serialize(sel->trie, NULL, 0, status);
  if (*status != U_BUFFER_OVERFLOW_ERROR && U_FAILURE(*status)) {
    return 0;
  }
  *status = U_ZERO_ERROR;

  DataHeader header;
  uprv_memset(&header, 0, sizeof(header));
  header.dataHeader.headerSize = (uint16_t)((sizeof(header) + 15) & ~15);
  header.dataHeader.magic1 = 0xda;
  header.dataHeader.magic2 = 0x27;
  uprv_memcpy(&header.info, &dataInfo, sizeof(dataInfo));

  int32_t indexes[UCNVSEL_INDEX_COUNT] = {
    serializedTrieSize,
    sel->pvCount,
    sel->encodingsCount,
    sel->encodingStrLength
  };

  int32_t totalSize =
    header.dataHeader.headerSize +
    (int32_t)sizeof(indexes) +
    serializedTrieSize +
    sel->pvCount * 4 +
    sel->encodingStrLength;
  indexes[UCNVSEL_INDEX_SIZE] = totalSize - header.dataHeader.headerSize;
  if (totalSize > bufferCapacity) {
    *status = U_BUFFER_OVERFLOW_ERROR;
    return totalSize;
  }
  
  int32_t length = header.dataHeader.headerSize;
  uprv_memcpy(p, &header, sizeof(header));
  uprv_memset(p + sizeof(header), 0, length - sizeof(header));
  p += length;

  length = (int32_t)sizeof(indexes);
  uprv_memcpy(p, indexes, length);
  p += length;

  utrie2_serialize(sel->trie, p, serializedTrieSize, status);
  p += serializedTrieSize;

  length = sel->pvCount * 4;
  uprv_memcpy(p, sel->pv, length);
  p += length;

  uprv_memcpy(p, sel->encodings[0], sel->encodingStrLength);
  p += sel->encodingStrLength;

  return totalSize;
}


















static int32_t
ucnvsel_swap(const UDataSwapper *ds,
             const void *inData, int32_t length,
             void *outData, UErrorCode *status) {
  
  int32_t headerSize = udata_swapDataHeader(ds, inData, length, outData, status);
  if(U_FAILURE(*status)) {
    return 0;
  }

  
  const UDataInfo *pInfo = (const UDataInfo *)((const char *)inData + 4);
  if(!(
    pInfo->dataFormat[0] == 0x43 &&  
    pInfo->dataFormat[1] == 0x53 &&
    pInfo->dataFormat[2] == 0x65 &&
    pInfo->dataFormat[3] == 0x6c
  )) {
    udata_printError(ds, "ucnvsel_swap(): data format %02x.%02x.%02x.%02x is not recognized as UConverterSelector data\n",
                     pInfo->dataFormat[0], pInfo->dataFormat[1],
                     pInfo->dataFormat[2], pInfo->dataFormat[3]);
    *status = U_INVALID_FORMAT_ERROR;
    return 0;
  }
  if(pInfo->formatVersion[0] != 1) {
    udata_printError(ds, "ucnvsel_swap(): format version %02x is not supported\n",
                     pInfo->formatVersion[0]);
    *status = U_UNSUPPORTED_ERROR;
    return 0;
  }

  if(length >= 0) {
    length -= headerSize;
    if(length < 16*4) {
      udata_printError(ds, "ucnvsel_swap(): too few bytes (%d after header) for UConverterSelector data\n",
                       length);
      *status = U_INDEX_OUTOFBOUNDS_ERROR;
      return 0;
    }
  }

  const uint8_t *inBytes = (const uint8_t *)inData + headerSize;
  uint8_t *outBytes = (uint8_t *)outData + headerSize;

  
  const int32_t *inIndexes = (const int32_t *)inBytes;
  int32_t indexes[16];
  int32_t i;
  for(i = 0; i < 16; ++i) {
    indexes[i] = udata_readInt32(ds, inIndexes[i]);
  }

  
  int32_t size = indexes[UCNVSEL_INDEX_SIZE];
  if(length >= 0) {
    if(length < size) {
      udata_printError(ds, "ucnvsel_swap(): too few bytes (%d after header) for all of UConverterSelector data\n",
                       length);
      *status = U_INDEX_OUTOFBOUNDS_ERROR;
      return 0;
    }

    
    if(inBytes != outBytes) {
      uprv_memcpy(outBytes, inBytes, size);
    }

    int32_t offset = 0, count;

    
    count = UCNVSEL_INDEX_COUNT*4;
    ds->swapArray32(ds, inBytes, count, outBytes, status);
    offset += count;

    
    count = indexes[UCNVSEL_INDEX_TRIE_SIZE];
    utrie2_swap(ds, inBytes + offset, count, outBytes + offset, status);
    offset += count;

    
    count = indexes[UCNVSEL_INDEX_PV_COUNT]*4;
    ds->swapArray32(ds, inBytes + offset, count, outBytes + offset, status);
    offset += count;

    
    count = indexes[UCNVSEL_INDEX_NAMES_LENGTH];
    ds->swapInvChars(ds, inBytes + offset, count, outBytes + offset, status);
    offset += count;

    U_ASSERT(offset == size);
  }

  return headerSize + size;
}


U_CAPI UConverterSelector* U_EXPORT2
ucnvsel_openFromSerialized(const void* buffer, int32_t length, UErrorCode* status) {
  
  if (U_FAILURE(*status)) {
    return NULL;
  }
  
  const uint8_t *p = (const uint8_t *)buffer;
  if (length <= 0 ||
      (length > 0 && (p == NULL || (U_POINTER_MASK_LSB(p, 3) != 0)))
  ) {
    *status = U_ILLEGAL_ARGUMENT_ERROR;
    return NULL;
  }
  
  if (length < 32) {
    
    *status = U_INDEX_OUTOFBOUNDS_ERROR;
    return NULL;
  }
  const DataHeader *pHeader = (const DataHeader *)p;
  if (!(
    pHeader->dataHeader.magic1==0xda &&
    pHeader->dataHeader.magic2==0x27 &&
    pHeader->info.dataFormat[0] == 0x43 &&
    pHeader->info.dataFormat[1] == 0x53 &&
    pHeader->info.dataFormat[2] == 0x65 &&
    pHeader->info.dataFormat[3] == 0x6c
  )) {
    
    *status = U_INVALID_FORMAT_ERROR;
    return NULL;
  }
  if (pHeader->info.formatVersion[0] != 1) {
    *status = U_UNSUPPORTED_ERROR;
    return NULL;
  }
  uint8_t* swapped = NULL;
  if (pHeader->info.isBigEndian != U_IS_BIG_ENDIAN ||
      pHeader->info.charsetFamily != U_CHARSET_FAMILY
  ) {
    
    UDataSwapper *ds =
      udata_openSwapperForInputData(p, length, U_IS_BIG_ENDIAN, U_CHARSET_FAMILY, status);
    int32_t totalSize = ucnvsel_swap(ds, p, -1, NULL, status);
    if (U_FAILURE(*status)) {
      udata_closeSwapper(ds);
      return NULL;
    }
    if (length < totalSize) {
      udata_closeSwapper(ds);
      *status = U_INDEX_OUTOFBOUNDS_ERROR;
      return NULL;
    }
    swapped = (uint8_t*)uprv_malloc(totalSize);
    if (swapped == NULL) {
      udata_closeSwapper(ds);
      *status = U_MEMORY_ALLOCATION_ERROR;
      return NULL;
    }
    ucnvsel_swap(ds, p, length, swapped, status);
    udata_closeSwapper(ds);
    if (U_FAILURE(*status)) {
      uprv_free(swapped);
      return NULL;
    }
    p = swapped;
    pHeader = (const DataHeader *)p;
  }
  if (length < (pHeader->dataHeader.headerSize + 16 * 4)) {
    
    uprv_free(swapped);
    *status = U_INDEX_OUTOFBOUNDS_ERROR;
    return NULL;
  }
  p += pHeader->dataHeader.headerSize;
  length -= pHeader->dataHeader.headerSize;
  
  const int32_t *indexes = (const int32_t *)p;
  if (length < indexes[UCNVSEL_INDEX_SIZE]) {
    uprv_free(swapped);
    *status = U_INDEX_OUTOFBOUNDS_ERROR;
    return NULL;
  }
  p += UCNVSEL_INDEX_COUNT * 4;
  
  UConverterSelector* sel = (UConverterSelector*)uprv_malloc(sizeof(UConverterSelector));
  char **encodings =
    (char **)uprv_malloc(
      indexes[UCNVSEL_INDEX_NAMES_COUNT] * sizeof(char *));
  if (sel == NULL || encodings == NULL) {
    uprv_free(swapped);
    uprv_free(sel);
    uprv_free(encodings);
    *status = U_MEMORY_ALLOCATION_ERROR;
    return NULL;
  }
  uprv_memset(sel, 0, sizeof(UConverterSelector));
  sel->pvCount = indexes[UCNVSEL_INDEX_PV_COUNT];
  sel->encodings = encodings;
  sel->encodingsCount = indexes[UCNVSEL_INDEX_NAMES_COUNT];
  sel->encodingStrLength = indexes[UCNVSEL_INDEX_NAMES_LENGTH];
  sel->swapped = swapped;
  
  sel->trie = utrie2_openFromSerialized(UTRIE2_16_VALUE_BITS,
                                        p, indexes[UCNVSEL_INDEX_TRIE_SIZE], NULL,
                                        status);
  p += indexes[UCNVSEL_INDEX_TRIE_SIZE];
  if (U_FAILURE(*status)) {
    ucnvsel_close(sel);
    return NULL;
  }
  
  sel->pv = (uint32_t *)p;
  p += sel->pvCount * 4;
  
  char* s = (char*)p;
  for (int32_t i = 0; i < sel->encodingsCount; ++i) {
    sel->encodings[i] = s;
    s += uprv_strlen(s) + 1;
  }
  p += sel->encodingStrLength;

  return sel;
}



struct Enumerator {
  int16_t* index;
  int16_t length;
  int16_t cur;
  const UConverterSelector* sel;
};

U_CDECL_BEGIN

static void U_CALLCONV
ucnvsel_close_selector_iterator(UEnumeration *enumerator) {
  uprv_free(((Enumerator*)(enumerator->context))->index);
  uprv_free(enumerator->context);
  uprv_free(enumerator);
}


static int32_t U_CALLCONV
ucnvsel_count_encodings(UEnumeration *enumerator, UErrorCode *status) {
  
  if (U_FAILURE(*status)) {
    return 0;
  }
  return ((Enumerator*)(enumerator->context))->length;
}


static const char* U_CALLCONV ucnvsel_next_encoding(UEnumeration* enumerator,
                                                 int32_t* resultLength,
                                                 UErrorCode* status) {
  
  if (U_FAILURE(*status)) {
    return NULL;
  }

  int16_t cur = ((Enumerator*)(enumerator->context))->cur;
  const UConverterSelector* sel;
  const char* result;
  if (cur >= ((Enumerator*)(enumerator->context))->length) {
    return NULL;
  }
  sel = ((Enumerator*)(enumerator->context))->sel;
  result = sel->encodings[((Enumerator*)(enumerator->context))->index[cur] ];
  ((Enumerator*)(enumerator->context))->cur++;
  if (resultLength) {
    *resultLength = (int32_t)uprv_strlen(result);
  }
  return result;
}

static void U_CALLCONV ucnvsel_reset_iterator(UEnumeration* enumerator,
                                           UErrorCode* status) {
  
  if (U_FAILURE(*status)) {
    return ;
  }
  ((Enumerator*)(enumerator->context))->cur = 0;
}

U_CDECL_END


static const UEnumeration defaultEncodings = {
  NULL,
    NULL,
    ucnvsel_close_selector_iterator,
    ucnvsel_count_encodings,
    uenum_unextDefault,
    ucnvsel_next_encoding, 
    ucnvsel_reset_iterator
};




static UBool intersectMasks(uint32_t* dest, const uint32_t* source1, int32_t len) {
  int32_t i;
  uint32_t oredDest = 0;
  for (i = 0 ; i < len ; ++i) {
    oredDest |= (dest[i] &= source1[i]);
  }
  return oredDest == 0;
}



static int16_t countOnes(uint32_t* mask, int32_t len) {
  int32_t i, totalOnes = 0;
  for (i = 0 ; i < len ; ++i) {
    uint32_t ent = mask[i];
    for (; ent; totalOnes++)
    {
      ent &= ent - 1; 
    }
  }
  return totalOnes;
}



static UEnumeration *selectForMask(const UConverterSelector* sel,
                                   uint32_t *mask, UErrorCode *status) {
  
  
  struct Enumerator* result = (Enumerator*)uprv_malloc(sizeof(Enumerator));
  if (result == NULL) {
    uprv_free(mask);
    *status = U_MEMORY_ALLOCATION_ERROR;
    return NULL;
  }
  result->index = NULL;  
  result->length = result->cur = 0;
  result->sel = sel;

  UEnumeration *en = (UEnumeration *)uprv_malloc(sizeof(UEnumeration));
  if (en == NULL) {
    
    uprv_free(mask);
    uprv_free(result);
    *status = U_MEMORY_ALLOCATION_ERROR;
    return NULL;
  }
  memcpy(en, &defaultEncodings, sizeof(UEnumeration));
  en->context = result;

  int32_t columns = (sel->encodingsCount+31)/32;
  int16_t numOnes = countOnes(mask, columns);
  
  if (numOnes > 0) {
    result->index = (int16_t*) uprv_malloc(numOnes * sizeof(int16_t));

    int32_t i, j;
    int16_t k = 0;
    for (j = 0 ; j < columns; j++) {
      uint32_t v = mask[j];
      for (i = 0 ; i < 32 && k < sel->encodingsCount; i++, k++) {
        if ((v & 1) != 0) {
          result->index[result->length++] = k;
        }
        v >>= 1;
      }
    }
  } 
    
  uprv_free(mask);
  return en;
}


U_CAPI UEnumeration * U_EXPORT2
ucnvsel_selectForString(const UConverterSelector* sel,
                        const UChar *s, int32_t length, UErrorCode *status) {
  
  if (U_FAILURE(*status)) {
    return NULL;
  }
  
  if (sel == NULL || (s == NULL && length != 0)) {
    *status = U_ILLEGAL_ARGUMENT_ERROR;
    return NULL;
  }

  int32_t columns = (sel->encodingsCount+31)/32;
  uint32_t* mask = (uint32_t*) uprv_malloc(columns * 4);
  if (mask == NULL) {
    *status = U_MEMORY_ALLOCATION_ERROR;
    return NULL;
  }
  uprv_memset(mask, ~0, columns *4);

  if(s!=NULL) {
    const UChar *limit;
    if (length >= 0) {
      limit = s + length;
    } else {
      limit = NULL;
    }
    
    while (limit == NULL ? *s != 0 : s != limit) {
      UChar32 c;
      uint16_t pvIndex;
      UTRIE2_U16_NEXT16(sel->trie, s, limit, c, pvIndex);
      if (intersectMasks(mask, sel->pv+pvIndex, columns)) {
        break;
      }
    }
  }
  return selectForMask(sel, mask, status);
}


U_CAPI UEnumeration * U_EXPORT2
ucnvsel_selectForUTF8(const UConverterSelector* sel,
                      const char *s, int32_t length, UErrorCode *status) {
  
  if (U_FAILURE(*status)) {
    return NULL;
  }
  
  if (sel == NULL || (s == NULL && length != 0)) {
    *status = U_ILLEGAL_ARGUMENT_ERROR;
    return NULL;
  }

  int32_t columns = (sel->encodingsCount+31)/32;
  uint32_t* mask = (uint32_t*) uprv_malloc(columns * 4);
  if (mask == NULL) {
    *status = U_MEMORY_ALLOCATION_ERROR;
    return NULL;
  }
  uprv_memset(mask, ~0, columns *4);

  if (length < 0) {
    length = (int32_t)uprv_strlen(s);
  }

  if(s!=NULL) {
    const char *limit = s + length;
    
    while (s != limit) {
      uint16_t pvIndex;
      UTRIE2_U8_NEXT16(sel->trie, s, limit, pvIndex);
      if (intersectMasks(mask, sel->pv+pvIndex, columns)) {
        break;
      }
    }
  }
  return selectForMask(sel, mask, status);
}

#endif  
