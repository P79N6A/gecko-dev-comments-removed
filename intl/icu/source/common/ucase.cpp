


















#include "unicode/utypes.h"
#include "unicode/unistr.h"
#include "unicode/uset.h"
#include "unicode/udata.h" 
#include "unicode/utf16.h"
#include "ucmndata.h" 
#include "udatamem.h"
#include "umutex.h"
#include "uassert.h"
#include "cmemory.h"
#include "utrie2.h"
#include "ucase.h"
#include "ucln_cmn.h"

struct UCaseProps {
    UDataMemory *mem;
    const int32_t *indexes;
    const uint16_t *exceptions;
    const uint16_t *unfold;

    UTrie2 trie;
    uint8_t formatVersion[4];
};


#define INCLUDED_FROM_UCASE_CPP
#include "ucase_props_data.h"



U_CAPI const UCaseProps * U_EXPORT2
ucase_getSingleton() {
    return &ucase_props_singleton;
}



static UBool U_CALLCONV
_enumPropertyStartsRange(const void *context, UChar32 start, UChar32 , uint32_t ) {
    
    const USetAdder *sa=(const USetAdder *)context;
    sa->add(sa->set, start);
    return TRUE;
}

U_CFUNC void U_EXPORT2
ucase_addPropertyStarts(const UCaseProps *csp, const USetAdder *sa, UErrorCode *pErrorCode) {
    if(U_FAILURE(*pErrorCode)) {
        return;
    }

    
    utrie2_enum(&csp->trie, NULL, _enumPropertyStartsRange, sa);

    

    

    



}



#define GET_EXCEPTIONS(csp, props) ((csp)->exceptions+((props)>>UCASE_EXC_SHIFT))

#define PROPS_HAS_EXCEPTION(props) ((props)&UCASE_EXCEPTION)


static const uint8_t flagsOffset[256]={
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
};

#define HAS_SLOT(flags, idx) ((flags)&(1<<(idx)))
#define SLOT_OFFSET(flags, idx) flagsOffset[(flags)&((1<<(idx))-1)]










#define GET_SLOT_VALUE(excWord, idx, pExc16, value) \
    if(((excWord)&UCASE_EXC_DOUBLE_SLOTS)==0) { \
        (pExc16)+=SLOT_OFFSET(excWord, idx); \
        (value)=*pExc16; \
    } else { \
        (pExc16)+=2*SLOT_OFFSET(excWord, idx); \
        (value)=*pExc16++; \
        (value)=((value)<<16)|*pExc16; \
    }



U_CAPI UChar32 U_EXPORT2
ucase_tolower(const UCaseProps *csp, UChar32 c) {
    uint16_t props=UTRIE2_GET16(&csp->trie, c);
    if(!PROPS_HAS_EXCEPTION(props)) {
        if(UCASE_GET_TYPE(props)>=UCASE_UPPER) {
            c+=UCASE_GET_DELTA(props);
        }
    } else {
        const uint16_t *pe=GET_EXCEPTIONS(csp, props);
        uint16_t excWord=*pe++;
        if(HAS_SLOT(excWord, UCASE_EXC_LOWER)) {
            GET_SLOT_VALUE(excWord, UCASE_EXC_LOWER, pe, c);
        }
    }
    return c;
}

U_CAPI UChar32 U_EXPORT2
ucase_toupper(const UCaseProps *csp, UChar32 c) {
    uint16_t props=UTRIE2_GET16(&csp->trie, c);
    if(!PROPS_HAS_EXCEPTION(props)) {
        if(UCASE_GET_TYPE(props)==UCASE_LOWER) {
            c+=UCASE_GET_DELTA(props);
        }
    } else {
        const uint16_t *pe=GET_EXCEPTIONS(csp, props);
        uint16_t excWord=*pe++;
        if(HAS_SLOT(excWord, UCASE_EXC_UPPER)) {
            GET_SLOT_VALUE(excWord, UCASE_EXC_UPPER, pe, c);
        }
    }
    return c;
}

