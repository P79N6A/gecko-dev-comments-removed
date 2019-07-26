
























#include "unicode/utypes.h"
#include "unicode/putil.h"
#include "unicode/udata.h"




#include "unicode/std_string.h"

#include "cmemory.h"
#include "cstring.h"
#include "uinvchar.h"
#include "uassert.h"
#include "uarrsort.h"
#include "ucmndata.h"
#include "udataswp.h"



#include "uresdata.h"
#include "ucnv_io.h"
#include "uprops.h"
#include "ucase.h"
#include "ubidi_props.h"
#include "ucol_swp.h"
#include "ucnv_bld.h"
#include "unormimp.h"
#include "normalizer2impl.h"
#include "sprpimpl.h"
#include "propname.h"
#include "rbbidata.h"
#include "utrie2.h"
#include "dictionarydata.h"



#if !UCONFIG_NO_NORMALIZATION
#include "uspoof_impl.h"
#endif

U_NAMESPACE_USE



#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))



static int32_t U_CALLCONV
upname_swap(const UDataSwapper *ds,
            const void *inData, int32_t length, void *outData,
            UErrorCode *pErrorCode) {
    
    int32_t headerSize=udata_swapDataHeader(ds, inData, length, outData, pErrorCode);
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    
    const UDataInfo *pInfo=
        reinterpret_cast<const UDataInfo *>(
            static_cast<const char *>(inData)+4);
    if(!(
        pInfo->dataFormat[0]==0x70 &&   
        pInfo->dataFormat[1]==0x6e &&
        pInfo->dataFormat[2]==0x61 &&
        pInfo->dataFormat[3]==0x6d &&
        pInfo->formatVersion[0]==2
    )) {
        udata_printError(ds, "upname_swap(): data format %02x.%02x.%02x.%02x (format version %02x) is not recognized as pnames.icu\n",
                         pInfo->dataFormat[0], pInfo->dataFormat[1],
                         pInfo->dataFormat[2], pInfo->dataFormat[3],
                         pInfo->formatVersion[0]);
        *pErrorCode=U_UNSUPPORTED_ERROR;
        return 0;
    }

    const uint8_t *inBytes=static_cast<const uint8_t *>(inData)+headerSize;
    uint8_t *outBytes=static_cast<uint8_t *>(outData)+headerSize;

    if(length>=0) {
        length-=headerSize;
        
        if(length<32) {
            udata_printError(ds, "upname_swap(): too few bytes (%d after header) for pnames.icu\n",
                             (int)length);
            *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
            return 0;
        }
    }

    const int32_t *inIndexes=reinterpret_cast<const int32_t *>(inBytes);
    int32_t totalSize=udata_readInt32(ds, inIndexes[PropNameData::IX_TOTAL_SIZE]);
    if(length>=0) {
        if(length<totalSize) {
            udata_printError(ds, "upname_swap(): too few bytes (%d after header, should be %d) "
                             "for pnames.icu\n",
                             (int)length, (int)totalSize);
            *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
            return 0;
        }

        int32_t numBytesIndexesAndValueMaps=
            udata_readInt32(ds, inIndexes[PropNameData::IX_BYTE_TRIES_OFFSET]);

        
        ds->swapArray32(ds, inBytes, numBytesIndexesAndValueMaps, outBytes, pErrorCode);

        
        if(inBytes!=outBytes) {
            uprv_memcpy(outBytes+numBytesIndexesAndValueMaps,
                        inBytes+numBytesIndexesAndValueMaps,
                        totalSize-numBytesIndexesAndValueMaps);
        }

        
        
        
        
        
        
        
        
        
        
        
    }

    return headerSize+totalSize;
}



