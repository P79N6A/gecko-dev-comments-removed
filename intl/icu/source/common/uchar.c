




















#include "unicode/utypes.h"
#include "unicode/uchar.h"
#include "unicode/uscript.h"
#include "unicode/udata.h"
#include "uassert.h"
#include "cmemory.h"
#include "ucln_cmn.h"
#include "utrie2.h"
#include "udataswp.h"
#include "uprops.h"
#include "ustr_imp.h"

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))


#define INCLUDED_FROM_UCHAR_C
#include "uchar_props_data.h"




#define GET_PROPS(c, result) ((result)=UTRIE2_GET16(&propsTrie, c));

U_CFUNC UBool
uprv_haveProperties(UErrorCode *pErrorCode) {
    if(U_FAILURE(*pErrorCode)) {
        return FALSE;
    }
    return TRUE;
}




U_CAPI int8_t U_EXPORT2
u_charType(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return (int8_t)GET_CATEGORY(props);
}


struct _EnumTypeCallback {
    UCharEnumTypeRange *enumRange;
    const void *context;
};

static uint32_t U_CALLCONV
_enumTypeValue(const void *context, uint32_t value) {
    return GET_CATEGORY(value);
}

static UBool U_CALLCONV
_enumTypeRange(const void *context, UChar32 start, UChar32 end, uint32_t value) {
    
    return ((struct _EnumTypeCallback *)context)->
        enumRange(((struct _EnumTypeCallback *)context)->context,
                  start, end+1, (UCharCategory)value);
}

U_CAPI void U_EXPORT2
u_enumCharTypes(UCharEnumTypeRange *enumRange, const void *context) {
    struct _EnumTypeCallback callback;

    if(enumRange==NULL) {
        return;
    }

    callback.enumRange=enumRange;
    callback.context=context;
    utrie2_enum(&propsTrie, _enumTypeValue, _enumTypeRange, &callback);
}


U_CAPI UBool U_EXPORT2
u_islower(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return (UBool)(GET_CATEGORY(props)==U_LOWERCASE_LETTER);
}


U_CAPI UBool U_EXPORT2
u_isupper(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return (UBool)(GET_CATEGORY(props)==U_UPPERCASE_LETTER);
}


U_CAPI UBool U_EXPORT2
u_istitle(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return (UBool)(GET_CATEGORY(props)==U_TITLECASE_LETTER);
}


U_CAPI UBool U_EXPORT2
u_isdigit(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return (UBool)(GET_CATEGORY(props)==U_DECIMAL_DIGIT_NUMBER);
}

U_CAPI UBool U_EXPORT2
u_isxdigit(UChar32 c) {
    uint32_t props;

    
    if(
        (c<=0x66 && c>=0x41 && (c<=0x46 || c>=0x61)) ||
        (c>=0xff21 && c<=0xff46 && (c<=0xff26 || c>=0xff41))
    ) {
        return TRUE;
    }

    GET_PROPS(c, props);
    return (UBool)(GET_CATEGORY(props)==U_DECIMAL_DIGIT_NUMBER);
}


U_CAPI UBool U_EXPORT2
u_isalpha(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return (UBool)((CAT_MASK(props)&U_GC_L_MASK)!=0);
}

U_CAPI UBool U_EXPORT2
u_isUAlphabetic(UChar32 c) {
    return (u_getUnicodeProperties(c, 1)&U_MASK(UPROPS_ALPHABETIC))!=0;
}


U_CAPI UBool U_EXPORT2
u_isalnum(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return (UBool)((CAT_MASK(props)&(U_GC_L_MASK|U_GC_ND_MASK))!=0);
}





U_CFUNC UBool
u_isalnumPOSIX(UChar32 c) {
    return (UBool)(u_isUAlphabetic(c) || u_isdigit(c));
}


U_CAPI UBool U_EXPORT2
u_isdefined(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return (UBool)(GET_CATEGORY(props)!=0);
}


U_CAPI UBool U_EXPORT2
u_isbase(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return (UBool)((CAT_MASK(props)&(U_GC_L_MASK|U_GC_N_MASK|U_GC_MC_MASK|U_GC_ME_MASK))!=0);
}