U_CAPI UChar32 U_EXPORT2
ucase_totitle(const UCaseProps *csp, UChar32 c) {
    uint16_t props=UTRIE2_GET16(&csp->trie, c);
    if(!PROPS_HAS_EXCEPTION(props)) {
        if(UCASE_GET_TYPE(props)==UCASE_LOWER) {
            c+=UCASE_GET_DELTA(props);
        }
    } else {
        const uint16_t *pe=GET_EXCEPTIONS(csp, props);
        uint16_t excWord=*pe++;
        int32_t idx;
        if(HAS_SLOT(excWord, UCASE_EXC_TITLE)) {
            idx=UCASE_EXC_TITLE;
        } else if(HAS_SLOT(excWord, UCASE_EXC_UPPER)) {
            idx=UCASE_EXC_UPPER;
        } else {
            return c;
        }
        GET_SLOT_VALUE(excWord, idx, pe, c);
    }
    return c;
}

static const UChar iDot[2] = { 0x69, 0x307 };
static const UChar jDot[2] = { 0x6a, 0x307 };
static const UChar iOgonekDot[3] = { 0x12f, 0x307 };
static const UChar iDotGrave[3] = { 0x69, 0x307, 0x300 };
static const UChar iDotAcute[3] = { 0x69, 0x307, 0x301 };
static const UChar iDotTilde[3] = { 0x69, 0x307, 0x303 };


U_CFUNC void U_EXPORT2
ucase_addCaseClosure(const UCaseProps *csp, UChar32 c, const USetAdder *sa) {
    uint16_t props;

    







    switch(c) {
    case 0x49:
        
        sa->add(sa->set, 0x69);
        return;
    case 0x69:
        sa->add(sa->set, 0x49);
        return;
    case 0x130:
        
        sa->addString(sa->set, iDot, 2);
        return;
    case 0x131:
        
        return;
    default:
        
        break;
    }

    props=UTRIE2_GET16(&csp->trie, c);
    if(!PROPS_HAS_EXCEPTION(props)) {
        if(UCASE_GET_TYPE(props)!=UCASE_NONE) {
            
            int32_t delta=UCASE_GET_DELTA(props);
            if(delta!=0) {
                sa->add(sa->set, c+delta);
            }
        }
    } else {
        



        const uint16_t *pe0, *pe=GET_EXCEPTIONS(csp, props);
        const UChar *closure;
        uint16_t excWord=*pe++;
        int32_t idx, closureLength, fullLength, length;

        pe0=pe;

        
        for(idx=UCASE_EXC_LOWER; idx<=UCASE_EXC_TITLE; ++idx) {
            if(HAS_SLOT(excWord, idx)) {
                pe=pe0;
                GET_SLOT_VALUE(excWord, idx, pe, c);
                sa->add(sa->set, c);
            }
        }

        
        if(HAS_SLOT(excWord, UCASE_EXC_CLOSURE)) {
            pe=pe0;
            GET_SLOT_VALUE(excWord, UCASE_EXC_CLOSURE, pe, closureLength);
            closureLength&=UCASE_CLOSURE_MAX_LENGTH; 
            closure=(const UChar *)pe+1; 
        } else {
            closureLength=0;
            closure=NULL;
        }

        
        if(HAS_SLOT(excWord, UCASE_EXC_FULL_MAPPINGS)) {
            pe=pe0;
            GET_SLOT_VALUE(excWord, UCASE_EXC_FULL_MAPPINGS, pe, fullLength);

            
            ++pe;

            fullLength&=0xffff; 

            
            pe+=fullLength&UCASE_FULL_LOWER;
            fullLength>>=4;

            
            length=fullLength&0xf;
            if(length!=0) {
                sa->addString(sa->set, (const UChar *)pe, length);
                pe+=length;
            }

            
            fullLength>>=4;
            pe+=fullLength&0xf;
            fullLength>>=4;
            pe+=fullLength;

            closure=(const UChar *)pe; 
        }

        
        for(idx=0; idx<closureLength;) {
            U16_NEXT_UNSAFE(closure, idx, c);
            sa->add(sa->set, c);
        }
    }
}





