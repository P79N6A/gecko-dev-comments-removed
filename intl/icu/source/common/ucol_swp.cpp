

















#include "unicode/udata.h" 
#include "utrie.h"
#include "utrie2.h"
#include "udataswp.h"
#include "cmemory.h"
#include "ucol_data.h"
#include "ucol_swp.h"







U_CAPI int32_t U_EXPORT2
utrie_swap(const UDataSwapper *ds,
           const void *inData, int32_t length, void *outData,
           UErrorCode *pErrorCode) {
    const UTrieHeader *inTrie;
    UTrieHeader trie;
    int32_t size;
    UBool dataIs32;

    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }
    if(ds==NULL || inData==NULL || (length>=0 && outData==NULL)) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    
    if(length>=0 && (uint32_t)length<sizeof(UTrieHeader)) {
        *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
        return 0;
    }

    inTrie=(const UTrieHeader *)inData;
    trie.signature=ds->readUInt32(inTrie->signature);
    trie.options=ds->readUInt32(inTrie->options);
    trie.indexLength=udata_readInt32(ds, inTrie->indexLength);
    trie.dataLength=udata_readInt32(ds, inTrie->dataLength);

    if( trie.signature!=0x54726965 ||
        (trie.options&UTRIE_OPTIONS_SHIFT_MASK)!=UTRIE_SHIFT ||
        ((trie.options>>UTRIE_OPTIONS_INDEX_SHIFT)&UTRIE_OPTIONS_SHIFT_MASK)!=UTRIE_INDEX_SHIFT ||
        trie.indexLength<UTRIE_BMP_INDEX_LENGTH ||
        (trie.indexLength&(UTRIE_SURROGATE_BLOCK_COUNT-1))!=0 ||
        trie.dataLength<UTRIE_DATA_BLOCK_LENGTH ||
        (trie.dataLength&(UTRIE_DATA_GRANULARITY-1))!=0 ||
        ((trie.options&UTRIE_OPTIONS_LATIN1_IS_LINEAR)!=0 && trie.dataLength<(UTRIE_DATA_BLOCK_LENGTH+0x100))
    ) {
        *pErrorCode=U_INVALID_FORMAT_ERROR; 
        return 0;
    }

    dataIs32=(UBool)((trie.options&UTRIE_OPTIONS_DATA_IS_32_BIT)!=0);
    size=sizeof(UTrieHeader)+trie.indexLength*2+trie.dataLength*(dataIs32?4:2);

    if(length>=0) {
        UTrieHeader *outTrie;

        if(length<size) {
            *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
            return 0;
        }

        outTrie=(UTrieHeader *)outData;

        
        ds->swapArray32(ds, inTrie, sizeof(UTrieHeader), outTrie, pErrorCode);

        
        if(dataIs32) {
            ds->swapArray16(ds, inTrie+1, trie.indexLength*2, outTrie+1, pErrorCode);
            ds->swapArray32(ds, (const uint16_t *)(inTrie+1)+trie.indexLength, trie.dataLength*4,
                                     (uint16_t *)(outTrie+1)+trie.indexLength, pErrorCode);
        } else {
            ds->swapArray16(ds, inTrie+1, (trie.indexLength+trie.dataLength)*2, outTrie+1, pErrorCode);
        }
    }

    return size;
}

#if !UCONFIG_NO_COLLATION