U_CAPI UBool U_EXPORT2
u_iscntrl(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return (UBool)((CAT_MASK(props)&(U_GC_CC_MASK|U_GC_CF_MASK|U_GC_ZL_MASK|U_GC_ZP_MASK))!=0);
}

U_CAPI UBool U_EXPORT2
u_isISOControl(UChar32 c) {
    return (uint32_t)c<=0x9f && (c<=0x1f || c>=0x7f);
}


#define IS_THAT_CONTROL_SPACE(c) \
    (c<=0x9f && ((c>=TAB && c<=CR) || (c>=0x1c && c <=0x1f) || c==NL))


#define IS_THAT_ASCII_CONTROL_SPACE(c) \
    (c<=0x1f && c>=TAB && (c<=CR || c>=0x1c))


U_CAPI UBool U_EXPORT2
u_isspace(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return (UBool)((CAT_MASK(props)&U_GC_Z_MASK)!=0 || IS_THAT_CONTROL_SPACE(c));
}

U_CAPI UBool U_EXPORT2
u_isJavaSpaceChar(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return (UBool)((CAT_MASK(props)&U_GC_Z_MASK)!=0);
}


U_CAPI UBool U_EXPORT2
u_isWhitespace(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return (UBool)(
                ((CAT_MASK(props)&U_GC_Z_MASK)!=0 &&
                    c!=NBSP && c!=FIGURESP && c!=NNBSP) || 
                IS_THAT_ASCII_CONTROL_SPACE(c)
           );
}

U_CAPI UBool U_EXPORT2
u_isblank(UChar32 c) {
    if((uint32_t)c<=0x9f) {
        return c==9 || c==0x20; 
    } else {
        
        uint32_t props;
        GET_PROPS(c, props);
        return (UBool)(GET_CATEGORY(props)==U_SPACE_SEPARATOR);
    }
}

U_CAPI UBool U_EXPORT2
u_isUWhiteSpace(UChar32 c) {
    return (u_getUnicodeProperties(c, 1)&U_MASK(UPROPS_WHITE_SPACE))!=0;
}


U_CAPI UBool U_EXPORT2
u_isprint(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    
    return (UBool)((CAT_MASK(props)&U_GC_C_MASK)==0);
}






U_CFUNC UBool
u_isprintPOSIX(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    



    return (UBool)((GET_CATEGORY(props)==U_SPACE_SEPARATOR) || u_isgraphPOSIX(c));
}

U_CAPI UBool U_EXPORT2
u_isgraph(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    
    return (UBool)((CAT_MASK(props)&
                    (U_GC_CC_MASK|U_GC_CF_MASK|U_GC_CS_MASK|U_GC_CN_MASK|U_GC_Z_MASK))
                   ==0);
}








U_CFUNC UBool
u_isgraphPOSIX(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    
    
    return (UBool)((CAT_MASK(props)&
                    (U_GC_CC_MASK|U_GC_CS_MASK|U_GC_CN_MASK|U_GC_Z_MASK))
                   ==0);
}

U_CAPI UBool U_EXPORT2
u_ispunct(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return (UBool)((CAT_MASK(props)&U_GC_P_MASK)!=0);
}


U_CAPI UBool U_EXPORT2
u_isIDStart(UChar32 c) {
    
    uint32_t props;
    GET_PROPS(c, props);
    return (UBool)((CAT_MASK(props)&(U_GC_L_MASK|U_GC_NL_MASK))!=0);
}



U_CAPI UBool U_EXPORT2
u_isIDPart(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return (UBool)(
           (CAT_MASK(props)&
            (U_GC_ND_MASK|U_GC_NL_MASK|
             U_GC_L_MASK|
             U_GC_PC_MASK|U_GC_MC_MASK|U_GC_MN_MASK)
           )!=0 ||
           u_isIDIgnorable(c));
}