static inline int32_t
strcmpMax(const UChar *s, int32_t length, const UChar *t, int32_t max) {
    int32_t c1, c2;

    max-=length; 
    do {
        c1=*s++;
        c2=*t++;
        if(c2==0) {
            return 1; 
        }
        c1-=c2;
        if(c1!=0) {
            return c1; 
        }
    } while(--length>0);
    

    if(max==0 || *t==0) {
        return 0; 
    } else {
        return -max; 
    }
}

U_CFUNC UBool U_EXPORT2
ucase_addStringCaseClosure(const UCaseProps *csp, const UChar *s, int32_t length, const USetAdder *sa) {
    int32_t i, start, limit, result, unfoldRows, unfoldRowWidth, unfoldStringWidth;

    if(csp->unfold==NULL || s==NULL) {
        return FALSE; 
    }
    if(length<=1) {
        
        





        return FALSE;
    }

    const uint16_t *unfold=csp->unfold;
    unfoldRows=unfold[UCASE_UNFOLD_ROWS];
    unfoldRowWidth=unfold[UCASE_UNFOLD_ROW_WIDTH];
    unfoldStringWidth=unfold[UCASE_UNFOLD_STRING_WIDTH];
    unfold+=unfoldRowWidth;

    if(length>unfoldStringWidth) {
        
        return FALSE;
    }

    
    start=0;
    limit=unfoldRows;
    while(start<limit) {
        i=(start+limit)/2;
        const UChar *p=reinterpret_cast<const UChar *>(unfold+(i*unfoldRowWidth));
        result=strcmpMax(s, length, p, unfoldStringWidth);

        if(result==0) {
            
            UChar32 c;

            for(i=unfoldStringWidth; i<unfoldRowWidth && p[i]!=0;) {
                U16_NEXT_UNSAFE(p, i, c);
                sa->add(sa->set, c);
                ucase_addCaseClosure(csp, c, sa);
            }
            return TRUE;
        } else if(result<0) {
            limit=i;
        } else  {
            start=i+1;
        }
    }

    return FALSE; 
}

U_NAMESPACE_BEGIN

FullCaseFoldingIterator::FullCaseFoldingIterator()
        : unfold(reinterpret_cast<const UChar *>(ucase_props_singleton.unfold)),
          unfoldRows(unfold[UCASE_UNFOLD_ROWS]),
          unfoldRowWidth(unfold[UCASE_UNFOLD_ROW_WIDTH]),
          unfoldStringWidth(unfold[UCASE_UNFOLD_STRING_WIDTH]),
          currentRow(0),
          rowCpIndex(unfoldStringWidth) {
    unfold+=unfoldRowWidth;
}

UChar32
FullCaseFoldingIterator::next(UnicodeString &full) {
    
    const UChar *p=unfold+(currentRow*unfoldRowWidth);
    if(rowCpIndex>=unfoldRowWidth || p[rowCpIndex]==0) {
        ++currentRow;
        p+=unfoldRowWidth;
        rowCpIndex=unfoldStringWidth;
    }
    if(currentRow>=unfoldRows) { return U_SENTINEL; }
    
    int32_t length=unfoldStringWidth;
    while(length>0 && p[length-1]==0) { --length; }
    full.setTo(FALSE, p, length);
    
    UChar32 c;
    U16_NEXT_UNSAFE(p, rowCpIndex, c);
    return c;
}

U_NAMESPACE_END


U_CAPI int32_t U_EXPORT2
ucase_getType(const UCaseProps *csp, UChar32 c) {
    uint16_t props=UTRIE2_GET16(&csp->trie, c);
    return UCASE_GET_TYPE(props);
}


