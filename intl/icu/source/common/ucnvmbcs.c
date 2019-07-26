









































#include "unicode/utypes.h"

#if !UCONFIG_NO_CONVERSION && !UCONFIG_NO_LEGACY_CONVERSION

#include "unicode/ucnv.h"
#include "unicode/ucnv_cb.h"
#include "unicode/udata.h"
#include "unicode/uset.h"
#include "unicode/utf8.h"
#include "unicode/utf16.h"
#include "ucnv_bld.h"
#include "ucnvmbcs.h"
#include "ucnv_ext.h"
#include "ucnv_cnv.h"
#include "cmemory.h"
#include "cstring.h"
#include "umutex.h"


#define MBCS_UNROLL_SINGLE_TO_BMP 1
#define MBCS_UNROLL_SINGLE_FROM_BMP 0










































































































































































































































































































static const UConverterImpl _SBCSUTF8Impl;
static const UConverterImpl _DBCSUTF8Impl;




#define LINEAR_18030(a, b, c, d) ((((a)*10+(b))*126L+(c))*10L+(d))

#define LINEAR_18030_BASE LINEAR_18030(0x81, 0x30, 0x81, 0x30)

#define LINEAR(x) LINEAR_18030(x>>24, (x>>16)&0xff, (x>>8)&0xff, x&0xff)










static const uint32_t
gb18030Ranges[14][4]={
    {0x10000, 0x10FFFF, LINEAR(0x90308130), LINEAR(0xE3329A35)},
    {0x9FA6, 0xD7FF, LINEAR(0x82358F33), LINEAR(0x8336C738)},
    {0x0452, 0x1E3E, LINEAR(0x8130D330), LINEAR(0x8135F436)},
    {0x1E40, 0x200F, LINEAR(0x8135F438), LINEAR(0x8136A531)},
    {0xE865, 0xF92B, LINEAR(0x8336D030), LINEAR(0x84308534)},
    {0x2643, 0x2E80, LINEAR(0x8137A839), LINEAR(0x8138FD38)},
    {0xFA2A, 0xFE2F, LINEAR(0x84309C38), LINEAR(0x84318537)},
    {0x3CE1, 0x4055, LINEAR(0x8231D438), LINEAR(0x8232AF32)},
    {0x361B, 0x3917, LINEAR(0x8230A633), LINEAR(0x8230F237)},
    {0x49B8, 0x4C76, LINEAR(0x8234A131), LINEAR(0x8234E733)},
    {0x4160, 0x4336, LINEAR(0x8232C937), LINEAR(0x8232F837)},
    {0x478E, 0x4946, LINEAR(0x8233E838), LINEAR(0x82349638)},
    {0x44D7, 0x464B, LINEAR(0x8233A339), LINEAR(0x8233C931)},
    {0xFFE6, 0xFFFF, LINEAR(0x8431A234), LINEAR(0x8431A439)}
};


#define _MBCS_OPTION_GB18030 0x8000


#define _MBCS_OPTION_KEIS 0x01000
#define _MBCS_OPTION_JEF  0x02000
#define _MBCS_OPTION_JIPS 0x04000

#define KEIS_SO_CHAR_1 0x0A
#define KEIS_SO_CHAR_2 0x42
#define KEIS_SI_CHAR_1 0x0A
#define KEIS_SI_CHAR_2 0x41

#define JEF_SO_CHAR 0x28
#define JEF_SI_CHAR 0x29

#define JIPS_SO_CHAR_1 0x1A
#define JIPS_SO_CHAR_2 0x70
#define JIPS_SI_CHAR_1 0x1A
#define JIPS_SI_CHAR_2 0x71

enum SISO_Option {
    SI,
    SO
};
typedef enum SISO_Option SISO_Option;

static int32_t getSISOBytes(SISO_Option option, uint32_t cnvOption, uint8_t *value) {
    int32_t SISOLength = 0;

    switch (option) {
        case SI:
            if ((cnvOption&_MBCS_OPTION_KEIS)!=0) {
                value[0] = KEIS_SI_CHAR_1;
                value[1] = KEIS_SI_CHAR_2;
                SISOLength = 2;
            } else if ((cnvOption&_MBCS_OPTION_JEF)!=0) {
                value[0] = JEF_SI_CHAR;
                SISOLength = 1;
            } else if ((cnvOption&_MBCS_OPTION_JIPS)!=0) {
                value[0] = JIPS_SI_CHAR_1;
                value[1] = JIPS_SI_CHAR_2;
                SISOLength = 2;
            } else {
                value[0] = UCNV_SI;
                SISOLength = 1;
            }
            break;
        case SO:
            if ((cnvOption&_MBCS_OPTION_KEIS)!=0) {
                value[0] = KEIS_SO_CHAR_1;
                value[1] = KEIS_SO_CHAR_2;
                SISOLength = 2;
            } else if ((cnvOption&_MBCS_OPTION_JEF)!=0) {
                value[0] = JEF_SO_CHAR;
                SISOLength = 1;
            } else if ((cnvOption&_MBCS_OPTION_JIPS)!=0) {
                value[0] = JIPS_SO_CHAR_1;
                value[1] = JIPS_SO_CHAR_2;
                SISOLength = 2;
            } else {
                value[0] = UCNV_SO;
                SISOLength = 1;
            }
            break;
        default:
            
            break;
    }

    return SISOLength;
}
















typedef UBool U_CALLCONV
UConverterEnumToUCallback(const void *context, uint32_t value, UChar32 codePoints[32]);


static UBool
enumToU(UConverterMBCSTable *mbcsTable, int8_t stateProps[],
        int32_t state, uint32_t offset,
        uint32_t value,
        UConverterEnumToUCallback *callback, const void *context,
        UErrorCode *pErrorCode) {
    UChar32 codePoints[32];
    const int32_t *row;
    const uint16_t *unicodeCodeUnits;
    UChar32 anyCodePoints;
    int32_t b, limit;

    row=mbcsTable->stateTable[state];
    unicodeCodeUnits=mbcsTable->unicodeCodeUnits;

    value<<=8;
    anyCodePoints=-1;  

    b=(stateProps[state]&0x38)<<2;
    if(b==0 && stateProps[state]>=0x40) {
        
        codePoints[0]=U_SENTINEL;
        b=1;
    }
    limit=((stateProps[state]&7)+1)<<5;
    while(b<limit) {
        int32_t entry=row[b];
        if(MBCS_ENTRY_IS_TRANSITION(entry)) {
            int32_t nextState=MBCS_ENTRY_TRANSITION_STATE(entry);
            if(stateProps[nextState]>=0) {
                
                if(!enumToU(
                        mbcsTable, stateProps, nextState,
                        offset+MBCS_ENTRY_TRANSITION_OFFSET(entry),
                        value|(uint32_t)b,
                        callback, context,
                        pErrorCode)) {
                    return FALSE;
                }
            }
            codePoints[b&0x1f]=U_SENTINEL;
        } else {
            UChar32 c;
            int32_t action;

            



            action=MBCS_ENTRY_FINAL_ACTION(entry);
            if(action==MBCS_STATE_VALID_DIRECT_16) {
                
                c=(UChar)MBCS_ENTRY_FINAL_VALUE_16(entry);
            } else if(action==MBCS_STATE_VALID_16) {
                int32_t finalOffset=offset+MBCS_ENTRY_FINAL_VALUE_16(entry);
                c=unicodeCodeUnits[finalOffset];
                if(c<0xfffe) {
                    
                } else {
                    c=U_SENTINEL;
                }
            } else if(action==MBCS_STATE_VALID_16_PAIR) {
                int32_t finalOffset=offset+MBCS_ENTRY_FINAL_VALUE_16(entry);
                c=unicodeCodeUnits[finalOffset++];
                if(c<0xd800) {
                    
                } else if(c<=0xdbff) {
                    
                    c=((c&0x3ff)<<10)+unicodeCodeUnits[finalOffset]+(0x10000-0xdc00);
                } else if(c==0xe000) {
                    
                    c=unicodeCodeUnits[finalOffset];
                } else {
                    c=U_SENTINEL;
                }
            } else if(action==MBCS_STATE_VALID_DIRECT_20) {
                
                c=(UChar32)(MBCS_ENTRY_FINAL_VALUE(entry)+0x10000);
            } else {
                c=U_SENTINEL;
            }

            codePoints[b&0x1f]=c;
            anyCodePoints&=c;
        }
        if(((++b)&0x1f)==0) {
            if(anyCodePoints>=0) {
                if(!callback(context, value|(uint32_t)(b-0x20), codePoints)) {
                    return FALSE;
                }
                anyCodePoints=-1;
            }
        }
    }
    return TRUE;
}






static int8_t
getStateProp(const int32_t (*stateTable)[256], int8_t stateProps[], int state) {
    const int32_t *row;
    int32_t min, max, entry, nextState;

    row=stateTable[state];
    stateProps[state]=0;

    
    for(min=0;; ++min) {
        entry=row[min];
        nextState=MBCS_ENTRY_STATE(entry);
        if(stateProps[nextState]==-1) {
            getStateProp(stateTable, stateProps, nextState);
        }
        if(MBCS_ENTRY_IS_TRANSITION(entry)) {
            if(stateProps[nextState]>=0) {
                break;
            }
        } else if(MBCS_ENTRY_FINAL_ACTION(entry)<MBCS_STATE_UNASSIGNED) {
            break;
        }
        if(min==0xff) {
            stateProps[state]=-0x40;  
            return stateProps[state];
        }
    }
    stateProps[state]|=(int8_t)((min>>5)<<3);

    
    for(max=0xff; min<max; --max) {
        entry=row[max];
        nextState=MBCS_ENTRY_STATE(entry);
        if(stateProps[nextState]==-1) {
            getStateProp(stateTable, stateProps, nextState);
        }
        if(MBCS_ENTRY_IS_TRANSITION(entry)) {
            if(stateProps[nextState]>=0) {
                break;
            }
        } else if(MBCS_ENTRY_FINAL_ACTION(entry)<MBCS_STATE_UNASSIGNED) {
            break;
        }
    }
    stateProps[state]|=(int8_t)(max>>5);

    
    while(min<=max) {
        entry=row[min];
        nextState=MBCS_ENTRY_STATE(entry);
        if(stateProps[nextState]==-1) {
            getStateProp(stateTable, stateProps, nextState);
        }
        if(MBCS_ENTRY_IS_FINAL(entry)) {
            stateProps[nextState]|=0x40;
            if(MBCS_ENTRY_FINAL_ACTION(entry)<=MBCS_STATE_FALLBACK_DIRECT_20) {
                stateProps[state]|=0x40;
            }
        }
        ++min;
    }
    return stateProps[state];
}









static void
ucnv_MBCSEnumToUnicode(UConverterMBCSTable *mbcsTable,
                       UConverterEnumToUCallback *callback, const void *context,
                       UErrorCode *pErrorCode) {
    

















    int8_t stateProps[MBCS_MAX_STATE_COUNT];
    int32_t state;

    uprv_memset(stateProps, -1, sizeof(stateProps));

    
    getStateProp(mbcsTable->stateTable, stateProps, 0);

    for(state=0; state<mbcsTable->countStates; ++state) {
        


        if(stateProps[state]>=0x40) {
            
            enumToU(
                mbcsTable, stateProps, state, 0, 0,
                callback, context,
                pErrorCode);
        }
    }
}

U_CFUNC void
ucnv_MBCSGetFilteredUnicodeSetForUnicode(const UConverterSharedData *sharedData,
                                         const USetAdder *sa,
                                         UConverterUnicodeSet which,
                                         UConverterSetFilter filter,
                                         UErrorCode *pErrorCode) {
    const UConverterMBCSTable *mbcsTable;
    const uint16_t *table;

    uint32_t st3;
    uint16_t st1, maxStage1, st2;

    UChar32 c;

    
    mbcsTable=&sharedData->mbcs;
    table=mbcsTable->fromUnicodeTable;
    if(mbcsTable->unicodeMask&UCNV_HAS_SUPPLEMENTARY) {
        maxStage1=0x440;
    } else {
        maxStage1=0x40;
    }

    c=0; 

    if(mbcsTable->outputType==MBCS_OUTPUT_1) {
        const uint16_t *stage2, *stage3, *results;
        uint16_t minValue;

        results=(const uint16_t *)mbcsTable->fromUnicodeBytes;

        




        if(which==UCNV_ROUNDTRIP_SET) {
            
            minValue=0xf00;
        } else  {
            
            minValue=0x800;
        }

        for(st1=0; st1<maxStage1; ++st1) {
            st2=table[st1];
            if(st2>maxStage1) {
                stage2=table+st2;
                for(st2=0; st2<64; ++st2) {
                    if((st3=stage2[st2])!=0) {
                        
                        stage3=results+st3;

                        do {
                            if(*stage3++>=minValue) {
                                sa->add(sa->set, c);
                            }
                        } while((++c&0xf)!=0);
                    } else {
                        c+=16; 
                    }
                }
            } else {
                c+=1024; 
            }
        }
    } else {
        const uint32_t *stage2;
        const uint8_t *stage3, *bytes;
        uint32_t st3Multiplier;
        uint32_t value;
        UBool useFallback;

        bytes=mbcsTable->fromUnicodeBytes;

        useFallback=(UBool)(which==UCNV_ROUNDTRIP_AND_FALLBACK_SET);

        switch(mbcsTable->outputType) {
        case MBCS_OUTPUT_3:
        case MBCS_OUTPUT_4_EUC:
            st3Multiplier=3;
            break;
        case MBCS_OUTPUT_4:
            st3Multiplier=4;
            break;
        default:
            st3Multiplier=2;
            break;
        }

        for(st1=0; st1<maxStage1; ++st1) {
            st2=table[st1];
            if(st2>(maxStage1>>1)) {
                stage2=(const uint32_t *)table+st2;
                for(st2=0; st2<64; ++st2) {
                    if((st3=stage2[st2])!=0) {
                        
                        stage3=bytes+st3Multiplier*16*(uint32_t)(uint16_t)st3;

                        
                        st3>>=16;

                        




                        switch(filter) {
                        case UCNV_SET_FILTER_NONE:
                            do {
                                if(st3&1) {
                                    sa->add(sa->set, c);
                                    stage3+=st3Multiplier;
                                } else if(useFallback) {
                                    uint8_t b=0;
                                    switch(st3Multiplier) {
                                    case 4:
                                        b|=*stage3++;
                                    case 3: 
                                        b|=*stage3++;
                                    case 2: 
                                        b|=stage3[0]|stage3[1];
                                        stage3+=2;
                                    default:
                                        break;
                                    }
                                    if(b!=0) {
                                        sa->add(sa->set, c);
                                    }
                                }
                                st3>>=1;
                            } while((++c&0xf)!=0);
                            break;
                        case UCNV_SET_FILTER_DBCS_ONLY:
                             
                            do {
                                if(((st3&1)!=0 || useFallback) && *((const uint16_t *)stage3)>=0x100) {
                                    sa->add(sa->set, c);
                                }
                                st3>>=1;
                                stage3+=2;  
                            } while((++c&0xf)!=0);
                            break;
                        case UCNV_SET_FILTER_2022_CN:
                             
                            do {
                                if(((st3&1)!=0 || useFallback) && ((value=*stage3)==0x81 || value==0x82)) {
                                    sa->add(sa->set, c);
                                }
                                st3>>=1;
                                stage3+=3;  
                            } while((++c&0xf)!=0);
                            break;
                        case UCNV_SET_FILTER_SJIS:
                             
                            do {
                                if(((st3&1)!=0 || useFallback) && (value=*((const uint16_t *)stage3))>=0x8140 && value<=0xeffc) {
                                    sa->add(sa->set, c);
                                }
                                st3>>=1;
                                stage3+=2;  
                            } while((++c&0xf)!=0);
                            break;
                        case UCNV_SET_FILTER_GR94DBCS:
                            
                            do {
                                if( ((st3&1)!=0 || useFallback) &&
                                    (uint16_t)((value=*((const uint16_t *)stage3)) - 0xa1a1)<=(0xfefe - 0xa1a1) &&
                                    (uint8_t)(value-0xa1)<=(0xfe - 0xa1)
                                ) {
                                    sa->add(sa->set, c);
                                }
                                st3>>=1;
                                stage3+=2;  
                            } while((++c&0xf)!=0);
                            break;
                        case UCNV_SET_FILTER_HZ:
                            
                            do {
                                if( ((st3&1)!=0 || useFallback) &&
                                    (uint16_t)((value=*((const uint16_t *)stage3))-0xa1a1)<=(0xfdfe - 0xa1a1) &&
                                    (uint8_t)(value-0xa1)<=(0xfe - 0xa1)
                                ) {
                                    sa->add(sa->set, c);
                                }
                                st3>>=1;
                                stage3+=2;  
                            } while((++c&0xf)!=0);
                            break;
                        default:
                            *pErrorCode=U_INTERNAL_PROGRAM_ERROR;
                            return;
                        }
                    } else {
                        c+=16; 
                    }
                }
            } else {
                c+=1024; 
            }
        }
    }

    ucnv_extGetUnicodeSet(sharedData, sa, which, filter, pErrorCode);
}

U_CFUNC void
ucnv_MBCSGetUnicodeSetForUnicode(const UConverterSharedData *sharedData,
                                 const USetAdder *sa,
                                 UConverterUnicodeSet which,
                                 UErrorCode *pErrorCode) {
    ucnv_MBCSGetFilteredUnicodeSetForUnicode(
        sharedData, sa, which,
        sharedData->mbcs.outputType==MBCS_OUTPUT_DBCS_ONLY ?
            UCNV_SET_FILTER_DBCS_ONLY :
            UCNV_SET_FILTER_NONE,
        pErrorCode);
}

static void
ucnv_MBCSGetUnicodeSet(const UConverter *cnv,
                   const USetAdder *sa,
                   UConverterUnicodeSet which,
                   UErrorCode *pErrorCode) {
    if(cnv->options&_MBCS_OPTION_GB18030) {
        sa->addRange(sa->set, 0, 0xd7ff);
        sa->addRange(sa->set, 0xe000, 0x10ffff);
    } else {
        ucnv_MBCSGetUnicodeSetForUnicode(cnv->sharedData, sa, which, pErrorCode);
    }
}


