U_CAPI UBool U_EXPORT2
u_isIDIgnorable(UChar32 c) {
    if(c<=0x9f) {
        return u_isISOControl(c) && !IS_THAT_ASCII_CONTROL_SPACE(c);
    } else {
        uint32_t props;
        GET_PROPS(c, props);
        return (UBool)(GET_CATEGORY(props)==U_FORMAT_CHAR);
    }
}


U_CAPI UBool U_EXPORT2
u_isJavaIDStart(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return (UBool)((CAT_MASK(props)&(U_GC_L_MASK|U_GC_SC_MASK|U_GC_PC_MASK))!=0);
}




U_CAPI UBool U_EXPORT2
u_isJavaIDPart(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return (UBool)(
           (CAT_MASK(props)&
            (U_GC_ND_MASK|U_GC_NL_MASK|
             U_GC_L_MASK|
             U_GC_SC_MASK|U_GC_PC_MASK|
             U_GC_MC_MASK|U_GC_MN_MASK)
           )!=0 ||
           u_isIDIgnorable(c));
}

U_CAPI int32_t U_EXPORT2
u_charDigitValue(UChar32 c) {
    uint32_t props;
    int32_t value;
    GET_PROPS(c, props);
    value=(int32_t)GET_NUMERIC_TYPE_VALUE(props)-UPROPS_NTV_DECIMAL_START;
    if(value<=9) {
        return value;
    } else {
        return -1;
    }
}

U_CAPI double U_EXPORT2
u_getNumericValue(UChar32 c) {
    uint32_t props;
    int32_t ntv;
    GET_PROPS(c, props);
    ntv=(int32_t)GET_NUMERIC_TYPE_VALUE(props);

    if(ntv==UPROPS_NTV_NONE) {
        return U_NO_NUMERIC_VALUE;
    } else if(ntv<UPROPS_NTV_DIGIT_START) {
        
        return ntv-UPROPS_NTV_DECIMAL_START;
    } else if(ntv<UPROPS_NTV_NUMERIC_START) {
        
        return ntv-UPROPS_NTV_DIGIT_START;
    } else if(ntv<UPROPS_NTV_FRACTION_START) {
        
        return ntv-UPROPS_NTV_NUMERIC_START;
    } else if(ntv<UPROPS_NTV_LARGE_START) {
        
        int32_t numerator=(ntv>>4)-12;
        int32_t denominator=(ntv&0xf)+1;
        return (double)numerator/denominator;
    } else if(ntv<UPROPS_NTV_BASE60_START) {
        
        double numValue;
        int32_t mant=(ntv>>5)-14;
        int32_t exp=(ntv&0x1f)+2;
        numValue=mant;

        
        while(exp>=4) {
            numValue*=10000.;
            exp-=4;
        }
        switch(exp) {
        case 3:
            numValue*=1000.;
            break;
        case 2:
            numValue*=100.;
            break;
        case 1:
            numValue*=10.;
            break;
        case 0:
        default:
            break;
        }

        return numValue;
    } else if(ntv<UPROPS_NTV_RESERVED_START) {
        
        int32_t numValue=(ntv>>2)-0xbf;
        int32_t exp=(ntv&3)+1;

        switch(exp) {
        case 4:
            numValue*=60*60*60*60;
            break;
        case 3:
            numValue*=60*60*60;
            break;
        case 2:
            numValue*=60*60;
            break;
        case 1:
            numValue*=60;
            break;
        case 0:
        default:
            break;
        }

        return numValue;
    } else {
        
        return U_NO_NUMERIC_VALUE;
    }
}

U_CAPI int32_t U_EXPORT2
u_digit(UChar32 ch, int8_t radix) {
    int8_t value;
    if((uint8_t)(radix-2)<=(36-2)) {
        value=(int8_t)u_charDigitValue(ch);
        if(value<0) {
            
            if(ch>=0x61 && ch<=0x7A) {
                value=(int8_t)(ch-0x57);  
            } else if(ch>=0x41 && ch<=0x5A) {
                value=(int8_t)(ch-0x37);  
            } else if(ch>=0xFF41 && ch<=0xFF5A) {
                value=(int8_t)(ch-0xFF37);  
            } else if(ch>=0xFF21 && ch<=0xFF3A) {
                value=(int8_t)(ch-0xFF17);  
            }
        }
    } else {
        value=-1;   
    }
    return (int8_t)((value<radix) ? value : -1);
}