static int32_t U_CALLCONV
uprops_swap(const UDataSwapper *ds,
            const void *inData, int32_t length, void *outData,
            UErrorCode *pErrorCode) {
    const UDataInfo *pInfo;
    int32_t headerSize, i;

    int32_t dataIndexes[UPROPS_INDEX_COUNT];
    const int32_t *inData32;

    
    headerSize=udata_swapDataHeader(ds, inData, length, outData, pErrorCode);
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    
    pInfo=(const UDataInfo *)((const char *)inData+4);
    if(!(
        pInfo->dataFormat[0]==0x55 &&   
        pInfo->dataFormat[1]==0x50 &&
        pInfo->dataFormat[2]==0x72 &&
        pInfo->dataFormat[3]==0x6f &&
        (3<=pInfo->formatVersion[0] && pInfo->formatVersion[0]<=7) &&
        (pInfo->formatVersion[0]>=7 ||
            (pInfo->formatVersion[2]==UTRIE_SHIFT &&
             pInfo->formatVersion[3]==UTRIE_INDEX_SHIFT))
    )) {
        udata_printError(ds, "uprops_swap(): data format %02x.%02x.%02x.%02x (format version %02x) is not a Unicode properties file\n",
                         pInfo->dataFormat[0], pInfo->dataFormat[1],
                         pInfo->dataFormat[2], pInfo->dataFormat[3],
                         pInfo->formatVersion[0]);
        *pErrorCode=U_UNSUPPORTED_ERROR;
        return 0;
    }

    
    if(length>=0 && (length-headerSize)<(int32_t)sizeof(dataIndexes)) {
        udata_printError(ds, "uprops_swap(): too few bytes (%d after header) for a Unicode properties file\n",
                         length-headerSize);
        *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
        return 0;
    }

    
    inData32=(const int32_t *)((const char *)inData+headerSize);
    for(i=0; i<UPROPS_INDEX_COUNT; ++i) {
        dataIndexes[i]=udata_readInt32(ds, inData32[i]);
    }

    



    int32_t dataTop;
    if(length>=0) {
        int32_t *outData32;

        




        for(i=UPROPS_DATA_TOP_INDEX; i>0 && (dataTop=dataIndexes[i])==0; --i) {}

        if((length-headerSize)<(4*dataTop)) {
            udata_printError(ds, "uprops_swap(): too few bytes (%d after header) for a Unicode properties file\n",
                             length-headerSize);
            *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
            return 0;
        }

        outData32=(int32_t *)((char *)outData+headerSize);

        
        if(inData32!=outData32) {
            uprv_memcpy(outData32, inData32, 4*dataTop);
        }

        
        ds->swapArray32(ds, inData32, 4*UPROPS_INDEX_COUNT, outData32, pErrorCode);

        



        utrie2_swapAnyVersion(ds,
            inData32+UPROPS_INDEX_COUNT,
            4*(dataIndexes[UPROPS_PROPS32_INDEX]-UPROPS_INDEX_COUNT),
            outData32+UPROPS_INDEX_COUNT,
            pErrorCode);

        




        ds->swapArray32(ds,
            inData32+dataIndexes[UPROPS_PROPS32_INDEX],
            4*(dataIndexes[UPROPS_EXCEPTIONS_TOP_INDEX]-dataIndexes[UPROPS_PROPS32_INDEX]),
            outData32+dataIndexes[UPROPS_PROPS32_INDEX],
            pErrorCode);

        



        ds->swapArray16(ds,
            inData32+dataIndexes[UPROPS_EXCEPTIONS_TOP_INDEX],
            4*(dataIndexes[UPROPS_ADDITIONAL_TRIE_INDEX]-dataIndexes[UPROPS_EXCEPTIONS_TOP_INDEX]),
            outData32+dataIndexes[UPROPS_EXCEPTIONS_TOP_INDEX],
            pErrorCode);

        



        utrie2_swapAnyVersion(ds,
            inData32+dataIndexes[UPROPS_ADDITIONAL_TRIE_INDEX],
            4*(dataIndexes[UPROPS_ADDITIONAL_VECTORS_INDEX]-dataIndexes[UPROPS_ADDITIONAL_TRIE_INDEX]),
            outData32+dataIndexes[UPROPS_ADDITIONAL_TRIE_INDEX],
            pErrorCode);

        



        ds->swapArray32(ds,
            inData32+dataIndexes[UPROPS_ADDITIONAL_VECTORS_INDEX],
            4*(dataIndexes[UPROPS_SCRIPT_EXTENSIONS_INDEX]-dataIndexes[UPROPS_ADDITIONAL_VECTORS_INDEX]),
            outData32+dataIndexes[UPROPS_ADDITIONAL_VECTORS_INDEX],
            pErrorCode);

        
        
        ds->swapArray16(ds,
            inData32+dataIndexes[UPROPS_SCRIPT_EXTENSIONS_INDEX],
            4*(dataIndexes[UPROPS_RESERVED_INDEX_7]-dataIndexes[UPROPS_SCRIPT_EXTENSIONS_INDEX]),
            outData32+dataIndexes[UPROPS_SCRIPT_EXTENSIONS_INDEX],
            pErrorCode);
    }

    
    return headerSize+4*dataIndexes[UPROPS_RESERVED_INDEX_7];
}



