















#include <stdio.h>
#include "unicode/utypes.h"
#include "cstring.h"
#include "cmemory.h"
#include "unewdata.h"
#include "ucnv_cnv.h"
#include "ucnvmbcs.h"
#include "ucm.h"
#include "makeconv.h"
#include "genmbcs.h"






struct MBCSData {
    NewConverter newConverter;

    UCMFile *ucm;

    
    _MBCSToUFallback toUFallbacks[MBCS_MAX_FALLBACK_COUNT];
    int32_t countToUFallbacks;
    uint16_t *unicodeCodeUnits;

    
    uint16_t stage1[MBCS_STAGE_1_SIZE];
    uint16_t stage2Single[MBCS_STAGE_2_SIZE]; 
    uint32_t stage2[MBCS_STAGE_2_SIZE]; 
    uint8_t *fromUBytes;
    uint32_t stage2Top, stage3Top;

    
    uint16_t stageUTF8[0x10000>>MBCS_UTF8_STAGE_SHIFT];  

    




    uint16_t utf8Max;

    UBool utf8Friendly;
    UBool omitFromU;
};


static void
MBCSClose(NewConverter *cnvData);

static UBool
MBCSStartMappings(MBCSData *mbcsData);

static UBool
MBCSAddToUnicode(MBCSData *mbcsData,
                 const uint8_t *bytes, int32_t length,
                 UChar32 c,
                 int8_t flag);

static UBool
MBCSIsValid(NewConverter *cnvData,
            const uint8_t *bytes, int32_t length);

static UBool
MBCSSingleAddFromUnicode(MBCSData *mbcsData,
                         const uint8_t *bytes, int32_t length,
                         UChar32 c,
                         int8_t flag);

static UBool
MBCSAddFromUnicode(MBCSData *mbcsData,
                   const uint8_t *bytes, int32_t length,
                   UChar32 c,
                   int8_t flag);

static void
MBCSPostprocess(MBCSData *mbcsData, const UConverterStaticData *staticData);

static UBool
MBCSAddTable(NewConverter *cnvData, UCMTable *table, UConverterStaticData *staticData);

static uint32_t
MBCSWrite(NewConverter *cnvData, const UConverterStaticData *staticData,
          UNewDataMemory *pData, int32_t tableType);



static inline char
hexDigit(uint8_t digit) {
    return digit<=9 ? (char)('0'+digit) : (char)('a'-10+digit);
}

static inline char *
printBytes(char *buffer, const uint8_t *bytes, int32_t length) {
    char *s=buffer;
    while(length>0) {
        *s++=hexDigit((uint8_t)(*bytes>>4));
        *s++=hexDigit((uint8_t)(*bytes&0xf));
        ++bytes;
        --length;
    }

    *s=0;
    return buffer;
}



static MBCSData gDummy;

U_CFUNC const MBCSData *
MBCSGetDummy() {
    uprv_memset(&gDummy, 0, sizeof(MBCSData));

    






    gDummy.utf8Friendly=TRUE;
    if(SMALL) {
        gDummy.utf8Max=0xffff;
        gDummy.omitFromU=TRUE;
    } else {
        gDummy.utf8Max=MBCS_UTF8_MAX;
    }
    return &gDummy;
}

static void
MBCSInit(MBCSData *mbcsData, UCMFile *ucm) {
    uprv_memset(mbcsData, 0, sizeof(MBCSData));

    mbcsData->ucm=ucm; 

    mbcsData->newConverter.close=MBCSClose;
    mbcsData->newConverter.isValid=MBCSIsValid;
    mbcsData->newConverter.addTable=MBCSAddTable;
    mbcsData->newConverter.write=MBCSWrite;
}

NewConverter *
MBCSOpen(UCMFile *ucm) {
    MBCSData *mbcsData=(MBCSData *)uprv_malloc(sizeof(MBCSData));
    if(mbcsData==NULL) {
        printf("out of memory\n");
        exit(U_MEMORY_ALLOCATION_ERROR);
    }

    MBCSInit(mbcsData, ucm);
    return &mbcsData->newConverter;
}

static void
MBCSDestruct(MBCSData *mbcsData) {
    uprv_free(mbcsData->unicodeCodeUnits);
    uprv_free(mbcsData->fromUBytes);
}

static void
MBCSClose(NewConverter *cnvData) {
    MBCSData *mbcsData=(MBCSData *)cnvData;
    if(mbcsData!=NULL) {
        MBCSDestruct(mbcsData);
        uprv_free(mbcsData);
    }
}

static UBool
MBCSStartMappings(MBCSData *mbcsData) {
    int32_t i, sum, maxCharLength,
            stage2NullLength, stage2AllocLength,
            stage3NullLength, stage3AllocLength;

    

    
    sum=mbcsData->ucm->states.countToUCodeUnits;
    if(VERBOSE) {
        printf("the total number of offsets is 0x%lx=%ld\n", (long)sum, (long)sum);
    }

    if(sum>0) {
        mbcsData->unicodeCodeUnits=(uint16_t *)uprv_malloc(sum*sizeof(uint16_t));
        if(mbcsData->unicodeCodeUnits==NULL) {
            fprintf(stderr, "error: out of memory allocating %ld 16-bit code units\n",
                (long)sum);
            return FALSE;
        }
        for(i=0; i<sum; ++i) {
            mbcsData->unicodeCodeUnits[i]=0xfffe;
        }
    }

    
    maxCharLength=mbcsData->ucm->states.maxCharLength;

    
    if(maxCharLength==1) {
        
        sum=0x20000;
    } else {
        
        sum=0x100000*maxCharLength;
    }
    mbcsData->fromUBytes=(uint8_t *)uprv_malloc(sum);
    if(mbcsData->fromUBytes==NULL) {
        fprintf(stderr, "error: out of memory allocating %ld B for target mappings\n", (long)sum);
        return FALSE;
    }
    uprv_memset(mbcsData->fromUBytes, 0, sum);

    






























    
    stage2NullLength=MBCS_STAGE_2_BLOCK_SIZE;
    stage2AllocLength=MBCS_STAGE_2_BLOCK_SIZE;

    stage3NullLength=MBCS_UTF8_STAGE_3_BLOCK_SIZE;
    stage3AllocLength=128; 

    
    sum=stage2NullLength;
    for(i=0; i<(stage2AllocLength>>MBCS_STAGE_2_BLOCK_SIZE_SHIFT); ++i) {
        mbcsData->stage1[i]=sum;
        sum+=MBCS_STAGE_2_BLOCK_SIZE;
    }
    mbcsData->stage2Top=stage2NullLength+stage2AllocLength; 

    





    if(maxCharLength==1) {
        sum=stage3NullLength;
        for(i=0; i<(stage3AllocLength/MBCS_STAGE_3_BLOCK_SIZE); ++i) {
            mbcsData->stage2Single[mbcsData->stage1[0]+i]=sum;
            sum+=MBCS_STAGE_3_BLOCK_SIZE;
        }
    } else {
        sum=stage3NullLength/MBCS_STAGE_3_GRANULARITY;
        for(i=0; i<(stage3AllocLength/MBCS_STAGE_3_BLOCK_SIZE); ++i) {
            mbcsData->stage2[mbcsData->stage1[0]+i]=sum;
            sum+=MBCS_STAGE_3_BLOCK_SIZE/MBCS_STAGE_3_GRANULARITY;
        }
    }

    sum=stage3NullLength;
    for(i=0; i<(stage3AllocLength/MBCS_UTF8_STAGE_3_BLOCK_SIZE); ++i) {
        mbcsData->stageUTF8[i]=sum;
        sum+=MBCS_UTF8_STAGE_3_BLOCK_SIZE;
    }

    




    mbcsData->stage3Top=(stage3NullLength+stage3AllocLength)*maxCharLength; 

    return TRUE;
}