U_CAPI int32_t U_EXPORT2
ucase_getTypeOrIgnorable(const UCaseProps *csp, UChar32 c) {
    uint16_t props=UTRIE2_GET16(&csp->trie, c);
    return UCASE_GET_TYPE_AND_IGNORABLE(props);
}


static inline int32_t
getDotType(const UCaseProps *csp, UChar32 c) {
    uint16_t props=UTRIE2_GET16(&csp->trie, c);
    if(!PROPS_HAS_EXCEPTION(props)) {
        return props&UCASE_DOT_MASK;
    } else {
        const uint16_t *pe=GET_EXCEPTIONS(csp, props);
        return (*pe>>UCASE_EXC_DOT_SHIFT)&UCASE_DOT_MASK;
    }
}

U_CAPI UBool U_EXPORT2
ucase_isSoftDotted(const UCaseProps *csp, UChar32 c) {
    return (UBool)(getDotType(csp, c)==UCASE_SOFT_DOTTED);
}

U_CAPI UBool U_EXPORT2
ucase_isCaseSensitive(const UCaseProps *csp, UChar32 c) {
    uint16_t props=UTRIE2_GET16(&csp->trie, c);
    return (UBool)((props&UCASE_SENSITIVE)!=0);
}

















































































#define is_a(c) ((c)=='a' || (c)=='A')
#define is_d(c) ((c)=='d' || (c)=='D')
#define is_e(c) ((c)=='e' || (c)=='E')
#define is_i(c) ((c)=='i' || (c)=='I')
#define is_l(c) ((c)=='l' || (c)=='L')
#define is_n(c) ((c)=='n' || (c)=='N')
#define is_r(c) ((c)=='r' || (c)=='R')
#define is_t(c) ((c)=='t' || (c)=='T')
#define is_u(c) ((c)=='u' || (c)=='U')
#define is_z(c) ((c)=='z' || (c)=='Z')


#define is_sep(c) ((c)=='_' || (c)=='-' || (c)==0)






U_CFUNC int32_t
ucase_getCaseLocale(const char *locale, int32_t *locCache) {
    int32_t result;
    char c;

    if(locCache!=NULL && (result=*locCache)!=UCASE_LOC_UNKNOWN) {
        return result;
    }

    result=UCASE_LOC_ROOT;

    








    c=*locale++;
    if(is_t(c)) {
        
        c=*locale++;
        if(is_u(c)) {
            c=*locale++;
        }
        if(is_r(c)) {
            c=*locale;
            if(is_sep(c)) {
                result=UCASE_LOC_TURKISH;
            }
        }
    } else if(is_a(c)) {
        
        c=*locale++;
        if(is_z(c)) {
            c=*locale++;
            if(is_e(c)) {
                c=*locale;
            }
            if(is_sep(c)) {
                result=UCASE_LOC_TURKISH;
            }
        }
    } else if(is_l(c)) {
        
        c=*locale++;
        if(is_i(c)) {
            c=*locale++;
        }
        if(is_t(c)) {
            c=*locale;
            if(is_sep(c)) {
                result=UCASE_LOC_LITHUANIAN;
            }
        }
    } else if(is_n(c)) {
        
        c=*locale++;
        if(is_l(c)) {
            c=*locale++;
            if(is_d(c)) {
                c=*locale;
            }
            if(is_sep(c)) {
                result=UCASE_LOC_DUTCH;
            }
        }
    }

    if(locCache!=NULL) {
        *locCache=result;
    }
    return result;
}









static UBool
isFollowedByCasedLetter(const UCaseProps *csp, UCaseContextIterator *iter, void *context, int8_t dir) {
    UChar32 c;

    if(iter==NULL) {
        return FALSE;
    }

    for(; (c=iter(context, dir))>=0; dir=0) {
        int32_t type=ucase_getTypeOrIgnorable(csp, c);
        if(type&4) {
            
        } else if(type!=UCASE_NONE) {
            return TRUE; 
        } else {
            return FALSE; 
        }
    }

    return FALSE; 
}