static int32_t U_CALLCONV
ucase_swap(const UDataSwapper *ds,
           const void *inData, int32_t length, void *outData,
           UErrorCode *pErrorCode) {
    const UDataInfo *pInfo;
    int32_t headerSize;

    const uint8_t *inBytes;
    uint8_t *outBytes;

    const int32_t *inIndexes;
    int32_t indexes[16];

    int32_t i, offset, count, size;

    
    headerSize=udata_swapDataHeader(ds, inData, length, outData, pErrorCode);
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    
    pInfo=(const UDataInfo *)((const char *)inData+4);
    if(!(
        pInfo->dataFormat[0]==UCASE_FMT_0 &&    
        pInfo->dataFormat[1]==UCASE_FMT_1 &&
        pInfo->dataFormat[2]==UCASE_FMT_2 &&
        pInfo->dataFormat[3]==UCASE_FMT_3 &&
        ((pInfo->formatVersion[0]==1 &&
          pInfo->formatVersion[2]==UTRIE_SHIFT &&
          pInfo->formatVersion[3]==UTRIE_INDEX_SHIFT) ||
         pInfo->formatVersion[0]==2 || pInfo->formatVersion[0]==3)
    )) {
        udata_printError(ds, "ucase_swap(): data format %02x.%02x.%02x.%02x (format version %02x) is not recognized as case mapping data\n",
                         pInfo->dataFormat[0], pInfo->dataFormat[1],
                         pInfo->dataFormat[2], pInfo->dataFormat[3],
                         pInfo->formatVersion[0]);
        *pErrorCode=U_UNSUPPORTED_ERROR;
        return 0;
    }

    inBytes=(const uint8_t *)inData+headerSize;
    outBytes=(uint8_t *)outData+headerSize;

    inIndexes=(const int32_t *)inBytes;

    if(length>=0) {
        length-=headerSize;
        if(length<16*4) {
            udata_printError(ds, "ucase_swap(): too few bytes (%d after header) for case mapping data\n",
                             length);
            *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
            return 0;
        }
    }

    
    for(i=0; i<16; ++i) {
        indexes[i]=udata_readInt32(ds, inIndexes[i]);
    }

    
    size=indexes[UCASE_IX_LENGTH];

    if(length>=0) {
        if(length<size) {
            udata_printError(ds, "ucase_swap(): too few bytes (%d after header) for all of case mapping data\n",
                             length);
            *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
            return 0;
        }

        
        if(inBytes!=outBytes) {
            uprv_memcpy(outBytes, inBytes, size);
        }

        offset=0;

        
        count=indexes[UCASE_IX_INDEX_TOP]*4;
        ds->swapArray32(ds, inBytes, count, outBytes, pErrorCode);
        offset+=count;

        
        count=indexes[UCASE_IX_TRIE_SIZE];
        utrie2_swapAnyVersion(ds, inBytes+offset, count, outBytes+offset, pErrorCode);
        offset+=count;

        
        count=(indexes[UCASE_IX_EXC_LENGTH]+indexes[UCASE_IX_UNFOLD_LENGTH])*2;
        ds->swapArray16(ds, inBytes+offset, count, outBytes+offset, pErrorCode);
        offset+=count;

        U_ASSERT(offset==size);
    }

    return headerSize+size;
}