U_CAPI UBool U_EXPORT2
ucol_looksLikeCollationBinary(const UDataSwapper *ds,
                              const void *inData, int32_t length) {
    if(ds==NULL || inData==NULL || length<-1) {
        return FALSE;
    }

    
    UErrorCode errorCode=U_ZERO_ERROR;
    (void)udata_swapDataHeader(ds, inData, -1, NULL, &errorCode);
    if(U_SUCCESS(errorCode)) {
        const UDataInfo &info=*(const UDataInfo *)((const char *)inData+4);
        if(info.dataFormat[0]==0x55 &&   
                info.dataFormat[1]==0x43 &&
                info.dataFormat[2]==0x6f &&
                info.dataFormat[3]==0x6c) {
            return TRUE;
        }
    }

    
    const UCATableHeader *inHeader=(const UCATableHeader *)inData;

    





    UCATableHeader header;
    uprv_memset(&header, 0, sizeof(header));
    if(length<0) {
        header.size=udata_readInt32(ds, inHeader->size);
    } else if((length<(42*4) || length<(header.size=udata_readInt32(ds, inHeader->size)))) {
        return FALSE;
    }

    header.magic=ds->readUInt32(inHeader->magic);
    if(!(
        header.magic==UCOL_HEADER_MAGIC &&
        inHeader->formatVersion[0]==3 

    )) {
        return FALSE;
    }

    if(inHeader->isBigEndian!=ds->inIsBigEndian || inHeader->charSetFamily!=ds->inCharset) {
        return FALSE;
    }

    return TRUE;
}