static UBool
setFallback(MBCSData *mbcsData, uint32_t offset, UChar32 c) {
    int32_t i=ucm_findFallback(mbcsData->toUFallbacks, mbcsData->countToUFallbacks, offset);
    if(i>=0) {
        
        mbcsData->toUFallbacks[i].codePoint=c;
        return TRUE;
    } else {
        
        i=mbcsData->countToUFallbacks;
        if(i>=MBCS_MAX_FALLBACK_COUNT) {
            fprintf(stderr, "error: too many toUnicode fallbacks, currently at: U+%x\n", (int)c);
            return FALSE;
        } else {
            mbcsData->toUFallbacks[i].offset=offset;
            mbcsData->toUFallbacks[i].codePoint=c;
            mbcsData->countToUFallbacks=i+1;
            return TRUE;
        }
    }
}


static int32_t
removeFallback(MBCSData *mbcsData, uint32_t offset) {
    int32_t i=ucm_findFallback(mbcsData->toUFallbacks, mbcsData->countToUFallbacks, offset);
    if(i>=0) {
        _MBCSToUFallback *toUFallbacks;
        int32_t limit, old;

        toUFallbacks=mbcsData->toUFallbacks;
        limit=mbcsData->countToUFallbacks;
        old=(int32_t)toUFallbacks[i].codePoint;

        
        toUFallbacks[i].offset=toUFallbacks[limit-1].offset;
        toUFallbacks[i].codePoint=toUFallbacks[limit-1].codePoint;
        mbcsData->countToUFallbacks=limit-1;
        return old;
    } else {
        return -1;
    }
}