static int32_t U_CALLCONV
ubidi_swap(const UDataSwapper *ds,
           const void *inData, int32_t length, void *outData,
           UErrorCode *pErrorCode) {
    const UDataInfo *pInfo;
    int32_t headerSize;

    const uint8_t *inBytes;
    uint8_t *outBytes;

    const int32_t *inIndexes;
    int32_t indexes[16];

    int32_t i, offset, count, size;

    
    headerSize=udata_swapDataHeader(ds, inData, length, outData, pErrorCode);
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    
    pInfo=(const UDataInfo *)((const char *)inData+4);
    if(!(
        pInfo->dataFormat[0]==UBIDI_FMT_0 &&    
        pInfo->dataFormat[1]==UBIDI_FMT_1 &&
        pInfo->dataFormat[2]==UBIDI_FMT_2 &&
        pInfo->dataFormat[3]==UBIDI_FMT_3 &&
        ((pInfo->formatVersion[0]==1 &&
          pInfo->formatVersion[2]==UTRIE_SHIFT &&
          pInfo->formatVersion[3]==UTRIE_INDEX_SHIFT) ||
         pInfo->formatVersion[0]==2)
    )) {
        udata_printError(ds, "ubidi_swap(): data format %02x.%02x.%02x.%02x (format version %02x) is not recognized as bidi/shaping data\n",
                         pInfo->dataFormat[0], pInfo->dataFormat[1],
                         pInfo->dataFormat[2], pInfo->dataFormat[3],
                         pInfo->formatVersion[0]);
        *pErrorCode=U_UNSUPPORTED_ERROR;
        return 0;
    }

    inBytes=(const uint8_t *)inData+headerSize;
    outBytes=(uint8_t *)outData+headerSize;

    inIndexes=(const int32_t *)inBytes;

    if(length>=0) {
        length-=headerSize;
        if(length<16*4) {
            udata_printError(ds, "ubidi_swap(): too few bytes (%d after header) for bidi/shaping data\n",
                             length);
            *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
            return 0;
        }
    }

    
    for(i=0; i<16; ++i) {
        indexes[i]=udata_readInt32(ds, inIndexes[i]);
    }

    
    size=indexes[UBIDI_IX_LENGTH];

    if(length>=0) {
        if(length<size) {
            udata_printError(ds, "ubidi_swap(): too few bytes (%d after header) for all of bidi/shaping data\n",
                             length);
            *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
            return 0;
        }

        
        if(inBytes!=outBytes) {
            uprv_memcpy(outBytes, inBytes, size);
        }

        offset=0;

        
        count=indexes[UBIDI_IX_INDEX_TOP]*4;
        ds->swapArray32(ds, inBytes, count, outBytes, pErrorCode);
        offset+=count;

        
        count=indexes[UBIDI_IX_TRIE_SIZE];
        utrie2_swapAnyVersion(ds, inBytes+offset, count, outBytes+offset, pErrorCode);
        offset+=count;

        
        count=indexes[UBIDI_IX_MIRROR_LENGTH]*4;
        ds->swapArray32(ds, inBytes+offset, count, outBytes+offset, pErrorCode);
        offset+=count;

        
        count=indexes[UBIDI_IX_JG_LIMIT]-indexes[UBIDI_IX_JG_START];
        offset+=count;

        U_ASSERT(offset==size);
    }

    return headerSize+size;
}



#if !UCONFIG_NO_NORMALIZATION

static int32_t U_CALLCONV
unorm_swap(const UDataSwapper *ds,
           const void *inData, int32_t length, void *outData,
           UErrorCode *pErrorCode) {
    const UDataInfo *pInfo;
    int32_t headerSize;

    const uint8_t *inBytes;
    uint8_t *outBytes;

    const int32_t *inIndexes;
    int32_t indexes[32];

    int32_t i, offset, count, size;

    
    headerSize=udata_swapDataHeader(ds, inData, length, outData, pErrorCode);
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    
    pInfo=(const UDataInfo *)((const char *)inData+4);
    if(!(
        pInfo->dataFormat[0]==0x4e &&   
        pInfo->dataFormat[1]==0x6f &&
        pInfo->dataFormat[2]==0x72 &&
        pInfo->dataFormat[3]==0x6d &&
        pInfo->formatVersion[0]==2
    )) {
        udata_printError(ds, "unorm_swap(): data format %02x.%02x.%02x.%02x (format version %02x) is not recognized as unorm.icu\n",
                         pInfo->dataFormat[0], pInfo->dataFormat[1],
                         pInfo->dataFormat[2], pInfo->dataFormat[3],
                         pInfo->formatVersion[0]);
        *pErrorCode=U_UNSUPPORTED_ERROR;
        return 0;
    }

    inBytes=(const uint8_t *)inData+headerSize;
    outBytes=(uint8_t *)outData+headerSize;

    inIndexes=(const int32_t *)inBytes;

    if(length>=0) {
        length-=headerSize;
        if(length<32*4) {
            udata_printError(ds, "unorm_swap(): too few bytes (%d after header) for unorm.icu\n",
                             length);
            *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
            return 0;
        }
    }

    
    for(i=0; i<32; ++i) {
        indexes[i]=udata_readInt32(ds, inIndexes[i]);
    }

    
    size=
        32*4+ 
        indexes[_NORM_INDEX_TRIE_SIZE]+
        indexes[_NORM_INDEX_UCHAR_COUNT]*2+
        indexes[_NORM_INDEX_COMBINE_DATA_COUNT]*2+
        indexes[_NORM_INDEX_FCD_TRIE_SIZE]+
        indexes[_NORM_INDEX_AUX_TRIE_SIZE]+
        indexes[_NORM_INDEX_CANON_SET_COUNT]*2;

    if(length>=0) {
        if(length<size) {
            udata_printError(ds, "unorm_swap(): too few bytes (%d after header) for all of unorm.icu\n",
                             length);
            *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
            return 0;
        }

        
        if(inBytes!=outBytes) {
            uprv_memcpy(outBytes, inBytes, size);
        }

        offset=0;

        
        count=32*4;
        ds->swapArray32(ds, inBytes, count, outBytes, pErrorCode);
        offset+=count;

        
        count=indexes[_NORM_INDEX_TRIE_SIZE];
        utrie_swap(ds, inBytes+offset, count, outBytes+offset, pErrorCode);
        offset+=count;

        
        count=(indexes[_NORM_INDEX_UCHAR_COUNT]+indexes[_NORM_INDEX_COMBINE_DATA_COUNT])*2;
        ds->swapArray16(ds, inBytes+offset, count, outBytes+offset, pErrorCode);
        offset+=count;

        
        count=indexes[_NORM_INDEX_FCD_TRIE_SIZE];
        if(count!=0) {
            utrie_swap(ds, inBytes+offset, count, outBytes+offset, pErrorCode);
            offset+=count;
        }

        
        count=indexes[_NORM_INDEX_AUX_TRIE_SIZE];
        if(count!=0) {
            utrie_swap(ds, inBytes+offset, count, outBytes+offset, pErrorCode);
            offset+=count;
        }

        
        count=indexes[_NORM_INDEX_CANON_SET_COUNT]*2;
        ds->swapArray16(ds, inBytes+offset, count, outBytes+offset, pErrorCode);
        offset+=count;
    }

    return headerSize+size;
}