namespace {


int32_t
swapFormatVersion3(const UDataSwapper *ds,
                   const void *inData, int32_t length, void *outData,
                   UErrorCode *pErrorCode) {
    const uint8_t *inBytes;
    uint8_t *outBytes;

    const UCATableHeader *inHeader;
    UCATableHeader *outHeader;
    UCATableHeader header;

    uint32_t count;

    
    if(U_FAILURE(*pErrorCode)) {
        return 0;
    }
    if(ds==NULL || inData==NULL || length<-1 || (length>0 && outData==NULL)) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    inBytes=(const uint8_t *)inData;
    outBytes=(uint8_t *)outData;

    inHeader=(const UCATableHeader *)inData;
    outHeader=(UCATableHeader *)outData;

    





    uprv_memset(&header, 0, sizeof(header));
    if(length<0) {
        header.size=udata_readInt32(ds, inHeader->size);
    } else if((length<(42*4) || length<(header.size=udata_readInt32(ds, inHeader->size)))) {
        udata_printError(ds, "ucol_swap(formatVersion=3): too few bytes (%d after header) for collation data\n",
                         length);
        *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
        return 0;
    }

    header.magic=ds->readUInt32(inHeader->magic);
    if(!(
        header.magic==UCOL_HEADER_MAGIC &&
        inHeader->formatVersion[0]==3 

    )) {
        udata_printError(ds, "ucol_swap(formatVersion=3): magic 0x%08x or format version %02x.%02x is not a collation binary\n",
                         header.magic,
                         inHeader->formatVersion[0], inHeader->formatVersion[1]);
        *pErrorCode=U_UNSUPPORTED_ERROR;
        return 0;
    }

    if(inHeader->isBigEndian!=ds->inIsBigEndian || inHeader->charSetFamily!=ds->inCharset) {
        udata_printError(ds, "ucol_swap(formatVersion=3): endianness %d or charset %d does not match the swapper\n",
                         inHeader->isBigEndian, inHeader->charSetFamily);
        *pErrorCode=U_INVALID_FORMAT_ERROR;
        return 0;
    }

    if(length>=0) {
        
        if(inBytes!=outBytes) {
            uprv_memcpy(outBytes, inBytes, header.size);
        }

        

        
        header.options=                 ds->readUInt32(inHeader->options);
        header.UCAConsts=               ds->readUInt32(inHeader->UCAConsts);
        header.contractionUCACombos=    ds->readUInt32(inHeader->contractionUCACombos);
        header.mappingPosition=         ds->readUInt32(inHeader->mappingPosition);
        header.expansion=               ds->readUInt32(inHeader->expansion);
        header.contractionIndex=        ds->readUInt32(inHeader->contractionIndex);
        header.contractionCEs=          ds->readUInt32(inHeader->contractionCEs);
        header.contractionSize=         ds->readUInt32(inHeader->contractionSize);
        header.endExpansionCE=          ds->readUInt32(inHeader->endExpansionCE);
        header.expansionCESize=         ds->readUInt32(inHeader->expansionCESize);
        header.endExpansionCECount=     udata_readInt32(ds, inHeader->endExpansionCECount);
        header.contractionUCACombosSize=udata_readInt32(ds, inHeader->contractionUCACombosSize);
        header.scriptToLeadByte=        ds->readUInt32(inHeader->scriptToLeadByte);
        header.leadByteToScript=        ds->readUInt32(inHeader->leadByteToScript);
        
        
        ds->swapArray32(ds, inHeader, (int32_t)((const char *)&inHeader->jamoSpecial-(const char *)inHeader),
                           outHeader, pErrorCode);
        ds->swapArray32(ds, &(inHeader->scriptToLeadByte), sizeof(header.scriptToLeadByte) + sizeof(header.leadByteToScript),
                           &(outHeader->scriptToLeadByte), pErrorCode);
        
        outHeader->isBigEndian=ds->outIsBigEndian;
        outHeader->charSetFamily=ds->outCharset;

        
        if(header.options!=0) {
            ds->swapArray32(ds, inBytes+header.options, header.expansion-header.options,
                               outBytes+header.options, pErrorCode);
        }

        
        if(header.mappingPosition!=0 && header.expansion!=0) {
            if(header.contractionIndex!=0) {
                
                count=header.contractionIndex-header.expansion;
            } else {
                
                count=header.mappingPosition-header.expansion;
            }
            ds->swapArray32(ds, inBytes+header.expansion, (int32_t)count,
                               outBytes+header.expansion, pErrorCode);
        }

        
        if(header.contractionSize!=0) {
            
            ds->swapArray16(ds, inBytes+header.contractionIndex, header.contractionSize*2,
                               outBytes+header.contractionIndex, pErrorCode);

            
            ds->swapArray32(ds, inBytes+header.contractionCEs, header.contractionSize*4,
                               outBytes+header.contractionCEs, pErrorCode);
        }

        
        if(header.mappingPosition!=0) {
            count=header.endExpansionCE-header.mappingPosition;
            utrie_swap(ds, inBytes+header.mappingPosition, (int32_t)count,
                          outBytes+header.mappingPosition, pErrorCode);
        }

        
        if(header.endExpansionCECount!=0) {
            ds->swapArray32(ds, inBytes+header.endExpansionCE, header.endExpansionCECount*4,
                               outBytes+header.endExpansionCE, pErrorCode);
        }

        

        
        if(header.UCAConsts!=0) {
            



            ds->swapArray32(ds, inBytes+header.UCAConsts, header.contractionUCACombos-header.UCAConsts,
                               outBytes+header.UCAConsts, pErrorCode);
        }

        
        if(header.contractionUCACombosSize!=0) {
            count=header.contractionUCACombosSize*inHeader->contractionUCACombosWidth*U_SIZEOF_UCHAR;
            ds->swapArray16(ds, inBytes+header.contractionUCACombos, (int32_t)count,
                               outBytes+header.contractionUCACombos, pErrorCode);
        }
        
        
        if(header.scriptToLeadByte!=0) {
            int indexCount = ds->readUInt16(*((uint16_t*)(inBytes+header.scriptToLeadByte))); 
            int dataCount = ds->readUInt16(*((uint16_t*)(inBytes+header.scriptToLeadByte + 2))); 
            ds->swapArray16(ds, inBytes+header.scriptToLeadByte, 
                                4 + (4 * indexCount) + (2 * dataCount),
                                outBytes+header.scriptToLeadByte, pErrorCode);
        }
        
        
        if(header.leadByteToScript!=0) {
            int indexCount = ds->readUInt16(*((uint16_t*)(inBytes+header.leadByteToScript))); 
            int dataCount = ds->readUInt16(*((uint16_t*)(inBytes+header.leadByteToScript + 2))); 
            ds->swapArray16(ds, inBytes+header.leadByteToScript, 
                                4 + (2 * indexCount) + (2 * dataCount),
                                outBytes+header.leadByteToScript, pErrorCode);
        }
    }

    return header.size;
}







enum {
    IX_INDEXES_LENGTH,  
    IX_OPTIONS,
    IX_RESERVED2,
    IX_RESERVED3,