U_CAPI UChar32 U_EXPORT2
u_forDigit(int32_t digit, int8_t radix) {
    if((uint8_t)(radix-2)>(36-2) || (uint32_t)digit>=(uint32_t)radix) {
        return 0;
    } else if(digit<10) {
        return (UChar32)(0x30+digit);
    } else {
        return (UChar32)((0x61-10)+digit);
    }
}



U_CAPI void U_EXPORT2
u_getUnicodeVersion(UVersionInfo versionArray) {
    if(versionArray!=NULL) {
        uprv_memcpy(versionArray, dataVersion, U_MAX_VERSION_LENGTH);
    }
}

U_CFUNC uint32_t
u_getMainProperties(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return props;
}

U_CFUNC uint32_t
u_getUnicodeProperties(UChar32 c, int32_t column) {
    U_ASSERT(column>=0);
    if(column>=propsVectorsColumns) {
        return 0;
    } else {
        uint16_t vecIndex=UTRIE2_GET16(&propsVectorsTrie, c);
        return propsVectors[vecIndex+column];
    }
}

U_CFUNC int32_t
uprv_getMaxValues(int32_t column) {
    switch(column) {
    case 0:
        return indexes[UPROPS_MAX_VALUES_INDEX];
    case 2:
        return indexes[UPROPS_MAX_VALUES_2_INDEX];
    default:
        return 0;
    }
}

U_CAPI void U_EXPORT2
u_charAge(UChar32 c, UVersionInfo versionArray) {
    if(versionArray!=NULL) {
        uint32_t version=u_getUnicodeProperties(c, 0)>>UPROPS_AGE_SHIFT;
        versionArray[0]=(uint8_t)(version>>4);
        versionArray[1]=(uint8_t)(version&0xf);
        versionArray[2]=versionArray[3]=0;
    }
}

U_CAPI UScriptCode U_EXPORT2
uscript_getScript(UChar32 c, UErrorCode *pErrorCode) {
    uint32_t scriptX;
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return USCRIPT_INVALID_CODE;
    }
    if((uint32_t)c>0x10ffff) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return USCRIPT_INVALID_CODE;
    }
    scriptX=u_getUnicodeProperties(c, 0)&UPROPS_SCRIPT_X_MASK;
    if(scriptX<UPROPS_SCRIPT_X_WITH_COMMON) {
        return (UScriptCode)scriptX;
    } else if(scriptX<UPROPS_SCRIPT_X_WITH_INHERITED) {
        return USCRIPT_COMMON;
    } else if(scriptX<UPROPS_SCRIPT_X_WITH_OTHER) {
        return USCRIPT_INHERITED;
    } else {
        return (UScriptCode)scriptExtensions[scriptX&UPROPS_SCRIPT_MASK];
    }
}

U_CAPI UBool U_EXPORT2
uscript_hasScript(UChar32 c, UScriptCode sc) {
    const uint16_t *scx;
    uint32_t scriptX=u_getUnicodeProperties(c, 0)&UPROPS_SCRIPT_X_MASK;
    if(scriptX<UPROPS_SCRIPT_X_WITH_COMMON) {
        return sc==(UScriptCode)scriptX;
    }

    scx=scriptExtensions+(scriptX&UPROPS_SCRIPT_MASK);
    if(scriptX>=UPROPS_SCRIPT_X_WITH_OTHER) {
        scx=scriptExtensions+scx[1];
    }
    if(sc>=USCRIPT_CODE_LIMIT) {
        
        return FALSE;
    }
    while(sc>*scx) {
        ++scx;
    }
    return sc==(*scx&0x7fff);
}