#endif


static int32_t U_CALLCONV
test_swap(const UDataSwapper *ds,
           const void *inData, int32_t length, void *outData,
           UErrorCode *pErrorCode) {
    const UDataInfo *pInfo;
    int32_t headerSize;

    const uint8_t *inBytes;
    uint8_t *outBytes;

    int32_t offset;

    
    headerSize=udata_swapDataHeader(ds, inData, length, outData, pErrorCode);
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        udata_printError(ds, "test_swap(): data header swap failed %s\n", pErrorCode != NULL ? u_errorName(*pErrorCode) : "pErrorCode is NULL");
        return 0;
    }

    
    pInfo=(const UDataInfo *)((const char *)inData+4);
    if(!(
        pInfo->dataFormat[0]==0x54 &&   
        pInfo->dataFormat[1]==0x65 &&
        pInfo->dataFormat[2]==0x73 &&
        pInfo->dataFormat[3]==0x74 &&
        pInfo->formatVersion[0]==1
    )) {
        udata_printError(ds, "test_swap(): data format %02x.%02x.%02x.%02x (format version %02x) is not recognized as testdata\n",
                         pInfo->dataFormat[0], pInfo->dataFormat[1],
                         pInfo->dataFormat[2], pInfo->dataFormat[3],
                         pInfo->formatVersion[0]);
        *pErrorCode=U_UNSUPPORTED_ERROR;
        return 0;
    }

    inBytes=(const uint8_t *)inData+headerSize;
    outBytes=(uint8_t *)outData+headerSize;

    int32_t size16 = 2; 
    int32_t sizeStr = 5; 
    int32_t size = size16 + sizeStr;

    if(length>=0) {
        if(length<size) {
            udata_printError(ds, "test_swap(): too few bytes (%d after header, wanted %d) for all of testdata\n",
                             length, size);
            *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
            return 0;
        }

	offset =0;
	
        ds->swapArray16(ds, inBytes+offset, size16, outBytes+offset, pErrorCode);
	offset+=size16;
	ds->swapInvChars(ds, inBytes+offset, sizeStr, outBytes+offset, pErrorCode);
    }

    return headerSize+size;
}