static UBool
isPrecededBySoftDotted(const UCaseProps *csp, UCaseContextIterator *iter, void *context) {
    UChar32 c;
    int32_t dotType;
    int8_t dir;

    if(iter==NULL) {
        return FALSE;
    }

    for(dir=-1; (c=iter(context, dir))>=0; dir=0) {
        dotType=getDotType(csp, c);
        if(dotType==UCASE_SOFT_DOTTED) {
            return TRUE; 
        } else if(dotType!=UCASE_OTHER_ACCENT) {
            return FALSE; 
        }
    }

    return FALSE; 
}




































static UBool
isPrecededBy_I(const UCaseProps *csp, UCaseContextIterator *iter, void *context) {
    UChar32 c;
    int32_t dotType;
    int8_t dir;

    if(iter==NULL) {
        return FALSE;
    }

    for(dir=-1; (c=iter(context, dir))>=0; dir=0) {
        if(c==0x49) {
            return TRUE; 
        }
        dotType=getDotType(csp, c);
        if(dotType!=UCASE_OTHER_ACCENT) {
            return FALSE; 
        }
    }

    return FALSE; 
}


static UBool
isFollowedByMoreAbove(const UCaseProps *csp, UCaseContextIterator *iter, void *context) {
    UChar32 c;
    int32_t dotType;
    int8_t dir;

    if(iter==NULL) {
        return FALSE;
    }

    for(dir=1; (c=iter(context, dir))>=0; dir=0) {
        dotType=getDotType(csp, c);
        if(dotType==UCASE_ABOVE) {
            return TRUE; 
        } else if(dotType!=UCASE_OTHER_ACCENT) {
            return FALSE; 
        }
    }

    return FALSE; 
}


static UBool
isFollowedByDotAbove(const UCaseProps *csp, UCaseContextIterator *iter, void *context) {
    UChar32 c;
    int32_t dotType;
    int8_t dir;

    if(iter==NULL) {
        return FALSE;
    }

    for(dir=1; (c=iter(context, dir))>=0; dir=0) {
        if(c==0x307) {
            return TRUE;
        }
        dotType=getDotType(csp, c);
        if(dotType!=UCASE_OTHER_ACCENT) {
            return FALSE; 
        }
    }

    return FALSE; 
}

