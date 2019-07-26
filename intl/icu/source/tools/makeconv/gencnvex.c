















#include <stdio.h>
#include "unicode/utypes.h"
#include "unicode/ustring.h"
#include "cstring.h"
#include "cmemory.h"
#include "ucnv_cnv.h"
#include "ucnvmbcs.h"
#include "toolutil.h"
#include "unewdata.h"
#include "ucm.h"
#include "makeconv.h"
#include "genmbcs.h"

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))


static void
CnvExtClose(NewConverter *cnvData);

static UBool
CnvExtIsValid(NewConverter *cnvData,
              const uint8_t *bytes, int32_t length);

static UBool
CnvExtAddTable(NewConverter *cnvData, UCMTable *table, UConverterStaticData *staticData);

static uint32_t
CnvExtWrite(NewConverter *cnvData, const UConverterStaticData *staticData,
            UNewDataMemory *pData, int32_t tableType);

typedef struct CnvExtData {
    NewConverter newConverter;

    UCMFile *ucm;

    
    UToolMemory *toUTable, *toUUChars;

    
    UToolMemory *fromUTableUChars, *fromUTableValues, *fromUBytes;

    uint16_t stage1[MBCS_STAGE_1_SIZE];
    uint16_t stage2[MBCS_STAGE_2_SIZE];
    uint16_t stage3[0x10000<<UCNV_EXT_STAGE_2_LEFT_SHIFT]; 
    uint32_t stage3b[0x10000];

    int32_t stage1Top, stage2Top, stage3Top, stage3bTop;

    
    uint16_t stage3Sub1Block;

    
    int32_t
        maxInBytes, maxOutBytes, maxBytesPerUChar,
        maxInUChars, maxOutUChars, maxUCharsPerByte;
} CnvExtData;

NewConverter *
CnvExtOpen(UCMFile *ucm) {
    CnvExtData *extData;
    
    extData=(CnvExtData *)uprv_malloc(sizeof(CnvExtData));
    if(extData==NULL) {
        printf("out of memory\n");
        exit(U_MEMORY_ALLOCATION_ERROR);
    }
    uprv_memset(extData, 0, sizeof(CnvExtData));

    extData->ucm=ucm; 

    extData->newConverter.close=CnvExtClose;
    extData->newConverter.isValid=CnvExtIsValid;
    extData->newConverter.addTable=CnvExtAddTable;
    extData->newConverter.write=CnvExtWrite;
    return &extData->newConverter;
}

static void
CnvExtClose(NewConverter *cnvData) {
    CnvExtData *extData=(CnvExtData *)cnvData;
    if(extData!=NULL) {
        utm_close(extData->toUTable);
        utm_close(extData->toUUChars);
        utm_close(extData->fromUTableUChars);
        utm_close(extData->fromUTableValues);
        utm_close(extData->fromUBytes);
        uprv_free(extData);
    }
}


static UBool
CnvExtIsValid(NewConverter *cnvData,
        const uint8_t *bytes, int32_t length) {
    return FALSE;
}