static UBool
MBCSAddToUnicode(MBCSData *mbcsData,
                 const uint8_t *bytes, int32_t length,
                 UChar32 c,
                 int8_t flag) {
    char buffer[10];
    uint32_t offset=0;
    int32_t i=0, entry, old;
    uint8_t state=0;

    if(mbcsData->ucm->states.countStates==0) {
        fprintf(stderr, "error: there is no state information!\n");
        return FALSE;
    }

    
    if(length==2 && mbcsData->ucm->states.outputType==MBCS_OUTPUT_2_SISO) {
        state=1;
    }

    




    for(i=0;;) {
        entry=mbcsData->ucm->states.stateTable[state][bytes[i++]];
        if(MBCS_ENTRY_IS_TRANSITION(entry)) {
            if(i==length) {
                fprintf(stderr, "error: byte sequence too short, ends in non-final state %hu: 0x%s (U+%x)\n",
                    (short)state, printBytes(buffer, bytes, length), (int)c);
                return FALSE;
            }
            state=(uint8_t)MBCS_ENTRY_TRANSITION_STATE(entry);
            offset+=MBCS_ENTRY_TRANSITION_OFFSET(entry);
        } else {
            if(i<length) {
                fprintf(stderr, "error: byte sequence too long by %d bytes, final state %u: 0x%s (U+%x)\n",
                    (int)(length-i), state, printBytes(buffer, bytes, length), (int)c);
                return FALSE;
            }
            switch(MBCS_ENTRY_FINAL_ACTION(entry)) {
            case MBCS_STATE_ILLEGAL:
                fprintf(stderr, "error: byte sequence ends in illegal state at U+%04x<->0x%s\n",
                    (int)c, printBytes(buffer, bytes, length));
                return FALSE;
            case MBCS_STATE_CHANGE_ONLY:
                fprintf(stderr, "error: byte sequence ends in state-change-only at U+%04x<->0x%s\n",
                    (int)c, printBytes(buffer, bytes, length));
                return FALSE;
            case MBCS_STATE_UNASSIGNED:
                fprintf(stderr, "error: byte sequence ends in unassigned state at U+%04x<->0x%s\n",
                    (int)c, printBytes(buffer, bytes, length));
                return FALSE;
            case MBCS_STATE_FALLBACK_DIRECT_16:
            case MBCS_STATE_VALID_DIRECT_16:
            case MBCS_STATE_FALLBACK_DIRECT_20:
            case MBCS_STATE_VALID_DIRECT_20:
                if(MBCS_ENTRY_SET_STATE(entry, 0)!=MBCS_ENTRY_FINAL(0, MBCS_STATE_VALID_DIRECT_16, 0xfffe)) {
                    
                    if(MBCS_ENTRY_FINAL_ACTION(entry)==MBCS_STATE_VALID_DIRECT_16 || MBCS_ENTRY_FINAL_ACTION(entry)==MBCS_STATE_FALLBACK_DIRECT_16) {
                        old=MBCS_ENTRY_FINAL_VALUE(entry);
                    } else {
                        old=0x10000+MBCS_ENTRY_FINAL_VALUE(entry);
                    }
                    if(flag>=0) {
                        fprintf(stderr, "error: duplicate codepage byte sequence at U+%04x<->0x%s see U+%04x\n",
                            (int)c, printBytes(buffer, bytes, length), (int)old);
                        return FALSE;
                    } else if(VERBOSE) {
                        fprintf(stderr, "duplicate codepage byte sequence at U+%04x<->0x%s see U+%04x\n",
                            (int)c, printBytes(buffer, bytes, length), (int)old);
                    }
                    



                }
                
                entry=MBCS_ENTRY_FINAL_SET_ACTION(entry, (MBCS_STATE_VALID_DIRECT_16+(flag==3 ? 2 : 0)+(c>=0x10000 ? 1 : 0)));

                
                if(c<=0xffff) {
                    entry=MBCS_ENTRY_FINAL_SET_VALUE(entry, c);
                } else {
                    entry=MBCS_ENTRY_FINAL_SET_VALUE(entry, c-0x10000);
                }
                mbcsData->ucm->states.stateTable[state][bytes[i-1]]=entry;
                break;
            case MBCS_STATE_VALID_16:
                
                
                offset+=MBCS_ENTRY_FINAL_VALUE_16(entry);
                
                if((old=mbcsData->unicodeCodeUnits[offset])!=0xfffe || (old=removeFallback(mbcsData, offset))!=-1) {
                    if(flag>=0) {
                        fprintf(stderr, "error: duplicate codepage byte sequence at U+%04x<->0x%s see U+%04x\n",
                            (int)c, printBytes(buffer, bytes, length), (int)old);
                        return FALSE;
                    } else if(VERBOSE) {
                        fprintf(stderr, "duplicate codepage byte sequence at U+%04x<->0x%s see U+%04x\n",
                            (int)c, printBytes(buffer, bytes, length), (int)old);
                    }
                }
                if(c>=0x10000) {
                    fprintf(stderr, "error: code point does not fit into valid-16-bit state at U+%04x<->0x%s\n",
                        (int)c, printBytes(buffer, bytes, length));
                    return FALSE;
                }
                if(flag>0) {
                    
                    if(mbcsData->unicodeCodeUnits[offset]==0xfffe) {
                        return setFallback(mbcsData, offset, c);
                    }
                } else {
                    mbcsData->unicodeCodeUnits[offset]=(uint16_t)c;
                }
                break;
            case MBCS_STATE_VALID_16_PAIR:
                
                
                offset+=MBCS_ENTRY_FINAL_VALUE_16(entry);
                
                old=mbcsData->unicodeCodeUnits[offset];
                if(old<0xfffe) {
                    int32_t real;
                    if(old<0xd800) {
                        real=old;
                    } else if(old<=0xdfff) {
                        real=0x10000+((old&0x3ff)<<10)+((mbcsData->unicodeCodeUnits[offset+1])&0x3ff);
                    } else  {
                        real=mbcsData->unicodeCodeUnits[offset+1];
                    }
                    if(flag>=0) {
                        fprintf(stderr, "error: duplicate codepage byte sequence at U+%04x<->0x%s see U+%04x\n",
                            (int)c, printBytes(buffer, bytes, length), (int)real);
                        return FALSE;
                    } else if(VERBOSE) {
                        fprintf(stderr, "duplicate codepage byte sequence at U+%04x<->0x%s see U+%04x\n",
                            (int)c, printBytes(buffer, bytes, length), (int)real);
                    }
                }
                if(flag>0) {
                    
                    if(old<=0xdbff || old==0xe000) {
                        
                    } else if(c<=0xffff) {
                        
                        mbcsData->unicodeCodeUnits[offset++]=0xe001;
                        mbcsData->unicodeCodeUnits[offset]=(uint16_t)c;
                    } else {
                        
                        mbcsData->unicodeCodeUnits[offset++]=(uint16_t)(0xdbc0+(c>>10));
                        mbcsData->unicodeCodeUnits[offset]=(uint16_t)(0xdc00+(c&0x3ff));
                    }
                } else {
                    if(c<0xd800) {
                        
                        mbcsData->unicodeCodeUnits[offset]=(uint16_t)c;
                    } else if(c<=0xffff) {
                        
                        mbcsData->unicodeCodeUnits[offset++]=0xe000;
                        mbcsData->unicodeCodeUnits[offset]=(uint16_t)c;
                    } else {
                        
                        mbcsData->unicodeCodeUnits[offset++]=(uint16_t)(0xd7c0+(c>>10));
                        mbcsData->unicodeCodeUnits[offset]=(uint16_t)(0xdc00+(c&0x3ff));
                    }
                }
                break;
            default:
                
                fprintf(stderr, "internal error: byte sequence reached reserved action code, entry 0x%02x: 0x%s (U+%x)\n",
                    (int)entry, printBytes(buffer, bytes, length), (int)c);
                return FALSE;
            }

            return TRUE;
        }
    }
}


static UBool
MBCSIsValid(NewConverter *cnvData,
            const uint8_t *bytes, int32_t length) {
    MBCSData *mbcsData=(MBCSData *)cnvData;

    return (UBool)(1==ucm_countChars(&mbcsData->ucm->states, bytes, length));
}