U_CAPI int32_t U_EXPORT2
ucase_toFullLower(const UCaseProps *csp, UChar32 c,
                  UCaseContextIterator *iter, void *context,
                  const UChar **pString,
                  const char *locale, int32_t *locCache)
{
    UChar32 result=c;
    uint16_t props=UTRIE2_GET16(&csp->trie, c);
    if(!PROPS_HAS_EXCEPTION(props)) {
        if(UCASE_GET_TYPE(props)>=UCASE_UPPER) {
            result=c+UCASE_GET_DELTA(props);
        }
    } else {
        const uint16_t *pe=GET_EXCEPTIONS(csp, props), *pe2;
        uint16_t excWord=*pe++;
        int32_t full;

        pe2=pe;

        if(excWord&UCASE_EXC_CONDITIONAL_SPECIAL) {
            
            int32_t loc=ucase_getCaseLocale(locale, locCache);

            





            if( loc==UCASE_LOC_LITHUANIAN &&
                    
                    (((c==0x49 || c==0x4a || c==0x12e) &&
                        isFollowedByMoreAbove(csp, iter, context)) ||
                    
                    (c==0xcc || c==0xcd || c==0x128))
            ) {
                















                switch(c) {
                case 0x49:  
                    *pString=iDot;
                    return 2;
                case 0x4a:  
                    *pString=jDot;
                    return 2;
                case 0x12e: 
                    *pString=iOgonekDot;
                    return 2;
                case 0xcc:  
                    *pString=iDotGrave;
                    return 3;
                case 0xcd:  
                    *pString=iDotAcute;
                    return 3;
                case 0x128: 
                    *pString=iDotTilde;
                    return 3;
                default:
                    return 0; 
                }
            
            } else if(loc==UCASE_LOC_TURKISH && c==0x130) {
                






                return 0x69;
            } else if(loc==UCASE_LOC_TURKISH && c==0x307 && isPrecededBy_I(csp, iter, context)) {
                






                return 0; 
            } else if(loc==UCASE_LOC_TURKISH && c==0x49 && !isFollowedByDotAbove(csp, iter, context)) {
                





                return 0x131;
            } else if(c==0x130) {
                




                *pString=iDot;
                return 2;
            } else if(  c==0x3a3 &&
                        !isFollowedByCasedLetter(csp, iter, context, 1) &&
                        isFollowedByCasedLetter(csp, iter, context, -1) 
            ) {
                
                




                return 0x3c2; 
            } else {
                
            }
        } else if(HAS_SLOT(excWord, UCASE_EXC_FULL_MAPPINGS)) {
            GET_SLOT_VALUE(excWord, UCASE_EXC_FULL_MAPPINGS, pe, full);
            full&=UCASE_FULL_LOWER;
            if(full!=0) {
                
                *pString=reinterpret_cast<const UChar *>(pe+1);

                
                return full;
            }
        }

        if(HAS_SLOT(excWord, UCASE_EXC_LOWER)) {
            GET_SLOT_VALUE(excWord, UCASE_EXC_LOWER, pe2, result);
        }
    }

    return (result==c) ? ~result : result;
}


static int32_t
toUpperOrTitle(const UCaseProps *csp, UChar32 c,
               UCaseContextIterator *iter, void *context,
               const UChar **pString,
               const char *locale, int32_t *locCache,
               UBool upperNotTitle) {
    UChar32 result=c;
    uint16_t props=UTRIE2_GET16(&csp->trie, c);
    if(!PROPS_HAS_EXCEPTION(props)) {
        if(UCASE_GET_TYPE(props)==UCASE_LOWER) {
            result=c+UCASE_GET_DELTA(props);
        }
    } else {
        const uint16_t *pe=GET_EXCEPTIONS(csp, props), *pe2;
        uint16_t excWord=*pe++;
        int32_t full, idx;

        pe2=pe;

        if(excWord&UCASE_EXC_CONDITIONAL_SPECIAL) {
            
            int32_t loc=ucase_getCaseLocale(locale, locCache);

            if(loc==UCASE_LOC_TURKISH && c==0x69) {
                










                return 0x130;
            } else if(loc==UCASE_LOC_LITHUANIAN && c==0x307 && isPrecededBySoftDotted(csp, iter, context)) {
                








                return 0; 
            } else {
                
            }
        } else if(HAS_SLOT(excWord, UCASE_EXC_FULL_MAPPINGS)) {
            GET_SLOT_VALUE(excWord, UCASE_EXC_FULL_MAPPINGS, pe, full);

            
            ++pe;

            
            pe+=full&UCASE_FULL_LOWER;
            full>>=4;
            pe+=full&0xf;
            full>>=4;

            if(upperNotTitle) {
                full&=0xf;
            } else {
                
                pe+=full&0xf;
                full=(full>>4)&0xf;
            }

            if(full!=0) {
                
                *pString=reinterpret_cast<const UChar *>(pe);

                
                return full;
            }
        }

        if(!upperNotTitle && HAS_SLOT(excWord, UCASE_EXC_TITLE)) {
            idx=UCASE_EXC_TITLE;
        } else if(HAS_SLOT(excWord, UCASE_EXC_UPPER)) {
            
            idx=UCASE_EXC_UPPER;
        } else {
            return ~c;
        }
        GET_SLOT_VALUE(excWord, idx, pe2, result);
    }

    return (result==c) ? ~result : result;
}