static uint32_t
CnvExtWrite(NewConverter *cnvData, const UConverterStaticData *staticData,
            UNewDataMemory *pData, int32_t tableType) {
    CnvExtData *extData=(CnvExtData *)cnvData;
    int32_t length, top, headerSize;

    int32_t indexes[UCNV_EXT_INDEXES_MIN_LENGTH]={ 0 };

    if(tableType&TABLE_BASE) {
        headerSize=0;
    } else {
        _MBCSHeader header={ { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 };

        
        length=(int32_t)uprv_strlen(extData->ucm->baseName)+1;
        while(length&3) {
            
            extData->ucm->baseName[length++]=0;
        }

        headerSize=MBCS_HEADER_V4_LENGTH*4+length;

        
        header.version[0]=4;
        header.version[1]=2;
        header.flags=(uint32_t)((headerSize<<8)|MBCS_OUTPUT_EXT_ONLY);

        
        udata_writeBlock(pData, &header, MBCS_HEADER_V4_LENGTH*4);
        udata_writeBlock(pData, extData->ucm->baseName, length);
    }

    
    top=0;

    indexes[UCNV_EXT_INDEXES_LENGTH]=length=UCNV_EXT_INDEXES_MIN_LENGTH;
    top+=length*4;

    indexes[UCNV_EXT_TO_U_INDEX]=top;
    indexes[UCNV_EXT_TO_U_LENGTH]=length=utm_countItems(extData->toUTable);
    top+=length*4;

    indexes[UCNV_EXT_TO_U_UCHARS_INDEX]=top;
    indexes[UCNV_EXT_TO_U_UCHARS_LENGTH]=length=utm_countItems(extData->toUUChars);
    top+=length*2;

    indexes[UCNV_EXT_FROM_U_UCHARS_INDEX]=top;
    length=utm_countItems(extData->fromUTableUChars);
    top+=length*2;

    if(top&3) {
        
        *((UChar *)utm_alloc(extData->fromUTableUChars))=0;
        *((uint32_t *)utm_alloc(extData->fromUTableValues))=0;
        ++length;
        top+=2;
    }
    indexes[UCNV_EXT_FROM_U_LENGTH]=length;

    indexes[UCNV_EXT_FROM_U_VALUES_INDEX]=top;
    top+=length*4;

    indexes[UCNV_EXT_FROM_U_BYTES_INDEX]=top;
    length=utm_countItems(extData->fromUBytes);
    top+=length;

    if(top&1) {
        
        *((uint8_t *)utm_alloc(extData->fromUBytes))=0;
        ++length;
        ++top;
    }
    indexes[UCNV_EXT_FROM_U_BYTES_LENGTH]=length;

    indexes[UCNV_EXT_FROM_U_STAGE_12_INDEX]=top;
    indexes[UCNV_EXT_FROM_U_STAGE_1_LENGTH]=length=extData->stage1Top;
    indexes[UCNV_EXT_FROM_U_STAGE_12_LENGTH]=length+=extData->stage2Top;
    top+=length*2;

    indexes[UCNV_EXT_FROM_U_STAGE_3_INDEX]=top;
    length=extData->stage3Top;
    top+=length*2;

    if(top&3) {
        
        extData->stage3[extData->stage3Top++]=0;
        ++length;
        top+=2;
    }
    indexes[UCNV_EXT_FROM_U_STAGE_3_LENGTH]=length;

    indexes[UCNV_EXT_FROM_U_STAGE_3B_INDEX]=top;
    indexes[UCNV_EXT_FROM_U_STAGE_3B_LENGTH]=length=extData->stage3bTop;
    top+=length*4;

    indexes[UCNV_EXT_SIZE]=top;

    
    indexes[UCNV_EXT_COUNT_BYTES]=
        (extData->maxInBytes<<16)|
        (extData->maxOutBytes<<8)|
        extData->maxBytesPerUChar;
    indexes[UCNV_EXT_COUNT_UCHARS]=
        (extData->maxInUChars<<16)|
        (extData->maxOutUChars<<8)|
        extData->maxUCharsPerByte;

    indexes[UCNV_EXT_FLAGS]=extData->ucm->ext->unicodeMask;

    
    udata_writeBlock(pData, indexes, sizeof(indexes));
    udata_writeBlock(pData, utm_getStart(extData->toUTable), indexes[UCNV_EXT_TO_U_LENGTH]*4);
    udata_writeBlock(pData, utm_getStart(extData->toUUChars), indexes[UCNV_EXT_TO_U_UCHARS_LENGTH]*2);

    udata_writeBlock(pData, utm_getStart(extData->fromUTableUChars), indexes[UCNV_EXT_FROM_U_LENGTH]*2);
    udata_writeBlock(pData, utm_getStart(extData->fromUTableValues), indexes[UCNV_EXT_FROM_U_LENGTH]*4);
    udata_writeBlock(pData, utm_getStart(extData->fromUBytes), indexes[UCNV_EXT_FROM_U_BYTES_LENGTH]);

    udata_writeBlock(pData, extData->stage1, extData->stage1Top*2);
    udata_writeBlock(pData, extData->stage2, extData->stage2Top*2);
    udata_writeBlock(pData, extData->stage3, extData->stage3Top*2);
    udata_writeBlock(pData, extData->stage3b, extData->stage3bTop*4);

#if 0
    {
        int32_t i, j;

        length=extData->stage1Top;
        printf("\nstage1[%x]:\n", length);

        for(i=0; i<length; ++i) {
            if(extData->stage1[i]!=length) {
                printf("stage1[%04x]=%04x\n", i, extData->stage1[i]);
            }
        }

        j=length;
        length=extData->stage2Top;
        printf("\nstage2[%x]:\n", length);

        for(i=0; i<length; ++j, ++i) {
            if(extData->stage2[i]!=0) {
                printf("stage12[%04x]=%04x\n", j, extData->stage2[i]);
            }
        }

        length=extData->stage3Top;
        printf("\nstage3[%x]:\n", length);

        for(i=0; i<length; ++i) {
            if(extData->stage3[i]!=0) {
                printf("stage3[%04x]=%04x\n", i, extData->stage3[i]);
            }
        }

        length=extData->stage3bTop;
        printf("\nstage3b[%x]:\n", length);

        for(i=0; i<length; ++i) {
            if(extData->stage3b[i]!=0) {
                printf("stage3b[%04x]=%08x\n", i, extData->stage3b[i]);
            }
        }
    }
#endif

    if(VERBOSE) {
        printf("size of extension data: %ld\n", (long)top);
    }

    
    return (uint32_t)(headerSize+top);
}











static int32_t
reduceToUMappings(UCMTable *table) {
    UCMapping *mappings;
    int32_t *map;
    int32_t i, j, count;
    int8_t flag;

    mappings=table->mappings;
    map=table->reverseMap;
    count=table->mappingsLength;

    
    for(i=j=0; i<count; ++i) {
        flag=mappings[map[i]].f;
        if(flag!=0 && flag!=3) {
            break;
        }
    }

    
    for(j=i; i<count; ++i) {
        flag=mappings[map[i]].f;
        if(flag==0 || flag==3) {
            map[j++]=map[i];
        }
    }

    return j;
}

static uint32_t
getToUnicodeValue(CnvExtData *extData, UCMTable *table, UCMapping *m) {
    UChar32 *u32;
    UChar *u;
    uint32_t value;
    int32_t u16Length, ratio;
    UErrorCode errorCode;

    
    if(m->uLen==1) {
        u16Length=U16_LENGTH(m->u);
        value=(uint32_t)(UCNV_EXT_TO_U_MIN_CODE_POINT+m->u);
    } else {
        

        
        u32=UCM_GET_CODE_POINTS(table, m);
        errorCode=U_ZERO_ERROR;
        u_strFromUTF32(NULL, 0, &u16Length, u32, m->uLen, &errorCode);
        if(U_FAILURE(errorCode) && errorCode!=U_BUFFER_OVERFLOW_ERROR) {
            exit(errorCode);
        }

        
        value=
            (((uint32_t)u16Length+UCNV_EXT_TO_U_LENGTH_OFFSET)<<UCNV_EXT_TO_U_LENGTH_SHIFT)|
            ((uint32_t)utm_countItems(extData->toUUChars));
        u=utm_allocN(extData->toUUChars, u16Length);

        
        errorCode=U_ZERO_ERROR;
        u_strFromUTF32(u, u16Length, NULL, u32, m->uLen, &errorCode);
        if(U_FAILURE(errorCode) && errorCode!=U_BUFFER_OVERFLOW_ERROR) {
            exit(errorCode);
        }
    }
    if(m->f==0) {
        value|=UCNV_EXT_TO_U_ROUNDTRIP_FLAG;
    }

    
    if(m->bLen>extData->maxInBytes) {
        extData->maxInBytes=m->bLen;
    }
    if(u16Length>extData->maxOutUChars) {
        extData->maxOutUChars=u16Length;
    }

    ratio=(u16Length+(m->bLen-1))/m->bLen;
    if(ratio>extData->maxUCharsPerByte) {
        extData->maxUCharsPerByte=ratio;
    }

    return value;
}




























static UBool
generateToUTable(CnvExtData *extData, UCMTable *table,
                 int32_t start, int32_t limit, int32_t unitIndex,
                 uint32_t defaultValue) {
    UCMapping *mappings, *m;
    int32_t *map;
    int32_t i, j, uniqueCount, count, subStart, subLimit;

    uint8_t *bytes;
    int32_t low, high, prev;

    uint32_t *section;

    mappings=table->mappings;
    map=table->reverseMap;

    
    m=mappings+map[start];
    bytes=UCM_GET_BYTES(table, m);
    low=bytes[unitIndex];
    uniqueCount=1;

    prev=high=low;
    for(i=start+1; i<limit; ++i) {
        m=mappings+map[i];
        bytes=UCM_GET_BYTES(table, m);
        high=bytes[unitIndex];

        if(high!=prev) {
            prev=high;
            ++uniqueCount;
        }
    }

    
    count=(high-low)+1;
    if(count<0x100 && (unitIndex==0 || uniqueCount>=(3*count)/4)) {
        







    } else {
        count=uniqueCount;
    }

    if(count>=0x100) {
        fprintf(stderr, "error: toUnicode extension table section overflow: %ld section entries\n", (long)count);
        return FALSE;
    }

    
    section=(uint32_t *)utm_allocN(extData->toUTable, 1+count);

    
    *section++=((uint32_t)count<<UCNV_EXT_TO_U_BYTE_SHIFT)|defaultValue;

    
    prev=low-1; 
    j=0; 
    for(i=start; i<limit; ++i) {
        m=mappings+map[i];
        bytes=UCM_GET_BYTES(table, m);
        high=bytes[unitIndex];

        if(high!=prev) {
            
            if(count>uniqueCount) {
                
                while(++prev<high) {
                    section[j++]=((uint32_t)prev<<UCNV_EXT_TO_U_BYTE_SHIFT)|(uint32_t)i;
                }
            } else {
                prev=high;
            }

            
            section[j++]=((uint32_t)high<<UCNV_EXT_TO_U_BYTE_SHIFT)|(uint32_t)i;
        }
    }
    

    
    subLimit=UCNV_EXT_TO_U_GET_VALUE(section[0]);
    for(j=0; j<count; ++j) {
        subStart=subLimit;
        subLimit= (j+1)<count ? UCNV_EXT_TO_U_GET_VALUE(section[j+1]) : limit;

        
        section[j]&=~UCNV_EXT_TO_U_VALUE_MASK;

        if(subStart==subLimit) {
            
            continue;
        }

        
        defaultValue=0;
        m=mappings+map[subStart];
        if(m->bLen==unitIndex+1) {
            
            ++subStart;

            if(subStart<subLimit && mappings[map[subStart]].bLen==unitIndex+1) {
                
                fprintf(stderr, "error: multiple mappings from same bytes\n");
                ucm_printMapping(table, m, stderr);
                ucm_printMapping(table, mappings+map[subStart], stderr);
                return FALSE;
            }

            defaultValue=getToUnicodeValue(extData, table, m);
        }

        if(subStart==subLimit) {
            
            section[j]|=defaultValue;
        } else {
            
            section[j]|=(uint32_t)utm_countItems(extData->toUTable);

            
            if(!generateToUTable(extData, table, subStart, subLimit, unitIndex+1, defaultValue)) {
                return FALSE;
            }
        }
    }
    return TRUE;
}






static UBool
makeToUTable(CnvExtData *extData, UCMTable *table) {
    int32_t toUCount;

    toUCount=reduceToUMappings(table);

    extData->toUTable=utm_open("cnv extension toUTable", 0x10000, UCNV_EXT_TO_U_MIN_CODE_POINT, 4);
    extData->toUUChars=utm_open("cnv extension toUUChars", 0x10000, UCNV_EXT_TO_U_INDEX_MASK+1, 2);

    return generateToUTable(extData, table, 0, toUCount, 0, 0);
}


























static int32_t
prepareFromUMappings(UCMTable *table) {
    UCMapping *mappings, *m;
    int32_t *map;
    int32_t i, j, count;
    int8_t flag;

    mappings=table->mappings;
    map=table->reverseMap;
    count=table->mappingsLength;

    



    m=mappings;

    for(i=j=0; i<count; ++m, ++i) {
        flag=m->f;
        if(flag>=0) {
            flag&=MBCS_FROM_U_EXT_MASK;
            m->f=flag;
        }
        if(flag==0 || flag==1 || (flag==2 && m->bLen==1)) {
            map[j++]=i;

            if(m->uLen>1) {
                
                UChar32 *u32;
                UChar *u;
                UChar32 c;
                int32_t q, r;

                u32=UCM_GET_CODE_POINTS(table, m);
                u=(UChar *)u32; 
                for(r=2, q=1; q<m->uLen; ++q) {
                    c=u32[q];
                    U16_APPEND_UNSAFE(u, r, c);
                }

                
                m->uLen=(int8_t)r;
            }
        }
    }

    return j;
}

static uint32_t
getFromUBytesValue(CnvExtData *extData, UCMTable *table, UCMapping *m) {
    uint8_t *bytes, *resultBytes;
    uint32_t value;
    int32_t u16Length, ratio;

    if(m->f==2) {
        







        return UCNV_EXT_FROM_U_SUBCHAR1;
    }

    bytes=UCM_GET_BYTES(table, m);
    value=0;
    switch(m->bLen) {
        
    case 3:
        value=((uint32_t)*bytes++)<<16;
    case 2:
        value|=((uint32_t)*bytes++)<<8;
    case 1:
        value|=*bytes;
        break;
    default:
        
        
        value=(uint32_t)utm_countItems(extData->fromUBytes);
        resultBytes=utm_allocN(extData->fromUBytes, m->bLen);
        uprv_memcpy(resultBytes, bytes, m->bLen);
        break;
    }
    value|=(uint32_t)m->bLen<<UCNV_EXT_FROM_U_LENGTH_SHIFT;
    if(m->f==0) {
        value|=UCNV_EXT_FROM_U_ROUNDTRIP_FLAG;
    }

    
    if(m->uLen==1) {
        u16Length=U16_LENGTH(m->u);
    } else {
        u16Length=U16_LENGTH(UCM_GET_CODE_POINTS(table, m)[0])+(m->uLen-2);
    }

    
    if(u16Length>extData->maxInUChars) {
        extData->maxInUChars=u16Length;
    }
    if(m->bLen>extData->maxOutBytes) {
        extData->maxOutBytes=m->bLen;
    }

    ratio=(m->bLen+(u16Length-1))/u16Length;
    if(ratio>extData->maxBytesPerUChar) {
        extData->maxBytesPerUChar=ratio;
    }

    return value;
}









static UBool
generateFromUTable(CnvExtData *extData, UCMTable *table,
                   int32_t start, int32_t limit, int32_t unitIndex,
                   uint32_t defaultValue) {
    UCMapping *mappings, *m;
    int32_t *map;
    int32_t i, j, uniqueCount, count, subStart, subLimit;

    UChar *uchars;
    UChar32 low, high, prev;

    UChar *sectionUChars;
    uint32_t *sectionValues;

    mappings=table->mappings;
    map=table->reverseMap;

    
    m=mappings+map[start];
    uchars=(UChar *)UCM_GET_CODE_POINTS(table, m);
    low=uchars[unitIndex];
    uniqueCount=1;

    prev=high=low;
    for(i=start+1; i<limit; ++i) {
        m=mappings+map[i];
        uchars=(UChar *)UCM_GET_CODE_POINTS(table, m);
        high=uchars[unitIndex];

        if(high!=prev) {
            prev=high;
            ++uniqueCount;
        }
    }

    
    
    count=uniqueCount;

    
    sectionUChars=(UChar *)utm_allocN(extData->fromUTableUChars, 1+count);
    sectionValues=(uint32_t *)utm_allocN(extData->fromUTableValues, 1+count);

    
    *sectionUChars++=(UChar)count;
    *sectionValues++=defaultValue;

    
    prev=low-1; 
    j=0; 
    for(i=start; i<limit; ++i) {
        m=mappings+map[i];
        uchars=(UChar *)UCM_GET_CODE_POINTS(table, m);
        high=uchars[unitIndex];

        if(high!=prev) {
            
            prev=high;

            
            sectionUChars[j]=(UChar)high;
            sectionValues[j]=(uint32_t)i;
            ++j;
        }
    }
    

    
    subLimit=(int32_t)(sectionValues[0]);
    for(j=0; j<count; ++j) {
        subStart=subLimit;
        subLimit= (j+1)<count ? (int32_t)(sectionValues[j+1]) : limit;

        
        defaultValue=0;
        m=mappings+map[subStart];
        if(m->uLen==unitIndex+1) {
            
            ++subStart;

            if(subStart<subLimit && mappings[map[subStart]].uLen==unitIndex+1) {
                
                fprintf(stderr, "error: multiple mappings from same Unicode code points\n");
                ucm_printMapping(table, m, stderr);
                ucm_printMapping(table, mappings+map[subStart], stderr);
                return FALSE;
            }

            defaultValue=getFromUBytesValue(extData, table, m);
        }

        if(subStart==subLimit) {
            
            sectionValues[j]=defaultValue;
        } else {
            
            sectionValues[j]=(uint32_t)utm_countItems(extData->fromUTableValues);

            
            if(!generateFromUTable(extData, table, subStart, subLimit, unitIndex+1, defaultValue)) {
                return FALSE;
            }
        }
    }
    return TRUE;
}






static void
addFromUTrieEntry(CnvExtData *extData, UChar32 c, uint32_t value) {
    int32_t i1, i2, i3, i3b, nextOffset, min, newBlock;

    if(value==0) {
        return;
    }

    




    i1=c>>10;
    if(i1>=extData->stage1Top) {
        extData->stage1Top=i1+1;
    }

    nextOffset=(c>>4)&0x3f;

    if(extData->stage1[i1]==0) {
        
        newBlock=extData->stage2Top;
        min=newBlock-nextOffset; 
        while(min<newBlock && extData->stage2[newBlock-1]==0) {
            --newBlock;
        }

        extData->stage1[i1]=(uint16_t)newBlock;
        extData->stage2Top=newBlock+MBCS_STAGE_2_BLOCK_SIZE;
        if(extData->stage2Top>LENGTHOF(extData->stage2)) {
            fprintf(stderr, "error: too many stage 2 entries at U+%04x\n", (int)c);
            exit(U_MEMORY_ALLOCATION_ERROR);
        }
    }

    i2=extData->stage1[i1]+nextOffset;
    nextOffset=c&0xf;

    if(extData->stage2[i2]==0) {
        
        newBlock=extData->stage3Top;
        min=newBlock-nextOffset; 
        while(min<newBlock && extData->stage3[newBlock-1]==0) {
            --newBlock;
        }

        
        newBlock=(newBlock+(UCNV_EXT_STAGE_3_GRANULARITY-1))&~(UCNV_EXT_STAGE_3_GRANULARITY-1);
        extData->stage2[i2]=(uint16_t)(newBlock>>UCNV_EXT_STAGE_2_LEFT_SHIFT);

        extData->stage3Top=newBlock+MBCS_STAGE_3_BLOCK_SIZE;
        if(extData->stage3Top>LENGTHOF(extData->stage3)) {
            fprintf(stderr, "error: too many stage 3 entries at U+%04x\n", (int)c);
            exit(U_MEMORY_ALLOCATION_ERROR);
        }
    }

    i3=((int32_t)extData->stage2[i2]<<UCNV_EXT_STAGE_2_LEFT_SHIFT)+nextOffset;
    




    if(value==UCNV_EXT_FROM_U_SUBCHAR1) {
        
        extData->stage3[i3]=1;

        







        
        if(nextOffset==MBCS_STAGE_3_BLOCK_SIZE-1) {
            for(min=i3-nextOffset;
                min<i3 && extData->stage3[min]==1;
                ++min) {}

            if(min==i3) {
                
                if(extData->stage3Sub1Block!=0) {
                    
                    extData->stage2[i2]=extData->stage3Sub1Block;
                    extData->stage3Top-=MBCS_STAGE_3_BLOCK_SIZE;
                    uprv_memset(extData->stage3+extData->stage3Top, 0, MBCS_STAGE_3_BLOCK_SIZE*2);
                } else {
                    
                    extData->stage3Sub1Block=extData->stage2[i2];
                }
            }
        }
    } else {
        if((i3b=extData->stage3bTop++)>=LENGTHOF(extData->stage3b)) {
            fprintf(stderr, "error: too many stage 3b entries at U+%04x\n", (int)c);
            exit(U_MEMORY_ALLOCATION_ERROR);
        }

        
        extData->stage3[i3]=(uint16_t)i3b;
        extData->stage3b[i3b]=value;
    }
}

static UBool
generateFromUTrie(CnvExtData *extData, UCMTable *table, int32_t mapLength) {
    UCMapping *mappings, *m;
    int32_t *map;
    uint32_t value;
    int32_t subStart, subLimit;

    UChar32 *codePoints;
    UChar32 c, next;

    if(mapLength==0) {
        return TRUE;
    }

    mappings=table->mappings;
    map=table->reverseMap;

    





    m=mappings+map[0];
    codePoints=UCM_GET_CODE_POINTS(table, m);
    next=codePoints[0];
    subLimit=0;
    while(subLimit<mapLength) {
        
        subStart=subLimit;
        c=next;
        while(next==c && ++subLimit<mapLength) {
            m=mappings+map[subLimit];
            codePoints=UCM_GET_CODE_POINTS(table, m);
            next=codePoints[0];
        }

        




        value=0;
        m=mappings+map[subStart];
        codePoints=UCM_GET_CODE_POINTS(table, m);
        if(m->uLen==1) {
            
            ++subStart;

            if(subStart<subLimit && mappings[map[subStart]].uLen==1) {
                
                fprintf(stderr, "error: multiple mappings from same Unicode code points\n");
                ucm_printMapping(table, m, stderr);
                ucm_printMapping(table, mappings+map[subStart], stderr);
                return FALSE;
            }

            value=getFromUBytesValue(extData, table, m);
        }

        if(subStart==subLimit) {
            
            addFromUTrieEntry(extData, c, value);
        } else {
            
            addFromUTrieEntry(extData, c, (uint32_t)utm_countItems(extData->fromUTableValues));

            
            if(!generateFromUTable(extData, table, subStart, subLimit, 2, value)) {
                return FALSE;
            }
        }
    }
    return TRUE;
}






static UBool
makeFromUTable(CnvExtData *extData, UCMTable *table) {
    uint16_t *stage1;
    int32_t i, stage1Top, fromUCount;

    fromUCount=prepareFromUMappings(table);

    extData->fromUTableUChars=utm_open("cnv extension fromUTableUChars", 0x10000, UCNV_EXT_FROM_U_DATA_MASK+1, 2);
    extData->fromUTableValues=utm_open("cnv extension fromUTableValues", 0x10000, UCNV_EXT_FROM_U_DATA_MASK+1, 4);
    extData->fromUBytes=utm_open("cnv extension fromUBytes", 0x10000, UCNV_EXT_FROM_U_DATA_MASK+1, 1);

    
    extData->stage2Top=MBCS_STAGE_2_FIRST_ASSIGNED;
    extData->stage3Top=MBCS_STAGE_3_FIRST_ASSIGNED;

    




    extData->stage3b[1]=UCNV_EXT_FROM_U_SUBCHAR1;
    extData->stage3bTop=2;

    
    utm_alloc(extData->fromUTableUChars);
    utm_alloc(extData->fromUTableValues);

    if(!generateFromUTrie(extData, table, fromUCount)) {
        return FALSE;
    }

    



    stage1=extData->stage1;
    stage1Top=extData->stage1Top;
    for(i=0; i<stage1Top; ++i) {
        stage1[i]=(uint16_t)(stage1[i]+stage1Top);
    }

    return TRUE;
}



static UBool
CnvExtAddTable(NewConverter *cnvData, UCMTable *table, UConverterStaticData *staticData) {
    CnvExtData *extData;

    if(table->unicodeMask&UCNV_HAS_SURROGATES) {
        fprintf(stderr, "error: contains mappings for surrogate code points\n");
        return FALSE;
    }

    staticData->conversionType=UCNV_MBCS;

    extData=(CnvExtData *)cnvData;

    






    return
        makeToUTable(extData, table) &&
        makeFromUTable(extData, table);
}