static const struct {
    uint8_t dataFormat[4];
    UDataSwapFn *swapFn;
} swapFns[]={
    { { 0x52, 0x65, 0x73, 0x42 }, ures_swap },          
#if !UCONFIG_NO_LEGACY_CONVERSION
    { { 0x63, 0x6e, 0x76, 0x74 }, ucnv_swap },          
#endif
#if !UCONFIG_NO_CONVERSION
    { { 0x43, 0x76, 0x41, 0x6c }, ucnv_swapAliases },   
#endif
#if !UCONFIG_NO_IDNA
    { { 0x53, 0x50, 0x52, 0x50 }, usprep_swap },        
#endif
    
    { { 0x55, 0x50, 0x72, 0x6f }, uprops_swap },        

    { { UCASE_FMT_0, UCASE_FMT_1, UCASE_FMT_2, UCASE_FMT_3 },
                                  ucase_swap },         

    { { UBIDI_FMT_0, UBIDI_FMT_1, UBIDI_FMT_2, UBIDI_FMT_3 },
                                  ubidi_swap },         

#if !UCONFIG_NO_NORMALIZATION
    { { 0x4e, 0x6f, 0x72, 0x6d }, unorm_swap },         
    { { 0x4e, 0x72, 0x6d, 0x32 }, unorm2_swap },        
#endif
#if !UCONFIG_NO_COLLATION
    { { 0x55, 0x43, 0x6f, 0x6c }, ucol_swap },          
    { { 0x49, 0x6e, 0x76, 0x43 }, ucol_swapInverseUCA },
#endif
#if !UCONFIG_NO_BREAK_ITERATION
    { { 0x42, 0x72, 0x6b, 0x20 }, ubrk_swap },          
    { { 0x44, 0x69, 0x63, 0x74 }, udict_swap },         
#endif
    { { 0x70, 0x6e, 0x61, 0x6d }, upname_swap },        
    { { 0x75, 0x6e, 0x61, 0x6d }, uchar_swapNames },    
#if !UCONFIG_NO_NORMALIZATION
    { { 0x43, 0x66, 0x75, 0x20 }, uspoof_swap },         
#endif
    { { 0x54, 0x65, 0x73, 0x74 }, test_swap }            
};

U_CAPI int32_t U_EXPORT2
udata_swap(const UDataSwapper *ds,
           const void *inData, int32_t length, void *outData,
           UErrorCode *pErrorCode) {
    char dataFormatChars[4];
    const UDataInfo *pInfo;
    int32_t i, swappedLength;

    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    






    udata_swapDataHeader(ds, inData, -1, NULL, pErrorCode);

    



    if(U_FAILURE(*pErrorCode)) {
        return 0; 
    }

    pInfo=(const UDataInfo *)((const char *)inData+4);

    {
        
        UChar u[4]={
             pInfo->dataFormat[0], pInfo->dataFormat[1],
             pInfo->dataFormat[2], pInfo->dataFormat[3]
        };

        if(uprv_isInvariantUString(u, 4)) {
            u_UCharsToChars(u, dataFormatChars, 4);
        } else {
            dataFormatChars[0]=dataFormatChars[1]=dataFormatChars[2]=dataFormatChars[3]='?';
        }
    }

    
    for(i=0; i<LENGTHOF(swapFns); ++i) {
        if(0==memcmp(swapFns[i].dataFormat, pInfo->dataFormat, 4)) {
            swappedLength=swapFns[i].swapFn(ds, inData, length, outData, pErrorCode);

            if(U_FAILURE(*pErrorCode)) {
                udata_printError(ds, "udata_swap(): failure swapping data format %02x.%02x.%02x.%02x (\"%c%c%c%c\") - %s\n",
                                 pInfo->dataFormat[0], pInfo->dataFormat[1],
                                 pInfo->dataFormat[2], pInfo->dataFormat[3],
                                 dataFormatChars[0], dataFormatChars[1],
                                 dataFormatChars[2], dataFormatChars[3],
                                 u_errorName(*pErrorCode));
            } else if(swappedLength<(length-15)) {
                
                udata_printError(ds, "udata_swap() warning: swapped only %d out of %d bytes - data format %02x.%02x.%02x.%02x (\"%c%c%c%c\")\n",
                                 swappedLength, length,
                                 pInfo->dataFormat[0], pInfo->dataFormat[1],
                                 pInfo->dataFormat[2], pInfo->dataFormat[3],
                                 dataFormatChars[0], dataFormatChars[1],
                                 dataFormatChars[2], dataFormatChars[3],
                                 u_errorName(*pErrorCode));
            }

            return swappedLength;
        }
    }

    
    udata_printError(ds, "udata_swap(): unknown data format %02x.%02x.%02x.%02x (\"%c%c%c%c\")\n",
                     pInfo->dataFormat[0], pInfo->dataFormat[1],
                     pInfo->dataFormat[2], pInfo->dataFormat[3],
                     dataFormatChars[0], dataFormatChars[1],
                     dataFormatChars[2], dataFormatChars[3]);

    *pErrorCode=U_UNSUPPORTED_ERROR;
    return 0;
}