static UBool
MBCSSingleAddFromUnicode(MBCSData *mbcsData,
                         const uint8_t *bytes, int32_t ,
                         UChar32 c,
                         int8_t flag) {
    uint16_t *stage3, *p;
    uint32_t idx;
    uint16_t old;
    uint8_t b;

    uint32_t blockSize, newTop, i, nextOffset, newBlock, min;

    
    if(flag==2) {
        return TRUE;
    }

    





    stage3=(uint16_t *)mbcsData->fromUBytes;
    b=*bytes;

    
    idx=c>>MBCS_STAGE_1_SHIFT;
    if(mbcsData->utf8Friendly && c<=SBCS_UTF8_MAX) {
        nextOffset=(c>>MBCS_STAGE_2_SHIFT)&MBCS_STAGE_2_BLOCK_MASK&~(MBCS_UTF8_STAGE_3_BLOCKS-1);
    } else {
        nextOffset=(c>>MBCS_STAGE_2_SHIFT)&MBCS_STAGE_2_BLOCK_MASK;
    }
    if(mbcsData->stage1[idx]==MBCS_STAGE_2_ALL_UNASSIGNED_INDEX) {
        
        newBlock=mbcsData->stage2Top;
        if(mbcsData->utf8Friendly) {
            min=newBlock-nextOffset; 
            while(min<newBlock && mbcsData->stage2Single[newBlock-1]==0) {
                --newBlock;
            }
        }
        newTop=newBlock+MBCS_STAGE_2_BLOCK_SIZE;

        if(newTop>MBCS_MAX_STAGE_2_TOP) {
            fprintf(stderr, "error: too many stage 2 entries at U+%04x<->0x%02x\n", (int)c, b);
            return FALSE;
        }

        



        mbcsData->stage1[idx]=(uint16_t)newBlock;
        mbcsData->stage2Top=newTop;
    }

    
    idx=mbcsData->stage1[idx]+nextOffset;
    if(mbcsData->utf8Friendly && c<=SBCS_UTF8_MAX) {
        
        blockSize=MBCS_UTF8_STAGE_3_BLOCK_SIZE;
        nextOffset=c&MBCS_UTF8_STAGE_3_BLOCK_MASK;
    } else {
        blockSize=MBCS_STAGE_3_BLOCK_SIZE;
        nextOffset=c&MBCS_STAGE_3_BLOCK_MASK;
    }
    if(mbcsData->stage2Single[idx]==0) {
        
        newBlock=mbcsData->stage3Top;
        if(mbcsData->utf8Friendly) {
            min=newBlock-nextOffset; 
            while(min<newBlock && stage3[newBlock-1]==0) {
                --newBlock;
            }
        }
        newTop=newBlock+blockSize;

        if(newTop>MBCS_STAGE_3_SBCS_SIZE) {
            fprintf(stderr, "error: too many code points at U+%04x<->0x%02x\n", (int)c, b);
            return FALSE;
        }
        
        i=idx;
        while(newBlock<newTop) {
            mbcsData->stage2Single[i++]=(uint16_t)newBlock;
            newBlock+=MBCS_STAGE_3_BLOCK_SIZE;
        }
        mbcsData->stage3Top=newTop; 
    }

    
    p=stage3+mbcsData->stage2Single[idx]+nextOffset;
    old=*p;
    if(flag<=0) {
        *p=(uint16_t)(0xf00|b);
    } else if(IS_PRIVATE_USE(c)) {
        *p=(uint16_t)(0xc00|b);
    } else {
        *p=(uint16_t)(0x800|b);
    }

    
    if(old>=0x100) {
        if(flag>=0) {
            fprintf(stderr, "error: duplicate Unicode code point at U+%04x<->0x%02x see 0x%02x\n",
                (int)c, b, old&0xff);
            return FALSE;
        } else if(VERBOSE) {
            fprintf(stderr, "duplicate Unicode code point at U+%04x<->0x%02x see 0x%02x\n",
                (int)c, b, old&0xff);
        }
        
    }

    return TRUE;
}