    IX_JAMO_CE32S_START,  
    IX_REORDER_CODES_OFFSET,
    IX_REORDER_TABLE_OFFSET,
    IX_TRIE_OFFSET,

    IX_RESERVED8_OFFSET,  
    IX_CES_OFFSET,
    IX_RESERVED10_OFFSET,
    IX_CE32S_OFFSET,

    IX_ROOT_ELEMENTS_OFFSET,  
    IX_CONTEXTS_OFFSET,
    IX_UNSAFE_BWD_OFFSET,
    IX_FAST_LATIN_TABLE_OFFSET,

    IX_SCRIPTS_OFFSET,  
    IX_COMPRESSIBLE_BYTES_OFFSET,
    IX_RESERVED18_OFFSET,
    IX_TOTAL_SIZE
};

int32_t
swapFormatVersion4(const UDataSwapper *ds,
                   const void *inData, int32_t length, void *outData,
                   UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) { return 0; }

    const uint8_t *inBytes=(const uint8_t *)inData;
    uint8_t *outBytes=(uint8_t *)outData;

    const int32_t *inIndexes=(const int32_t *)inBytes;
    int32_t indexes[IX_TOTAL_SIZE+1];

    
    if(0<=length && length<8) {
        udata_printError(ds, "ucol_swap(formatVersion=4): too few bytes "
                         "(%d after header) for collation data\n",
                         length);
        errorCode=U_INDEX_OUTOFBOUNDS_ERROR;
        return 0;
    }

    int32_t indexesLength=indexes[0]=udata_readInt32(ds, inIndexes[0]);
    if(0<=length && length<(indexesLength*4)) {
        udata_printError(ds, "ucol_swap(formatVersion=4): too few bytes "
                         "(%d after header) for collation data\n",
                         length);
        errorCode=U_INDEX_OUTOFBOUNDS_ERROR;
        return 0;
    }

    for(int32_t i=1; i<=IX_TOTAL_SIZE && i<indexesLength; ++i) {
        indexes[i]=udata_readInt32(ds, inIndexes[i]);
    }
    for(int32_t i=indexesLength; i<=IX_TOTAL_SIZE; ++i) {
        indexes[i]=-1;
    }
    inIndexes=NULL;  

    
    int32_t size;
    if(indexesLength>IX_TOTAL_SIZE) {
        size=indexes[IX_TOTAL_SIZE];
    } else if(indexesLength>IX_REORDER_CODES_OFFSET) {
        size=indexes[indexesLength-1];
    } else {
        size=indexesLength*4;
    }
    if(length<0) { return size; }

    if(length<size) {
        udata_printError(ds, "ucol_swap(formatVersion=4): too few bytes "
                         "(%d after header) for collation data\n",
                         length);
        errorCode=U_INDEX_OUTOFBOUNDS_ERROR;
        return 0;
    }

    
    if(inBytes!=outBytes) {
        uprv_memcpy(outBytes, inBytes, size);
    }

    
    ds->swapArray32(ds, inBytes, indexesLength * 4, outBytes, &errorCode);

    
    
    
    int32_t index;  
    int32_t offset;  
    

    index = IX_REORDER_CODES_OFFSET;
    offset = indexes[index];
    length = indexes[index + 1] - offset;
    if(length > 0) {
        ds->swapArray32(ds, inBytes + offset, length, outBytes + offset, &errorCode);
    }

    

    index = IX_TRIE_OFFSET;
    offset = indexes[index];
    length = indexes[index + 1] - offset;
    if(length > 0) {
        utrie2_swap(ds, inBytes + offset, length, outBytes + offset, &errorCode);
    }

