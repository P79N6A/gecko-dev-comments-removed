










#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/ucol.h"
#include "unicode/udata.h"
#include "unicode/uscript.h"
#include "cmemory.h"
#include "collation.h"
#include "collationdata.h"
#include "collationdatareader.h"
#include "collationfastlatin.h"
#include "collationkeys.h"
#include "collationrootelements.h"
#include "collationsettings.h"
#include "collationtailoring.h"
#include "normalizer2impl.h"
#include "uassert.h"
#include "ucmndata.h"
#include "utrie2.h"

U_NAMESPACE_BEGIN

namespace {

int32_t getIndex(const int32_t *indexes, int32_t length, int32_t i) {
    return (i < length) ? indexes[i] : -1;
}

}  

void
CollationDataReader::read(const CollationTailoring *base, const uint8_t *inBytes, int32_t inLength,
                          CollationTailoring &tailoring, UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) { return; }
    if(base != NULL) {
        if(inBytes == NULL || (0 <= inLength && inLength < 24)) {
            errorCode = U_ILLEGAL_ARGUMENT_ERROR;
            return;
        }
        const DataHeader *header = reinterpret_cast<const DataHeader *>(inBytes);
        if(!(header->dataHeader.magic1 == 0xda && header->dataHeader.magic2 == 0x27 &&
                isAcceptable(tailoring.version, NULL, NULL, &header->info))) {
            errorCode = U_INVALID_FORMAT_ERROR;
            return;
        }
        if(base->getUCAVersion() != tailoring.getUCAVersion()) {
            errorCode = U_COLLATOR_VERSION_MISMATCH;
            return;
        }
        int32_t headerLength = header->dataHeader.headerSize;
        inBytes += headerLength;
        if(inLength >= 0) {
            inLength -= headerLength;
        }
    }

    if(inBytes == NULL || (0 <= inLength && inLength < 8)) {
        errorCode = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    const int32_t *inIndexes = reinterpret_cast<const int32_t *>(inBytes);
    int32_t indexesLength = inIndexes[IX_INDEXES_LENGTH];
    if(indexesLength < 2 || (0 <= inLength && inLength < indexesLength * 4)) {
        errorCode = U_INVALID_FORMAT_ERROR;  
        return;
    }

    
    

    
    

    int32_t index;  
    int32_t offset;  
    int32_t length;  

    if(indexesLength > IX_TOTAL_SIZE) {
        length = inIndexes[IX_TOTAL_SIZE];
    } else if(indexesLength > IX_REORDER_CODES_OFFSET) {
        length = inIndexes[indexesLength - 1];
    } else {
        length = 0;  
    }
    if(0 <= inLength && inLength < length) {
        errorCode = U_INVALID_FORMAT_ERROR;
        return;
    }

    const CollationData *baseData = base == NULL ? NULL : base->data;
    const int32_t *reorderCodes = NULL;
    int32_t reorderCodesLength = 0;
    const uint32_t *reorderRanges = NULL;
    int32_t reorderRangesLength = 0;
    index = IX_REORDER_CODES_OFFSET;
    offset = getIndex(inIndexes, indexesLength, index);
    length = getIndex(inIndexes, indexesLength, index + 1) - offset;
    if(length >= 4) {
        if(baseData == NULL) {
            
            
            errorCode = U_INVALID_FORMAT_ERROR;
            return;
        }
        reorderCodes = reinterpret_cast<const int32_t *>(inBytes + offset);
        reorderCodesLength = length / 4;

        
        
        
        
        while(reorderRangesLength < reorderCodesLength &&
                (reorderCodes[reorderCodesLength - reorderRangesLength - 1] & 0xffff0000) != 0) {
            ++reorderRangesLength;
        }
        U_ASSERT(reorderRangesLength < reorderCodesLength);
        if(reorderRangesLength != 0) {
            reorderCodesLength -= reorderRangesLength;
            reorderRanges = reinterpret_cast<const uint32_t *>(reorderCodes + reorderCodesLength);
        }
    }

    
    
    
    const uint8_t *reorderTable = NULL;
    index = IX_REORDER_TABLE_OFFSET;
    offset = getIndex(inIndexes, indexesLength, index);
    length = getIndex(inIndexes, indexesLength, index + 1) - offset;
    if(length >= 256) {
        if(reorderCodesLength == 0) {
            errorCode = U_INVALID_FORMAT_ERROR;  
            return;
        }
        reorderTable = inBytes + offset;
    } else {
        
        
    }

    if(baseData != NULL && baseData->numericPrimary != (inIndexes[IX_OPTIONS] & 0xff000000)) {
        errorCode = U_INVALID_FORMAT_ERROR;
        return;
    }
    CollationData *data = NULL;  

    index = IX_TRIE_OFFSET;
    offset = getIndex(inIndexes, indexesLength, index);
    length = getIndex(inIndexes, indexesLength, index + 1) - offset;
    if(length >= 8) {
        if(!tailoring.ensureOwnedData(errorCode)) { return; }
        data = tailoring.ownedData;
        data->base = baseData;
        data->numericPrimary = inIndexes[IX_OPTIONS] & 0xff000000;
        data->trie = tailoring.trie = utrie2_openFromSerialized(
            UTRIE2_32_VALUE_BITS, inBytes + offset, length, NULL,
            &errorCode);
        if(U_FAILURE(errorCode)) { return; }
    } else if(baseData != NULL) {
        
        tailoring.data = baseData;
    } else {
        errorCode = U_INVALID_FORMAT_ERROR;  
        return;
    }

    index = IX_CES_OFFSET;
    offset = getIndex(inIndexes, indexesLength, index);
    length = getIndex(inIndexes, indexesLength, index + 1) - offset;
    if(length >= 8) {
        if(data == NULL) {
            errorCode = U_INVALID_FORMAT_ERROR;  
            return;
        }
        data->ces = reinterpret_cast<const int64_t *>(inBytes + offset);
        data->cesLength = length / 8;
    }

    index = IX_CE32S_OFFSET;
    offset = getIndex(inIndexes, indexesLength, index);
    length = getIndex(inIndexes, indexesLength, index + 1) - offset;
    if(length >= 4) {
        if(data == NULL) {
            errorCode = U_INVALID_FORMAT_ERROR;  
            return;
        }
        data->ce32s = reinterpret_cast<const uint32_t *>(inBytes + offset);
        data->ce32sLength = length / 4;
    }

    int32_t jamoCE32sStart = getIndex(inIndexes, indexesLength, IX_JAMO_CE32S_START);
    if(jamoCE32sStart >= 0) {
        if(data == NULL || data->ce32s == NULL) {
            errorCode = U_INVALID_FORMAT_ERROR;  
            return;
        }
        data->jamoCE32s = data->ce32s + jamoCE32sStart;
    } else if(data == NULL) {
        
    } else if(baseData != NULL) {
        data->jamoCE32s = baseData->jamoCE32s;
    } else {
        errorCode = U_INVALID_FORMAT_ERROR;  
        return;
    }

    index = IX_ROOT_ELEMENTS_OFFSET;
    offset = getIndex(inIndexes, indexesLength, index);
    length = getIndex(inIndexes, indexesLength, index + 1) - offset;
    if(length >= 4) {
        length /= 4;
        if(data == NULL || length <= CollationRootElements::IX_SEC_TER_BOUNDARIES) {
            errorCode = U_INVALID_FORMAT_ERROR;
            return;
        }
        data->rootElements = reinterpret_cast<const uint32_t *>(inBytes + offset);
        data->rootElementsLength = length;
        uint32_t commonSecTer = data->rootElements[CollationRootElements::IX_COMMON_SEC_AND_TER_CE];
        if(commonSecTer != Collation::COMMON_SEC_AND_TER_CE) {
            errorCode = U_INVALID_FORMAT_ERROR;
            return;
        }
        uint32_t secTerBoundaries = data->rootElements[CollationRootElements::IX_SEC_TER_BOUNDARIES];
        if((secTerBoundaries >> 24) < CollationKeys::SEC_COMMON_HIGH) {
            
            
            errorCode = U_INVALID_FORMAT_ERROR;
            return;
        }
    }

    index = IX_CONTEXTS_OFFSET;
    offset = getIndex(inIndexes, indexesLength, index);
    length = getIndex(inIndexes, indexesLength, index + 1) - offset;
    if(length >= 2) {
        if(data == NULL) {
            errorCode = U_INVALID_FORMAT_ERROR;  
            return;
        }
        data->contexts = reinterpret_cast<const UChar *>(inBytes + offset);
        data->contextsLength = length / 2;
    }

    index = IX_UNSAFE_BWD_OFFSET;
    offset = getIndex(inIndexes, indexesLength, index);
    length = getIndex(inIndexes, indexesLength, index + 1) - offset;
    if(length >= 2) {
        if(data == NULL) {
            errorCode = U_INVALID_FORMAT_ERROR;
            return;
        }
        if(baseData == NULL) {
            
            
            
            
            
            
            
            
            
            
            
            tailoring.unsafeBackwardSet = new UnicodeSet(0xdc00, 0xdfff);  
            if(tailoring.unsafeBackwardSet == NULL) {
                errorCode = U_MEMORY_ALLOCATION_ERROR;
                return;
            }
            data->nfcImpl.addLcccChars(*tailoring.unsafeBackwardSet);
        } else {
            
            tailoring.unsafeBackwardSet = static_cast<UnicodeSet *>(
                baseData->unsafeBackwardSet->cloneAsThawed());
            if(tailoring.unsafeBackwardSet == NULL) {
                errorCode = U_MEMORY_ALLOCATION_ERROR;
                return;
            }
        }
        
        USerializedSet sset;
        const uint16_t *unsafeData = reinterpret_cast<const uint16_t *>(inBytes + offset);
        if(!uset_getSerializedSet(&sset, unsafeData, length / 2)) {
            errorCode = U_INVALID_FORMAT_ERROR;
            return;
        }
        int32_t count = uset_getSerializedRangeCount(&sset);
        for(int32_t i = 0; i < count; ++i) {
            UChar32 start, end;
            uset_getSerializedRange(&sset, i, &start, &end);
            tailoring.unsafeBackwardSet->add(start, end);
        }
        
        
        UChar32 c = 0x10000;
        for(UChar lead = 0xd800; lead < 0xdc00; ++lead, c += 0x400) {
            if(!tailoring.unsafeBackwardSet->containsNone(c, c + 0x3ff)) {
                tailoring.unsafeBackwardSet->add(lead);
            }
        }
        tailoring.unsafeBackwardSet->freeze();
        data->unsafeBackwardSet = tailoring.unsafeBackwardSet;
    } else if(data == NULL) {
        
    } else if(baseData != NULL) {
        
        data->unsafeBackwardSet = baseData->unsafeBackwardSet;
    } else {
        errorCode = U_INVALID_FORMAT_ERROR;  
        return;
    }

    
    
    
    if(data != NULL) {
        data->fastLatinTable = NULL;
        data->fastLatinTableLength = 0;
        if(((inIndexes[IX_OPTIONS] >> 16) & 0xff) == CollationFastLatin::VERSION) {
            index = IX_FAST_LATIN_TABLE_OFFSET;
            offset = getIndex(inIndexes, indexesLength, index);
            length = getIndex(inIndexes, indexesLength, index + 1) - offset;
            if(length >= 2) {
                data->fastLatinTable = reinterpret_cast<const uint16_t *>(inBytes + offset);
                data->fastLatinTableLength = length / 2;
                if((*data->fastLatinTable >> 8) != CollationFastLatin::VERSION) {
                    errorCode = U_INVALID_FORMAT_ERROR;  
                    return;
                }
            } else if(baseData != NULL) {
                data->fastLatinTable = baseData->fastLatinTable;
                data->fastLatinTableLength = baseData->fastLatinTableLength;
            }
        }
    }

    index = IX_SCRIPTS_OFFSET;
    offset = getIndex(inIndexes, indexesLength, index);
    length = getIndex(inIndexes, indexesLength, index + 1) - offset;
    if(length >= 2) {
        if(data == NULL) {
            errorCode = U_INVALID_FORMAT_ERROR;
            return;
        }
        const uint16_t *scripts = reinterpret_cast<const uint16_t *>(inBytes + offset);
        int32_t scriptsLength = length / 2;
        data->numScripts = scripts[0];
        
        data->scriptStartsLength = scriptsLength - (1 + data->numScripts + 16);
        if(data->scriptStartsLength <= 2 ||
                CollationData::MAX_NUM_SCRIPT_RANGES < data->scriptStartsLength) {
            errorCode = U_INVALID_FORMAT_ERROR;
            return;
        }
        data->scriptsIndex = scripts + 1;
        data->scriptStarts = scripts + 1 + data->numScripts + 16;
        if(!(data->scriptStarts[0] == 0 &&
                data->scriptStarts[1] == ((Collation::MERGE_SEPARATOR_BYTE + 1) << 8) &&
                data->scriptStarts[data->scriptStartsLength - 1] ==
                        (Collation::TRAIL_WEIGHT_BYTE << 8))) {
            errorCode = U_INVALID_FORMAT_ERROR;
            return;
        }
    } else if(data == NULL) {
        
    } else if(baseData != NULL) {
        data->numScripts = baseData->numScripts;
        data->scriptsIndex = baseData->scriptsIndex;
        data->scriptStarts = baseData->scriptStarts;
        data->scriptStartsLength = baseData->scriptStartsLength;
    }

    index = IX_COMPRESSIBLE_BYTES_OFFSET;
    offset = getIndex(inIndexes, indexesLength, index);
    length = getIndex(inIndexes, indexesLength, index + 1) - offset;
    if(length >= 256) {
        if(data == NULL) {
            errorCode = U_INVALID_FORMAT_ERROR;
            return;
        }
        data->compressibleBytes = reinterpret_cast<const UBool *>(inBytes + offset);
    } else if(data == NULL) {
        
    } else if(baseData != NULL) {
        data->compressibleBytes = baseData->compressibleBytes;
    } else {
        errorCode = U_INVALID_FORMAT_ERROR;  
        return;
    }

    const CollationSettings &ts = *tailoring.settings;
    int32_t options = inIndexes[IX_OPTIONS] & 0xffff;
    uint16_t fastLatinPrimaries[CollationFastLatin::LATIN_LIMIT];
    int32_t fastLatinOptions = CollationFastLatin::getOptions(
            tailoring.data, ts, fastLatinPrimaries, UPRV_LENGTHOF(fastLatinPrimaries));
    if(options == ts.options && ts.variableTop != 0 &&
            reorderCodesLength == ts.reorderCodesLength &&
            uprv_memcmp(reorderCodes, ts.reorderCodes, reorderCodesLength * 4) == 0 &&
            fastLatinOptions == ts.fastLatinOptions &&
            (fastLatinOptions < 0 ||
                uprv_memcmp(fastLatinPrimaries, ts.fastLatinPrimaries,
                            sizeof(fastLatinPrimaries)) == 0)) {
        return;
    }

    CollationSettings *settings = SharedObject::copyOnWrite(tailoring.settings);
    if(settings == NULL) {
        errorCode = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    settings->options = options;
    
    settings->variableTop = tailoring.data->getLastPrimaryForGroup(
            UCOL_REORDER_CODE_FIRST + settings->getMaxVariable());
    if(settings->variableTop == 0) {
        errorCode = U_INVALID_FORMAT_ERROR;
        return;
    }

    if(reorderCodesLength != 0) {
        settings->aliasReordering(*baseData, reorderCodes, reorderCodesLength,
                                  reorderRanges, reorderRangesLength,
                                  reorderTable, errorCode);
    }

    settings->fastLatinOptions = CollationFastLatin::getOptions(
        tailoring.data, *settings,
        settings->fastLatinPrimaries, UPRV_LENGTHOF(settings->fastLatinPrimaries));
}

UBool U_CALLCONV
CollationDataReader::isAcceptable(void *context,
                                  const char * , const char * ,
                                  const UDataInfo *pInfo) {
    if(
        pInfo->size >= 20 &&
        pInfo->isBigEndian == U_IS_BIG_ENDIAN &&
        pInfo->charsetFamily == U_CHARSET_FAMILY &&
        pInfo->dataFormat[0] == 0x55 &&  
        pInfo->dataFormat[1] == 0x43 &&
        pInfo->dataFormat[2] == 0x6f &&
        pInfo->dataFormat[3] == 0x6c &&
        pInfo->formatVersion[0] == 5
    ) {
        UVersionInfo *version = static_cast<UVersionInfo *>(context);
        if(version != NULL) {
            uprv_memcpy(version, pInfo->dataVersion, 4);
        }
        return TRUE;
    } else {
        return FALSE;
    }
}

U_NAMESPACE_END

#endif  