static UBool
MBCSAddFromUnicode(MBCSData *mbcsData,
                   const uint8_t *bytes, int32_t length,
                   UChar32 c,
                   int8_t flag) {
    char buffer[10];
    const uint8_t *pb;
    uint8_t *stage3, *p;
    uint32_t idx, b, old, stage3Index;
    int32_t maxCharLength;

    uint32_t blockSize, newTop, i, nextOffset, newBlock, min, overlap, maxOverlap;

    maxCharLength=mbcsData->ucm->states.maxCharLength;

    if( mbcsData->ucm->states.outputType==MBCS_OUTPUT_2_SISO &&
        (!IGNORE_SISO_CHECK && (*bytes==0xe || *bytes==0xf))
    ) {
        fprintf(stderr, "error: illegal mapping to SI or SO for SI/SO codepage: U+%04x<->0x%s\n",
            (int)c, printBytes(buffer, bytes, length));
        return FALSE;
    }

    if(flag==1 && length==1 && *bytes==0) {
        fprintf(stderr, "error: unable to encode a |1 fallback from U+%04x to 0x%02x\n",
            (int)c, *bytes);
        return FALSE;
    }

    






    stage3=mbcsData->fromUBytes;

    
    idx=c>>MBCS_STAGE_1_SHIFT;
    if(mbcsData->utf8Friendly && c<=mbcsData->utf8Max) {
        nextOffset=(c>>MBCS_STAGE_2_SHIFT)&MBCS_STAGE_2_BLOCK_MASK&~(MBCS_UTF8_STAGE_3_BLOCKS-1);
    } else {
        nextOffset=(c>>MBCS_STAGE_2_SHIFT)&MBCS_STAGE_2_BLOCK_MASK;
    }
    if(mbcsData->stage1[idx]==MBCS_STAGE_2_ALL_UNASSIGNED_INDEX) {
        
        newBlock=mbcsData->stage2Top;
        if(mbcsData->utf8Friendly) {
            min=newBlock-nextOffset; 
            while(min<newBlock && mbcsData->stage2[newBlock-1]==0) {
                --newBlock;
            }
        }
        newTop=newBlock+MBCS_STAGE_2_BLOCK_SIZE;

        if(newTop>MBCS_MAX_STAGE_2_TOP) {
            fprintf(stderr, "error: too many stage 2 entries at U+%04x<->0x%s\n",
                (int)c, printBytes(buffer, bytes, length));
            return FALSE;
        }

        



        i=idx;
        while(newBlock<newTop) {
            mbcsData->stage1[i++]=(uint16_t)newBlock;
            newBlock+=MBCS_STAGE_2_BLOCK_SIZE;
        }
        mbcsData->stage2Top=newTop; 
    }

    
    idx=mbcsData->stage1[idx]+nextOffset;
    if(mbcsData->utf8Friendly && c<=mbcsData->utf8Max) {
        
        blockSize=MBCS_UTF8_STAGE_3_BLOCK_SIZE*maxCharLength;
        nextOffset=c&MBCS_UTF8_STAGE_3_BLOCK_MASK;
    } else {
        blockSize=MBCS_STAGE_3_BLOCK_SIZE*maxCharLength;
        nextOffset=c&MBCS_STAGE_3_BLOCK_MASK;
    }
    if(mbcsData->stage2[idx]==0) {
        
        newBlock=mbcsData->stage3Top;
        if(mbcsData->utf8Friendly && nextOffset>=MBCS_STAGE_3_GRANULARITY) {
            



            maxOverlap=(nextOffset&~(MBCS_STAGE_3_GRANULARITY-1))*maxCharLength;
            for(overlap=0;
                overlap<maxOverlap && stage3[newBlock-overlap-1]==0;
                ++overlap) {}

            overlap=(overlap/MBCS_STAGE_3_GRANULARITY)/maxCharLength;
            overlap=(overlap*MBCS_STAGE_3_GRANULARITY)*maxCharLength;

            newBlock-=overlap;
        }
        newTop=newBlock+blockSize;

        if(newTop>MBCS_STAGE_3_MBCS_SIZE*(uint32_t)maxCharLength) {
            fprintf(stderr, "error: too many code points at U+%04x<->0x%s\n",
                (int)c, printBytes(buffer, bytes, length));
            return FALSE;
        }
        
        i=idx;
        while(newBlock<newTop) {
            mbcsData->stage2[i++]=(newBlock/MBCS_STAGE_3_GRANULARITY)/maxCharLength;
            newBlock+=MBCS_STAGE_3_BLOCK_SIZE*maxCharLength;
        }
        mbcsData->stage3Top=newTop; 
    }

    stage3Index=MBCS_STAGE_3_GRANULARITY*(uint32_t)(uint16_t)mbcsData->stage2[idx];

    
    if(mbcsData->utf8Friendly && c<=mbcsData->utf8Max) {
        
        if(stage3Index>0xffff) {
            

















            mbcsData->utf8Max=0xfeff;
        } else {
            



            mbcsData->stageUTF8[c>>MBCS_UTF8_STAGE_SHIFT]=(uint16_t)stage3Index;
        }
    }

    

    
    pb=bytes;
    b=0;
    switch(length) {
    case 4:
        b=*pb++;
    case 3:
        b=(b<<8)|*pb++;
    case 2:
        b=(b<<8)|*pb++;
    case 1:
    default:
        b=(b<<8)|*pb++;
        break;
    }

    old=0;
    p=stage3+(stage3Index+nextOffset)*maxCharLength;
    switch(maxCharLength) {
    case 2:
        old=*(uint16_t *)p;
        *(uint16_t *)p=(uint16_t)b;
        break;
    case 3:
        old=(uint32_t)*p<<16;
        *p++=(uint8_t)(b>>16);
        old|=(uint32_t)*p<<8;
        *p++=(uint8_t)(b>>8);
        old|=*p;
        *p=(uint8_t)b;
        break;
    case 4:
        old=*(uint32_t *)p;
        *(uint32_t *)p=b;
        break;
    default:
        
        break;
    }

    
    if((mbcsData->stage2[idx+(nextOffset>>MBCS_STAGE_2_SHIFT)]&(1UL<<(16+(c&0xf))))!=0 || old!=0) {
        if(flag>=0) {
            fprintf(stderr, "error: duplicate Unicode code point at U+%04x<->0x%s see 0x%02x\n",
                (int)c, printBytes(buffer, bytes, length), (int)old);
            return FALSE;
        } else if(VERBOSE) {
            fprintf(stderr, "duplicate Unicode code point at U+%04x<->0x%s see 0x%02x\n",
                (int)c, printBytes(buffer, bytes, length), (int)old);
        }
        

    }
    if(flag<=0) {
        
        mbcsData->stage2[idx+(nextOffset>>4)]|=(1UL<<(16+(c&0xf)));
    }

    return TRUE;
}

U_CFUNC UBool
MBCSOkForBaseFromUnicode(const MBCSData *mbcsData,
                         const uint8_t *bytes, int32_t length,
                         UChar32 c, int8_t flag) {
    









    if( (flag==2 && length==1) ||
        (flag==1 && bytes[0]==0) || 
        (flag<=1 && length>1 && bytes[0]==0)
    ) {
        return FALSE;
    }

    






    if(mbcsData->utf8Friendly && flag<=1 && c<=mbcsData->utf8Max && (bytes[0]==0 || flag==1)) {
        return FALSE;
    }

    




    if(mbcsData->omitFromU && flag!=0) {
        return FALSE;
    }

    
    return TRUE;
}