U_CAPI int32_t U_EXPORT2
ucase_toFullUpper(const UCaseProps *csp, UChar32 c,
                  UCaseContextIterator *iter, void *context,
                  const UChar **pString,
                  const char *locale, int32_t *locCache) {
    return toUpperOrTitle(csp, c, iter, context, pString, locale, locCache, TRUE);
}

U_CAPI int32_t U_EXPORT2
ucase_toFullTitle(const UCaseProps *csp, UChar32 c,
                  UCaseContextIterator *iter, void *context,
                  const UChar **pString,
                  const char *locale, int32_t *locCache) {
    return toUpperOrTitle(csp, c, iter, context, pString, locale, locCache, FALSE);
}











































U_CAPI UChar32 U_EXPORT2
ucase_fold(const UCaseProps *csp, UChar32 c, uint32_t options) {
    uint16_t props=UTRIE2_GET16(&csp->trie, c);
    if(!PROPS_HAS_EXCEPTION(props)) {
        if(UCASE_GET_TYPE(props)>=UCASE_UPPER) {
            c+=UCASE_GET_DELTA(props);
        }
    } else {
        const uint16_t *pe=GET_EXCEPTIONS(csp, props);
        uint16_t excWord=*pe++;
        int32_t idx;
        if(excWord&UCASE_EXC_CONDITIONAL_FOLD) {
            
            if((options&_FOLD_CASE_OPTIONS_MASK)==U_FOLD_CASE_DEFAULT) {
                
                if(c==0x49) {
                    
                    return 0x69;
                } else if(c==0x130) {
                    
                    return c;
                }
            } else {
                
                if(c==0x49) {
                    
                    return 0x131;
                } else if(c==0x130) {
                    
                    return 0x69;
                }
            }
        }
        if(HAS_SLOT(excWord, UCASE_EXC_FOLD)) {
            idx=UCASE_EXC_FOLD;
        } else if(HAS_SLOT(excWord, UCASE_EXC_LOWER)) {
            idx=UCASE_EXC_LOWER;
        } else {
            return c;
        }
        GET_SLOT_VALUE(excWord, idx, pe, c);
    }
    return c;
}
















U_CAPI int32_t U_EXPORT2
ucase_toFullFolding(const UCaseProps *csp, UChar32 c,
                    const UChar **pString,
                    uint32_t options)
{
    UChar32 result=c;
    uint16_t props=UTRIE2_GET16(&csp->trie, c);
    if(!PROPS_HAS_EXCEPTION(props)) {
        if(UCASE_GET_TYPE(props)>=UCASE_UPPER) {
            result=c+UCASE_GET_DELTA(props);
        }
    } else {
        const uint16_t *pe=GET_EXCEPTIONS(csp, props), *pe2;
        uint16_t excWord=*pe++;
        int32_t full, idx;

        pe2=pe;

        if(excWord&UCASE_EXC_CONDITIONAL_FOLD) {
            
            if((options&_FOLD_CASE_OPTIONS_MASK)==U_FOLD_CASE_DEFAULT) {
                
                if(c==0x49) {
                    
                    return 0x69;
                } else if(c==0x130) {
                    
                    *pString=iDot;
                    return 2;
                }
            } else {
                
                if(c==0x49) {
                    
                    return 0x131;
                } else if(c==0x130) {
                    
                    return 0x69;
                }
            }
        } else if(HAS_SLOT(excWord, UCASE_EXC_FULL_MAPPINGS)) {
            GET_SLOT_VALUE(excWord, UCASE_EXC_FULL_MAPPINGS, pe, full);

            
            ++pe;

            
            pe+=full&UCASE_FULL_LOWER;
            full=(full>>4)&0xf;

            if(full!=0) {
                
                *pString=reinterpret_cast<const UChar *>(pe);

                
                return full;
            }
        }

        if(HAS_SLOT(excWord, UCASE_EXC_FOLD)) {
            idx=UCASE_EXC_FOLD;
        } else if(HAS_SLOT(excWord, UCASE_EXC_LOWER)) {
            idx=UCASE_EXC_LOWER;
        } else {
            return ~c;
        }
        GET_SLOT_VALUE(excWord, idx, pe2, result);
    }

    return (result==c) ? ~result : result;
}