    index = IX_RESERVED8_OFFSET;
    offset = indexes[index];
    length = indexes[index + 1] - offset;
    if(length > 0) {
        udata_printError(ds, "ucol_swap(formatVersion=4): unknown data at IX_RESERVED8_OFFSET\n", length);
        errorCode = U_UNSUPPORTED_ERROR;
        return 0;
    }

    index = IX_CES_OFFSET;
    offset = indexes[index];
    length = indexes[index + 1] - offset;
    if(length > 0) {
        ds->swapArray64(ds, inBytes + offset, length, outBytes + offset, &errorCode);
    }

    index = IX_RESERVED10_OFFSET;
    offset = indexes[index];
    length = indexes[index + 1] - offset;
    if(length > 0) {
        udata_printError(ds, "ucol_swap(formatVersion=4): unknown data at IX_RESERVED10_OFFSET\n", length);
        errorCode = U_UNSUPPORTED_ERROR;
        return 0;
    }

    index = IX_CE32S_OFFSET;
    offset = indexes[index];
    length = indexes[index + 1] - offset;
    if(length > 0) {
        ds->swapArray32(ds, inBytes + offset, length, outBytes + offset, &errorCode);
    }

    index = IX_ROOT_ELEMENTS_OFFSET;
    offset = indexes[index];
    length = indexes[index + 1] - offset;
    if(length > 0) {
        ds->swapArray32(ds, inBytes + offset, length, outBytes + offset, &errorCode);
    }

    index = IX_CONTEXTS_OFFSET;
    offset = indexes[index];
    length = indexes[index + 1] - offset;
    if(length > 0) {
        ds->swapArray16(ds, inBytes + offset, length, outBytes + offset, &errorCode);
    }

    index = IX_UNSAFE_BWD_OFFSET;
    offset = indexes[index];
    length = indexes[index + 1] - offset;
    if(length > 0) {
        ds->swapArray16(ds, inBytes + offset, length, outBytes + offset, &errorCode);
    }

    index = IX_FAST_LATIN_TABLE_OFFSET;
    offset = indexes[index];
    length = indexes[index + 1] - offset;
    if(length > 0) {
        ds->swapArray16(ds, inBytes + offset, length, outBytes + offset, &errorCode);
    }

    index = IX_SCRIPTS_OFFSET;
    offset = indexes[index];
    length = indexes[index + 1] - offset;
    if(length > 0) {
        ds->swapArray16(ds, inBytes + offset, length, outBytes + offset, &errorCode);
    }

    

    index = IX_RESERVED18_OFFSET;
    offset = indexes[index];
    length = indexes[index + 1] - offset;
    if(length > 0) {
        udata_printError(ds, "ucol_swap(formatVersion=4): unknown data at IX_RESERVED18_OFFSET\n", length);
        errorCode = U_UNSUPPORTED_ERROR;
        return 0;
    }

    return size;
}

}  


U_CAPI int32_t U_EXPORT2
ucol_swap(const UDataSwapper *ds,
          const void *inData, int32_t length, void *outData,
          UErrorCode *pErrorCode) {
    if(U_FAILURE(*pErrorCode)) { return 0; }

    
    int32_t headerSize=udata_swapDataHeader(ds, inData, length, outData, pErrorCode);
    if(U_FAILURE(*pErrorCode)) {
        
        *pErrorCode=U_ZERO_ERROR;
        return swapFormatVersion3(ds, inData, length, outData, pErrorCode);
    }

    
    const UDataInfo &info=*(const UDataInfo *)((const char *)inData+4);
    if(!(
        info.dataFormat[0]==0x55 &&   
        info.dataFormat[1]==0x43 &&
        info.dataFormat[2]==0x6f &&
        info.dataFormat[3]==0x6c &&
        (3<=info.formatVersion[0] && info.formatVersion[0]<=5)
    )) {
        udata_printError(ds, "ucol_swap(): data format %02x.%02x.%02x.%02x "
                         "(format version %02x.%02x) is not recognized as collation data\n",
                         info.dataFormat[0], info.dataFormat[1],
                         info.dataFormat[2], info.dataFormat[3],
                         info.formatVersion[0], info.formatVersion[1]);
        *pErrorCode=U_UNSUPPORTED_ERROR;
        return 0;
    }

    inData=(const char *)inData+headerSize;
    if(length>=0) { length-=headerSize; }
    outData=(char *)outData+headerSize;
    int32_t collationSize;
    if(info.formatVersion[0]>=4) {
        collationSize=swapFormatVersion4(ds, inData, length, outData, *pErrorCode);
    } else {
        collationSize=swapFormatVersion3(ds, inData, length, outData, pErrorCode);
    }
    if(U_SUCCESS(*pErrorCode)) {
        return headerSize+collationSize;
    } else {
        return 0;
    }
}