static UBool
MBCSAddTable(NewConverter *cnvData, UCMTable *table, UConverterStaticData *staticData) {
    MBCSData *mbcsData;
    UCMapping *m;
    UChar32 c;
    int32_t i, maxCharLength;
    int8_t f;
    UBool isOK, utf8Friendly;

    staticData->unicodeMask=table->unicodeMask;
    if(staticData->unicodeMask==3) {
        fprintf(stderr, "error: contains mappings for both supplementary and surrogate code points\n");
        return FALSE;
    }

    staticData->conversionType=UCNV_MBCS;

    mbcsData=(MBCSData *)cnvData;
    maxCharLength=mbcsData->ucm->states.maxCharLength;

    




    mbcsData->utf8Friendly=utf8Friendly=(UBool)((table->flagsType&UCM_FLAGS_EXPLICIT)!=0);
    if(utf8Friendly) {
        mbcsData->utf8Max=MBCS_UTF8_MAX;
        if(SMALL && maxCharLength>1) {
            mbcsData->omitFromU=TRUE;
        }
    } else {
        mbcsData->utf8Max=0;
        if(SMALL && maxCharLength>1) {
            fprintf(stderr,
                "makeconv warning: --small not available for .ucm files without |0 etc.\n");
        }
    }

    if(!MBCSStartMappings(mbcsData)) {
        return FALSE;
    }

    staticData->hasFromUnicodeFallback=FALSE;
    staticData->hasToUnicodeFallback=FALSE;

    isOK=TRUE;

    m=table->mappings;
    for(i=0; i<table->mappingsLength; ++m, ++i) {
        c=m->u;
        f=m->f;

        














        if( mbcsData->omitFromU && f<=1 &&
            mbcsData->utf8Max<c && c<=0xffff &&
            mbcsData->utf8Max<0xfeff
        ) {
            mbcsData->utf8Max=0xffff;
        }

        switch(f) {
        case -1:
            
            
        case 0:
            
            isOK&=MBCSAddToUnicode(mbcsData, m->b.bytes, m->bLen, c, f);

            if(maxCharLength==1) {
                isOK&=MBCSSingleAddFromUnicode(mbcsData, m->b.bytes, m->bLen, c, f);
            } else if(MBCSOkForBaseFromUnicode(mbcsData, m->b.bytes, m->bLen, c, f)) {
                isOK&=MBCSAddFromUnicode(mbcsData, m->b.bytes, m->bLen, c, f);
            } else {
                m->f|=MBCS_FROM_U_EXT_FLAG;
                m->moveFlag=UCM_MOVE_TO_EXT;
            }
            break;
        case 1:
            
            if(maxCharLength==1) {
                staticData->hasFromUnicodeFallback=TRUE;
                isOK&=MBCSSingleAddFromUnicode(mbcsData, m->b.bytes, m->bLen, c, f);
            } else if(MBCSOkForBaseFromUnicode(mbcsData, m->b.bytes, m->bLen, c, f)) {
                staticData->hasFromUnicodeFallback=TRUE;
                isOK&=MBCSAddFromUnicode(mbcsData, m->b.bytes, m->bLen, c, f);
            } else {
                m->f|=MBCS_FROM_U_EXT_FLAG;
                m->moveFlag=UCM_MOVE_TO_EXT;
            }
            break;
        case 2:
            
            if(maxCharLength>1 && m->bLen==1) {
                m->f|=MBCS_FROM_U_EXT_FLAG;
                m->moveFlag=UCM_MOVE_TO_EXT;
            }
            break;
        case 3:
            
            staticData->hasToUnicodeFallback=TRUE;
            isOK&=MBCSAddToUnicode(mbcsData, m->b.bytes, m->bLen, c, f);
            break;
        default:
            
            fprintf(stderr, "error: illegal fallback indicator %d\n", f);
            return FALSE;
        }
    }

    MBCSPostprocess(mbcsData, staticData);

    return isOK;
}

static UBool
transformEUC(MBCSData *mbcsData) {
    uint8_t *p8;
    uint32_t i, value, oldLength, old3Top;
    uint8_t b;

    oldLength=mbcsData->ucm->states.maxCharLength;
    if(oldLength<3) {
        return FALSE;
    }

    old3Top=mbcsData->stage3Top;

    

    
    p8=mbcsData->fromUBytes;

#if !U_IS_BIG_ENDIAN
    if(oldLength==4) {
        p8+=3;
    }
#endif

    for(i=0; i<old3Top; i+=oldLength) {
        b=p8[i];
        if(b!=0 && b!=0x8e && b!=0x8f) {
            
            return FALSE;
        }
    }
    
    p8=mbcsData->fromUBytes;

    
    mbcsData->ucm->states.outputType=(int8_t)(MBCS_OUTPUT_3_EUC+oldLength-3);
    mbcsData->stage3Top=(old3Top*(oldLength-1))/oldLength;

    






    if(oldLength==3) {
        uint16_t *q=(uint16_t *)p8;
        for(i=0; i<old3Top; i+=oldLength) {
            b=*p8;
            if(b==0) {
                
                
                (*q++)=(uint16_t)((p8[1]<<8)|p8[2]);
            } else if(b==0x8e) {
                
                (*q++)=(uint16_t)(((p8[1]&0x7f)<<8)|p8[2]);
            } else  {
                
                (*q++)=(uint16_t)((p8[1]<<8)|(p8[2]&0x7f));
            }
            p8+=3;
        }
    } else  {
        uint8_t *q=p8;
        uint32_t *p32=(uint32_t *)p8;
        for(i=0; i<old3Top; i+=4) {
            value=(*p32++);
            if(value<=0xffffff) {
                
                
                (*q++)=(uint8_t)(value>>16);
                (*q++)=(uint8_t)(value>>8);
                (*q++)=(uint8_t)value;
            } else if(value<=0x8effffff) {
                
                (*q++)=(uint8_t)((value>>16)&0x7f);
                (*q++)=(uint8_t)(value>>8);
                (*q++)=(uint8_t)value;
            } else  {
                
                (*q++)=(uint8_t)(value>>16);
                (*q++)=(uint8_t)((value>>8)&0x7f);
                (*q++)=(uint8_t)value;
            }
        }
    }

    return TRUE;
}








static void
singleCompactStage2(MBCSData *mbcsData) {
    
    uint16_t map[MBCS_STAGE_2_MAX_BLOCKS];
    uint16_t i, start, prevEnd, newStart;

    
    map[0]=MBCS_STAGE_2_ALL_UNASSIGNED_INDEX;

    
    start=newStart=MBCS_STAGE_2_FIRST_ASSIGNED;
    while(start<mbcsData->stage2Top) {
        prevEnd=(uint16_t)(newStart-1);

        
        for(i=0; i<MBCS_STAGE_2_BLOCK_SIZE && mbcsData->stage2Single[start+i]==0 && mbcsData->stage2Single[prevEnd-i]==0; ++i) {}

        if(i>0) {
            map[start>>MBCS_STAGE_2_BLOCK_SIZE_SHIFT]=(uint16_t)(newStart-i);

            
            start+=i;
            for(i=(uint16_t)(MBCS_STAGE_2_BLOCK_SIZE-i); i>0; --i) {
                mbcsData->stage2Single[newStart++]=mbcsData->stage2Single[start++];
            }
        } else if(newStart<start) {
            
            map[start>>MBCS_STAGE_2_BLOCK_SIZE_SHIFT]=newStart;
            for(i=MBCS_STAGE_2_BLOCK_SIZE; i>0; --i) {
                mbcsData->stage2Single[newStart++]=mbcsData->stage2Single[start++];
            }
        } else  {
            map[start>>MBCS_STAGE_2_BLOCK_SIZE_SHIFT]=start;
            start=newStart+=MBCS_STAGE_2_BLOCK_SIZE;
        }
    }

    
    if(VERBOSE && newStart<mbcsData->stage2Top) {
        printf("compacting stage 2 from stage2Top=0x%lx to 0x%lx, saving %ld bytes\n",
                (unsigned long)mbcsData->stage2Top, (unsigned long)newStart,
                (long)(mbcsData->stage2Top-newStart)*2);
    }
    mbcsData->stage2Top=newStart;

    
    for(i=0; i<MBCS_STAGE_1_SIZE; ++i) {
        mbcsData->stage1[i]=map[mbcsData->stage1[i]>>MBCS_STAGE_2_BLOCK_SIZE_SHIFT];
    }
}