static UChar32
_extFromU(UConverter *cnv, const UConverterSharedData *sharedData,
          UChar32 cp,
          const UChar **source, const UChar *sourceLimit,
          uint8_t **target, const uint8_t *targetLimit,
          int32_t **offsets, int32_t sourceIndex,
          UBool flush,
          UErrorCode *pErrorCode) {
    const int32_t *cx;

    cnv->useSubChar1=FALSE;

    if( (cx=sharedData->mbcs.extIndexes)!=NULL &&
        ucnv_extInitialMatchFromU(
            cnv, cx,
            cp, source, sourceLimit,
            (char **)target, (char *)targetLimit,
            offsets, sourceIndex,
            flush,
            pErrorCode)
    ) {
        return 0; 
    }

    
    if((cnv->options&_MBCS_OPTION_GB18030)!=0) {
        const uint32_t *range;
        int32_t i;

        range=gb18030Ranges[0];
        for(i=0; i<sizeof(gb18030Ranges)/sizeof(gb18030Ranges[0]); range+=4, ++i) {
            if(range[0]<=(uint32_t)cp && (uint32_t)cp<=range[1]) {
                
                uint32_t linear;
                char bytes[4];

                
                linear=range[2]-LINEAR_18030_BASE;

                
                linear+=((uint32_t)cp-range[0]);

                
                bytes[3]=(char)(0x30+linear%10); linear/=10;
                bytes[2]=(char)(0x81+linear%126); linear/=126;
                bytes[1]=(char)(0x30+linear%10); linear/=10;
                bytes[0]=(char)(0x81+linear);

                
                ucnv_fromUWriteBytes(cnv,
                                     bytes, 4, (char **)target, (char *)targetLimit,
                                     offsets, sourceIndex, pErrorCode);
                return 0;
            }
        }
    }

    
    *pErrorCode=U_INVALID_CHAR_FOUND;
    return cp;
}






static int8_t
_extToU(UConverter *cnv, const UConverterSharedData *sharedData,
        int8_t length,
        const uint8_t **source, const uint8_t *sourceLimit,
        UChar **target, const UChar *targetLimit,
        int32_t **offsets, int32_t sourceIndex,
        UBool flush,
        UErrorCode *pErrorCode) {
    const int32_t *cx;

    if( (cx=sharedData->mbcs.extIndexes)!=NULL &&
        ucnv_extInitialMatchToU(
            cnv, cx,
            length, (const char **)source, (const char *)sourceLimit,
            target, targetLimit,
            offsets, sourceIndex,
            flush,
            pErrorCode)
    ) {
        return 0; 
    }

    
    if(length==4 && (cnv->options&_MBCS_OPTION_GB18030)!=0) {
        const uint32_t *range;
        uint32_t linear;
        int32_t i;

        linear=LINEAR_18030(cnv->toUBytes[0], cnv->toUBytes[1], cnv->toUBytes[2], cnv->toUBytes[3]);
        range=gb18030Ranges[0];
        for(i=0; i<sizeof(gb18030Ranges)/sizeof(gb18030Ranges[0]); range+=4, ++i) {
            if(range[2]<=linear && linear<=range[3]) {
                
                *pErrorCode=U_ZERO_ERROR;

                
                linear=range[0]+(linear-range[2]);

                
                ucnv_toUWriteCodePoint(cnv, linear, target, targetLimit, offsets, sourceIndex, pErrorCode);

                return 0;
            }
        }
    }

    
    *pErrorCode=U_INVALID_CHAR_FOUND;
    return length;
}

























#define EBCDIC_LF 0x25
#define EBCDIC_NL 0x15


#define EBCDIC_RT_LF 0xf25
#define EBCDIC_RT_NL 0xf15


#define U_LF 0x0a
#define U_NL 0x85

static UBool
_EBCDICSwapLFNL(UConverterSharedData *sharedData, UErrorCode *pErrorCode) {
    UConverterMBCSTable *mbcsTable;

    const uint16_t *table, *results;
    const uint8_t *bytes;

    int32_t (*newStateTable)[256];
    uint16_t *newResults;
    uint8_t *p;
    char *name;

    uint32_t stage2Entry;
    uint32_t size, sizeofFromUBytes;

    mbcsTable=&sharedData->mbcs;

    table=mbcsTable->fromUnicodeTable;
    bytes=mbcsTable->fromUnicodeBytes;
    results=(const uint16_t *)bytes;

    





    if(!(
         (mbcsTable->outputType==MBCS_OUTPUT_1 || mbcsTable->outputType==MBCS_OUTPUT_2_SISO) &&
         mbcsTable->stateTable[0][EBCDIC_LF]==MBCS_ENTRY_FINAL(0, MBCS_STATE_VALID_DIRECT_16, U_LF) &&
         mbcsTable->stateTable[0][EBCDIC_NL]==MBCS_ENTRY_FINAL(0, MBCS_STATE_VALID_DIRECT_16, U_NL)
    )) {
        return FALSE;
    }

    if(mbcsTable->outputType==MBCS_OUTPUT_1) {
        if(!(
             EBCDIC_RT_LF==MBCS_SINGLE_RESULT_FROM_U(table, results, U_LF) &&
             EBCDIC_RT_NL==MBCS_SINGLE_RESULT_FROM_U(table, results, U_NL)
        )) {
            return FALSE;
        }
    } else  {
        stage2Entry=MBCS_STAGE_2_FROM_U(table, U_LF);
        if(!(
             MBCS_FROM_U_IS_ROUNDTRIP(stage2Entry, U_LF)!=0 &&
             EBCDIC_LF==MBCS_VALUE_2_FROM_STAGE_2(bytes, stage2Entry, U_LF)
        )) {
            return FALSE;
        }

        stage2Entry=MBCS_STAGE_2_FROM_U(table, U_NL);
        if(!(
             MBCS_FROM_U_IS_ROUNDTRIP(stage2Entry, U_NL)!=0 &&
             EBCDIC_NL==MBCS_VALUE_2_FROM_STAGE_2(bytes, stage2Entry, U_NL)
        )) {
            return FALSE;
        }
    }

    if(mbcsTable->fromUBytesLength>0) {
        



        sizeofFromUBytes=mbcsTable->fromUBytesLength;
    } else {
        











        *pErrorCode=U_INVALID_FORMAT_ERROR;
        return FALSE;
    }

    






    size=
        mbcsTable->countStates*1024+
        sizeofFromUBytes+
        UCNV_MAX_CONVERTER_NAME_LENGTH+20;
    p=(uint8_t *)uprv_malloc(size);
    if(p==NULL) {
        *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
        return FALSE;
    }

    
    newStateTable=(int32_t (*)[256])p;
    uprv_memcpy(newStateTable, mbcsTable->stateTable, mbcsTable->countStates*1024);

    newStateTable[0][EBCDIC_LF]=MBCS_ENTRY_FINAL(0, MBCS_STATE_VALID_DIRECT_16, U_NL);
    newStateTable[0][EBCDIC_NL]=MBCS_ENTRY_FINAL(0, MBCS_STATE_VALID_DIRECT_16, U_LF);

    
    newResults=(uint16_t *)newStateTable[mbcsTable->countStates];
    uprv_memcpy(newResults, bytes, sizeofFromUBytes);

    
    if(mbcsTable->outputType==MBCS_OUTPUT_1) {
        MBCS_SINGLE_RESULT_FROM_U(table, newResults, U_LF)=EBCDIC_RT_NL;
        MBCS_SINGLE_RESULT_FROM_U(table, newResults, U_NL)=EBCDIC_RT_LF;
    } else  {
        stage2Entry=MBCS_STAGE_2_FROM_U(table, U_LF);
        MBCS_VALUE_2_FROM_STAGE_2(newResults, stage2Entry, U_LF)=EBCDIC_NL;

        stage2Entry=MBCS_STAGE_2_FROM_U(table, U_NL);
        MBCS_VALUE_2_FROM_STAGE_2(newResults, stage2Entry, U_NL)=EBCDIC_LF;
    }

    
    name=(char *)newResults+sizeofFromUBytes;
    uprv_strcpy(name, sharedData->staticData->name);
    uprv_strcat(name, UCNV_SWAP_LFNL_OPTION_STRING);

    
    umtx_lock(NULL);
    if(mbcsTable->swapLFNLStateTable==NULL) {
        mbcsTable->swapLFNLStateTable=newStateTable;
        mbcsTable->swapLFNLFromUnicodeBytes=(uint8_t *)newResults;
        mbcsTable->swapLFNLName=name;

        newStateTable=NULL;
    }
    umtx_unlock(NULL);

    
    if(newStateTable!=NULL) {
        uprv_free(newStateTable);
    }
    return TRUE;
}




static UBool U_CALLCONV
writeStage3Roundtrip(const void *context, uint32_t value, UChar32 codePoints[32]) {
    UConverterMBCSTable *mbcsTable=(UConverterMBCSTable *)context;
    const uint16_t *table;
    uint32_t *stage2;
    uint8_t *bytes, *p;
    UChar32 c;
    int32_t i, st3;

    table=mbcsTable->fromUnicodeTable;
    bytes=(uint8_t *)mbcsTable->fromUnicodeBytes;

    
    switch(mbcsTable->outputType) {
    case MBCS_OUTPUT_3_EUC:
        if(value<=0xffff) {
            
            
        } else if(value<=0x8effff) {
            
            value&=0x7fff;
        } else  {
            
            value&=0xff7f;
        }
        break;
    case MBCS_OUTPUT_4_EUC:
        if(value<=0xffffff) {
            
            
        } else if(value<=0x8effffff) {
            
            value&=0x7fffff;
        } else  {
            
            value&=0xff7fff;
        }
        break;
    default:
        break;
    }

    for(i=0; i<=0x1f; ++value, ++i) {
        c=codePoints[i];
        if(c<0) {
            continue;
        }

        
        stage2=((uint32_t *)table)+table[c>>10]+((c>>4)&0x3f);
        p=bytes;
        st3=(int32_t)(uint16_t)*stage2*16+(c&0xf);

        
        switch(mbcsTable->outputType) {
        case MBCS_OUTPUT_3:
        case MBCS_OUTPUT_4_EUC:
            p+=st3*3;
            p[0]=(uint8_t)(value>>16);
            p[1]=(uint8_t)(value>>8);
            p[2]=(uint8_t)value;
            break;
        case MBCS_OUTPUT_4:
            ((uint32_t *)p)[st3]=value;
            break;
        default:
            
            ((uint16_t *)p)[st3]=(uint16_t)value;
            break;
        }

        
        *stage2|=(1UL<<(16+(c&0xf)));
    }
    return TRUE;
 }