#define GET_CASE_PROPS() &ucase_props_singleton



U_CAPI UBool U_EXPORT2
u_isULowercase(UChar32 c) {
    return (UBool)(UCASE_LOWER==ucase_getType(GET_CASE_PROPS(), c));
}

U_CAPI UBool U_EXPORT2
u_isUUppercase(UChar32 c) {
    return (UBool)(UCASE_UPPER==ucase_getType(GET_CASE_PROPS(), c));
}


U_CAPI UChar32 U_EXPORT2
u_tolower(UChar32 c) {
    return ucase_tolower(GET_CASE_PROPS(), c);
}
    

U_CAPI UChar32 U_EXPORT2
u_toupper(UChar32 c) {
    return ucase_toupper(GET_CASE_PROPS(), c);
}


U_CAPI UChar32 U_EXPORT2
u_totitle(UChar32 c) {
    return ucase_totitle(GET_CASE_PROPS(), c);
}


U_CAPI UChar32 U_EXPORT2
u_foldCase(UChar32 c, uint32_t options) {
    return ucase_fold(GET_CASE_PROPS(), c, options);
}

U_CFUNC int32_t U_EXPORT2
ucase_hasBinaryProperty(UChar32 c, UProperty which) {
    
    const UChar *resultString;
    int32_t locCache;
    const UCaseProps *csp=GET_CASE_PROPS();
    if(csp==NULL) {
        return FALSE;
    }
    switch(which) {
    case UCHAR_LOWERCASE:
        return (UBool)(UCASE_LOWER==ucase_getType(csp, c));
    case UCHAR_UPPERCASE:
        return (UBool)(UCASE_UPPER==ucase_getType(csp, c));
    case UCHAR_SOFT_DOTTED:
        return ucase_isSoftDotted(csp, c);
    case UCHAR_CASE_SENSITIVE:
        return ucase_isCaseSensitive(csp, c);
    case UCHAR_CASED:
        return (UBool)(UCASE_NONE!=ucase_getType(csp, c));
    case UCHAR_CASE_IGNORABLE:
        return (UBool)(ucase_getTypeOrIgnorable(csp, c)>>2);
    











    case UCHAR_CHANGES_WHEN_LOWERCASED:
        locCache=UCASE_LOC_ROOT;
        return (UBool)(ucase_toFullLower(csp, c, NULL, NULL, &resultString, "", &locCache)>=0);
    case UCHAR_CHANGES_WHEN_UPPERCASED:
        locCache=UCASE_LOC_ROOT;
        return (UBool)(ucase_toFullUpper(csp, c, NULL, NULL, &resultString, "", &locCache)>=0);
    case UCHAR_CHANGES_WHEN_TITLECASED:
        locCache=UCASE_LOC_ROOT;
        return (UBool)(ucase_toFullTitle(csp, c, NULL, NULL, &resultString, "", &locCache)>=0);
    
    case UCHAR_CHANGES_WHEN_CASEMAPPED:
        locCache=UCASE_LOC_ROOT;
        return (UBool)(
            ucase_toFullLower(csp, c, NULL, NULL, &resultString, "", &locCache)>=0 ||
            ucase_toFullUpper(csp, c, NULL, NULL, &resultString, "", &locCache)>=0 ||
            ucase_toFullTitle(csp, c, NULL, NULL, &resultString, "", &locCache)>=0);
    default:
        return FALSE;
    }
}