static void
singleCompactStage3(MBCSData *mbcsData) {
    uint16_t *stage3=(uint16_t *)mbcsData->fromUBytes;

    
    uint16_t map[0x1000];
    uint16_t i, start, prevEnd, newStart;

    
    map[0]=0;

    
    start=newStart=16;
    while(start<mbcsData->stage3Top) {
        prevEnd=(uint16_t)(newStart-1);

        
        for(i=0; i<16 && stage3[start+i]==0 && stage3[prevEnd-i]==0; ++i) {}

        if(i>0) {
            map[start>>4]=(uint16_t)(newStart-i);

            
            start+=i;
            for(i=(uint16_t)(16-i); i>0; --i) {
                stage3[newStart++]=stage3[start++];
            }
        } else if(newStart<start) {
            
            map[start>>4]=newStart;
            for(i=16; i>0; --i) {
                stage3[newStart++]=stage3[start++];
            }
        } else  {
            map[start>>4]=start;
            start=newStart+=16;
        }
    }

    
    if(VERBOSE && newStart<mbcsData->stage3Top) {
        printf("compacting stage 3 from stage3Top=0x%lx to 0x%lx, saving %ld bytes\n",
                (unsigned long)mbcsData->stage3Top, (unsigned long)newStart,
                (long)(mbcsData->stage3Top-newStart)*2);
    }
    mbcsData->stage3Top=newStart;

    
    for(i=0; i<mbcsData->stage2Top; ++i) {
        mbcsData->stage2Single[i]=map[mbcsData->stage2Single[i]>>4];
    }
}








static void
compactStage2(MBCSData *mbcsData) {
    
    uint16_t map[MBCS_STAGE_2_MAX_BLOCKS];
    uint16_t i, start, prevEnd, newStart;

    
    map[0]=MBCS_STAGE_2_ALL_UNASSIGNED_INDEX;

    
    start=newStart=MBCS_STAGE_2_FIRST_ASSIGNED;
    while(start<mbcsData->stage2Top) {
        prevEnd=(uint16_t)(newStart-1);

        
        for(i=0; i<MBCS_STAGE_2_BLOCK_SIZE && mbcsData->stage2[start+i]==0 && mbcsData->stage2[prevEnd-i]==0; ++i) {}

        if(i>0) {
            map[start>>MBCS_STAGE_2_BLOCK_SIZE_SHIFT]=(uint16_t)(newStart-i);

            
            start+=i;
            for(i=(uint16_t)(MBCS_STAGE_2_BLOCK_SIZE-i); i>0; --i) {
                mbcsData->stage2[newStart++]=mbcsData->stage2[start++];
            }
        } else if(newStart<start) {
            
            map[start>>MBCS_STAGE_2_BLOCK_SIZE_SHIFT]=newStart;
            for(i=MBCS_STAGE_2_BLOCK_SIZE; i>0; --i) {
                mbcsData->stage2[newStart++]=mbcsData->stage2[start++];
            }
        } else  {
            map[start>>MBCS_STAGE_2_BLOCK_SIZE_SHIFT]=start;
            start=newStart+=MBCS_STAGE_2_BLOCK_SIZE;
        }
    }

    
    if(VERBOSE && newStart<mbcsData->stage2Top) {
        printf("compacting stage 2 from stage2Top=0x%lx to 0x%lx, saving %ld bytes\n",
                (unsigned long)mbcsData->stage2Top, (unsigned long)newStart,
                (long)(mbcsData->stage2Top-newStart)*4);
    }
    mbcsData->stage2Top=newStart;

    
    for(i=0; i<MBCS_STAGE_1_SIZE; ++i) {
        mbcsData->stage1[i]=map[mbcsData->stage1[i]>>MBCS_STAGE_2_BLOCK_SIZE_SHIFT];
    }
}

static void
MBCSPostprocess(MBCSData *mbcsData, const UConverterStaticData * ) {
    UCMStates *states;
    int32_t maxCharLength, stage3Width;

    states=&mbcsData->ucm->states;
    stage3Width=maxCharLength=states->maxCharLength;

    ucm_optimizeStates(states,
                       &mbcsData->unicodeCodeUnits,
                       mbcsData->toUFallbacks, mbcsData->countToUFallbacks,
                       VERBOSE);

    
    if(transformEUC(mbcsData)) {
        --stage3Width;
    }

    









    if(!mbcsData->utf8Friendly) {
        if(maxCharLength==1) {
            singleCompactStage3(mbcsData);
            singleCompactStage2(mbcsData);
        } else {
            compactStage2(mbcsData);
        }
    }

    if(VERBOSE) {
        

        printf("fromUnicode number of uint%s_t in stage 2: 0x%lx=%lu\n",
               maxCharLength==1 ? "16" : "32",
               (unsigned long)mbcsData->stage2Top,
               (unsigned long)mbcsData->stage2Top);
        printf("fromUnicode number of %d-byte stage 3 mapping entries: 0x%lx=%lu\n",
               (int)stage3Width,
               (unsigned long)mbcsData->stage3Top/stage3Width,
               (unsigned long)mbcsData->stage3Top/stage3Width);
#if 0
        c=0;
        for(i1=0; i1<MBCS_STAGE_1_SIZE; ++i1) {
            i2=mbcsData->stage1[i1];
            if(i2==0) {
                c+=MBCS_STAGE_2_BLOCK_SIZE*MBCS_STAGE_3_BLOCK_SIZE;
                continue;
            }
            for(i2Limit=i2+MBCS_STAGE_2_BLOCK_SIZE; i2<i2Limit; ++i2) {
                if(maxCharLength==1) {
                    i3=mbcsData->stage2Single[i2];
                } else {
                    i3=(uint16_t)mbcsData->stage2[i2];
                }
                if(i3==0) {
                    c+=MBCS_STAGE_3_BLOCK_SIZE;
                    continue;
                }
                printf("U+%04lx i1=0x%02lx i2=0x%04lx i3=0x%04lx\n",
                       (unsigned long)c,
                       (unsigned long)i1,
                       (unsigned long)i2,
                       (unsigned long)i3);
                c+=MBCS_STAGE_3_BLOCK_SIZE;
            }
        }
#endif
    }
}