static void
reconstituteData(UConverterMBCSTable *mbcsTable,
                 uint32_t stage1Length, uint32_t stage2Length,
                 uint32_t fullStage2Length,  
                 UErrorCode *pErrorCode) {
    uint16_t *stage1;
    uint32_t *stage2;
    uint32_t dataLength=stage1Length*2+fullStage2Length*4+mbcsTable->fromUBytesLength;
    mbcsTable->reconstitutedData=(uint8_t *)uprv_malloc(dataLength);
    if(mbcsTable->reconstitutedData==NULL) {
        *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    uprv_memset(mbcsTable->reconstitutedData, 0, dataLength);

    
    stage1=(uint16_t *)mbcsTable->reconstitutedData;
    uprv_memcpy(stage1, mbcsTable->fromUnicodeTable, stage1Length*2);

    stage2=(uint32_t *)(stage1+stage1Length);
    uprv_memcpy(stage2+(fullStage2Length-stage2Length),
                mbcsTable->fromUnicodeTable+stage1Length,
                stage2Length*4);

    mbcsTable->fromUnicodeTable=stage1;
    mbcsTable->fromUnicodeBytes=(uint8_t *)(stage2+fullStage2Length);

    
    stage2=(uint32_t *)stage1;

    
    {
        int32_t stageUTF8Length=((int32_t)mbcsTable->maxFastUChar+1)>>6;
        int32_t stageUTF8Index=0;
        int32_t st1, st2, st3, i;

        for(st1=0; stageUTF8Index<stageUTF8Length; ++st1) {
            st2=stage1[st1];
            if(st2!=stage1Length/2) {
                
                for(i=0; i<16; ++i) {
                    st3=mbcsTable->mbcsIndex[stageUTF8Index++];
                    if(st3!=0) {
                        
                        st3>>=4;
                        



                        stage2[st2++]=st3++;
                        stage2[st2++]=st3++;
                        stage2[st2++]=st3++;
                        stage2[st2++]=st3;
                    } else {
                        
                        st2+=4;
                    }
                }
            } else {
                
                stageUTF8Index+=16;
            }
        }
    }

    
    ucnv_MBCSEnumToUnicode(mbcsTable, writeStage3Roundtrip, mbcsTable, pErrorCode);
}



static void
ucnv_MBCSLoad(UConverterSharedData *sharedData,
          UConverterLoadArgs *pArgs,
          const uint8_t *raw,
          UErrorCode *pErrorCode) {
    UDataInfo info;
    UConverterMBCSTable *mbcsTable=&sharedData->mbcs;
    _MBCSHeader *header=(_MBCSHeader *)raw;
    uint32_t offset;
    uint32_t headerLength;
    UBool noFromU=FALSE;

    if(header->version[0]==4) {
        headerLength=MBCS_HEADER_V4_LENGTH;
    } else if(header->version[0]==5 && header->version[1]>=3 &&
              (header->options&MBCS_OPT_UNKNOWN_INCOMPATIBLE_MASK)==0) {
        headerLength=header->options&MBCS_OPT_LENGTH_MASK;
        noFromU=(UBool)((header->options&MBCS_OPT_NO_FROM_U)!=0);
    } else {
        *pErrorCode=U_INVALID_TABLE_FORMAT;
        return;
    }

    mbcsTable->outputType=(uint8_t)header->flags;
    if(noFromU && mbcsTable->outputType==MBCS_OUTPUT_1) {
        *pErrorCode=U_INVALID_TABLE_FORMAT;
        return;
    }

    
    offset=header->flags>>8;
    if(offset!=0) {
        mbcsTable->extIndexes=(const int32_t *)(raw+offset);
    }

    if(mbcsTable->outputType==MBCS_OUTPUT_EXT_ONLY) {
        UConverterLoadArgs args={ 0 };
        UConverterSharedData *baseSharedData;
        const int32_t *extIndexes;
        const char *baseName;

        
        if((extIndexes=mbcsTable->extIndexes)==NULL) {
            
            *pErrorCode=U_INVALID_TABLE_FORMAT;
            return;
        }

        if(pArgs->nestedLoads!=1) {
            
            *pErrorCode=U_INVALID_TABLE_FILE;
            return;
        }

        
        baseName=(const char *)header+headerLength*4;
        if(0==uprv_strcmp(baseName, sharedData->staticData->name)) {
            
            *pErrorCode=U_INVALID_TABLE_FORMAT;
            return;
        }

        
        args.size=sizeof(UConverterLoadArgs);
        args.nestedLoads=2;
        args.onlyTestIsLoadable=pArgs->onlyTestIsLoadable;
        args.reserved=pArgs->reserved;
        args.options=pArgs->options;
        args.pkg=pArgs->pkg;
        args.name=baseName;
        baseSharedData=ucnv_load(&args, pErrorCode);
        if(U_FAILURE(*pErrorCode)) {
            return;
        }
        if( baseSharedData->staticData->conversionType!=UCNV_MBCS ||
            baseSharedData->mbcs.baseSharedData!=NULL
        ) {
            ucnv_unload(baseSharedData);
            *pErrorCode=U_INVALID_TABLE_FORMAT;
            return;
        }
        if(pArgs->onlyTestIsLoadable) {
            





            ucnv_unload(baseSharedData);
            return;
        }

        
        uprv_memcpy(mbcsTable, &baseSharedData->mbcs, sizeof(UConverterMBCSTable));

        
        mbcsTable->baseSharedData=baseSharedData;
        mbcsTable->extIndexes=extIndexes;

        






        mbcsTable->swapLFNLStateTable=NULL;
        mbcsTable->swapLFNLFromUnicodeBytes=NULL;
        mbcsTable->swapLFNLName=NULL;

        



        mbcsTable->reconstitutedData=NULL;

        



        if( sharedData->staticData->conversionType==UCNV_DBCS ||
                (sharedData->staticData->conversionType==UCNV_MBCS &&
                 sharedData->staticData->minBytesPerChar>=2)
        ) {
            if(baseSharedData->mbcs.outputType==MBCS_OUTPUT_2_SISO) {
                
                int32_t entry;

                
                entry=mbcsTable->stateTable[0][0xe];
                if( MBCS_ENTRY_IS_FINAL(entry) &&
                    MBCS_ENTRY_FINAL_ACTION(entry)==MBCS_STATE_CHANGE_ONLY &&
                    MBCS_ENTRY_FINAL_STATE(entry)!=0
                ) {
                    mbcsTable->dbcsOnlyState=(uint8_t)MBCS_ENTRY_FINAL_STATE(entry);

                    mbcsTable->outputType=MBCS_OUTPUT_DBCS_ONLY;
                }
            } else if(
                baseSharedData->staticData->conversionType==UCNV_MBCS &&
                baseSharedData->staticData->minBytesPerChar==1 &&
                baseSharedData->staticData->maxBytesPerChar==2 &&
                mbcsTable->countStates<=127
            ) {
                
                int32_t (*newStateTable)[256];
                int32_t *state;
                int32_t i, count;

                
                count=mbcsTable->countStates;
                newStateTable=(int32_t (*)[256])uprv_malloc((count+1)*1024);
                if(newStateTable==NULL) {
                    ucnv_unload(baseSharedData);
                    *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
                    return;
                }

                uprv_memcpy(newStateTable, mbcsTable->stateTable, count*1024);

                
                state=newStateTable[0];
                for(i=0; i<256; ++i) {
                    if(MBCS_ENTRY_IS_FINAL(state[i])) {
                        state[i]=MBCS_ENTRY_TRANSITION(count, 0);
                    }
                }

                
                state=newStateTable[count];
                for(i=0; i<256; ++i) {
                    state[i]=MBCS_ENTRY_FINAL(0, MBCS_STATE_ILLEGAL, 0);
                }
                mbcsTable->stateTable=(const int32_t (*)[256])newStateTable;
                mbcsTable->countStates=(uint8_t)(count+1);
                mbcsTable->stateTableOwned=TRUE;

                mbcsTable->outputType=MBCS_OUTPUT_DBCS_ONLY;
            }
        }

        






    } else {
        
        
        switch(mbcsTable->outputType) {
        case MBCS_OUTPUT_1:
        case MBCS_OUTPUT_2:
        case MBCS_OUTPUT_3:
        case MBCS_OUTPUT_4:
        case MBCS_OUTPUT_3_EUC:
        case MBCS_OUTPUT_4_EUC:
        case MBCS_OUTPUT_2_SISO:
            
            break;
        default:
            *pErrorCode=U_INVALID_TABLE_FORMAT;
            return;
        }
        if(pArgs->onlyTestIsLoadable) {
            





            return;
        }

        mbcsTable->countStates=(uint8_t)header->countStates;
        mbcsTable->countToUFallbacks=header->countToUFallbacks;
        mbcsTable->stateTable=(const int32_t (*)[256])(raw+headerLength*4);
        mbcsTable->toUFallbacks=(const _MBCSToUFallback *)(mbcsTable->stateTable+header->countStates);
        mbcsTable->unicodeCodeUnits=(const uint16_t *)(raw+header->offsetToUCodeUnits);

        mbcsTable->fromUnicodeTable=(const uint16_t *)(raw+header->offsetFromUTable);
        mbcsTable->fromUnicodeBytes=(const uint8_t *)(raw+header->offsetFromUBytes);
        mbcsTable->fromUBytesLength=header->fromUBytesLength;

        



        info.size=sizeof(UDataInfo);
        udata_getInfo((UDataMemory *)sharedData->dataMemory, &info);
        if(info.formatVersion[0]>6 || (info.formatVersion[0]==6 && info.formatVersion[1]>=1)) {
            
            mbcsTable->unicodeMask=(uint8_t)(sharedData->staticData->unicodeMask&3);
        } else {
            
            mbcsTable->unicodeMask=UCNV_HAS_SUPPLEMENTARY|UCNV_HAS_SURROGATES;
        }

        







        if( header->version[1]>=3 &&
            (mbcsTable->unicodeMask&UCNV_HAS_SURROGATES)==0 &&
            (mbcsTable->countStates==1 ?
                (header->version[2]>=(SBCS_FAST_MAX>>8)) :
                (header->version[2]>=(MBCS_FAST_MAX>>8))
            )
        ) {
            mbcsTable->utf8Friendly=TRUE;

            if(mbcsTable->countStates==1) {
                




                int32_t i;
                for(i=0; i<(SBCS_FAST_LIMIT>>6); ++i) {
                    mbcsTable->sbcsIndex[i]=mbcsTable->fromUnicodeTable[mbcsTable->fromUnicodeTable[i>>4]+((i<<2)&0x3c)];
                }
                
                mbcsTable->maxFastUChar=SBCS_FAST_MAX;
            } else {
                




                mbcsTable->mbcsIndex=(const uint16_t *)
                    (mbcsTable->fromUnicodeBytes+
                     (noFromU ? 0 : mbcsTable->fromUBytesLength));
                mbcsTable->maxFastUChar=(((UChar)header->version[2])<<8)|0xff;
            }
        }

        
        {
            uint32_t asciiRoundtrips=0xffffffff;
            int32_t i;

            for(i=0; i<0x80; ++i) {
                if(mbcsTable->stateTable[0][i]!=MBCS_ENTRY_FINAL(0, MBCS_STATE_VALID_DIRECT_16, i)) {
                    asciiRoundtrips&=~((uint32_t)1<<(i>>2));
                }
            }
            mbcsTable->asciiRoundtrips=asciiRoundtrips;
        }

        if(noFromU) {
            uint32_t stage1Length=
                mbcsTable->unicodeMask&UCNV_HAS_SUPPLEMENTARY ?
                    0x440 : 0x40;
            uint32_t stage2Length=
                (header->offsetFromUBytes-header->offsetFromUTable)/4-
                stage1Length/2;
            reconstituteData(mbcsTable, stage1Length, stage2Length, header->fullStage2Length, pErrorCode);
        }
    }

    
    if(mbcsTable->utf8Friendly) {
        if(mbcsTable->countStates==1) {
            sharedData->impl=&_SBCSUTF8Impl;
        } else {
            if(mbcsTable->outputType==MBCS_OUTPUT_2) {
                sharedData->impl=&_DBCSUTF8Impl;
            }
        }
    }

    if(mbcsTable->outputType==MBCS_OUTPUT_DBCS_ONLY || mbcsTable->outputType==MBCS_OUTPUT_2_SISO) {
        



        mbcsTable->asciiRoundtrips=0;
    }
}

static void
ucnv_MBCSUnload(UConverterSharedData *sharedData) {
    UConverterMBCSTable *mbcsTable=&sharedData->mbcs;

    if(mbcsTable->swapLFNLStateTable!=NULL) {
        uprv_free(mbcsTable->swapLFNLStateTable);
    }
    if(mbcsTable->stateTableOwned) {
        uprv_free((void *)mbcsTable->stateTable);
    }
    if(mbcsTable->baseSharedData!=NULL) {
        ucnv_unload(mbcsTable->baseSharedData);
    }
    if(mbcsTable->reconstitutedData!=NULL) {
        uprv_free(mbcsTable->reconstitutedData);
    }
}

static void
ucnv_MBCSOpen(UConverter *cnv,
              UConverterLoadArgs *pArgs,
              UErrorCode *pErrorCode) {
    UConverterMBCSTable *mbcsTable;
    const int32_t *extIndexes;
    uint8_t outputType;
    int8_t maxBytesPerUChar;

    if(pArgs->onlyTestIsLoadable) {
        return;
    }

    mbcsTable=&cnv->sharedData->mbcs;
    outputType=mbcsTable->outputType;

    if(outputType==MBCS_OUTPUT_DBCS_ONLY) {
        
        cnv->options=pArgs->options&=~UCNV_OPTION_SWAP_LFNL;
    }

    if((pArgs->options&UCNV_OPTION_SWAP_LFNL)!=0) {
        
        UBool isCached;

        umtx_lock(NULL);
        isCached=mbcsTable->swapLFNLStateTable!=NULL;
        umtx_unlock(NULL);

        if(!isCached) {
            if(!_EBCDICSwapLFNL(cnv->sharedData, pErrorCode)) {
                if(U_FAILURE(*pErrorCode)) {
                    return; 
                }

                
                cnv->options=pArgs->options&=~UCNV_OPTION_SWAP_LFNL;
            }
        }
    }

    if(uprv_strstr(pArgs->name, "18030")!=NULL) {
        if(uprv_strstr(pArgs->name, "gb18030")!=NULL || uprv_strstr(pArgs->name, "GB18030")!=NULL) {
            
            cnv->options|=_MBCS_OPTION_GB18030;
        }
    } else if((uprv_strstr(pArgs->name, "KEIS")!=NULL) || (uprv_strstr(pArgs->name, "keis")!=NULL)) {
        
        cnv->options|=_MBCS_OPTION_KEIS;
    } else if((uprv_strstr(pArgs->name, "JEF")!=NULL) || (uprv_strstr(pArgs->name, "jef")!=NULL)) {
        
        cnv->options|=_MBCS_OPTION_JEF;
    } else if((uprv_strstr(pArgs->name, "JIPS")!=NULL) || (uprv_strstr(pArgs->name, "jips")!=NULL)) {
        
        cnv->options|=_MBCS_OPTION_JIPS;
    }

    
    if(outputType==MBCS_OUTPUT_2_SISO) {
        cnv->maxBytesPerUChar=3; 
    }

    extIndexes=mbcsTable->extIndexes;
    if(extIndexes!=NULL) {
        maxBytesPerUChar=(int8_t)UCNV_GET_MAX_BYTES_PER_UCHAR(extIndexes);
        if(outputType==MBCS_OUTPUT_2_SISO) {
            ++maxBytesPerUChar; 
        }

        if(maxBytesPerUChar>cnv->maxBytesPerUChar) {
            cnv->maxBytesPerUChar=maxBytesPerUChar;
        }
    }

#if 0
    




    
    cnv->toUnicodeStatus=0;     
    cnv->mode=0;                
    cnv->toULength=0;           

    
    cnv->fromUChar32=0;
    cnv->fromUnicodeStatus=1;   
#endif
}

static const char *
ucnv_MBCSGetName(const UConverter *cnv) {
    if((cnv->options&UCNV_OPTION_SWAP_LFNL)!=0 && cnv->sharedData->mbcs.swapLFNLName!=NULL) {
        return cnv->sharedData->mbcs.swapLFNLName;
    } else {
        return cnv->sharedData->staticData->name;
    }
}



static UChar32
ucnv_MBCSGetFallback(UConverterMBCSTable *mbcsTable, uint32_t offset) {
    const _MBCSToUFallback *toUFallbacks;
    uint32_t i, start, limit;

    limit=mbcsTable->countToUFallbacks;
    if(limit>0) {
        
        toUFallbacks=mbcsTable->toUFallbacks;
        start=0;
        while(start<limit-1) {
            i=(start+limit)/2;
            if(offset<toUFallbacks[i].offset) {
                limit=i;
            } else {
                start=i;
            }
        }

        
        if(offset==toUFallbacks[start].offset) {
            return toUFallbacks[start].codePoint;
        }
    }

    return 0xfffe;
}


static void
ucnv_MBCSSingleToUnicodeWithOffsets(UConverterToUnicodeArgs *pArgs,
                                UErrorCode *pErrorCode) {
    UConverter *cnv;
    const uint8_t *source, *sourceLimit;
    UChar *target;
    const UChar *targetLimit;
    int32_t *offsets;

    const int32_t (*stateTable)[256];

    int32_t sourceIndex;

    int32_t entry;
    UChar c;
    uint8_t action;

    
    cnv=pArgs->converter;
    source=(const uint8_t *)pArgs->source;
    sourceLimit=(const uint8_t *)pArgs->sourceLimit;
    target=pArgs->target;
    targetLimit=pArgs->targetLimit;
    offsets=pArgs->offsets;

    if((cnv->options&UCNV_OPTION_SWAP_LFNL)!=0) {
        stateTable=(const int32_t (*)[256])cnv->sharedData->mbcs.swapLFNLStateTable;
    } else {
        stateTable=cnv->sharedData->mbcs.stateTable;
    }

    
    sourceIndex=0;

    
    while(source<sourceLimit) {
        







        if(target>=targetLimit) {
            
            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
            break;
        }

        entry=stateTable[0][*source++];
        

        
        if(MBCS_ENTRY_FINAL_IS_VALID_DIRECT_16(entry)) {
            
            *target++=(UChar)MBCS_ENTRY_FINAL_VALUE_16(entry);
            if(offsets!=NULL) {
                *offsets++=sourceIndex;
            }

            
            ++sourceIndex;
            continue;
        }

        



        action=(uint8_t)(MBCS_ENTRY_FINAL_ACTION(entry));
        if(action==MBCS_STATE_VALID_DIRECT_20 ||
           (action==MBCS_STATE_FALLBACK_DIRECT_20 && UCNV_TO_U_USE_FALLBACK(cnv))
        ) {
            entry=MBCS_ENTRY_FINAL_VALUE(entry);
            
            *target++=(UChar)(0xd800|(UChar)(entry>>10));
            if(offsets!=NULL) {
                *offsets++=sourceIndex;
            }
            c=(UChar)(0xdc00|(UChar)(entry&0x3ff));
            if(target<targetLimit) {
                *target++=c;
                if(offsets!=NULL) {
                    *offsets++=sourceIndex;
                }
            } else {
                
                cnv->UCharErrorBuffer[0]=c;
                cnv->UCharErrorBufferLength=1;
                *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                break;
            }

            ++sourceIndex;
            continue;
        } else if(action==MBCS_STATE_FALLBACK_DIRECT_16) {
            if(UCNV_TO_U_USE_FALLBACK(cnv)) {
                
                *target++=(UChar)MBCS_ENTRY_FINAL_VALUE_16(entry);
                if(offsets!=NULL) {
                    *offsets++=sourceIndex;
                }

                ++sourceIndex;
                continue;
            }
        } else if(action==MBCS_STATE_UNASSIGNED) {
            
        } else if(action==MBCS_STATE_ILLEGAL) {
            
            *pErrorCode=U_ILLEGAL_CHAR_FOUND;
        } else {
            
            ++sourceIndex;
            continue;
        }

        if(U_FAILURE(*pErrorCode)) {
            
            break;
        } else  {
            
            pArgs->source=(const char *)source;
            cnv->toUBytes[0]=*(source-1);
            cnv->toULength=_extToU(cnv, cnv->sharedData,
                                    1, &source, sourceLimit,
                                    &target, targetLimit,
                                    &offsets, sourceIndex,
                                    pArgs->flush,
                                    pErrorCode);
            sourceIndex+=1+(int32_t)(source-(const uint8_t *)pArgs->source);

            if(U_FAILURE(*pErrorCode)) {
                
                break;
            }
        }
    }

    
    pArgs->source=(const char *)source;
    pArgs->target=target;
    pArgs->offsets=offsets;
}







static void
ucnv_MBCSSingleToBMPWithOffsets(UConverterToUnicodeArgs *pArgs,
                            UErrorCode *pErrorCode) {
    UConverter *cnv;
    const uint8_t *source, *sourceLimit, *lastSource;
    UChar *target;
    int32_t targetCapacity, length;
    int32_t *offsets;

    const int32_t (*stateTable)[256];

    int32_t sourceIndex;

    int32_t entry;
    uint8_t action;

    
    cnv=pArgs->converter;
    source=(const uint8_t *)pArgs->source;
    sourceLimit=(const uint8_t *)pArgs->sourceLimit;
    target=pArgs->target;
    targetCapacity=(int32_t)(pArgs->targetLimit-pArgs->target);
    offsets=pArgs->offsets;

    if((cnv->options&UCNV_OPTION_SWAP_LFNL)!=0) {
        stateTable=(const int32_t (*)[256])cnv->sharedData->mbcs.swapLFNLStateTable;
    } else {
        stateTable=cnv->sharedData->mbcs.stateTable;
    }

    
    sourceIndex=0;
    lastSource=source;

    



    length=(int32_t)(sourceLimit-source);
    if(length<targetCapacity) {
        targetCapacity=length;
    }

#if MBCS_UNROLL_SINGLE_TO_BMP
    
    
unrolled:
    if(targetCapacity>=16) {
        int32_t count, loops, oredEntries;

        loops=count=targetCapacity>>4;
        do {
            oredEntries=entry=stateTable[0][*source++];
            *target++=(UChar)MBCS_ENTRY_FINAL_VALUE_16(entry);
            oredEntries|=entry=stateTable[0][*source++];
            *target++=(UChar)MBCS_ENTRY_FINAL_VALUE_16(entry);
            oredEntries|=entry=stateTable[0][*source++];
            *target++=(UChar)MBCS_ENTRY_FINAL_VALUE_16(entry);
            oredEntries|=entry=stateTable[0][*source++];
            *target++=(UChar)MBCS_ENTRY_FINAL_VALUE_16(entry);
            oredEntries|=entry=stateTable[0][*source++];
            *target++=(UChar)MBCS_ENTRY_FINAL_VALUE_16(entry);
            oredEntries|=entry=stateTable[0][*source++];
            *target++=(UChar)MBCS_ENTRY_FINAL_VALUE_16(entry);
            oredEntries|=entry=stateTable[0][*source++];
            *target++=(UChar)MBCS_ENTRY_FINAL_VALUE_16(entry);
            oredEntries|=entry=stateTable[0][*source++];
            *target++=(UChar)MBCS_ENTRY_FINAL_VALUE_16(entry);
            oredEntries|=entry=stateTable[0][*source++];
            *target++=(UChar)MBCS_ENTRY_FINAL_VALUE_16(entry);
            oredEntries|=entry=stateTable[0][*source++];
            *target++=(UChar)MBCS_ENTRY_FINAL_VALUE_16(entry);
            oredEntries|=entry=stateTable[0][*source++];
            *target++=(UChar)MBCS_ENTRY_FINAL_VALUE_16(entry);
            oredEntries|=entry=stateTable[0][*source++];
            *target++=(UChar)MBCS_ENTRY_FINAL_VALUE_16(entry);
            oredEntries|=entry=stateTable[0][*source++];
            *target++=(UChar)MBCS_ENTRY_FINAL_VALUE_16(entry);
            oredEntries|=entry=stateTable[0][*source++];
            *target++=(UChar)MBCS_ENTRY_FINAL_VALUE_16(entry);
            oredEntries|=entry=stateTable[0][*source++];
            *target++=(UChar)MBCS_ENTRY_FINAL_VALUE_16(entry);
            oredEntries|=entry=stateTable[0][*source++];
            *target++=(UChar)MBCS_ENTRY_FINAL_VALUE_16(entry);

            
            if(!MBCS_ENTRY_FINAL_IS_VALID_DIRECT_16(oredEntries)) {
                
                source-=16;
                target-=16;
                break;
            }
        } while(--count>0);
        count=loops-count;
        targetCapacity-=16*count;

        if(offsets!=NULL) {
            lastSource+=16*count;
            while(count>0) {
                *offsets++=sourceIndex++;
                *offsets++=sourceIndex++;
                *offsets++=sourceIndex++;
                *offsets++=sourceIndex++;
                *offsets++=sourceIndex++;
                *offsets++=sourceIndex++;
                *offsets++=sourceIndex++;
                *offsets++=sourceIndex++;
                *offsets++=sourceIndex++;
                *offsets++=sourceIndex++;
                *offsets++=sourceIndex++;
                *offsets++=sourceIndex++;
                *offsets++=sourceIndex++;
                *offsets++=sourceIndex++;
                *offsets++=sourceIndex++;
                *offsets++=sourceIndex++;
                --count;
            }
        }
    }
#endif

    
    while(targetCapacity > 0 && source < sourceLimit) {
        entry=stateTable[0][*source++];
        

        
        if(MBCS_ENTRY_FINAL_IS_VALID_DIRECT_16(entry)) {
            
            *target++=(UChar)MBCS_ENTRY_FINAL_VALUE_16(entry);
            --targetCapacity;
            continue;
        }

        



        action=(uint8_t)(MBCS_ENTRY_FINAL_ACTION(entry));
        if(action==MBCS_STATE_FALLBACK_DIRECT_16) {
            if(UCNV_TO_U_USE_FALLBACK(cnv)) {
                
                *target++=(UChar)MBCS_ENTRY_FINAL_VALUE_16(entry);
                --targetCapacity;
                continue;
            }
        } else if(action==MBCS_STATE_UNASSIGNED) {
            
        } else if(action==MBCS_STATE_ILLEGAL) {
            
            *pErrorCode=U_ILLEGAL_CHAR_FOUND;
        } else {
            
            continue;
        }

        
        if(offsets!=NULL) {
            int32_t count=(int32_t)(source-lastSource);

            
            while(--count>0) {
                *offsets++=sourceIndex++;
            }
            
        }

        if(U_FAILURE(*pErrorCode)) {
            
            break;
        } else  {
            
            lastSource=source;
            cnv->toUBytes[0]=*(source-1);
            cnv->toULength=_extToU(cnv, cnv->sharedData,
                                    1, &source, sourceLimit,
                                    &target, pArgs->targetLimit,
                                    &offsets, sourceIndex,
                                    pArgs->flush,
                                    pErrorCode);
            sourceIndex+=1+(int32_t)(source-lastSource);

            if(U_FAILURE(*pErrorCode)) {
                
                break;
            }

            
            targetCapacity=(int32_t)(pArgs->targetLimit-target);
            length=(int32_t)(sourceLimit-source);
            if(length<targetCapacity) {
                targetCapacity=length;
            }
        }

#if MBCS_UNROLL_SINGLE_TO_BMP
        
        goto unrolled;
#endif
    }

    if(U_SUCCESS(*pErrorCode) && source<sourceLimit && target>=pArgs->targetLimit) {
        
        *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
    }

    
    if(offsets!=NULL) {
        size_t count=source-lastSource;
        while(count>0) {
            *offsets++=sourceIndex++;
            --count;
        }
    }

    
    pArgs->source=(const char *)source;
    pArgs->target=target;
    pArgs->offsets=offsets;
}

static UBool
hasValidTrailBytes(const int32_t (*stateTable)[256], uint8_t state) {
    const int32_t *row=stateTable[state];
    int32_t b, entry;
    
    entry=row[0xa1];
    if( !MBCS_ENTRY_IS_TRANSITION(entry) &&
        MBCS_ENTRY_FINAL_ACTION(entry)!=MBCS_STATE_ILLEGAL
    ) {
        return TRUE;
    }
    entry=row[0x41];
    if( !MBCS_ENTRY_IS_TRANSITION(entry) &&
        MBCS_ENTRY_FINAL_ACTION(entry)!=MBCS_STATE_ILLEGAL
    ) {
        return TRUE;
    }
    
    for(b=0; b<=0xff; ++b) {
        entry=row[b];
        if( !MBCS_ENTRY_IS_TRANSITION(entry) &&
            MBCS_ENTRY_FINAL_ACTION(entry)!=MBCS_STATE_ILLEGAL
        ) {
            return TRUE;
        }
    }
    
    for(b=0; b<=0xff; ++b) {
        entry=row[b];
        if( MBCS_ENTRY_IS_TRANSITION(entry) &&
            hasValidTrailBytes(stateTable, (uint8_t)MBCS_ENTRY_TRANSITION_STATE(entry))
        ) {
            return TRUE;
        }
    }
    return FALSE;
}






static UBool
isSingleOrLead(const int32_t (*stateTable)[256], uint8_t state, UBool isDBCSOnly, uint8_t b) {
    const int32_t *row=stateTable[state];
    int32_t entry=row[b];
    if(MBCS_ENTRY_IS_TRANSITION(entry)) {   
        return hasValidTrailBytes(stateTable, (uint8_t)MBCS_ENTRY_TRANSITION_STATE(entry));
    } else {
        uint8_t action=(uint8_t)(MBCS_ENTRY_FINAL_ACTION(entry));
        if(action==MBCS_STATE_CHANGE_ONLY && isDBCSOnly) {
            return FALSE;   
        } else {
            return action!=MBCS_STATE_ILLEGAL;
        }
    }
}

U_CFUNC void
ucnv_MBCSToUnicodeWithOffsets(UConverterToUnicodeArgs *pArgs,
                          UErrorCode *pErrorCode) {
    UConverter *cnv;
    const uint8_t *source, *sourceLimit;
    UChar *target;
    const UChar *targetLimit;
    int32_t *offsets;

    const int32_t (*stateTable)[256];
    const uint16_t *unicodeCodeUnits;

    uint32_t offset;
    uint8_t state;
    int8_t byteIndex;
    uint8_t *bytes;

    int32_t sourceIndex, nextSourceIndex;

    int32_t entry;
    UChar c;
    uint8_t action;

    
    cnv=pArgs->converter;

    if(cnv->preToULength>0) {
        



        ucnv_extContinueMatchToU(cnv, pArgs, -1, pErrorCode);

        if(U_FAILURE(*pErrorCode) || cnv->preToULength<0) {
            return;
        }
    }

    if(cnv->sharedData->mbcs.countStates==1) {
        if(!(cnv->sharedData->mbcs.unicodeMask&UCNV_HAS_SUPPLEMENTARY)) {
            ucnv_MBCSSingleToBMPWithOffsets(pArgs, pErrorCode);
        } else {
            ucnv_MBCSSingleToUnicodeWithOffsets(pArgs, pErrorCode);
        }
        return;
    }

    
    source=(const uint8_t *)pArgs->source;
    sourceLimit=(const uint8_t *)pArgs->sourceLimit;
    target=pArgs->target;
    targetLimit=pArgs->targetLimit;
    offsets=pArgs->offsets;

    if((cnv->options&UCNV_OPTION_SWAP_LFNL)!=0) {
        stateTable=(const int32_t (*)[256])cnv->sharedData->mbcs.swapLFNLStateTable;
    } else {
        stateTable=cnv->sharedData->mbcs.stateTable;
    }
    unicodeCodeUnits=cnv->sharedData->mbcs.unicodeCodeUnits;

    
    offset=cnv->toUnicodeStatus;
    byteIndex=cnv->toULength;
    bytes=cnv->toUBytes;

    




    if((state=(uint8_t)(cnv->mode))==0) {
        state=cnv->sharedData->mbcs.dbcsOnlyState;
    }

    
    sourceIndex=byteIndex==0 ? 0 : -1;
    nextSourceIndex=0;

    
    while(source<sourceLimit) {
        







        if(target>=targetLimit) {
            
            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
            break;
        }

        if(byteIndex==0) {
            
            if(offsets==NULL) {
                do {
                    entry=stateTable[state][*source];
                    if(MBCS_ENTRY_IS_TRANSITION(entry)) {
                        state=(uint8_t)MBCS_ENTRY_TRANSITION_STATE(entry);
                        offset=MBCS_ENTRY_TRANSITION_OFFSET(entry);

                        ++source;
                        if( source<sourceLimit &&
                            MBCS_ENTRY_IS_FINAL(entry=stateTable[state][*source]) &&
                            MBCS_ENTRY_FINAL_ACTION(entry)==MBCS_STATE_VALID_16 &&
                            (c=unicodeCodeUnits[offset+MBCS_ENTRY_FINAL_VALUE_16(entry)])<0xfffe
                        ) {
                            ++source;
                            *target++=c;
                            state=(uint8_t)MBCS_ENTRY_FINAL_STATE(entry); 
                            offset=0;
                        } else {
                            
                            bytes[0]=*(source-1);
                            byteIndex=1;
                            break;
                        }
                    } else {
                        if(MBCS_ENTRY_FINAL_IS_VALID_DIRECT_16(entry)) {
                            
                            ++source;
                            *target++=(UChar)MBCS_ENTRY_FINAL_VALUE_16(entry);
                            state=(uint8_t)MBCS_ENTRY_FINAL_STATE(entry); 
                        } else {
                            
                            break;
                        }
                    }
                } while(source<sourceLimit && target<targetLimit);
            } else  {
                do {
                    entry=stateTable[state][*source];
                    if(MBCS_ENTRY_IS_TRANSITION(entry)) {
                        state=(uint8_t)MBCS_ENTRY_TRANSITION_STATE(entry);
                        offset=MBCS_ENTRY_TRANSITION_OFFSET(entry);

                        ++source;
                        if( source<sourceLimit &&
                            MBCS_ENTRY_IS_FINAL(entry=stateTable[state][*source]) &&
                            MBCS_ENTRY_FINAL_ACTION(entry)==MBCS_STATE_VALID_16 &&
                            (c=unicodeCodeUnits[offset+MBCS_ENTRY_FINAL_VALUE_16(entry)])<0xfffe
                        ) {
                            ++source;
                            *target++=c;
                            if(offsets!=NULL) {
                                *offsets++=sourceIndex;
                                sourceIndex=(nextSourceIndex+=2);
                            }
                            state=(uint8_t)MBCS_ENTRY_FINAL_STATE(entry); 
                            offset=0;
                        } else {
                            
                            ++nextSourceIndex;
                            bytes[0]=*(source-1);
                            byteIndex=1;
                            break;
                        }
                    } else {
                        if(MBCS_ENTRY_FINAL_IS_VALID_DIRECT_16(entry)) {
                            
                            ++source;
                            *target++=(UChar)MBCS_ENTRY_FINAL_VALUE_16(entry);
                            if(offsets!=NULL) {
                                *offsets++=sourceIndex;
                                sourceIndex=++nextSourceIndex;
                            }
                            state=(uint8_t)MBCS_ENTRY_FINAL_STATE(entry); 
                        } else {
                            
                            break;
                        }
                    }
                } while(source<sourceLimit && target<targetLimit);
            }

            



            if(source>=sourceLimit) {
                break;
            }
            if(target>=targetLimit) {
                
                *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                break;
            }

            ++nextSourceIndex;
            bytes[byteIndex++]=*source++;
        } else  {
            ++nextSourceIndex;
            entry=stateTable[state][bytes[byteIndex++]=*source++];
        }

        if(MBCS_ENTRY_IS_TRANSITION(entry)) {
            state=(uint8_t)MBCS_ENTRY_TRANSITION_STATE(entry);
            offset+=MBCS_ENTRY_TRANSITION_OFFSET(entry);
            continue;
        }

        
        cnv->mode=state;

        
        state=(uint8_t)MBCS_ENTRY_FINAL_STATE(entry); 

        



        action=(uint8_t)(MBCS_ENTRY_FINAL_ACTION(entry));
        if(action==MBCS_STATE_VALID_16) {
            offset+=MBCS_ENTRY_FINAL_VALUE_16(entry);
            c=unicodeCodeUnits[offset];
            if(c<0xfffe) {
                
                *target++=c;
                if(offsets!=NULL) {
                    *offsets++=sourceIndex;
                }
                byteIndex=0;
            } else if(c==0xfffe) {
                if(UCNV_TO_U_USE_FALLBACK(cnv) && (entry=(int32_t)ucnv_MBCSGetFallback(&cnv->sharedData->mbcs, offset))!=0xfffe) {
                    
                    *target++=(UChar)entry;
                    if(offsets!=NULL) {
                        *offsets++=sourceIndex;
                    }
                    byteIndex=0;
                }
            } else {
                
                *pErrorCode=U_ILLEGAL_CHAR_FOUND;
            }
        } else if(action==MBCS_STATE_VALID_DIRECT_16) {
            
            *target++=(UChar)MBCS_ENTRY_FINAL_VALUE_16(entry);
            if(offsets!=NULL) {
                *offsets++=sourceIndex;
            }
            byteIndex=0;
        } else if(action==MBCS_STATE_VALID_16_PAIR) {
            offset+=MBCS_ENTRY_FINAL_VALUE_16(entry);
            c=unicodeCodeUnits[offset++];
            if(c<0xd800) {
                
                *target++=c;
                if(offsets!=NULL) {
                    *offsets++=sourceIndex;
                }
                byteIndex=0;
            } else if(UCNV_TO_U_USE_FALLBACK(cnv) ? c<=0xdfff : c<=0xdbff) {
                
                *target++=(UChar)(c&0xdbff);
                if(offsets!=NULL) {
                    *offsets++=sourceIndex;
                }
                byteIndex=0;
                if(target<targetLimit) {
                    *target++=unicodeCodeUnits[offset];
                    if(offsets!=NULL) {
                        *offsets++=sourceIndex;
                    }
                } else {
                    
                    cnv->UCharErrorBuffer[0]=unicodeCodeUnits[offset];
                    cnv->UCharErrorBufferLength=1;
                    *pErrorCode=U_BUFFER_OVERFLOW_ERROR;

                    offset=0;
                    break;
                }
            } else if(UCNV_TO_U_USE_FALLBACK(cnv) ? (c&0xfffe)==0xe000 : c==0xe000) {
                
                *target++=unicodeCodeUnits[offset];
                if(offsets!=NULL) {
                    *offsets++=sourceIndex;
                }
                byteIndex=0;
            } else if(c==0xffff) {
                
                *pErrorCode=U_ILLEGAL_CHAR_FOUND;
            }
        } else if(action==MBCS_STATE_VALID_DIRECT_20 ||
                  (action==MBCS_STATE_FALLBACK_DIRECT_20 && UCNV_TO_U_USE_FALLBACK(cnv))
        ) {
            entry=MBCS_ENTRY_FINAL_VALUE(entry);
            
            *target++=(UChar)(0xd800|(UChar)(entry>>10));
            if(offsets!=NULL) {
                *offsets++=sourceIndex;
            }
            byteIndex=0;
            c=(UChar)(0xdc00|(UChar)(entry&0x3ff));
            if(target<targetLimit) {
                *target++=c;
                if(offsets!=NULL) {
                    *offsets++=sourceIndex;
                }
            } else {
                
                cnv->UCharErrorBuffer[0]=c;
                cnv->UCharErrorBufferLength=1;
                *pErrorCode=U_BUFFER_OVERFLOW_ERROR;

                offset=0;
                break;
            }
        } else if(action==MBCS_STATE_CHANGE_ONLY) {
            






            if(cnv->sharedData->mbcs.dbcsOnlyState==0) {
                byteIndex=0;
            } else {
                
                state=(uint8_t)(cnv->mode); 

                
                *pErrorCode=U_ILLEGAL_CHAR_FOUND;
            }
        } else if(action==MBCS_STATE_FALLBACK_DIRECT_16) {
            if(UCNV_TO_U_USE_FALLBACK(cnv)) {
                
                *target++=(UChar)MBCS_ENTRY_FINAL_VALUE_16(entry);
                if(offsets!=NULL) {
                    *offsets++=sourceIndex;
                }
                byteIndex=0;
            }
        } else if(action==MBCS_STATE_UNASSIGNED) {
            
        } else if(action==MBCS_STATE_ILLEGAL) {
            
            *pErrorCode=U_ILLEGAL_CHAR_FOUND;
        } else {
            
            byteIndex=0;
        }

        
        offset=0;

        if(byteIndex==0) {
            sourceIndex=nextSourceIndex;
        } else if(U_FAILURE(*pErrorCode)) {
            
            if(byteIndex>1) {
                





                UBool isDBCSOnly=(UBool)(cnv->sharedData->mbcs.dbcsOnlyState!=0);
                int8_t i;
                for(i=1;
                    i<byteIndex && !isSingleOrLead(stateTable, state, isDBCSOnly, bytes[i]);
                    ++i) {}
                if(i<byteIndex) {
                    
                    int8_t backOutDistance=byteIndex-i;
                    int32_t bytesFromThisBuffer=(int32_t)(source-(const uint8_t *)pArgs->source);
                    byteIndex=i;  
                    if(backOutDistance<=bytesFromThisBuffer) {
                        source-=backOutDistance;
                    } else {
                        
                        cnv->preToULength=(int8_t)(bytesFromThisBuffer-backOutDistance);
                        
                        uprv_memcpy(cnv->preToU, bytes+i, -cnv->preToULength);
                        source=(const uint8_t *)pArgs->source;
                    }
                }
            }
            break;
        } else  {
            
            pArgs->source=(const char *)source;
            byteIndex=_extToU(cnv, cnv->sharedData,
                              byteIndex, &source, sourceLimit,
                              &target, targetLimit,
                              &offsets, sourceIndex,
                              pArgs->flush,
                              pErrorCode);
            sourceIndex=nextSourceIndex+=(int32_t)(source-(const uint8_t *)pArgs->source);

            if(U_FAILURE(*pErrorCode)) {
                
                break;
            }
        }
    }

    
    cnv->toUnicodeStatus=offset;
    cnv->mode=state;
    cnv->toULength=byteIndex;

    
    pArgs->source=(const char *)source;
    pArgs->target=target;
    pArgs->offsets=offsets;
}





static UChar32
ucnv_MBCSSingleGetNextUChar(UConverterToUnicodeArgs *pArgs,
                        UErrorCode *pErrorCode) {
    UConverter *cnv;
    const int32_t (*stateTable)[256];
    const uint8_t *source, *sourceLimit;

    int32_t entry;
    uint8_t action;

    
    cnv=pArgs->converter;
    source=(const uint8_t *)pArgs->source;
    sourceLimit=(const uint8_t *)pArgs->sourceLimit;
    if((cnv->options&UCNV_OPTION_SWAP_LFNL)!=0) {
        stateTable=(const int32_t (*)[256])cnv->sharedData->mbcs.swapLFNLStateTable;
    } else {
        stateTable=cnv->sharedData->mbcs.stateTable;
    }

    
    while(source<sourceLimit) {
        entry=stateTable[0][*source++];
        

        
        pArgs->source=(const char *)source;

        if(MBCS_ENTRY_FINAL_IS_VALID_DIRECT_16(entry)) {
            
            return (UChar)MBCS_ENTRY_FINAL_VALUE_16(entry);
        }

        



        action=(uint8_t)(MBCS_ENTRY_FINAL_ACTION(entry));
        if( action==MBCS_STATE_VALID_DIRECT_20 ||
            (action==MBCS_STATE_FALLBACK_DIRECT_20 && UCNV_TO_U_USE_FALLBACK(cnv))
        ) {
            
            return (UChar32)(MBCS_ENTRY_FINAL_VALUE(entry)+0x10000);
        } else if(action==MBCS_STATE_FALLBACK_DIRECT_16) {
            if(UCNV_TO_U_USE_FALLBACK(cnv)) {
                
                return (UChar)MBCS_ENTRY_FINAL_VALUE_16(entry);
            }
        } else if(action==MBCS_STATE_UNASSIGNED) {
            
        } else if(action==MBCS_STATE_ILLEGAL) {
            
            *pErrorCode=U_ILLEGAL_CHAR_FOUND;
        } else {
            
            continue;
        }

        if(U_FAILURE(*pErrorCode)) {
            
            break;
        } else  {
            
            pArgs->source=(const char *)source-1;
            return UCNV_GET_NEXT_UCHAR_USE_TO_U;
        }
    }

    
    *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
    return 0xffff;
}













static UChar32
ucnv_MBCSGetNextUChar(UConverterToUnicodeArgs *pArgs,
                  UErrorCode *pErrorCode) {
    UConverter *cnv;
    const uint8_t *source, *sourceLimit, *lastSource;

    const int32_t (*stateTable)[256];
    const uint16_t *unicodeCodeUnits;

    uint32_t offset;
    uint8_t state;

    int32_t entry;
    UChar32 c;
    uint8_t action;

    
    cnv=pArgs->converter;

    if(cnv->preToULength>0) {
        
        return UCNV_GET_NEXT_UCHAR_USE_TO_U;
    }

    if(cnv->sharedData->mbcs.unicodeMask&UCNV_HAS_SURROGATES) {
        




        return UCNV_GET_NEXT_UCHAR_USE_TO_U;
    } else if(cnv->sharedData->mbcs.countStates==1) {
        return ucnv_MBCSSingleGetNextUChar(pArgs, pErrorCode);
    }

    
    source=lastSource=(const uint8_t *)pArgs->source;
    sourceLimit=(const uint8_t *)pArgs->sourceLimit;

    if((cnv->options&UCNV_OPTION_SWAP_LFNL)!=0) {
        stateTable=(const int32_t (*)[256])cnv->sharedData->mbcs.swapLFNLStateTable;
    } else {
        stateTable=cnv->sharedData->mbcs.stateTable;
    }
    unicodeCodeUnits=cnv->sharedData->mbcs.unicodeCodeUnits;

    
    offset=cnv->toUnicodeStatus;

    




    if((state=(uint8_t)(cnv->mode))==0) {
        state=cnv->sharedData->mbcs.dbcsOnlyState;
    }

    
    c=U_SENTINEL;
    while(source<sourceLimit) {
        entry=stateTable[state][*source++];
        if(MBCS_ENTRY_IS_TRANSITION(entry)) {
            state=(uint8_t)MBCS_ENTRY_TRANSITION_STATE(entry);
            offset+=MBCS_ENTRY_TRANSITION_OFFSET(entry);

            
            if( source<sourceLimit &&
                MBCS_ENTRY_IS_FINAL(entry=stateTable[state][*source]) &&
                MBCS_ENTRY_FINAL_ACTION(entry)==MBCS_STATE_VALID_16 &&
                (c=unicodeCodeUnits[offset+MBCS_ENTRY_FINAL_VALUE_16(entry)])<0xfffe
            ) {
                ++source;
                state=(uint8_t)MBCS_ENTRY_FINAL_STATE(entry); 
                
                break;
            }
        } else {
            
            cnv->mode=state;

            
            state=(uint8_t)MBCS_ENTRY_FINAL_STATE(entry); 

            



            action=(uint8_t)(MBCS_ENTRY_FINAL_ACTION(entry));
            if(action==MBCS_STATE_VALID_DIRECT_16) {
                
                c=(UChar)MBCS_ENTRY_FINAL_VALUE_16(entry);
                break;
            } else if(action==MBCS_STATE_VALID_16) {
                offset+=MBCS_ENTRY_FINAL_VALUE_16(entry);
                c=unicodeCodeUnits[offset];
                if(c<0xfffe) {
                    
                    break;
                } else if(c==0xfffe) {
                    if(UCNV_TO_U_USE_FALLBACK(cnv) && (c=ucnv_MBCSGetFallback(&cnv->sharedData->mbcs, offset))!=0xfffe) {
                        break;
                    }
                } else {
                    
                    *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                }
            } else if(action==MBCS_STATE_VALID_16_PAIR) {
                offset+=MBCS_ENTRY_FINAL_VALUE_16(entry);
                c=unicodeCodeUnits[offset++];
                if(c<0xd800) {
                    
                    break;
                } else if(UCNV_TO_U_USE_FALLBACK(cnv) ? c<=0xdfff : c<=0xdbff) {
                    
                    c=((c&0x3ff)<<10)+unicodeCodeUnits[offset]+(0x10000-0xdc00);
                    break;
                } else if(UCNV_TO_U_USE_FALLBACK(cnv) ? (c&0xfffe)==0xe000 : c==0xe000) {
                    
                    c=unicodeCodeUnits[offset];
                    break;
                } else if(c==0xffff) {
                    
                    *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                }
            } else if(action==MBCS_STATE_VALID_DIRECT_20 ||
                      (action==MBCS_STATE_FALLBACK_DIRECT_20 && UCNV_TO_U_USE_FALLBACK(cnv))
            ) {
                
                c=(UChar32)(MBCS_ENTRY_FINAL_VALUE(entry)+0x10000);
                break;
            } else if(action==MBCS_STATE_CHANGE_ONLY) {
                






                if(cnv->sharedData->mbcs.dbcsOnlyState!=0) {
                    
                    state=(uint8_t)(cnv->mode); 

                    
                    *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                }
            } else if(action==MBCS_STATE_FALLBACK_DIRECT_16) {
                if(UCNV_TO_U_USE_FALLBACK(cnv)) {
                    
                    c=(UChar)MBCS_ENTRY_FINAL_VALUE_16(entry);
                    break;
                }
            } else if(action==MBCS_STATE_UNASSIGNED) {
                
            } else if(action==MBCS_STATE_ILLEGAL) {
                
                *pErrorCode=U_ILLEGAL_CHAR_FOUND;
            } else {
                
                offset=0;
                lastSource=source;
                continue;
            }

            
            offset=0;

            if(U_FAILURE(*pErrorCode)) {
                
                break;
            } else  {
                
                cnv->toUnicodeStatus=0;
                cnv->mode=state;
                pArgs->source=(const char *)lastSource;
                return UCNV_GET_NEXT_UCHAR_USE_TO_U;
            }
        }
    }

    if(c<0) {
        if(U_SUCCESS(*pErrorCode) && source==sourceLimit && lastSource<source) {
            
            uint8_t *bytes=cnv->toUBytes;
            cnv->toULength=(int8_t)(source-lastSource);
            do {
                *bytes++=*lastSource++;
            } while(lastSource<source);
            *pErrorCode=U_TRUNCATED_CHAR_FOUND;
        } else if(U_FAILURE(*pErrorCode)) {
            
            





            UBool isDBCSOnly=(UBool)(cnv->sharedData->mbcs.dbcsOnlyState!=0);
            uint8_t *bytes=cnv->toUBytes;
            *bytes++=*lastSource++;     
            if(lastSource==source) {
                cnv->toULength=1;
            } else  {
                int8_t i;
                for(i=1;
                    lastSource<source && !isSingleOrLead(stateTable, state, isDBCSOnly, *lastSource);
                    ++i
                ) {
                    *bytes++=*lastSource++;
                }
                cnv->toULength=i;
                source=lastSource;
            }
        } else {
            
            *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
        }
        c=0xffff;
    }

    
    cnv->toUnicodeStatus=0;
    cnv->mode=state;

    
    pArgs->source=(const char *)source;
    return c;
}

#if 0









U_CFUNC UChar32
ucnv_MBCSSingleSimpleGetNextUChar(UConverterSharedData *sharedData,
                              uint8_t b, UBool useFallback) {
    int32_t entry;
    uint8_t action;

    entry=sharedData->mbcs.stateTable[0][b];
    

    if(MBCS_ENTRY_FINAL_IS_VALID_DIRECT_16(entry)) {
        
        return (UChar)MBCS_ENTRY_FINAL_VALUE_16(entry);
    }

    



    action=(uint8_t)(MBCS_ENTRY_FINAL_ACTION(entry));
    if(action==MBCS_STATE_VALID_DIRECT_20) {
        
        return 0x10000+MBCS_ENTRY_FINAL_VALUE(entry);
    } else if(action==MBCS_STATE_FALLBACK_DIRECT_16) {
        if(!TO_U_USE_FALLBACK(useFallback)) {
            return 0xfffe;
        }
        
        return (UChar)MBCS_ENTRY_FINAL_VALUE_16(entry);
    } else if(action==MBCS_STATE_FALLBACK_DIRECT_20) {
        if(!TO_U_USE_FALLBACK(useFallback)) {
            return 0xfffe;
        }
        
        return 0x10000+MBCS_ENTRY_FINAL_VALUE(entry);
    } else if(action==MBCS_STATE_UNASSIGNED) {
        return 0xfffe;
    } else if(action==MBCS_STATE_ILLEGAL) {
        return 0xffff;
    } else {
        
        return 0xffff;
    }
}
#endif














U_CFUNC UChar32
ucnv_MBCSSimpleGetNextUChar(UConverterSharedData *sharedData,
                        const char *source, int32_t length,
                        UBool useFallback) {
    const int32_t (*stateTable)[256];
    const uint16_t *unicodeCodeUnits;

    uint32_t offset;
    uint8_t state, action;

    UChar32 c;
    int32_t i, entry;

    if(length<=0) {
        
        return 0xffff;
    }

#if 0






    
    if(sharedData->mbcs.countStates==1) {
        if(length==1) {
            return ucnv_MBCSSingleSimpleGetNextUChar(sharedData, (uint8_t)*source, useFallback);
        } else {
            return 0xffff; 
        }
    }
#endif

    
    stateTable=sharedData->mbcs.stateTable;
    unicodeCodeUnits=sharedData->mbcs.unicodeCodeUnits;

    
    offset=0;
    state=sharedData->mbcs.dbcsOnlyState;

    
    for(i=0;;) {
        entry=stateTable[state][(uint8_t)source[i++]];
        if(MBCS_ENTRY_IS_TRANSITION(entry)) {
            state=(uint8_t)MBCS_ENTRY_TRANSITION_STATE(entry);
            offset+=MBCS_ENTRY_TRANSITION_OFFSET(entry);

            if(i==length) {
                return 0xffff; 
            }
        } else {
            



            action=(uint8_t)(MBCS_ENTRY_FINAL_ACTION(entry));
            if(action==MBCS_STATE_VALID_16) {
                offset+=MBCS_ENTRY_FINAL_VALUE_16(entry);
                c=unicodeCodeUnits[offset];
                if(c!=0xfffe) {
                    
                } else if(UCNV_TO_U_USE_FALLBACK(cnv)) {
                    c=ucnv_MBCSGetFallback(&sharedData->mbcs, offset);
                
                }
                break;
            } else if(action==MBCS_STATE_VALID_DIRECT_16) {
                
                c=(UChar)MBCS_ENTRY_FINAL_VALUE_16(entry);
                break;
            } else if(action==MBCS_STATE_VALID_16_PAIR) {
                offset+=MBCS_ENTRY_FINAL_VALUE_16(entry);
                c=unicodeCodeUnits[offset++];
                if(c<0xd800) {
                    
                } else if(UCNV_TO_U_USE_FALLBACK(cnv) ? c<=0xdfff : c<=0xdbff) {
                    
                    c=(UChar32)(((c&0x3ff)<<10)+unicodeCodeUnits[offset]+(0x10000-0xdc00));
                } else if(UCNV_TO_U_USE_FALLBACK(cnv) ? (c&0xfffe)==0xe000 : c==0xe000) {
                    
                    c=unicodeCodeUnits[offset];
                } else if(c==0xffff) {
                    return 0xffff;
                } else {
                    c=0xfffe;
                }
                break;
            } else if(action==MBCS_STATE_VALID_DIRECT_20) {
                
                c=0x10000+MBCS_ENTRY_FINAL_VALUE(entry);
                break;
            } else if(action==MBCS_STATE_FALLBACK_DIRECT_16) {
                if(!TO_U_USE_FALLBACK(useFallback)) {
                    c=0xfffe;
                    break;
                }
                
                c=(UChar)MBCS_ENTRY_FINAL_VALUE_16(entry);
                break;
            } else if(action==MBCS_STATE_FALLBACK_DIRECT_20) {
                if(!TO_U_USE_FALLBACK(useFallback)) {
                    c=0xfffe;
                    break;
                }
                
                c=0x10000+MBCS_ENTRY_FINAL_VALUE(entry);
                break;
            } else if(action==MBCS_STATE_UNASSIGNED) {
                c=0xfffe;
                break;
            }

            



            return 0xffff;
        }
    }

    if(i!=length) {
        
        return 0xffff;
    }

    if(c==0xfffe) {
        
        const int32_t *cx=sharedData->mbcs.extIndexes;
        if(cx!=NULL) {
            return ucnv_extSimpleMatchToU(cx, source, length, useFallback);
        }
    }

    return c;
}




static void
ucnv_MBCSDoubleFromUnicodeWithOffsets(UConverterFromUnicodeArgs *pArgs,
                                  UErrorCode *pErrorCode) {
    UConverter *cnv;
    const UChar *source, *sourceLimit;
    uint8_t *target;
    int32_t targetCapacity;
    int32_t *offsets;

    const uint16_t *table;
    const uint16_t *mbcsIndex;
    const uint8_t *bytes;

    UChar32 c;

    int32_t sourceIndex, nextSourceIndex;

    uint32_t stage2Entry;
    uint32_t asciiRoundtrips;
    uint32_t value;
    uint8_t unicodeMask;

    
    cnv=pArgs->converter;
    unicodeMask=cnv->sharedData->mbcs.unicodeMask;

    
    source=pArgs->source;
    sourceLimit=pArgs->sourceLimit;
    target=(uint8_t *)pArgs->target;
    targetCapacity=(int32_t)(pArgs->targetLimit-pArgs->target);
    offsets=pArgs->offsets;

    table=cnv->sharedData->mbcs.fromUnicodeTable;
    mbcsIndex=cnv->sharedData->mbcs.mbcsIndex;
    if((cnv->options&UCNV_OPTION_SWAP_LFNL)!=0) {
        bytes=cnv->sharedData->mbcs.swapLFNLFromUnicodeBytes;
    } else {
        bytes=cnv->sharedData->mbcs.fromUnicodeBytes;
    }
    asciiRoundtrips=cnv->sharedData->mbcs.asciiRoundtrips;

    
    c=cnv->fromUChar32;

    
    sourceIndex= c==0 ? 0 : -1;
    nextSourceIndex=0;

    
    if(c!=0 && targetCapacity>0) {
        goto getTrail;
    }

    while(source<sourceLimit) {
        







        if(targetCapacity>0) {
            




            c=*source++;
            ++nextSourceIndex;
            if(c<=0x7f && IS_ASCII_ROUNDTRIP(c, asciiRoundtrips)) {
                *target++=(uint8_t)c;
                if(offsets!=NULL) {
                    *offsets++=sourceIndex;
                    sourceIndex=nextSourceIndex;
                }
                --targetCapacity;
                c=0;
                continue;
            }
            




            if(c<=0xd7ff) {
                value=DBCS_RESULT_FROM_MOST_BMP(mbcsIndex, (const uint16_t *)bytes, c);
                
                if(value==0) {
                    goto unassigned;
                }
                
            } else {
                




                if(U16_IS_SURROGATE(c) && !(unicodeMask&UCNV_HAS_SURROGATES)) {
                    if(U16_IS_SURROGATE_LEAD(c)) {
getTrail:
                        if(source<sourceLimit) {
                            
                            UChar trail=*source;
                            if(U16_IS_TRAIL(trail)) {
                                ++source;
                                ++nextSourceIndex;
                                c=U16_GET_SUPPLEMENTARY(c, trail);
                                if(!(unicodeMask&UCNV_HAS_SUPPLEMENTARY)) {
                                    
                                    
                                    goto unassigned;
                                }
                                
                                
                            } else {
                                
                                
                                *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                                break;
                            }
                        } else {
                            
                            break;
                        }
                    } else {
                        
                        
                        *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                        break;
                    }
                }

                
                stage2Entry=MBCS_STAGE_2_FROM_U(table, c);

                
                
                value=MBCS_VALUE_2_FROM_STAGE_2(bytes, stage2Entry, c);

                
                if(!(MBCS_FROM_U_IS_ROUNDTRIP(stage2Entry, c) ||
                     (UCNV_FROM_U_USE_FALLBACK(cnv, c) && value!=0))
                ) {
                    





unassigned:
                    
                    pArgs->source=source;
                    c=_extFromU(cnv, cnv->sharedData,
                                c, &source, sourceLimit,
                                &target, target+targetCapacity,
                                &offsets, sourceIndex,
                                pArgs->flush,
                                pErrorCode);
                    nextSourceIndex+=(int32_t)(source-pArgs->source);

                    if(U_FAILURE(*pErrorCode)) {
                        
                        break;
                    } else {
                        

                        
                        targetCapacity=(int32_t)(pArgs->targetLimit-(char *)target);

                        
                        sourceIndex=nextSourceIndex;
                        continue;
                    }
                }
            }

            
            
            if(value<=0xff) {
                
                *target++=(uint8_t)value;
                if(offsets!=NULL) {
                    *offsets++=sourceIndex;
                }
                --targetCapacity;
            } else  {
                *target++=(uint8_t)(value>>8);
                if(2<=targetCapacity) {
                    *target++=(uint8_t)value;
                    if(offsets!=NULL) {
                        *offsets++=sourceIndex;
                        *offsets++=sourceIndex;
                    }
                    targetCapacity-=2;
                } else {
                    if(offsets!=NULL) {
                        *offsets++=sourceIndex;
                    }
                    cnv->charErrorBuffer[0]=(char)value;
                    cnv->charErrorBufferLength=1;

                    
                    targetCapacity=0;
                    *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                    c=0;
                    break;
                }
            }

            
            c=0;
            sourceIndex=nextSourceIndex;
            continue;
        } else {
            
            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
            break;
        }
    }

    
    cnv->fromUChar32=c;

    
    pArgs->source=source;
    pArgs->target=(char *)target;
    pArgs->offsets=offsets;
}


static void
ucnv_MBCSSingleFromUnicodeWithOffsets(UConverterFromUnicodeArgs *pArgs,
                                  UErrorCode *pErrorCode) {
    UConverter *cnv;
    const UChar *source, *sourceLimit;
    uint8_t *target;
    int32_t targetCapacity;
    int32_t *offsets;

    const uint16_t *table;
    const uint16_t *results;

    UChar32 c;

    int32_t sourceIndex, nextSourceIndex;

    uint16_t value, minValue;
    UBool hasSupplementary;

    
    cnv=pArgs->converter;
    source=pArgs->source;
    sourceLimit=pArgs->sourceLimit;
    target=(uint8_t *)pArgs->target;
    targetCapacity=(int32_t)(pArgs->targetLimit-pArgs->target);
    offsets=pArgs->offsets;

    table=cnv->sharedData->mbcs.fromUnicodeTable;
    if((cnv->options&UCNV_OPTION_SWAP_LFNL)!=0) {
        results=(uint16_t *)cnv->sharedData->mbcs.swapLFNLFromUnicodeBytes;
    } else {
        results=(uint16_t *)cnv->sharedData->mbcs.fromUnicodeBytes;
    }

    if(cnv->useFallback) {
        
        minValue=0x800;
    } else {
        
        minValue=0xc00;
    }
    hasSupplementary=(UBool)(cnv->sharedData->mbcs.unicodeMask&UCNV_HAS_SUPPLEMENTARY);

    
    c=cnv->fromUChar32;

    
    sourceIndex= c==0 ? 0 : -1;
    nextSourceIndex=0;

    
    if(c!=0 && targetCapacity>0) {
        goto getTrail;
    }

    while(source<sourceLimit) {
        







        if(targetCapacity>0) {
            




            c=*source++;
            ++nextSourceIndex;
            if(U16_IS_SURROGATE(c)) {
                if(U16_IS_SURROGATE_LEAD(c)) {
getTrail:
                    if(source<sourceLimit) {
                        
                        UChar trail=*source;
                        if(U16_IS_TRAIL(trail)) {
                            ++source;
                            ++nextSourceIndex;
                            c=U16_GET_SUPPLEMENTARY(c, trail);
                            if(!hasSupplementary) {
                                
                                
                                goto unassigned;
                            }
                            
                            
                        } else {
                            
                            
                            *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                            break;
                        }
                    } else {
                        
                        break;
                    }
                } else {
                    
                    
                    *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                    break;
                }
            }

            
            value=MBCS_SINGLE_RESULT_FROM_U(table, results, c);

            
            if(value>=minValue) {
                
                
                
                *target++=(uint8_t)value;
                if(offsets!=NULL) {
                    *offsets++=sourceIndex;
                }
                --targetCapacity;

                
                c=0;
                sourceIndex=nextSourceIndex;
            } else { 
unassigned:
                
                pArgs->source=source;
                c=_extFromU(cnv, cnv->sharedData,
                            c, &source, sourceLimit,
                            &target, target+targetCapacity,
                            &offsets, sourceIndex,
                            pArgs->flush,
                            pErrorCode);
                nextSourceIndex+=(int32_t)(source-pArgs->source);

                if(U_FAILURE(*pErrorCode)) {
                    
                    break;
                } else {
                    

                    
                    targetCapacity=(int32_t)(pArgs->targetLimit-(char *)target);

                    
                    sourceIndex=nextSourceIndex;
                }
            }
        } else {
            
            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
            break;
        }
    }

    
    cnv->fromUChar32=c;

    
    pArgs->source=source;
    pArgs->target=(char *)target;
    pArgs->offsets=offsets;
}












static void
ucnv_MBCSSingleFromBMPWithOffsets(UConverterFromUnicodeArgs *pArgs,
                              UErrorCode *pErrorCode) {
    UConverter *cnv;
    const UChar *source, *sourceLimit, *lastSource;
    uint8_t *target;
    int32_t targetCapacity, length;
    int32_t *offsets;

    const uint16_t *table;
    const uint16_t *results;

    UChar32 c;

    int32_t sourceIndex;

    uint32_t asciiRoundtrips;
    uint16_t value, minValue;

    
    cnv=pArgs->converter;
    source=pArgs->source;
    sourceLimit=pArgs->sourceLimit;
    target=(uint8_t *)pArgs->target;
    targetCapacity=(int32_t)(pArgs->targetLimit-pArgs->target);
    offsets=pArgs->offsets;

    table=cnv->sharedData->mbcs.fromUnicodeTable;
    if((cnv->options&UCNV_OPTION_SWAP_LFNL)!=0) {
        results=(uint16_t *)cnv->sharedData->mbcs.swapLFNLFromUnicodeBytes;
    } else {
        results=(uint16_t *)cnv->sharedData->mbcs.fromUnicodeBytes;
    }
    asciiRoundtrips=cnv->sharedData->mbcs.asciiRoundtrips;

    if(cnv->useFallback) {
        
        minValue=0x800;
    } else {
        
        minValue=0xc00;
    }

    
    c=cnv->fromUChar32;

    
    sourceIndex= c==0 ? 0 : -1;
    lastSource=source;

    



    length=(int32_t)(sourceLimit-source);
    if(length<targetCapacity) {
        targetCapacity=length;
    }

    
    if(c!=0 && targetCapacity>0) {
        goto getTrail;
    }

#if MBCS_UNROLL_SINGLE_FROM_BMP
    
    
unrolled:
    if(targetCapacity>=4) {
        int32_t count, loops;
        uint16_t andedValues;

        loops=count=targetCapacity>>2;
        do {
            c=*source++;
            andedValues=value=MBCS_SINGLE_RESULT_FROM_U(table, results, c);
            *target++=(uint8_t)value;
            c=*source++;
            andedValues&=value=MBCS_SINGLE_RESULT_FROM_U(table, results, c);
            *target++=(uint8_t)value;
            c=*source++;
            andedValues&=value=MBCS_SINGLE_RESULT_FROM_U(table, results, c);
            *target++=(uint8_t)value;
            c=*source++;
            andedValues&=value=MBCS_SINGLE_RESULT_FROM_U(table, results, c);
            *target++=(uint8_t)value;

            
            if(andedValues<minValue) {
                
                source-=4;
                target-=4;
                break;
            }
        } while(--count>0);
        count=loops-count;
        targetCapacity-=4*count;

        if(offsets!=NULL) {
            lastSource+=4*count;
            while(count>0) {
                *offsets++=sourceIndex++;
                *offsets++=sourceIndex++;
                *offsets++=sourceIndex++;
                *offsets++=sourceIndex++;
                --count;
            }
        }

        c=0;
    }
#endif

    while(targetCapacity>0) {
        




        c=*source++;
        




        
        if(c<=0x7f && IS_ASCII_ROUNDTRIP(c, asciiRoundtrips)) {
            *target++=(uint8_t)c;
            --targetCapacity;
            c=0;
            continue;
        }
        value=MBCS_SINGLE_RESULT_FROM_U(table, results, c);
        
        if(value>=minValue) {
            
            
            
            *target++=(uint8_t)value;
            --targetCapacity;

            
            c=0;
            continue;
        } else if(!U16_IS_SURROGATE(c)) {
            
        } else if(U16_IS_SURROGATE_LEAD(c)) {
getTrail:
            if(source<sourceLimit) {
                
                UChar trail=*source;
                if(U16_IS_TRAIL(trail)) {
                    ++source;
                    c=U16_GET_SUPPLEMENTARY(c, trail);
                    
                    
                } else {
                    
                    
                    *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                    break;
                }
            } else {
                
                if (pArgs->flush) {
                    *pErrorCode=U_TRUNCATED_CHAR_FOUND;
                }
                break;
            }
        } else {
            
            
            *pErrorCode=U_ILLEGAL_CHAR_FOUND;
            break;
        }

        

        
        length=U16_LENGTH(c);

        
        if(offsets!=NULL) {
            int32_t count=(int32_t)(source-lastSource);

            
            count-=length;

            while(count>0) {
                *offsets++=sourceIndex++;
                --count;
            }
            
        }

        
        lastSource=source;
        c=_extFromU(cnv, cnv->sharedData,
                    c, &source, sourceLimit,
                    &target, (const uint8_t *)(pArgs->targetLimit),
                    &offsets, sourceIndex,
                    pArgs->flush,
                    pErrorCode);
        sourceIndex+=length+(int32_t)(source-lastSource);
        lastSource=source;

        if(U_FAILURE(*pErrorCode)) {
            
            break;
        } else {
            

            
            targetCapacity=(int32_t)(pArgs->targetLimit-(char *)target);
            length=(int32_t)(sourceLimit-source);
            if(length<targetCapacity) {
                targetCapacity=length;
            }
        }

#if MBCS_UNROLL_SINGLE_FROM_BMP
        
        goto unrolled;
#endif
    }

    if(U_SUCCESS(*pErrorCode) && source<sourceLimit && target>=(uint8_t *)pArgs->targetLimit) {
        
        *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
    }

    
    if(offsets!=NULL) {
        size_t count=source-lastSource;
        if (count > 0 && *pErrorCode == U_TRUNCATED_CHAR_FOUND) {
            




            count--;
        }
        while(count>0) {
            *offsets++=sourceIndex++;
            --count;
        }
    }

    
    cnv->fromUChar32=c;

    
    pArgs->source=source;
    pArgs->target=(char *)target;
    pArgs->offsets=offsets;
}

U_CFUNC void
ucnv_MBCSFromUnicodeWithOffsets(UConverterFromUnicodeArgs *pArgs,
                            UErrorCode *pErrorCode) {
    UConverter *cnv;
    const UChar *source, *sourceLimit;
    uint8_t *target;
    int32_t targetCapacity;
    int32_t *offsets;

    const uint16_t *table;
    const uint16_t *mbcsIndex;
    const uint8_t *p, *bytes;
    uint8_t outputType;

    UChar32 c;

    int32_t prevSourceIndex, sourceIndex, nextSourceIndex;

    uint32_t stage2Entry;
    uint32_t asciiRoundtrips;
    uint32_t value;
    uint8_t si_value[2] = {0, 0}; 
    uint8_t so_value[2] = {0, 0}; 
    uint8_t si_value_length, so_value_length;
    int32_t length = 0, prevLength;
    uint8_t unicodeMask;

    cnv=pArgs->converter;

    if(cnv->preFromUFirstCP>=0) {
        



        ucnv_extContinueMatchFromU(cnv, pArgs, -1, pErrorCode);

        if(U_FAILURE(*pErrorCode) || cnv->preFromULength<0) {
            return;
        }
    }

    
    outputType=cnv->sharedData->mbcs.outputType;
    unicodeMask=cnv->sharedData->mbcs.unicodeMask;
    if(outputType==MBCS_OUTPUT_1 && !(unicodeMask&UCNV_HAS_SURROGATES)) {
        if(!(unicodeMask&UCNV_HAS_SUPPLEMENTARY)) {
            ucnv_MBCSSingleFromBMPWithOffsets(pArgs, pErrorCode);
        } else {
            ucnv_MBCSSingleFromUnicodeWithOffsets(pArgs, pErrorCode);
        }
        return;
    } else if(outputType==MBCS_OUTPUT_2 && cnv->sharedData->mbcs.utf8Friendly) {
        ucnv_MBCSDoubleFromUnicodeWithOffsets(pArgs, pErrorCode);
        return;
    }

    
    source=pArgs->source;
    sourceLimit=pArgs->sourceLimit;
    target=(uint8_t *)pArgs->target;
    targetCapacity=(int32_t)(pArgs->targetLimit-pArgs->target);
    offsets=pArgs->offsets;

    table=cnv->sharedData->mbcs.fromUnicodeTable;
    if(cnv->sharedData->mbcs.utf8Friendly) {
        mbcsIndex=cnv->sharedData->mbcs.mbcsIndex;
    } else {
        mbcsIndex=NULL;
    }
    if((cnv->options&UCNV_OPTION_SWAP_LFNL)!=0) {
        bytes=cnv->sharedData->mbcs.swapLFNLFromUnicodeBytes;
    } else {
        bytes=cnv->sharedData->mbcs.fromUnicodeBytes;
    }
    asciiRoundtrips=cnv->sharedData->mbcs.asciiRoundtrips;

    
    c=cnv->fromUChar32;

    if(outputType==MBCS_OUTPUT_2_SISO) {
        prevLength=cnv->fromUnicodeStatus;
        if(prevLength==0) {
            
            prevLength=1;
        }
    } else {
        
        prevLength=0;
    }

    
    prevSourceIndex=-1;
    sourceIndex= c==0 ? 0 : -1;
    nextSourceIndex=0;

    
    si_value_length = getSISOBytes(SI, cnv->options, si_value);
    so_value_length = getSISOBytes(SO, cnv->options, so_value);

    
    












    if(c!=0 && targetCapacity>0) {
        goto getTrail;
    }

    while(source<sourceLimit) {
        







        if(targetCapacity>0) {
            




            c=*source++;
            ++nextSourceIndex;
            if(c<=0x7f && IS_ASCII_ROUNDTRIP(c, asciiRoundtrips)) {
                *target++=(uint8_t)c;
                if(offsets!=NULL) {
                    *offsets++=sourceIndex;
                    prevSourceIndex=sourceIndex;
                    sourceIndex=nextSourceIndex;
                }
                --targetCapacity;
                c=0;
                continue;
            }
            




            if(c<=0xd7ff && mbcsIndex!=NULL) {
                value=mbcsIndex[c>>6];

                
                
                switch(outputType) {
                case MBCS_OUTPUT_2:
                    value=((const uint16_t *)bytes)[value +(c&0x3f)];
                    if(value<=0xff) {
                        if(value==0) {
                            goto unassigned;
                        } else {
                            length=1;
                        }
                    } else {
                        length=2;
                    }
                    break;
                case MBCS_OUTPUT_2_SISO:
                    
                    








                    cnv->fromUnicodeStatus=prevLength; 
                    value=((const uint16_t *)bytes)[value +(c&0x3f)];
                    if(value<=0xff) {
                        if(value==0) {
                            goto unassigned;
                        } else if(prevLength<=1) {
                            length=1;
                        } else {
                            
                            if (si_value_length == 1) {
                                value|=(uint32_t)si_value[0]<<8;
                                length = 2;
                            } else if (si_value_length == 2) {
                                value|=(uint32_t)si_value[1]<<8;
                                value|=(uint32_t)si_value[0]<<16;
                                length = 3;
                            }
                            prevLength=1;
                        }
                    } else {
                        if(prevLength==2) {
                            length=2;
                        } else {
                            
                            if (so_value_length == 1) {
                                value|=(uint32_t)so_value[0]<<16;
                                length = 3;
                            } else if (so_value_length == 2) {
                                value|=(uint32_t)so_value[1]<<16;
                                value|=(uint32_t)so_value[0]<<24;
                                length = 4;
                            }
                            prevLength=2;
                        }
                    }
                    break;
                case MBCS_OUTPUT_DBCS_ONLY:
                    
                    value=((const uint16_t *)bytes)[value +(c&0x3f)];
                    if(value<=0xff) {
                        
                        goto unassigned;
                    } else {
                        length=2;
                    }
                    break;
                case MBCS_OUTPUT_3:
                    p=bytes+(value+(c&0x3f))*3;
                    value=((uint32_t)*p<<16)|((uint32_t)p[1]<<8)|p[2];
                    if(value<=0xff) {
                        if(value==0) {
                            goto unassigned;
                        } else {
                            length=1;
                        }
                    } else if(value<=0xffff) {
                        length=2;
                    } else {
                        length=3;
                    }
                    break;
                case MBCS_OUTPUT_4:
                    value=((const uint32_t *)bytes)[value +(c&0x3f)];
                    if(value<=0xff) {
                        if(value==0) {
                            goto unassigned;
                        } else {
                            length=1;
                        }
                    } else if(value<=0xffff) {
                        length=2;
                    } else if(value<=0xffffff) {
                        length=3;
                    } else {
                        length=4;
                    }
                    break;
                case MBCS_OUTPUT_3_EUC:
                    value=((const uint16_t *)bytes)[value +(c&0x3f)];
                    
                    if(value<=0xff) {
                        if(value==0) {
                            goto unassigned;
                        } else {
                            length=1;
                        }
                    } else if((value&0x8000)==0) {
                        value|=0x8e8000;
                        length=3;
                    } else if((value&0x80)==0) {
                        value|=0x8f0080;
                        length=3;
                    } else {
                        length=2;
                    }
                    break;
                case MBCS_OUTPUT_4_EUC:
                    p=bytes+(value+(c&0x3f))*3;
                    value=((uint32_t)*p<<16)|((uint32_t)p[1]<<8)|p[2];
                    
                    if(value<=0xff) {
                        if(value==0) {
                            goto unassigned;
                        } else {
                            length=1;
                        }
                    } else if(value<=0xffff) {
                        length=2;
                    } else if((value&0x800000)==0) {
                        value|=0x8e800000;
                        length=4;
                    } else if((value&0x8000)==0) {
                        value|=0x8f008000;
                        length=4;
                    } else {
                        length=3;
                    }
                    break;
                default:
                    
                    






                    value=0;
                    length=0;
                    break;
                }
                
            } else {
                




                if(U16_IS_SURROGATE(c) && !(unicodeMask&UCNV_HAS_SURROGATES)) {
                    if(U16_IS_SURROGATE_LEAD(c)) {
getTrail:
                        if(source<sourceLimit) {
                            
                            UChar trail=*source;
                            if(U16_IS_TRAIL(trail)) {
                                ++source;
                                ++nextSourceIndex;
                                c=U16_GET_SUPPLEMENTARY(c, trail);
                                if(!(unicodeMask&UCNV_HAS_SUPPLEMENTARY)) {
                                    
                                    cnv->fromUnicodeStatus=prevLength; 
                                    
                                    goto unassigned;
                                }
                                
                                
                            } else {
                                
                                
                                *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                                break;
                            }
                        } else {
                            
                            break;
                        }
                    } else {
                        
                        
                        *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                        break;
                    }
                }

                

                































                stage2Entry=MBCS_STAGE_2_FROM_U(table, c);

                
                switch(outputType) {
                case MBCS_OUTPUT_2:
                    value=MBCS_VALUE_2_FROM_STAGE_2(bytes, stage2Entry, c);
                    if(value<=0xff) {
                        length=1;
                    } else {
                        length=2;
                    }
                    break;
                case MBCS_OUTPUT_2_SISO:
                    
                    








                    cnv->fromUnicodeStatus=prevLength; 
                    value=MBCS_VALUE_2_FROM_STAGE_2(bytes, stage2Entry, c);
                    if(value<=0xff) {
                        if(value==0 && MBCS_FROM_U_IS_ROUNDTRIP(stage2Entry, c)==0) {
                            
                            length=0;
                        } else if(prevLength<=1) {
                            length=1;
                        } else {
                            
                            if (si_value_length == 1) {
                                value|=(uint32_t)si_value[0]<<8;
                                length = 2;
                            } else if (si_value_length == 2) {
                                value|=(uint32_t)si_value[1]<<8;
                                value|=(uint32_t)si_value[0]<<16;
                                length = 3;
                            }
                            prevLength=1;
                        }
                    } else {
                        if(prevLength==2) {
                            length=2;
                        } else {
                            
                            if (so_value_length == 1) {
                                value|=(uint32_t)so_value[0]<<16;
                                length = 3;
                            } else if (so_value_length == 2) {
                                value|=(uint32_t)so_value[1]<<16;
                                value|=(uint32_t)so_value[0]<<24;
                                length = 4;
                            }
                            prevLength=2;
                        }
                    }
                    break;
                case MBCS_OUTPUT_DBCS_ONLY:
                    
                    value=MBCS_VALUE_2_FROM_STAGE_2(bytes, stage2Entry, c);
                    if(value<=0xff) {
                        
                        value=stage2Entry=0; 
                        length=0;
                    } else {
                        length=2;
                    }
                    break;
                case MBCS_OUTPUT_3:
                    p=MBCS_POINTER_3_FROM_STAGE_2(bytes, stage2Entry, c);
                    value=((uint32_t)*p<<16)|((uint32_t)p[1]<<8)|p[2];
                    if(value<=0xff) {
                        length=1;
                    } else if(value<=0xffff) {
                        length=2;
                    } else {
                        length=3;
                    }
                    break;
                case MBCS_OUTPUT_4:
                    value=MBCS_VALUE_4_FROM_STAGE_2(bytes, stage2Entry, c);
                    if(value<=0xff) {
                        length=1;
                    } else if(value<=0xffff) {
                        length=2;
                    } else if(value<=0xffffff) {
                        length=3;
                    } else {
                        length=4;
                    }
                    break;
                case MBCS_OUTPUT_3_EUC:
                    value=MBCS_VALUE_2_FROM_STAGE_2(bytes, stage2Entry, c);
                    
                    if(value<=0xff) {
                        length=1;
                    } else if((value&0x8000)==0) {
                        value|=0x8e8000;
                        length=3;
                    } else if((value&0x80)==0) {
                        value|=0x8f0080;
                        length=3;
                    } else {
                        length=2;
                    }
                    break;
                case MBCS_OUTPUT_4_EUC:
                    p=MBCS_POINTER_3_FROM_STAGE_2(bytes, stage2Entry, c);
                    value=((uint32_t)*p<<16)|((uint32_t)p[1]<<8)|p[2];
                    
                    if(value<=0xff) {
                        length=1;
                    } else if(value<=0xffff) {
                        length=2;
                    } else if((value&0x800000)==0) {
                        value|=0x8e800000;
                        length=4;
                    } else if((value&0x8000)==0) {
                        value|=0x8f008000;
                        length=4;
                    } else {
                        length=3;
                    }
                    break;
                default:
                    
                    






                    value=stage2Entry=0; 
                    length=0;
                    break;
                }

                
                if(!(MBCS_FROM_U_IS_ROUNDTRIP(stage2Entry, c)!=0 ||
                     (UCNV_FROM_U_USE_FALLBACK(cnv, c) && value!=0))
                ) {
                    





unassigned:
                    
                    pArgs->source=source;
                    c=_extFromU(cnv, cnv->sharedData,
                                c, &source, sourceLimit,
                                &target, target+targetCapacity,
                                &offsets, sourceIndex,
                                pArgs->flush,
                                pErrorCode);
                    nextSourceIndex+=(int32_t)(source-pArgs->source);
                    prevLength=cnv->fromUnicodeStatus; 

                    if(U_FAILURE(*pErrorCode)) {
                        
                        break;
                    } else {
                        

                        
                        targetCapacity=(int32_t)(pArgs->targetLimit-(char *)target);

                        
                        if(offsets!=NULL) {
                            prevSourceIndex=sourceIndex;
                            sourceIndex=nextSourceIndex;
                        }
                        continue;
                    }
                }
            }

            
            
            if(length<=targetCapacity) {
                if(offsets==NULL) {
                    switch(length) {
                        
                    case 4:
                        *target++=(uint8_t)(value>>24);
                    case 3: 
                        *target++=(uint8_t)(value>>16);
                    case 2: 
                        *target++=(uint8_t)(value>>8);
                    case 1: 
                        *target++=(uint8_t)value;
                    default:
                        
                        break;
                    }
                } else {
                    switch(length) {
                        
                    case 4:
                        *target++=(uint8_t)(value>>24);
                        *offsets++=sourceIndex;
                    case 3: 
                        *target++=(uint8_t)(value>>16);
                        *offsets++=sourceIndex;
                    case 2: 
                        *target++=(uint8_t)(value>>8);
                        *offsets++=sourceIndex;
                    case 1: 
                        *target++=(uint8_t)value;
                        *offsets++=sourceIndex;
                    default:
                        
                        break;
                    }
                }
                targetCapacity-=length;
            } else {
                uint8_t *charErrorBuffer;

                





                
                length-=targetCapacity;
                charErrorBuffer=(uint8_t *)cnv->charErrorBuffer;
                switch(length) {
                    
                case 3:
                    *charErrorBuffer++=(uint8_t)(value>>16);
                case 2: 
                    *charErrorBuffer++=(uint8_t)(value>>8);
                case 1: 
                    *charErrorBuffer=(uint8_t)value;
                default:
                    
                    break;
                }
                cnv->charErrorBufferLength=(int8_t)length;

                
                value>>=8*length; 
                switch(targetCapacity) {
                    
                case 3:
                    *target++=(uint8_t)(value>>16);
                    if(offsets!=NULL) {
                        *offsets++=sourceIndex;
                    }
                case 2: 
                    *target++=(uint8_t)(value>>8);
                    if(offsets!=NULL) {
                        *offsets++=sourceIndex;
                    }
                case 1: 
                    *target++=(uint8_t)value;
                    if(offsets!=NULL) {
                        *offsets++=sourceIndex;
                    }
                default:
                    
                    break;
                }

                
                targetCapacity=0;
                *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                c=0;
                break;
            }

            
            c=0;
            if(offsets!=NULL) {
                prevSourceIndex=sourceIndex;
                sourceIndex=nextSourceIndex;
            }
            continue;
        } else {
            
            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
            break;
        }
    }

    









    if( U_SUCCESS(*pErrorCode) &&
        outputType==MBCS_OUTPUT_2_SISO && prevLength==2 &&
        pArgs->flush && source>=sourceLimit && c==0
    ) {
        
        if(targetCapacity>0) {
            *target++=(uint8_t)si_value[0];
            if (si_value_length == 2) {
                if (targetCapacity<2) {
                    cnv->charErrorBuffer[0]=(uint8_t)si_value[1];
                    cnv->charErrorBufferLength=1;
                    *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                } else {
                    *target++=(uint8_t)si_value[1];
                }
            }
            if(offsets!=NULL) {
                
                *offsets++=prevSourceIndex;
            }
        } else {
            
            cnv->charErrorBuffer[0]=(uint8_t)si_value[0];
            if (si_value_length == 2) {
                cnv->charErrorBuffer[1]=(uint8_t)si_value[1];
            }
            cnv->charErrorBufferLength=si_value_length;
            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
        }
        prevLength=1; 
    }

    
    cnv->fromUChar32=c;
    cnv->fromUnicodeStatus=prevLength;

    
    pArgs->source=source;
    pArgs->target=(char *)target;
    pArgs->offsets=offsets;
}


















U_CFUNC int32_t
ucnv_MBCSFromUChar32(UConverterSharedData *sharedData,
                 UChar32 c, uint32_t *pValue,
                 UBool useFallback) {
    const int32_t *cx;
    const uint16_t *table;
#if 0

    const uint8_t *p;
#endif
    uint32_t stage2Entry;
    uint32_t value;
    int32_t length;

    
    if(c<=0xffff || (sharedData->mbcs.unicodeMask&UCNV_HAS_SUPPLEMENTARY)) {
        table=sharedData->mbcs.fromUnicodeTable;

        
        if(sharedData->mbcs.outputType==MBCS_OUTPUT_1) {
            value=MBCS_SINGLE_RESULT_FROM_U(table, (uint16_t *)sharedData->mbcs.fromUnicodeBytes, c);
            
            if(useFallback ? value>=0x800 : value>=0xc00) {
                *pValue=value&0xff;
                return 1;
            }
        } else  {
            stage2Entry=MBCS_STAGE_2_FROM_U(table, c);

            
            switch(sharedData->mbcs.outputType) {
            case MBCS_OUTPUT_2:
                value=MBCS_VALUE_2_FROM_STAGE_2(sharedData->mbcs.fromUnicodeBytes, stage2Entry, c);
                if(value<=0xff) {
                    length=1;
                } else {
                    length=2;
                }
                break;
#if 0

            case MBCS_OUTPUT_DBCS_ONLY:
                
                value=MBCS_VALUE_2_FROM_STAGE_2(sharedData->mbcs.fromUnicodeBytes, stage2Entry, c);
                if(value<=0xff) {
                    
                    value=stage2Entry=0; 
                    length=0;
                } else {
                    length=2;
                }
                break;
            case MBCS_OUTPUT_3:
                p=MBCS_POINTER_3_FROM_STAGE_2(sharedData->mbcs.fromUnicodeBytes, stage2Entry, c);
                value=((uint32_t)*p<<16)|((uint32_t)p[1]<<8)|p[2];
                if(value<=0xff) {
                    length=1;
                } else if(value<=0xffff) {
                    length=2;
                } else {
                    length=3;
                }
                break;
            case MBCS_OUTPUT_4:
                value=MBCS_VALUE_4_FROM_STAGE_2(sharedData->mbcs.fromUnicodeBytes, stage2Entry, c);
                if(value<=0xff) {
                    length=1;
                } else if(value<=0xffff) {
                    length=2;
                } else if(value<=0xffffff) {
                    length=3;
                } else {
                    length=4;
                }
                break;
            case MBCS_OUTPUT_3_EUC:
                value=MBCS_VALUE_2_FROM_STAGE_2(sharedData->mbcs.fromUnicodeBytes, stage2Entry, c);
                
                if(value<=0xff) {
                    length=1;
                } else if((value&0x8000)==0) {
                    value|=0x8e8000;
                    length=3;
                } else if((value&0x80)==0) {
                    value|=0x8f0080;
                    length=3;
                } else {
                    length=2;
                }
                break;
            case MBCS_OUTPUT_4_EUC:
                p=MBCS_POINTER_3_FROM_STAGE_2(sharedData->mbcs.fromUnicodeBytes, stage2Entry, c);
                value=((uint32_t)*p<<16)|((uint32_t)p[1]<<8)|p[2];
                
                if(value<=0xff) {
                    length=1;
                } else if(value<=0xffff) {
                    length=2;
                } else if((value&0x800000)==0) {
                    value|=0x8e800000;
                    length=4;
                } else if((value&0x8000)==0) {
                    value|=0x8f008000;
                    length=4;
                } else {
                    length=3;
                }
                break;
#endif
            default:
                
                return -1;
            }

            
            if( MBCS_FROM_U_IS_ROUNDTRIP(stage2Entry, c) ||
                (FROM_U_USE_FALLBACK(useFallback, c) && value!=0)
            ) {
                




                
                *pValue=value;
                return length;
            }
        }
    }

    cx=sharedData->mbcs.extIndexes;
    if(cx!=NULL) {
        length=ucnv_extSimpleMatchFromU(cx, c, pValue, useFallback);
        return length>=0 ? length : -length;  
    }

    
    return 0;
}


#if 0












U_CFUNC int32_t
ucnv_MBCSSingleFromUChar32(UConverterSharedData *sharedData,
                       UChar32 c,
                       UBool useFallback) {
    const uint16_t *table;
    int32_t value;

    
    if(c>=0x10000 && !(sharedData->mbcs.unicodeMask&UCNV_HAS_SUPPLEMENTARY)) {
        return -1;
    }

    
    table=sharedData->mbcs.fromUnicodeTable;

    
    value=MBCS_SINGLE_RESULT_FROM_U(table, (uint16_t *)sharedData->mbcs.fromUnicodeBytes, c);
    
    if(useFallback ? value>=0x800 : value>=0xc00) {
        return value&0xff;
    } else {
        return -1;
    }
}
#endif




static const UChar32
utf8_minLegal[5]={ 0, 0, 0x80, 0x800, 0x10000 };


static const UChar32
utf8_offsets[7]={ 0, 0, 0x3080, 0xE2080, 0x3C82080 };

static void
ucnv_SBCSFromUTF8(UConverterFromUnicodeArgs *pFromUArgs,
                  UConverterToUnicodeArgs *pToUArgs,
                  UErrorCode *pErrorCode) {
    UConverter *utf8, *cnv;
    const uint8_t *source, *sourceLimit;
    uint8_t *target;
    int32_t targetCapacity;

    const uint16_t *table, *sbcsIndex;
    const uint16_t *results;

    int8_t oldToULength, toULength, toULimit;

    UChar32 c;
    uint8_t b, t1, t2;

    uint32_t asciiRoundtrips;
    uint16_t value, minValue;
    UBool hasSupplementary;

    
    utf8=pToUArgs->converter;
    cnv=pFromUArgs->converter;
    source=(uint8_t *)pToUArgs->source;
    sourceLimit=(uint8_t *)pToUArgs->sourceLimit;
    target=(uint8_t *)pFromUArgs->target;
    targetCapacity=(int32_t)(pFromUArgs->targetLimit-pFromUArgs->target);

    table=cnv->sharedData->mbcs.fromUnicodeTable;
    sbcsIndex=cnv->sharedData->mbcs.sbcsIndex;
    if((cnv->options&UCNV_OPTION_SWAP_LFNL)!=0) {
        results=(uint16_t *)cnv->sharedData->mbcs.swapLFNLFromUnicodeBytes;
    } else {
        results=(uint16_t *)cnv->sharedData->mbcs.fromUnicodeBytes;
    }
    asciiRoundtrips=cnv->sharedData->mbcs.asciiRoundtrips;

    if(cnv->useFallback) {
        
        minValue=0x800;
    } else {
        
        minValue=0xc00;
    }
    hasSupplementary=(UBool)(cnv->sharedData->mbcs.unicodeMask&UCNV_HAS_SUPPLEMENTARY);

    
    c=(UChar32)utf8->toUnicodeStatus;
    if(c!=0) {
        toULength=oldToULength=utf8->toULength;
        toULimit=(int8_t)utf8->mode;
    } else {
        toULength=oldToULength=toULimit=0;
    }

    







    {
        int32_t i, length;

        length=(int32_t)(sourceLimit-source) - (toULimit-oldToULength);
        for(i=0; i<3 && i<length;) {
            b=*(sourceLimit-i-1);
            if(U8_IS_TRAIL(b)) {
                ++i;
            } else {
                if(i<U8_COUNT_TRAIL_BYTES(b)) {
                    
                    sourceLimit-=i+1;
                }
                break;
            }
        }
    }

    if(c!=0 && targetCapacity>0) {
        utf8->toUnicodeStatus=0;
        utf8->toULength=0;
        goto moreBytes;
        


















    }

    
    while(source<sourceLimit) {
        if(targetCapacity>0) {
            b=*source++;
            if((int8_t)b>=0) {
                
                if(IS_ASCII_ROUNDTRIP(b, asciiRoundtrips)) {
                    *target++=(uint8_t)b;
                    --targetCapacity;
                    continue;
                } else {
                    c=b;
                    value=SBCS_RESULT_FROM_UTF8(sbcsIndex, results, 0, c);
                }
            } else {
                if(b<0xe0) {
                    if( 
                        b>=0xc2 &&
                        (t1=(uint8_t)(*source-0x80)) <= 0x3f
                    ) {
                        c=b&0x1f;
                        ++source;
                        value=SBCS_RESULT_FROM_UTF8(sbcsIndex, results, c, t1);
                        if(value>=minValue) {
                            *target++=(uint8_t)value;
                            --targetCapacity;
                            continue;
                        } else {
                            c=(c<<6)|t1;
                        }
                    } else {
                        c=-1;
                    }
                } else if(b==0xe0) {
                    if( 
                        (t1=(uint8_t)(source[0]-0x80)) <= 0x3f && t1 >= 0x20 &&
                        (t2=(uint8_t)(source[1]-0x80)) <= 0x3f
                    ) {
                        c=t1;
                        source+=2;
                        value=SBCS_RESULT_FROM_UTF8(sbcsIndex, results, c, t2);
                        if(value>=minValue) {
                            *target++=(uint8_t)value;
                            --targetCapacity;
                            continue;
                        } else {
                            c=(c<<6)|t2;
                        }
                    } else {
                        c=-1;
                    }
                } else {
                    c=-1;
                }

                if(c<0) {
                    
                    oldToULength=0;
                    toULength=1;
                    toULimit=U8_COUNT_TRAIL_BYTES(b)+1;
                    c=b;
moreBytes:
                    while(toULength<toULimit) {
                        






                        if(source<(uint8_t *)pToUArgs->sourceLimit) {
                            b=*source;
                            if(U8_IS_TRAIL(b)) {
                                ++source;
                                ++toULength;
                                c=(c<<6)+b;
                            } else {
                                break; 
                            }
                        } else {
                            
                            source-=(toULength-oldToULength);
                            while(oldToULength<toULength) {
                                utf8->toUBytes[oldToULength++]=*source++;
                            }
                            utf8->toUnicodeStatus=c;
                            utf8->toULength=toULength;
                            utf8->mode=toULimit;
                            pToUArgs->source=(char *)source;
                            pFromUArgs->target=(char *)target;
                            return;
                        }
                    }

                    if( toULength==toULimit &&      
                        (toULength==3 || toULength==2) &&             
                        (c-=utf8_offsets[toULength])>=utf8_minLegal[toULength] &&
                        (c<=0xd7ff || 0xe000<=c)    
                    ) {
                        value=MBCS_SINGLE_RESULT_FROM_U(table, results, c);
                    } else if(
                        toULength==toULimit && toULength==4 &&
                        (0x10000<=(c-=utf8_offsets[4]) && c<=0x10ffff)
                    ) {
                        
                        if(!hasSupplementary) {
                            
                            value=0;
                        } else {
                            value=MBCS_SINGLE_RESULT_FROM_U(table, results, c);
                        }
                    } else {
                        
                        source-=(toULength-oldToULength);
                        while(oldToULength<toULength) {
                            utf8->toUBytes[oldToULength++]=*source++;
                        }
                        utf8->toULength=toULength;
                        pToUArgs->source=(char *)source;
                        pFromUArgs->target=(char *)target;
                        *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                        return;
                    }
                }
            }

            if(value>=minValue) {
                
                *target++=(uint8_t)value;
                --targetCapacity;
            } else {
                
                





                static const UChar nul=0;
                const UChar *noSource=&nul;
                c=_extFromU(cnv, cnv->sharedData,
                            c, &noSource, noSource,
                            &target, target+targetCapacity,
                            NULL, -1,
                            pFromUArgs->flush,
                            pErrorCode);

                if(U_FAILURE(*pErrorCode)) {
                    
                    cnv->fromUChar32=c;
                    break;
                } else if(cnv->preFromUFirstCP>=0) {
                    





                    *pErrorCode=U_USING_DEFAULT_WARNING;
                    break;
                } else {
                    

                    
                    targetCapacity=(int32_t)(pFromUArgs->targetLimit-(char *)target);
                }
            }
        } else {
            
            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
            break;
        }
    }

    




    if(U_SUCCESS(*pErrorCode) &&
            cnv->preFromUFirstCP<0 &&
            source<(sourceLimit=(uint8_t *)pToUArgs->sourceLimit)) {
        c=utf8->toUBytes[0]=b=*source++;
        toULength=1;
        toULimit=U8_COUNT_TRAIL_BYTES(b)+1;
        while(source<sourceLimit) {
            utf8->toUBytes[toULength++]=b=*source++;
            c=(c<<6)+b;
        }
        utf8->toUnicodeStatus=c;
        utf8->toULength=toULength;
        utf8->mode=toULimit;
    }

    
    pToUArgs->source=(char *)source;
    pFromUArgs->target=(char *)target;
}

static void
ucnv_DBCSFromUTF8(UConverterFromUnicodeArgs *pFromUArgs,
                  UConverterToUnicodeArgs *pToUArgs,
                  UErrorCode *pErrorCode) {
    UConverter *utf8, *cnv;
    const uint8_t *source, *sourceLimit;
    uint8_t *target;
    int32_t targetCapacity;

    const uint16_t *table, *mbcsIndex;
    const uint16_t *results;

    int8_t oldToULength, toULength, toULimit;

    UChar32 c;
    uint8_t b, t1, t2;

    uint32_t stage2Entry;
    uint32_t asciiRoundtrips;
    uint16_t value;
    UBool hasSupplementary;

    
    utf8=pToUArgs->converter;
    cnv=pFromUArgs->converter;
    source=(uint8_t *)pToUArgs->source;
    sourceLimit=(uint8_t *)pToUArgs->sourceLimit;
    target=(uint8_t *)pFromUArgs->target;
    targetCapacity=(int32_t)(pFromUArgs->targetLimit-pFromUArgs->target);

    table=cnv->sharedData->mbcs.fromUnicodeTable;
    mbcsIndex=cnv->sharedData->mbcs.mbcsIndex;
    if((cnv->options&UCNV_OPTION_SWAP_LFNL)!=0) {
        results=(uint16_t *)cnv->sharedData->mbcs.swapLFNLFromUnicodeBytes;
    } else {
        results=(uint16_t *)cnv->sharedData->mbcs.fromUnicodeBytes;
    }
    asciiRoundtrips=cnv->sharedData->mbcs.asciiRoundtrips;

    hasSupplementary=(UBool)(cnv->sharedData->mbcs.unicodeMask&UCNV_HAS_SUPPLEMENTARY);

    
    c=(UChar32)utf8->toUnicodeStatus;
    if(c!=0) {
        toULength=oldToULength=utf8->toULength;
        toULimit=(int8_t)utf8->mode;
    } else {
        toULength=oldToULength=toULimit=0;
    }

    







    {
        int32_t i, length;

        length=(int32_t)(sourceLimit-source) - (toULimit-oldToULength);
        for(i=0; i<3 && i<length;) {
            b=*(sourceLimit-i-1);
            if(U8_IS_TRAIL(b)) {
                ++i;
            } else {
                if(i<U8_COUNT_TRAIL_BYTES(b)) {
                    
                    sourceLimit-=i+1;
                }
                break;
            }
        }
    }

    if(c!=0 && targetCapacity>0) {
        utf8->toUnicodeStatus=0;
        utf8->toULength=0;
        goto moreBytes;
        
    }

    
    while(source<sourceLimit) {
        if(targetCapacity>0) {
            b=*source++;
            if((int8_t)b>=0) {
                
                if(IS_ASCII_ROUNDTRIP(b, asciiRoundtrips)) {
                    *target++=b;
                    --targetCapacity;
                    continue;
                } else {
                    value=DBCS_RESULT_FROM_UTF8(mbcsIndex, results, 0, b);
                    if(value==0) {
                        c=b;
                        goto unassigned;
                    }
                }
            } else {
                if(b>0xe0) {
                    if( 
                        (((t1=(uint8_t)(source[0]-0x80), b<0xed) && (t1 <= 0x3f)) ||
                                                        (b==0xed && (t1 <= 0x1f))) &&
                        (t2=(uint8_t)(source[1]-0x80)) <= 0x3f
                    ) {
                        c=((b&0xf)<<6)|t1;
                        source+=2;
                        value=DBCS_RESULT_FROM_UTF8(mbcsIndex, results, c, t2);
                        if(value==0) {
                            c=(c<<6)|t2;
                            goto unassigned;
                        }
                    } else {
                        c=-1;
                    }
                } else if(b<0xe0) {
                    if( 
                        b>=0xc2 &&
                        (t1=(uint8_t)(*source-0x80)) <= 0x3f
                    ) {
                        c=b&0x1f;
                        ++source;
                        value=DBCS_RESULT_FROM_UTF8(mbcsIndex, results, c, t1);
                        if(value==0) {
                            c=(c<<6)|t1;
                            goto unassigned;
                        }
                    } else {
                        c=-1;
                    }
                } else {
                    c=-1;
                }

                if(c<0) {
                    
                    oldToULength=0;
                    toULength=1;
                    toULimit=U8_COUNT_TRAIL_BYTES(b)+1;
                    c=b;
moreBytes:
                    while(toULength<toULimit) {
                        






                        if(source<(uint8_t *)pToUArgs->sourceLimit) {
                            b=*source;
                            if(U8_IS_TRAIL(b)) {
                                ++source;
                                ++toULength;
                                c=(c<<6)+b;
                            } else {
                                break; 
                            }
                        } else {
                            
                            source-=(toULength-oldToULength);
                            while(oldToULength<toULength) {
                                utf8->toUBytes[oldToULength++]=*source++;
                            }
                            utf8->toUnicodeStatus=c;
                            utf8->toULength=toULength;
                            utf8->mode=toULimit;
                            pToUArgs->source=(char *)source;
                            pFromUArgs->target=(char *)target;
                            return;
                        }
                    }

                    if( toULength==toULimit &&      
                        (toULength==3 || toULength==2) &&             
                        (c-=utf8_offsets[toULength])>=utf8_minLegal[toULength] &&
                        (c<=0xd7ff || 0xe000<=c)    
                    ) {
                        stage2Entry=MBCS_STAGE_2_FROM_U(table, c);
                    } else if(
                        toULength==toULimit && toULength==4 &&
                        (0x10000<=(c-=utf8_offsets[4]) && c<=0x10ffff)
                    ) {
                        
                        if(!hasSupplementary) {
                            
                            stage2Entry=0;
                        } else {
                            stage2Entry=MBCS_STAGE_2_FROM_U(table, c);
                        }
                    } else {
                        
                        source-=(toULength-oldToULength);
                        while(oldToULength<toULength) {
                            utf8->toUBytes[oldToULength++]=*source++;
                        }
                        utf8->toULength=toULength;
                        pToUArgs->source=(char *)source;
                        pFromUArgs->target=(char *)target;
                        *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                        return;
                    }

                    
                    
                    value=MBCS_VALUE_2_FROM_STAGE_2(results, stage2Entry, c);

                    
                    if(!(MBCS_FROM_U_IS_ROUNDTRIP(stage2Entry, c) ||
                         (UCNV_FROM_U_USE_FALLBACK(cnv, c) && value!=0))
                    ) {
                        goto unassigned;
                    }
                }
            }

            
            
            if(value<=0xff) {
                
                *target++=(uint8_t)value;
                --targetCapacity;
            } else  {
                *target++=(uint8_t)(value>>8);
                if(2<=targetCapacity) {
                    *target++=(uint8_t)value;
                    targetCapacity-=2;
                } else {
                    cnv->charErrorBuffer[0]=(char)value;
                    cnv->charErrorBufferLength=1;

                    
                    *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                    break;
                }
            }
            continue;

unassigned:
            {
                





                static const UChar nul=0;
                const UChar *noSource=&nul;
                c=_extFromU(cnv, cnv->sharedData,
                            c, &noSource, noSource,
                            &target, target+targetCapacity,
                            NULL, -1,
                            pFromUArgs->flush,
                            pErrorCode);

                if(U_FAILURE(*pErrorCode)) {
                    
                    cnv->fromUChar32=c;
                    break;
                } else if(cnv->preFromUFirstCP>=0) {
                    





                    *pErrorCode=U_USING_DEFAULT_WARNING;
                    break;
                } else {
                    

                    
                    targetCapacity=(int32_t)(pFromUArgs->targetLimit-(char *)target);
                    continue;
                }
            }
        } else {
            
            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
            break;
        }
    }

    




    if(U_SUCCESS(*pErrorCode) &&
            cnv->preFromUFirstCP<0 &&
            source<(sourceLimit=(uint8_t *)pToUArgs->sourceLimit)) {
        c=utf8->toUBytes[0]=b=*source++;
        toULength=1;
        toULimit=U8_COUNT_TRAIL_BYTES(b)+1;
        while(source<sourceLimit) {
            utf8->toUBytes[toULength++]=b=*source++;
            c=(c<<6)+b;
        }
        utf8->toUnicodeStatus=c;
        utf8->toULength=toULength;
        utf8->mode=toULimit;
    }

    
    pToUArgs->source=(char *)source;
    pFromUArgs->target=(char *)target;
}



static void
ucnv_MBCSGetStarters(const UConverter* cnv,
                 UBool starters[256],
                 UErrorCode *pErrorCode) {
    const int32_t *state0;
    int i;

    state0=cnv->sharedData->mbcs.stateTable[cnv->sharedData->mbcs.dbcsOnlyState];
    for(i=0; i<256; ++i) {
        
        starters[i]= (UBool)MBCS_ENTRY_IS_TRANSITION(state0[i]);
    }
}





U_CFUNC UBool
ucnv_MBCSIsLeadByte(UConverterSharedData *sharedData, char byte) {
    return (UBool)MBCS_ENTRY_IS_TRANSITION(sharedData->mbcs.stateTable[0][(uint8_t)byte]);
}

static void
ucnv_MBCSWriteSub(UConverterFromUnicodeArgs *pArgs,
              int32_t offsetIndex,
              UErrorCode *pErrorCode) {
    UConverter *cnv=pArgs->converter;
    char *p, *subchar;
    char buffer[4];
    int32_t length;

    
    if( cnv->subChar1!=0 &&
        (cnv->sharedData->mbcs.extIndexes!=NULL ?
            cnv->useSubChar1 :
            (cnv->invalidUCharBuffer[0]<=0xff))
    ) {
        
        subchar=(char *)&cnv->subChar1;
        length=1;
    } else {
        
        subchar=(char *)cnv->subChars;
        length=cnv->subCharLen;
    }

    
    cnv->useSubChar1=FALSE;

    if (cnv->sharedData->mbcs.outputType == MBCS_OUTPUT_2_SISO) {
        p=buffer;

        
        switch(length) {
        case 1:
            if(cnv->fromUnicodeStatus==2) {
                
                cnv->fromUnicodeStatus=1;
                *p++=UCNV_SI;
            }
            *p++=subchar[0];
            break;
        case 2:
            if(cnv->fromUnicodeStatus<=1) {
                
                cnv->fromUnicodeStatus=2;
                *p++=UCNV_SO;
            }
            *p++=subchar[0];
            *p++=subchar[1];
            break;
        default:
            *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
            return;
        }
        subchar=buffer;
        length=(int32_t)(p-buffer);
    }

    ucnv_cbFromUWriteBytes(pArgs, subchar, length, offsetIndex, pErrorCode);
}

U_CFUNC UConverterType
ucnv_MBCSGetType(const UConverter* converter) {
    
    if(converter->sharedData->mbcs.countStates==1) {
        return (UConverterType)UCNV_SBCS;
    } else if((converter->sharedData->mbcs.outputType&0xff)==MBCS_OUTPUT_2_SISO) {
        return (UConverterType)UCNV_EBCDIC_STATEFUL;
    } else if(converter->sharedData->staticData->minBytesPerChar==2 && converter->sharedData->staticData->maxBytesPerChar==2) {
        return (UConverterType)UCNV_DBCS;
    }
    return (UConverterType)UCNV_MBCS;
}

static const UConverterImpl _SBCSUTF8Impl={
    UCNV_MBCS,

    ucnv_MBCSLoad,
    ucnv_MBCSUnload,

    ucnv_MBCSOpen,
    NULL,
    NULL,

    ucnv_MBCSToUnicodeWithOffsets,
    ucnv_MBCSToUnicodeWithOffsets,
    ucnv_MBCSFromUnicodeWithOffsets,
    ucnv_MBCSFromUnicodeWithOffsets,
    ucnv_MBCSGetNextUChar,

    ucnv_MBCSGetStarters,
    ucnv_MBCSGetName,
    ucnv_MBCSWriteSub,
    NULL,
    ucnv_MBCSGetUnicodeSet,

    NULL,
    ucnv_SBCSFromUTF8
};

static const UConverterImpl _DBCSUTF8Impl={
    UCNV_MBCS,

    ucnv_MBCSLoad,
    ucnv_MBCSUnload,

    ucnv_MBCSOpen,
    NULL,
    NULL,

    ucnv_MBCSToUnicodeWithOffsets,
    ucnv_MBCSToUnicodeWithOffsets,
    ucnv_MBCSFromUnicodeWithOffsets,
    ucnv_MBCSFromUnicodeWithOffsets,
    ucnv_MBCSGetNextUChar,

    ucnv_MBCSGetStarters,
    ucnv_MBCSGetName,
    ucnv_MBCSWriteSub,
    NULL,
    ucnv_MBCSGetUnicodeSet,

    NULL,
    ucnv_DBCSFromUTF8
};

static const UConverterImpl _MBCSImpl={
    UCNV_MBCS,

    ucnv_MBCSLoad,
    ucnv_MBCSUnload,

    ucnv_MBCSOpen,
    NULL,
    NULL,

    ucnv_MBCSToUnicodeWithOffsets,
    ucnv_MBCSToUnicodeWithOffsets,
    ucnv_MBCSFromUnicodeWithOffsets,
    ucnv_MBCSFromUnicodeWithOffsets,
    ucnv_MBCSGetNextUChar,

    ucnv_MBCSGetStarters,
    ucnv_MBCSGetName,
    ucnv_MBCSWriteSub,
    NULL,
    ucnv_MBCSGetUnicodeSet
};






const UConverterSharedData _MBCSData={
    sizeof(UConverterSharedData), 1,
    NULL, NULL, NULL, FALSE, &_MBCSImpl, 
    0
};

#endif 