U_CAPI int32_t U_EXPORT2
uscript_getScriptExtensions(UChar32 c,
                            UScriptCode *scripts, int32_t capacity,
                            UErrorCode *pErrorCode) {
    uint32_t scriptX;
    int32_t length;
    const uint16_t *scx;
    uint16_t sx;
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }
    if(capacity<0 || (capacity>0 && scripts==NULL)) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }
    scriptX=u_getUnicodeProperties(c, 0)&UPROPS_SCRIPT_X_MASK;
    if(scriptX<UPROPS_SCRIPT_X_WITH_COMMON) {
        if(capacity==0) {
            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
        } else {
            scripts[0]=(UScriptCode)scriptX;
        }
        return 1;
    }

    scx=scriptExtensions+(scriptX&UPROPS_SCRIPT_MASK);
    if(scriptX>=UPROPS_SCRIPT_X_WITH_OTHER) {
        scx=scriptExtensions+scx[1];
    }
    length=0;
    do {
        sx=*scx++;
        if(length<capacity) {
            scripts[length]=(UScriptCode)(sx&0x7fff);
        }
        ++length;
    } while(sx<0x8000);
    if(length>capacity) {
        *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
    }
    return length;
}

U_CAPI UBlockCode U_EXPORT2
ublock_getCode(UChar32 c) {
    return (UBlockCode)((u_getUnicodeProperties(c, 0)&UPROPS_BLOCK_MASK)>>UPROPS_BLOCK_SHIFT);
}



static UBool U_CALLCONV
_enumPropertyStartsRange(const void *context, UChar32 start, UChar32 end, uint32_t value) {
    
    const USetAdder *sa=(const USetAdder *)context;
    sa->add(sa->set, start);
    return TRUE;
}

#define USET_ADD_CP_AND_NEXT(sa, cp) sa->add(sa->set, cp); sa->add(sa->set, cp+1)

U_CFUNC void U_EXPORT2
uchar_addPropertyStarts(const USetAdder *sa, UErrorCode *pErrorCode) {
    if(U_FAILURE(*pErrorCode)) {
        return;
    }

    
    utrie2_enum(&propsTrie, NULL, _enumPropertyStartsRange, sa);

    

    
    USET_ADD_CP_AND_NEXT(sa, TAB);

    
    sa->add(sa->set, CR+1); 
    sa->add(sa->set, 0x1c);
    sa->add(sa->set, 0x1f+1);
    USET_ADD_CP_AND_NEXT(sa, NL);

    
    sa->add(sa->set, DEL); 
    sa->add(sa->set, HAIRSP);
    sa->add(sa->set, RLM+1);
    sa->add(sa->set, INHSWAP);
    sa->add(sa->set, NOMDIG+1);
    USET_ADD_CP_AND_NEXT(sa, ZWNBSP);

    
    USET_ADD_CP_AND_NEXT(sa, NBSP);
    USET_ADD_CP_AND_NEXT(sa, FIGURESP);
    USET_ADD_CP_AND_NEXT(sa, NNBSP);

    
    sa->add(sa->set, U_a);
    sa->add(sa->set, U_z+1);
    sa->add(sa->set, U_A);
    sa->add(sa->set, U_Z+1);
    sa->add(sa->set, U_FW_a);
    sa->add(sa->set, U_FW_z+1);
    sa->add(sa->set, U_FW_A);
    sa->add(sa->set, U_FW_Z+1);

    
    sa->add(sa->set, U_f+1);
    sa->add(sa->set, U_F+1);
    sa->add(sa->set, U_FW_f+1);
    sa->add(sa->set, U_FW_F+1);

    
    sa->add(sa->set, WJ); 
    sa->add(sa->set, 0xfff0);
    sa->add(sa->set, 0xfffb+1);
    sa->add(sa->set, 0xe0000);
    sa->add(sa->set, 0xe0fff+1);

    
    USET_ADD_CP_AND_NEXT(sa, CGJ);
}

U_CFUNC void U_EXPORT2
upropsvec_addPropertyStarts(const USetAdder *sa, UErrorCode *pErrorCode) {
    if(U_FAILURE(*pErrorCode)) {
        return;
    }

    
    if(propsVectorsColumns>0) {
        
        utrie2_enum(&propsVectorsTrie, NULL, _enumPropertyStartsRange, sa);
    }
}