static uint32_t
MBCSWrite(NewConverter *cnvData, const UConverterStaticData *staticData,
          UNewDataMemory *pData, int32_t tableType) {
    MBCSData *mbcsData=(MBCSData *)cnvData;
    uint32_t stage2Start, stage2Length;
    uint32_t top, stageUTF8Length=0;
    int32_t i, stage1Top;
    uint32_t headerLength;

    _MBCSHeader header=UCNV_MBCS_HEADER_INITIALIZER;

    stage2Length=mbcsData->stage2Top;
    if(mbcsData->omitFromU) {
        
        int32_t utf8Limit=(int32_t)mbcsData->utf8Max+1;
        uint32_t st2=0; 

        i=utf8Limit>>MBCS_STAGE_1_SHIFT;
        if((utf8Limit&((1<<MBCS_STAGE_1_SHIFT)-1))!=0 && (st2=mbcsData->stage1[i])!=0) {
            
            stage2Start=st2+((utf8Limit>>MBCS_STAGE_2_SHIFT)&MBCS_STAGE_2_BLOCK_MASK);
        } else {
            
            while(i>0 && (st2=mbcsData->stage1[--i])==0) {}
            
            stage2Start=st2+MBCS_STAGE_2_BLOCK_SIZE;
        }
        header.options|=MBCS_OPT_NO_FROM_U;
        header.fullStage2Length=stage2Length;
        stage2Length-=stage2Start;
        if(VERBOSE) {
            printf("+ omitting %lu out of %lu stage2 entries and %lu fromUBytes\n",
                    (unsigned long)stage2Start,
                    (unsigned long)mbcsData->stage2Top,
                    (unsigned long)mbcsData->stage3Top);
            printf("+ total size savings: %lu bytes\n", (unsigned long)stage2Start*4+mbcsData->stage3Top);
        }
    } else {
        stage2Start=0;
    }

    if(staticData->unicodeMask&UCNV_HAS_SUPPLEMENTARY) {
        stage1Top=MBCS_STAGE_1_SIZE; 
    } else {
        stage1Top=0x40; 
    }

    
    if(mbcsData->ucm->states.maxCharLength==1) {
        for(i=0; i<stage1Top; ++i) {
            mbcsData->stage1[i]+=(uint16_t)stage1Top;
        }

        
        
        stage2Length=(stage2Length*2+1)&~1;

        
        mbcsData->stage3Top*=2;

        if(mbcsData->utf8Friendly) {
            header.version[2]=(uint8_t)(SBCS_UTF8_MAX>>8); 
        }
    } else {
        for(i=0; i<stage1Top; ++i) {
            mbcsData->stage1[i]+=(uint16_t)stage1Top/2; 
        }

        
        stage2Length*=4;
        

        if(mbcsData->utf8Friendly) {
            stageUTF8Length=(mbcsData->utf8Max+1)>>MBCS_UTF8_STAGE_SHIFT;
            header.version[2]=(uint8_t)(mbcsData->utf8Max>>8); 
        }

        
    }

    
    mbcsData->stage3Top=(mbcsData->stage3Top+3)&~3;

    
    if(header.options&MBCS_OPT_INCOMPATIBLE_MASK) {
        header.version[0]=5;
        if(header.options&MBCS_OPT_NO_FROM_U) {
            headerLength=10;  
        } else {
            headerLength=MBCS_HEADER_V5_MIN_LENGTH;  
        }
    } else {
        header.version[0]=4;
        headerLength=MBCS_HEADER_V4_LENGTH;  
    }
    header.version[1]=3;
    

    header.options|=(uint32_t)headerLength;

    header.countStates=mbcsData->ucm->states.countStates;
    header.countToUFallbacks=mbcsData->countToUFallbacks;

    header.offsetToUCodeUnits=
        headerLength*4+
        mbcsData->ucm->states.countStates*1024+
        mbcsData->countToUFallbacks*sizeof(_MBCSToUFallback);
    header.offsetFromUTable=
        header.offsetToUCodeUnits+
        mbcsData->ucm->states.countToUCodeUnits*2;
    header.offsetFromUBytes=
        header.offsetFromUTable+
        stage1Top*2+
        stage2Length;
    header.fromUBytesLength=mbcsData->stage3Top;

    top=header.offsetFromUBytes+stageUTF8Length*2;
    if(!(header.options&MBCS_OPT_NO_FROM_U)) {
        top+=header.fromUBytesLength;
    }

    header.flags=(uint8_t)(mbcsData->ucm->states.outputType);

    if(tableType&TABLE_EXT) {
        if(top>0xffffff) {
            fprintf(stderr, "error: offset 0x%lx to extension table exceeds 0xffffff\n", (long)top);
            return 0;
        }

        header.flags|=top<<8;
    }

    
    udata_writeBlock(pData, &header, headerLength*4);
    udata_writeBlock(pData, mbcsData->ucm->states.stateTable, header.countStates*1024);
    udata_writeBlock(pData, mbcsData->toUFallbacks, mbcsData->countToUFallbacks*sizeof(_MBCSToUFallback));
    udata_writeBlock(pData, mbcsData->unicodeCodeUnits, mbcsData->ucm->states.countToUCodeUnits*2);
    udata_writeBlock(pData, mbcsData->stage1, stage1Top*2);
    if(mbcsData->ucm->states.maxCharLength==1) {
        udata_writeBlock(pData, mbcsData->stage2Single+stage2Start, stage2Length);
    } else {
        udata_writeBlock(pData, mbcsData->stage2+stage2Start, stage2Length);
    }
    if(!(header.options&MBCS_OPT_NO_FROM_U)) {
        udata_writeBlock(pData, mbcsData->fromUBytes, mbcsData->stage3Top);
    }

    if(stageUTF8Length>0) {
        udata_writeBlock(pData, mbcsData->stageUTF8, stageUTF8Length*2);
    }

    
    return top;
}