U_CAPI int32_t U_EXPORT2
ucol_swapInverseUCA(const UDataSwapper *ds,
                    const void *inData, int32_t length, void *outData,
                    UErrorCode *pErrorCode) {
    const UDataInfo *pInfo;
    int32_t headerSize;

    const uint8_t *inBytes;
    uint8_t *outBytes;

    const InverseUCATableHeader *inHeader;
    InverseUCATableHeader *outHeader;
    InverseUCATableHeader header={ 0,0,0,0,0,{0,0,0,0},{0,0,0,0,0,0,0,0} };

    
    headerSize=udata_swapDataHeader(ds, inData, length, outData, pErrorCode);
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    
    pInfo=(const UDataInfo *)((const char *)inData+4);
    if(!(
        pInfo->dataFormat[0]==0x49 &&   
        pInfo->dataFormat[1]==0x6e &&
        pInfo->dataFormat[2]==0x76 &&
        pInfo->dataFormat[3]==0x43 &&
        pInfo->formatVersion[0]==2 &&
        pInfo->formatVersion[1]>=1
    )) {
        udata_printError(ds, "ucol_swapInverseUCA(): data format %02x.%02x.%02x.%02x (format version %02x.%02x) is not an inverse UCA collation file\n",
                         pInfo->dataFormat[0], pInfo->dataFormat[1],
                         pInfo->dataFormat[2], pInfo->dataFormat[3],
                         pInfo->formatVersion[0], pInfo->formatVersion[1]);
        *pErrorCode=U_UNSUPPORTED_ERROR;
        return 0;
    }

    inBytes=(const uint8_t *)inData+headerSize;
    outBytes=(uint8_t *)outData+headerSize;

    inHeader=(const InverseUCATableHeader *)inBytes;
    outHeader=(InverseUCATableHeader *)outBytes;

    





    if(length<0) {
        header.byteSize=udata_readInt32(ds, inHeader->byteSize);
    } else if(
        ((length-headerSize)<(8*4) ||
         (uint32_t)(length-headerSize)<(header.byteSize=udata_readInt32(ds, inHeader->byteSize)))
    ) {
        udata_printError(ds, "ucol_swapInverseUCA(): too few bytes (%d after header) for inverse UCA collation data\n",
                         length);
        *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
        return 0;
    }

    if(length>=0) {
        
        if(inBytes!=outBytes) {
            uprv_memcpy(outBytes, inBytes, header.byteSize);
        }

        

        
        header.tableSize=   ds->readUInt32(inHeader->tableSize);
        header.contsSize=   ds->readUInt32(inHeader->contsSize);
        header.table=       ds->readUInt32(inHeader->table);
        header.conts=       ds->readUInt32(inHeader->conts);

        
        ds->swapArray32(ds, inHeader, 5*4, outHeader, pErrorCode);

        
        ds->swapArray32(ds, inBytes+header.table, header.tableSize*3*4,
                           outBytes+header.table, pErrorCode);

        
        ds->swapArray16(ds, inBytes+header.conts, header.contsSize*U_SIZEOF_UCHAR,
                           outBytes+header.conts, pErrorCode);
    }

    return headerSize+header.byteSize;
}

#endif 
