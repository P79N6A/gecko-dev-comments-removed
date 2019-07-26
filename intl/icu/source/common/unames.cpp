















#include "unicode/utypes.h"
#include "unicode/putil.h"
#include "unicode/uchar.h"
#include "unicode/udata.h"
#include "unicode/utf.h"
#include "unicode/utf16.h"
#include "ustr_imp.h"
#include "umutex.h"
#include "cmemory.h"
#include "cstring.h"
#include "ucln_cmn.h"
#include "udataswp.h"
#include "uprops.h"



#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

static const char DATA_NAME[] = "unames";
static const char DATA_TYPE[] = "icu";

#define GROUP_SHIFT 5
#define LINES_PER_GROUP (1L<<GROUP_SHIFT)
#define GROUP_MASK (LINES_PER_GROUP-1)

















enum {
    GROUP_MSB,
    GROUP_OFFSET_HIGH,
    GROUP_OFFSET_LOW,
    GROUP_LENGTH
};






#define GET_GROUP_OFFSET(group) ((int32_t)(group)[GROUP_OFFSET_HIGH]<<16|(group)[GROUP_OFFSET_LOW])

#define NEXT_GROUP(group) ((group)+GROUP_LENGTH)
#define PREV_GROUP(group) ((group)-GROUP_LENGTH)

typedef struct {
    uint32_t start, end;
    uint8_t type, variant;
    uint16_t size;
} AlgorithmicRange;

typedef struct {
    uint32_t tokenStringOffset, groupsOffset, groupStringOffset, algNamesOffset;
} UCharNames;










#define GET_GROUPS(names) (const uint16_t *)((const char *)names+names->groupsOffset)

typedef struct {
    const char *otherName;
    UChar32 code;
} FindName;

#define DO_FIND_NAME NULL

static UDataMemory *uCharNamesData=NULL;
static UCharNames *uCharNames=NULL;
static UErrorCode gLoadErrorCode=U_ZERO_ERROR;




static int32_t gMaxNameLength=0;





static uint32_t gNameSet[8]={ 0 };

#define U_NONCHARACTER_CODE_POINT U_CHAR_CATEGORY_COUNT
#define U_LEAD_SURROGATE U_CHAR_CATEGORY_COUNT + 1
#define U_TRAIL_SURROGATE U_CHAR_CATEGORY_COUNT + 2

#define U_CHAR_EXTENDED_CATEGORY_COUNT (U_CHAR_CATEGORY_COUNT + 3)

static const char * const charCatNames[U_CHAR_EXTENDED_CATEGORY_COUNT] = {
    "unassigned",
    "uppercase letter",
    "lowercase letter",
    "titlecase letter",
    "modifier letter",
    "other letter",
    "non spacing mark",
    "enclosing mark",
    "combining spacing mark",
    "decimal digit number",
    "letter number",
    "other number",
    "space separator",
    "line separator",
    "paragraph separator",
    "control",
    "format",
    "private use area",
    "surrogate",
    "dash punctuation",   
    "start punctuation",
    "end punctuation",
    "connector punctuation",
    "other punctuation",
    "math symbol",
    "currency symbol",
    "modifier symbol",
    "other symbol",
    "initial punctuation",
    "final punctuation",
    "noncharacter",
    "lead surrogate",
    "trail surrogate"
};



static UBool U_CALLCONV unames_cleanup(void)
{
    if(uCharNamesData) {
        udata_close(uCharNamesData);
        uCharNamesData = NULL;
    }
    if(uCharNames) {
        uCharNames = NULL;
    }
    gMaxNameLength=0;
    return TRUE;
}

static UBool U_CALLCONV
isAcceptable(void * ,
             const char * , const char * ,
             const UDataInfo *pInfo) {
    return (UBool)(
        pInfo->size>=20 &&
        pInfo->isBigEndian==U_IS_BIG_ENDIAN &&
        pInfo->charsetFamily==U_CHARSET_FAMILY &&
        pInfo->dataFormat[0]==0x75 &&   
        pInfo->dataFormat[1]==0x6e &&
        pInfo->dataFormat[2]==0x61 &&
        pInfo->dataFormat[3]==0x6d &&
        pInfo->formatVersion[0]==1);
}

static UBool
isDataLoaded(UErrorCode *pErrorCode) {
    
    UBool isCached;

    
    UMTX_CHECK(NULL, (uCharNames!=NULL), isCached);

    if(!isCached) {
        UCharNames *names;
        UDataMemory *data;

        
        if(U_FAILURE(gLoadErrorCode)) {
            *pErrorCode=gLoadErrorCode;
            return FALSE;
        }

        
        data=udata_openChoice(NULL, DATA_TYPE, DATA_NAME, isAcceptable, NULL, pErrorCode);
        if(U_FAILURE(*pErrorCode)) {
            gLoadErrorCode=*pErrorCode;
            return FALSE;
        }

        names=(UCharNames *)udata_getMemory(data);

        
        {
            umtx_lock(NULL);
            if(uCharNames==NULL) {
                uCharNamesData=data;
                uCharNames=names;
                data=NULL;
                names=NULL;
                ucln_common_registerCleanup(UCLN_COMMON_UNAMES, unames_cleanup);
            }
            umtx_unlock(NULL);
        }

        
        if(data!=NULL) {
            udata_close(data); 
        }
    }
    return TRUE;
}

#define WRITE_CHAR(buffer, bufferLength, bufferPos, c) { \
    if((bufferLength)>0) { \
        *(buffer)++=c; \
        --(bufferLength); \
    } \
    ++(bufferPos); \
}

#define U_ISO_COMMENT U_CHAR_NAME_CHOICE_COUNT













static uint16_t
expandName(UCharNames *names,
           const uint8_t *name, uint16_t nameLength, UCharNameChoice nameChoice,
           char *buffer, uint16_t bufferLength) {
    uint16_t *tokens=(uint16_t *)names+8;
    uint16_t token, tokenCount=*tokens++, bufferPos=0;
    uint8_t *tokenStrings=(uint8_t *)names+names->tokenStringOffset;
    uint8_t c;

    if(nameChoice!=U_UNICODE_CHAR_NAME && nameChoice!=U_EXTENDED_CHAR_NAME) {
        



        if((uint8_t)';'>=tokenCount || tokens[(uint8_t)';']==(uint16_t)(-1)) {
            int fieldIndex= nameChoice==U_ISO_COMMENT ? 2 : nameChoice;
            do {
                while(nameLength>0) {
                    --nameLength;
                    if(*name++==';') {
                        break;
                    }
                }
            } while(--fieldIndex>0);
        } else {
            




            nameLength=0;
        }
    }

    
    while(nameLength>0) {
        --nameLength;
        c=*name++;

        if(c>=tokenCount) {
            if(c!=';') {
                
                WRITE_CHAR(buffer, bufferLength, bufferPos, c);
            } else {
                
                break;
            }
        } else {
            token=tokens[c];
            if(token==(uint16_t)(-2)) {
                
                token=tokens[c<<8|*name++];
                --nameLength;
            }
            if(token==(uint16_t)(-1)) {
                if(c!=';') {
                    
                    WRITE_CHAR(buffer, bufferLength, bufferPos, c);
                } else {
                    


                    if(!bufferPos && nameChoice == U_EXTENDED_CHAR_NAME) {
                        if ((uint8_t)';'>=tokenCount || tokens[(uint8_t)';']==(uint16_t)(-1)) {
                            continue;
                        }
                    }
                    
                    break;
                }
            } else {
                
                uint8_t *tokenString=tokenStrings+token;
                while((c=*tokenString++)!=0) {
                    WRITE_CHAR(buffer, bufferLength, bufferPos, c);
                }
            }
        }
    }

    
    if(bufferLength>0) {
        *buffer=0;
    }

    return bufferPos;
}






static UBool
compareName(UCharNames *names,
            const uint8_t *name, uint16_t nameLength, UCharNameChoice nameChoice,
            const char *otherName) {
    uint16_t *tokens=(uint16_t *)names+8;
    uint16_t token, tokenCount=*tokens++;
    uint8_t *tokenStrings=(uint8_t *)names+names->tokenStringOffset;
    uint8_t c;
    const char *origOtherName = otherName;

    if(nameChoice!=U_UNICODE_CHAR_NAME && nameChoice!=U_EXTENDED_CHAR_NAME) {
        



        if((uint8_t)';'>=tokenCount || tokens[(uint8_t)';']==(uint16_t)(-1)) {
            int fieldIndex= nameChoice==U_ISO_COMMENT ? 2 : nameChoice;
            do {
                while(nameLength>0) {
                    --nameLength;
                    if(*name++==';') {
                        break;
                    }
                }
            } while(--fieldIndex>0);
        } else {
            




            nameLength=0;
        }
    }

    
    while(nameLength>0) {
        --nameLength;
        c=*name++;

        if(c>=tokenCount) {
            if(c!=';') {
                
                if((char)c!=*otherName++) {
                    return FALSE;
                }
            } else {
                
                break;
            }
        } else {
            token=tokens[c];
            if(token==(uint16_t)(-2)) {
                
                token=tokens[c<<8|*name++];
                --nameLength;
            }
            if(token==(uint16_t)(-1)) {
                if(c!=';') {
                    
                    if((char)c!=*otherName++) {
                        return FALSE;
                    }
                } else {
                    


                    if(otherName == origOtherName && nameChoice == U_EXTENDED_CHAR_NAME) {
                        if ((uint8_t)';'>=tokenCount || tokens[(uint8_t)';']==(uint16_t)(-1)) {
                            continue;
                        }
                    }
                    
                    break;
                }
            } else {
                
                uint8_t *tokenString=tokenStrings+token;
                while((c=*tokenString++)!=0) {
                    if((char)c!=*otherName++) {
                        return FALSE;
                    }
                }
            }
        }
    }

    
    return (UBool)(*otherName==0);
}

static uint8_t getCharCat(UChar32 cp) {
    uint8_t cat;

    if (U_IS_UNICODE_NONCHAR(cp)) {
        return U_NONCHARACTER_CODE_POINT;
    }

    if ((cat = u_charType(cp)) == U_SURROGATE) {
        cat = U_IS_LEAD(cp) ? U_LEAD_SURROGATE : U_TRAIL_SURROGATE;
    }

    return cat;
}

static const char *getCharCatName(UChar32 cp) {
    uint8_t cat = getCharCat(cp);

    


    if (cat >= LENGTHOF(charCatNames)) {
        return "unknown";
    } else {
        return charCatNames[cat];
    }
}

static uint16_t getExtName(uint32_t code, char *buffer, uint16_t bufferLength) {
    const char *catname = getCharCatName(code);
    uint16_t length = 0;

    UChar32 cp;
    int ndigits, i;
    
    WRITE_CHAR(buffer, bufferLength, length, '<');
    while (catname[length - 1]) {
        WRITE_CHAR(buffer, bufferLength, length, catname[length - 1]);
    }
    WRITE_CHAR(buffer, bufferLength, length, '-');
    for (cp = code, ndigits = 0; cp; ++ndigits, cp >>= 4)
        ;
    if (ndigits < 4)
        ndigits = 4;
    for (cp = code, i = ndigits; (cp || i > 0) && bufferLength; cp >>= 4, bufferLength--) {
        uint8_t v = (uint8_t)(cp & 0xf);
        buffer[--i] = (v < 10 ? '0' + v : 'A' + v - 10);
    }
    buffer += ndigits;
    length += ndigits;
    WRITE_CHAR(buffer, bufferLength, length, '>');

    return length;
}








static const uint16_t *
getGroup(UCharNames *names, uint32_t code) {
    const uint16_t *groups=GET_GROUPS(names);
    uint16_t groupMSB=(uint16_t)(code>>GROUP_SHIFT),
             start=0,
             limit=*groups++,
             number;

    
    while(start<limit-1) {
        number=(uint16_t)((start+limit)/2);
        if(groupMSB<groups[number*GROUP_LENGTH+GROUP_MSB]) {
            limit=number;
        } else {
            start=number;
        }
    }

    
    return groups+start*GROUP_LENGTH;
}











static const uint8_t *
expandGroupLengths(const uint8_t *s,
                   uint16_t offsets[LINES_PER_GROUP+1], uint16_t lengths[LINES_PER_GROUP+1]) {
    
    uint16_t i=0, offset=0, length=0;
    uint8_t lengthByte;

    
    while(i<LINES_PER_GROUP) {
        lengthByte=*s++;

        
        if(length>=12) {
            
            length=(uint16_t)(((length&0x3)<<4|lengthByte>>4)+12);
            lengthByte&=0xf;
        } else if((lengthByte )>=0xc0) {
            
            length=(uint16_t)((lengthByte&0x3f)+12);
        } else {
            
            length=(uint16_t)(lengthByte>>4);
            lengthByte&=0xf;
        }

        *offsets++=offset;
        *lengths++=length;

        offset+=length;
        ++i;

        
        if((lengthByte&0xf0)==0) {
            
            length=lengthByte;
            if(length<12) {
                
                *offsets++=offset;
                *lengths++=length;

                offset+=length;
                ++i;
            }
        } else {
            length=0;   
        }
    }

    
    return s;
}

static uint16_t
expandGroupName(UCharNames *names, const uint16_t *group,
                uint16_t lineNumber, UCharNameChoice nameChoice,
                char *buffer, uint16_t bufferLength) {
    uint16_t offsets[LINES_PER_GROUP+2], lengths[LINES_PER_GROUP+2];
    const uint8_t *s=(uint8_t *)names+names->groupStringOffset+GET_GROUP_OFFSET(group);
    s=expandGroupLengths(s, offsets, lengths);
    return expandName(names, s+offsets[lineNumber], lengths[lineNumber], nameChoice,
                      buffer, bufferLength);
}

static uint16_t
getName(UCharNames *names, uint32_t code, UCharNameChoice nameChoice,
        char *buffer, uint16_t bufferLength) {
    const uint16_t *group=getGroup(names, code);
    if((uint16_t)(code>>GROUP_SHIFT)==group[GROUP_MSB]) {
        return expandGroupName(names, group, (uint16_t)(code&GROUP_MASK), nameChoice,
                               buffer, bufferLength);
    } else {
        
        
        if(bufferLength>0) {
            *buffer=0;
        }
        return 0;
    }
}





static UBool
enumGroupNames(UCharNames *names, const uint16_t *group,
               UChar32 start, UChar32 end,
               UEnumCharNamesFn *fn, void *context,
               UCharNameChoice nameChoice) {
    uint16_t offsets[LINES_PER_GROUP+2], lengths[LINES_PER_GROUP+2];
    const uint8_t *s=(uint8_t *)names+names->groupStringOffset+GET_GROUP_OFFSET(group);

    s=expandGroupLengths(s, offsets, lengths);
    if(fn!=DO_FIND_NAME) {
        char buffer[200];
        uint16_t length;

        while(start<=end) {
            length=expandName(names, s+offsets[start&GROUP_MASK], lengths[start&GROUP_MASK], nameChoice, buffer, sizeof(buffer));
            if (!length && nameChoice == U_EXTENDED_CHAR_NAME) {
                buffer[length = getExtName(start, buffer, sizeof(buffer))] = 0;
            }
            
            if(length>0) {
                if(!fn(context, start, nameChoice, buffer, length)) {
                    return FALSE;
                }
            }
            ++start;
        }
    } else {
        const char *otherName=((FindName *)context)->otherName;
        while(start<=end) {
            if(compareName(names, s+offsets[start&GROUP_MASK], lengths[start&GROUP_MASK], nameChoice, otherName)) {
                ((FindName *)context)->code=start;
                return FALSE;
            }
            ++start;
        }
    }
    return TRUE;
}






 
static UBool
enumExtNames(UChar32 start, UChar32 end,
             UEnumCharNamesFn *fn, void *context)
{
    if(fn!=DO_FIND_NAME) {
        char buffer[200];
        uint16_t length;
        
        while(start<=end) {
            buffer[length = getExtName(start, buffer, sizeof(buffer))] = 0;
            
            if(length>0) {
                if(!fn(context, start, U_EXTENDED_CHAR_NAME, buffer, length)) {
                    return FALSE;
                }
            }
            ++start;
        }
    }

    return TRUE;
}

static UBool
enumNames(UCharNames *names,
          UChar32 start, UChar32 limit,
          UEnumCharNamesFn *fn, void *context,
          UCharNameChoice nameChoice) {
    uint16_t startGroupMSB, endGroupMSB, groupCount;
    const uint16_t *group, *groupLimit;

    startGroupMSB=(uint16_t)(start>>GROUP_SHIFT);
    endGroupMSB=(uint16_t)((limit-1)>>GROUP_SHIFT);

    
    group=getGroup(names, start);

    if(startGroupMSB<group[GROUP_MSB] && nameChoice==U_EXTENDED_CHAR_NAME) {
        
        UChar32 extLimit=((UChar32)group[GROUP_MSB]<<GROUP_SHIFT);
        if(extLimit>limit) {
            extLimit=limit;
        }
        if(!enumExtNames(start, extLimit-1, fn, context)) {
            return FALSE;
        }
        start=extLimit;
    }

    if(startGroupMSB==endGroupMSB) {
        if(startGroupMSB==group[GROUP_MSB]) {
            
            return enumGroupNames(names, group, start, limit-1, fn, context, nameChoice);
        }
    } else {
        const uint16_t *groups=GET_GROUPS(names);
        groupCount=*groups++;
        groupLimit=groups+groupCount*GROUP_LENGTH;

        if(startGroupMSB==group[GROUP_MSB]) {
            
            if((start&GROUP_MASK)!=0) {
                if(!enumGroupNames(names, group,
                                   start, ((UChar32)startGroupMSB<<GROUP_SHIFT)+LINES_PER_GROUP-1,
                                   fn, context, nameChoice)) {
                    return FALSE;
                }
                group=NEXT_GROUP(group); 
            }
        } else if(startGroupMSB>group[GROUP_MSB]) {
            
            const uint16_t *nextGroup=NEXT_GROUP(group);
            if (nextGroup < groupLimit && nextGroup[GROUP_MSB] > startGroupMSB && nameChoice == U_EXTENDED_CHAR_NAME) {
                UChar32 end = nextGroup[GROUP_MSB] << GROUP_SHIFT;
                if (end > limit) {
                    end = limit;
                }
                if (!enumExtNames(start, end - 1, fn, context)) {
                    return FALSE;
                }
            }
            group=nextGroup;
        }

        
        while(group<groupLimit && group[GROUP_MSB]<endGroupMSB) {
            const uint16_t *nextGroup;
            start=(UChar32)group[GROUP_MSB]<<GROUP_SHIFT;
            if(!enumGroupNames(names, group, start, start+LINES_PER_GROUP-1, fn, context, nameChoice)) {
                return FALSE;
            }
            nextGroup=NEXT_GROUP(group);
            if (nextGroup < groupLimit && nextGroup[GROUP_MSB] > group[GROUP_MSB] + 1 && nameChoice == U_EXTENDED_CHAR_NAME) {
                UChar32 end = nextGroup[GROUP_MSB] << GROUP_SHIFT;
                if (end > limit) {
                    end = limit;
                }
                if (!enumExtNames((group[GROUP_MSB] + 1) << GROUP_SHIFT, end - 1, fn, context)) {
                    return FALSE;
                }
            }
            group=nextGroup;
        }

        
        if(group<groupLimit && group[GROUP_MSB]==endGroupMSB) {
            return enumGroupNames(names, group, (limit-1)&~GROUP_MASK, limit-1, fn, context, nameChoice);
        } else if (nameChoice == U_EXTENDED_CHAR_NAME && group == groupLimit) {
            UChar32 next = (PREV_GROUP(group)[GROUP_MSB] + 1) << GROUP_SHIFT;
            if (next > start) {
                start = next;
            }
        } else {
            return TRUE;
        }
    }

    

    if (nameChoice == U_EXTENDED_CHAR_NAME) {
        if (limit > UCHAR_MAX_VALUE + 1) {
            limit = UCHAR_MAX_VALUE + 1;
        }
        return enumExtNames(start, limit - 1, fn, context);
    }
    
    return TRUE;
}

static uint16_t
writeFactorSuffix(const uint16_t *factors, uint16_t count,
                  const char *s, 
                  uint32_t code,
                  uint16_t indexes[8], 
                  const char *elementBases[8], const char *elements[8],
                  char *buffer, uint16_t bufferLength) {
    uint16_t i, factor, bufferPos=0;
    char c;

    

    





    --count;
    for(i=count; i>0; --i) {
        factor=factors[i];
        indexes[i]=(uint16_t)(code%factor);
        code/=factor;
    }
    



    indexes[0]=(uint16_t)code;

    
    for(;;) {
        if(elementBases!=NULL) {
            *elementBases++=s;
        }

        
        factor=indexes[i];
        while(factor>0) {
            while(*s++!=0) {}
            --factor;
        }
        if(elements!=NULL) {
            *elements++=s;
        }

        
        while((c=*s++)!=0) {
            WRITE_CHAR(buffer, bufferLength, bufferPos, c);
        }

        
        if(i>=count) {
            break;
        }

        
        factor=(uint16_t)(factors[i]-indexes[i]-1);
        while(factor>0) {
            while(*s++!=0) {}
            --factor;
        }

        ++i;
    }

    
    if(bufferLength>0) {
        *buffer=0;
    }

    return bufferPos;
}






static uint16_t
getAlgName(AlgorithmicRange *range, uint32_t code, UCharNameChoice nameChoice,
        char *buffer, uint16_t bufferLength) {
    uint16_t bufferPos=0;

    
    if(nameChoice!=U_UNICODE_CHAR_NAME && nameChoice!=U_EXTENDED_CHAR_NAME) {
        
        if(bufferLength>0) {
            *buffer=0;
        }
        return 0;
    }

    switch(range->type) {
    case 0: {
        
        const char *s=(const char *)(range+1);
        char c;

        uint16_t i, count;

        
        while((c=*s++)!=0) {
            WRITE_CHAR(buffer, bufferLength, bufferPos, c);
        }

        
        count=range->variant;

        
        if(count<bufferLength) {
            buffer[count]=0;
        }

        for(i=count; i>0;) {
            if(--i<bufferLength) {
                c=(char)(code&0xf);
                if(c<10) {
                    c+='0';
                } else {
                    c+='A'-10;
                }
                buffer[i]=c;
            }
            code>>=4;
        }

        bufferPos+=count;
        break;
    }
    case 1: {
        
        uint16_t indexes[8];
        const uint16_t *factors=(const uint16_t *)(range+1);
        uint16_t count=range->variant;
        const char *s=(const char *)(factors+count);
        char c;

        
        while((c=*s++)!=0) {
            WRITE_CHAR(buffer, bufferLength, bufferPos, c);
        }

        bufferPos+=writeFactorSuffix(factors, count,
                                     s, code-range->start, indexes, NULL, NULL, buffer, bufferLength);
        break;
    }
    default:
        
        
        if(bufferLength>0) {
            *buffer=0;
        }
        break;
    }

    return bufferPos;
}





static UBool
enumAlgNames(AlgorithmicRange *range,
             UChar32 start, UChar32 limit,
             UEnumCharNamesFn *fn, void *context,
             UCharNameChoice nameChoice) {
    char buffer[200];
    uint16_t length;

    if(nameChoice!=U_UNICODE_CHAR_NAME && nameChoice!=U_EXTENDED_CHAR_NAME) {
        return TRUE;
    }

    switch(range->type) {
    case 0: {
        char *s, *end;
        char c;

        
        length=getAlgName(range, (uint32_t)start, nameChoice, buffer, sizeof(buffer));
        if(length<=0) {
            return TRUE;
        }

        
        if(!fn(context, start, nameChoice, buffer, length)) {
            return FALSE;
        }

        
        end=buffer;
        while(*end!=0) {
            ++end;
        }

        
        while(++start<limit) {
            
            s=end;
            for (;;) {
                c=*--s;
                if(('0'<=c && c<'9') || ('A'<=c && c<'F')) {
                    *s=(char)(c+1);
                    break;
                } else if(c=='9') {
                    *s='A';
                    break;
                } else if(c=='F') {
                    *s='0';
                }
            }

            if(!fn(context, start, nameChoice, buffer, length)) {
                return FALSE;
            }
        }
        break;
    }
    case 1: {
        uint16_t indexes[8];
        const char *elementBases[8], *elements[8];
        const uint16_t *factors=(const uint16_t *)(range+1);
        uint16_t count=range->variant;
        const char *s=(const char *)(factors+count);
        char *suffix, *t;
        uint16_t prefixLength, i, idx;

        char c;

        

        
        suffix=buffer;
        prefixLength=0;
        while((c=*s++)!=0) {
            *suffix++=c;
            ++prefixLength;
        }

        
        length=(uint16_t)(prefixLength+writeFactorSuffix(factors, count,
                                              s, (uint32_t)start-range->start,
                                              indexes, elementBases, elements,
                                              suffix, (uint16_t)(sizeof(buffer)-prefixLength)));

        
        if(!fn(context, start, nameChoice, buffer, length)) {
            return FALSE;
        }

        
        while(++start<limit) {
            
            i=count;
            for (;;) {
                idx=(uint16_t)(indexes[--i]+1);
                if(idx<factors[i]) {
                    
                    indexes[i]=idx;
                    s=elements[i];
                    while(*s++!=0) {
                    }
                    elements[i]=s;
                    break;
                } else {
                    
                    indexes[i]=0;
                    elements[i]=elementBases[i];
                }
            }

            
            t=suffix;
            length=prefixLength;
            for(i=0; i<count; ++i) {
                s=elements[i];
                while((c=*s++)!=0) {
                    *t++=c;
                    ++length;
                }
            }
            
            *t=0;

            if(!fn(context, start, nameChoice, buffer, length)) {
                return FALSE;
            }
        }
        break;
    }
    default:
        
        break;
    }

    return TRUE;
}






static UChar32
findAlgName(AlgorithmicRange *range, UCharNameChoice nameChoice, const char *otherName) {
    UChar32 code;

    if(nameChoice!=U_UNICODE_CHAR_NAME && nameChoice!=U_EXTENDED_CHAR_NAME) {
        return 0xffff;
    }

    switch(range->type) {
    case 0: {
        
        const char *s=(const char *)(range+1);
        char c;

        uint16_t i, count;

        
        while((c=*s++)!=0) {
            if((char)c!=*otherName++) {
                return 0xffff;
            }
        }

        
        count=range->variant;
        code=0;
        for(i=0; i<count; ++i) {
            c=*otherName++;
            if('0'<=c && c<='9') {
                code=(code<<4)|(c-'0');
            } else if('A'<=c && c<='F') {
                code=(code<<4)|(c-'A'+10);
            } else {
                return 0xffff;
            }
        }

        
        if(*otherName==0 && range->start<=(uint32_t)code && (uint32_t)code<=range->end) {
            return code;
        }
        break;
    }
    case 1: {
        char buffer[64];
        uint16_t indexes[8];
        const char *elementBases[8], *elements[8];
        const uint16_t *factors=(const uint16_t *)(range+1);
        uint16_t count=range->variant;
        const char *s=(const char *)(factors+count), *t;
        UChar32 start, limit;
        uint16_t i, idx;

        char c;

        

        
        while((c=*s++)!=0) {
            if((char)c!=*otherName++) {
                return 0xffff;
            }
        }

        start=(UChar32)range->start;
        limit=(UChar32)(range->end+1);

        
        writeFactorSuffix(factors, count, s, 0,
                          indexes, elementBases, elements, buffer, sizeof(buffer));

        
        if(0==uprv_strcmp(otherName, buffer)) {
            return start;
        }

        
        while(++start<limit) {
            
            i=count;
            for (;;) {
                idx=(uint16_t)(indexes[--i]+1);
                if(idx<factors[i]) {
                    
                    indexes[i]=idx;
                    s=elements[i];
                    while(*s++!=0) {}
                    elements[i]=s;
                    break;
                } else {
                    
                    indexes[i]=0;
                    elements[i]=elementBases[i];
                }
            }

            
            t=otherName;
            for(i=0; i<count; ++i) {
                s=elements[i];
                while((c=*s++)!=0) {
                    if(c!=*t++) {
                        s=""; 
                        i=99;
                    }
                }
            }
            if(i<99 && *t==0) {
                return start;
            }
        }
        break;
    }
    default:
        
        break;
    }

    return 0xffff;
}



#define SET_ADD(set, c) ((set)[(uint8_t)c>>5]|=((uint32_t)1<<((uint8_t)c&0x1f)))
#define SET_CONTAINS(set, c) (((set)[(uint8_t)c>>5]&((uint32_t)1<<((uint8_t)c&0x1f)))!=0)

static int32_t
calcStringSetLength(uint32_t set[8], const char *s) {
    int32_t length=0;
    char c;

    while((c=*s++)!=0) {
        SET_ADD(set, c);
        ++length;
    }
    return length;
}

static int32_t
calcAlgNameSetsLengths(int32_t maxNameLength) {
    AlgorithmicRange *range;
    uint32_t *p;
    uint32_t rangeCount;
    int32_t length;

    
    p=(uint32_t *)((uint8_t *)uCharNames+uCharNames->algNamesOffset);
    rangeCount=*p;
    range=(AlgorithmicRange *)(p+1);
    while(rangeCount>0) {
        switch(range->type) {
        case 0:
            
            
            length=calcStringSetLength(gNameSet, (const char *)(range+1))+range->variant;
            if(length>maxNameLength) {
                maxNameLength=length;
            }
            break;
        case 1: {
            
            const uint16_t *factors=(const uint16_t *)(range+1);
            const char *s;
            int32_t i, count=range->variant, factor, factorLength, maxFactorLength;

            
            s=(const char *)(factors+count);
            length=calcStringSetLength(gNameSet, s);
            s+=length+1; 

            
            for(i=0; i<count; ++i) {
                maxFactorLength=0;
                for(factor=factors[i]; factor>0; --factor) {
                    factorLength=calcStringSetLength(gNameSet, s);
                    s+=factorLength+1;
                    if(factorLength>maxFactorLength) {
                        maxFactorLength=factorLength;
                    }
                }
                length+=maxFactorLength;
            }

            if(length>maxNameLength) {
                maxNameLength=length;
            }
            break;
        }
        default:
            
            break;
        }

        range=(AlgorithmicRange *)((uint8_t *)range+range->size);
        --rangeCount;
    }
    return maxNameLength;
}

static int32_t
calcExtNameSetsLengths(int32_t maxNameLength) {
    int32_t i, length;

    for(i=0; i<LENGTHOF(charCatNames); ++i) {
        






        length=9+calcStringSetLength(gNameSet, charCatNames[i]);
        if(length>maxNameLength) {
            maxNameLength=length;
        }
    }
    return maxNameLength;
}

static int32_t
calcNameSetLength(const uint16_t *tokens, uint16_t tokenCount, const uint8_t *tokenStrings, int8_t *tokenLengths,
                  uint32_t set[8],
                  const uint8_t **pLine, const uint8_t *lineLimit) {
    const uint8_t *line=*pLine;
    int32_t length=0, tokenLength;
    uint16_t c, token;

    while(line!=lineLimit && (c=*line++)!=(uint8_t)';') {
        if(c>=tokenCount) {
            
            SET_ADD(set, c);
            ++length;
        } else {
            token=tokens[c];
            if(token==(uint16_t)(-2)) {
                
                c=c<<8|*line++;
                token=tokens[c];
            }
            if(token==(uint16_t)(-1)) {
                
                SET_ADD(set, c);
                ++length;
            } else {
                
                if(tokenLengths!=NULL) {
                    
                    tokenLength=tokenLengths[c];
                    if(tokenLength==0) {
                        tokenLength=calcStringSetLength(set, (const char *)tokenStrings+token);
                        tokenLengths[c]=(int8_t)tokenLength;
                    }
                } else {
                    tokenLength=calcStringSetLength(set, (const char *)tokenStrings+token);
                }
                length+=tokenLength;
            }
        }
    }

    *pLine=line;
    return length;
}

static void
calcGroupNameSetsLengths(int32_t maxNameLength) {
    uint16_t offsets[LINES_PER_GROUP+2], lengths[LINES_PER_GROUP+2];

    uint16_t *tokens=(uint16_t *)uCharNames+8;
    uint16_t tokenCount=*tokens++;
    uint8_t *tokenStrings=(uint8_t *)uCharNames+uCharNames->tokenStringOffset;

    int8_t *tokenLengths;

    const uint16_t *group;
    const uint8_t *s, *line, *lineLimit;

    int32_t groupCount, lineNumber, length;

    tokenLengths=(int8_t *)uprv_malloc(tokenCount);
    if(tokenLengths!=NULL) {
        uprv_memset(tokenLengths, 0, tokenCount);
    }

    group=GET_GROUPS(uCharNames);
    groupCount=*group++;

    
    while(groupCount>0) {
        s=(uint8_t *)uCharNames+uCharNames->groupStringOffset+GET_GROUP_OFFSET(group);
        s=expandGroupLengths(s, offsets, lengths);

        
        for(lineNumber=0; lineNumber<LINES_PER_GROUP; ++lineNumber) {
            line=s+offsets[lineNumber];
            length=lengths[lineNumber];
            if(length==0) {
                continue;
            }

            lineLimit=line+length;

            
            length=calcNameSetLength(tokens, tokenCount, tokenStrings, tokenLengths, gNameSet, &line, lineLimit);
            if(length>maxNameLength) {
                maxNameLength=length;
            }
            if(line==lineLimit) {
                continue;
            }

            
            length=calcNameSetLength(tokens, tokenCount, tokenStrings, tokenLengths, gNameSet, &line, lineLimit);
            if(length>maxNameLength) {
                maxNameLength=length;
            }
            if(line==lineLimit) {
                continue;
            }

            
            
        }

        group=NEXT_GROUP(group);
        --groupCount;
    }

    if(tokenLengths!=NULL) {
        uprv_free(tokenLengths);
    }

    
    gMaxNameLength=maxNameLength;
}

static UBool
calcNameSetsLengths(UErrorCode *pErrorCode) {
    static const char extChars[]="0123456789ABCDEF<>-";
    int32_t i, maxNameLength;

    if(gMaxNameLength!=0) {
        return TRUE;
    }

    if(!isDataLoaded(pErrorCode)) {
        return FALSE;
    }

    
    for(i=0; i<(int32_t)sizeof(extChars)-1; ++i) {
        SET_ADD(gNameSet, extChars[i]);
    }

    
    maxNameLength=calcAlgNameSetsLengths(0);

    
    maxNameLength=calcExtNameSetsLengths(maxNameLength);

    
    calcGroupNameSetsLengths(maxNameLength);

    return TRUE;
}



U_CAPI int32_t U_EXPORT2
u_charName(UChar32 code, UCharNameChoice nameChoice,
           char *buffer, int32_t bufferLength,
           UErrorCode *pErrorCode) {
    AlgorithmicRange *algRange;
    uint32_t *p;
    uint32_t i;
    int32_t length;

    
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    } else if(nameChoice>=U_CHAR_NAME_CHOICE_COUNT ||
              bufferLength<0 || (bufferLength>0 && buffer==NULL)
    ) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    if((uint32_t)code>UCHAR_MAX_VALUE || !isDataLoaded(pErrorCode)) {
        return u_terminateChars(buffer, bufferLength, 0, pErrorCode);
    }

    length=0;

    
    p=(uint32_t *)((uint8_t *)uCharNames+uCharNames->algNamesOffset);
    i=*p;
    algRange=(AlgorithmicRange *)(p+1);
    while(i>0) {
        if(algRange->start<=(uint32_t)code && (uint32_t)code<=algRange->end) {
            length=getAlgName(algRange, (uint32_t)code, nameChoice, buffer, (uint16_t)bufferLength);
            break;
        }
        algRange=(AlgorithmicRange *)((uint8_t *)algRange+algRange->size);
        --i;
    }

    if(i==0) {
        if (nameChoice == U_EXTENDED_CHAR_NAME) {
            length = getName(uCharNames, (uint32_t )code, U_EXTENDED_CHAR_NAME, buffer, (uint16_t) bufferLength);
            if (!length) {
                
                length = getExtName((uint32_t) code, buffer, (uint16_t) bufferLength);
            }
        } else {
            
            length=getName(uCharNames, (uint32_t)code, nameChoice, buffer, (uint16_t)bufferLength);
        }
    }

    return u_terminateChars(buffer, bufferLength, length, pErrorCode);
}

U_CAPI int32_t U_EXPORT2
u_getISOComment(UChar32 ,
                char *dest, int32_t destCapacity,
                UErrorCode *pErrorCode) {
    
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    } else if(destCapacity<0 || (destCapacity>0 && dest==NULL)) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    return u_terminateChars(dest, destCapacity, 0, pErrorCode);
}

U_CAPI UChar32 U_EXPORT2
u_charFromName(UCharNameChoice nameChoice,
               const char *name,
               UErrorCode *pErrorCode) {
    char upper[120], lower[120];
    FindName findName;
    AlgorithmicRange *algRange;
    uint32_t *p;
    uint32_t i;
    UChar32 cp = 0;
    char c0;
    UChar32 error = 0xffff;     

    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return error;
    }

    if(nameChoice>=U_CHAR_NAME_CHOICE_COUNT || name==NULL || *name==0) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return error;
    }

    if(!isDataLoaded(pErrorCode)) {
        return error;
    }

    
    for(i=0; i<sizeof(upper); ++i) {
        if((c0=*name++)!=0) {
            upper[i]=uprv_toupper(c0);
            lower[i]=uprv_tolower(c0);
        } else {
            upper[i]=lower[i]=0;
            break;
        }
    }
    if(i==sizeof(upper)) {
        
        *pErrorCode = U_ILLEGAL_CHAR_FOUND;
        return error;
    }

    
    if (lower[0] == '<') {
        if (nameChoice == U_EXTENDED_CHAR_NAME) {
            if (lower[--i] == '>') {
                for (--i; lower[i] && lower[i] != '-'; --i) {
                }

                if (lower[i] == '-') { 
                    uint32_t cIdx;

                    lower[i] = 0;

                    for (++i; lower[i] != '>'; ++i) {
                        if (lower[i] >= '0' && lower[i] <= '9') {
                            cp = (cp << 4) + lower[i] - '0';
                        } else if (lower[i] >= 'a' && lower[i] <= 'f') {
                            cp = (cp << 4) + lower[i] - 'a' + 10;
                        } else {
                            *pErrorCode = U_ILLEGAL_CHAR_FOUND;
                            return error;
                        }
                    }

                    



                    for (lower[i] = 0, cIdx = 0; cIdx < LENGTHOF(charCatNames); ++cIdx) {

                        if (!uprv_strcmp(lower + 1, charCatNames[cIdx])) {
                            if (getCharCat(cp) == cIdx) {
                                return cp;
                            }
                            break;
                        }
                    }
                }
            }
        }

        *pErrorCode = U_ILLEGAL_CHAR_FOUND;
        return error;
    }

    
    p=(uint32_t *)((uint8_t *)uCharNames+uCharNames->algNamesOffset);
    i=*p;
    algRange=(AlgorithmicRange *)(p+1);
    while(i>0) {
        if((cp=findAlgName(algRange, nameChoice, upper))!=0xffff) {
            return cp;
        }
        algRange=(AlgorithmicRange *)((uint8_t *)algRange+algRange->size);
        --i;
    }

    
    findName.otherName=upper;
    findName.code=error;
    enumNames(uCharNames, 0, UCHAR_MAX_VALUE + 1, DO_FIND_NAME, &findName, nameChoice);
    if (findName.code == error) {
         *pErrorCode = U_ILLEGAL_CHAR_FOUND;
    }
    return findName.code;
}

U_CAPI void U_EXPORT2
u_enumCharNames(UChar32 start, UChar32 limit,
                UEnumCharNamesFn *fn,
                void *context,
                UCharNameChoice nameChoice,
                UErrorCode *pErrorCode) {
    AlgorithmicRange *algRange;
    uint32_t *p;
    uint32_t i;

    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return;
    }

    if(nameChoice>=U_CHAR_NAME_CHOICE_COUNT || fn==NULL) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }

    if((uint32_t) limit > UCHAR_MAX_VALUE + 1) {
        limit = UCHAR_MAX_VALUE + 1;
    }
    if((uint32_t)start>=(uint32_t)limit) {
        return;
    }

    if(!isDataLoaded(pErrorCode)) {
        return;
    }

    
    
    p=(uint32_t *)((uint8_t *)uCharNames+uCharNames->algNamesOffset);
    i=*p;
    algRange=(AlgorithmicRange *)(p+1);
    while(i>0) {
        
        
        if((uint32_t)start<algRange->start) {
            if((uint32_t)limit<=algRange->start) {
                enumNames(uCharNames, start, limit, fn, context, nameChoice);
                return;
            }
            if(!enumNames(uCharNames, start, (UChar32)algRange->start, fn, context, nameChoice)) {
                return;
            }
            start=(UChar32)algRange->start;
        }
        
        
        if((uint32_t)start<=algRange->end) {
            if((uint32_t)limit<=(algRange->end+1)) {
                enumAlgNames(algRange, start, limit, fn, context, nameChoice);
                return;
            }
            if(!enumAlgNames(algRange, start, (UChar32)algRange->end+1, fn, context, nameChoice)) {
                return;
            }
            start=(UChar32)algRange->end+1;
        }
        
        algRange=(AlgorithmicRange *)((uint8_t *)algRange+algRange->size);
        --i;
    }
    
    enumNames(uCharNames, start, limit, fn, context, nameChoice);
}

U_CAPI int32_t U_EXPORT2
uprv_getMaxCharNameLength() {
    UErrorCode errorCode=U_ZERO_ERROR;
    if(calcNameSetsLengths(&errorCode)) {
        return gMaxNameLength;
    } else {
        return 0;
    }
}






static void
charSetToUSet(uint32_t cset[8], const USetAdder *sa) {
    UChar us[256];
    char cs[256];

    int32_t i, length;
    UErrorCode errorCode;

    errorCode=U_ZERO_ERROR;

    if(!calcNameSetsLengths(&errorCode)) {
        return;
    }

    
    length=0;
    for(i=0; i<256; ++i) {
        if(SET_CONTAINS(cset, i)) {
            cs[length++]=(char)i;
        }
    }

    
    u_charsToUChars(cs, us, length);

    
    for(i=0; i<length; ++i) {
        if(us[i]!=0 || cs[i]==0) { 
            sa->add(sa->set, us[i]);
        }
    }
}





U_CAPI void U_EXPORT2
uprv_getCharNameCharacters(const USetAdder *sa) {
    charSetToUSet(gNameSet, sa);
}














static void
makeTokenMap(const UDataSwapper *ds,
             int16_t tokens[], uint16_t tokenCount,
             uint8_t map[256],
             UErrorCode *pErrorCode) {
    UBool usedOutChar[256];
    uint16_t i, j;
    uint8_t c1, c2;

    if(U_FAILURE(*pErrorCode)) {
        return;
    }

    if(ds->inCharset==ds->outCharset) {
        
        for(i=0; i<256; ++i) {
            map[i]=(uint8_t)i;
        }
    } else {
        uprv_memset(map, 0, 256);
        uprv_memset(usedOutChar, 0, 256);

        if(tokenCount>256) {
            tokenCount=256;
        }

        
        for(i=1; i<tokenCount; ++i) {
            if(tokens[i]==-1) {
                
                c1=(uint8_t)i;
                ds->swapInvChars(ds, &c1, 1, &c2, pErrorCode);
                if(U_FAILURE(*pErrorCode)) {
                    udata_printError(ds, "unames/makeTokenMap() finds variant character 0x%02x used (input charset family %d)\n",
                                     i, ds->inCharset);
                    return;
                }

                
                map[c1]=c2;
                usedOutChar[c2]=TRUE;
            }
        }

        
        for(i=j=1; i<tokenCount; ++i) {
            
            if(map[i]==0) {
                
                while(usedOutChar[j]) {
                    ++j;
                }
                map[i]=(uint8_t)j++;
            }
        }

        



    }
}

U_CAPI int32_t U_EXPORT2
uchar_swapNames(const UDataSwapper *ds,
                const void *inData, int32_t length, void *outData,
                UErrorCode *pErrorCode) {
    const UDataInfo *pInfo;
    int32_t headerSize;

    const uint8_t *inBytes;
    uint8_t *outBytes;

    uint32_t tokenStringOffset, groupsOffset, groupStringOffset, algNamesOffset,
             offset, i, count, stringsCount;

    const AlgorithmicRange *inRange;
    AlgorithmicRange *outRange;

    
    headerSize=udata_swapDataHeader(ds, inData, length, outData, pErrorCode);
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    
    pInfo=(const UDataInfo *)((const char *)inData+4);
    if(!(
        pInfo->dataFormat[0]==0x75 &&   
        pInfo->dataFormat[1]==0x6e &&
        pInfo->dataFormat[2]==0x61 &&
        pInfo->dataFormat[3]==0x6d &&
        pInfo->formatVersion[0]==1
    )) {
        udata_printError(ds, "uchar_swapNames(): data format %02x.%02x.%02x.%02x (format version %02x) is not recognized as unames.icu\n",
                         pInfo->dataFormat[0], pInfo->dataFormat[1],
                         pInfo->dataFormat[2], pInfo->dataFormat[3],
                         pInfo->formatVersion[0]);
        *pErrorCode=U_UNSUPPORTED_ERROR;
        return 0;
    }

    inBytes=(const uint8_t *)inData+headerSize;
    outBytes=(uint8_t *)outData+headerSize;
    if(length<0) {
        algNamesOffset=ds->readUInt32(((const uint32_t *)inBytes)[3]);
    } else {
        length-=headerSize;
        if( length<20 ||
            (uint32_t)length<(algNamesOffset=ds->readUInt32(((const uint32_t *)inBytes)[3]))
        ) {
            udata_printError(ds, "uchar_swapNames(): too few bytes (%d after header) for unames.icu\n",
                             length);
            *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
            return 0;
        }
    }

    if(length<0) {
        
        offset=algNamesOffset;
        count=ds->readUInt32(*((const uint32_t *)(inBytes+offset)));
        offset+=4;

        for(i=0; i<count; ++i) {
            inRange=(const AlgorithmicRange *)(inBytes+offset);
            offset+=ds->readUInt16(inRange->size);
        }
    } else {
        
        const uint16_t *p;
        uint16_t *q, *temp;

        int16_t tokens[512];
        uint16_t tokenCount;

        uint8_t map[256], trailMap[256];

        
        if(inBytes!=outBytes) {
            uprv_memcpy(outBytes, inBytes, length);
        }

        
        tokenStringOffset=ds->readUInt32(((const uint32_t *)inBytes)[0]);
        groupsOffset=ds->readUInt32(((const uint32_t *)inBytes)[1]);
        groupStringOffset=ds->readUInt32(((const uint32_t *)inBytes)[2]);
        ds->swapArray32(ds, inBytes, 16, outBytes, pErrorCode);

        



        p=(const uint16_t *)(inBytes+16);
        q=(uint16_t *)(outBytes+16);

        
        tokenCount=ds->readUInt16(*p);
        ds->swapArray16(ds, p, 2, q, pErrorCode);
        ++p;
        ++q;

        
        if(tokenCount<=512) {
            count=tokenCount;
        } else {
            count=512;
        }
        for(i=0; i<count; ++i) {
            tokens[i]=udata_readInt16(ds, p[i]);
        }
        for(; i<512; ++i) {
            tokens[i]=0; 
        }
        makeTokenMap(ds, tokens, tokenCount, map, pErrorCode);
        makeTokenMap(ds, tokens+256, (uint16_t)(tokenCount>256 ? tokenCount-256 : 0), trailMap, pErrorCode);
        if(U_FAILURE(*pErrorCode)) {
            return 0;
        }

        



        temp=(uint16_t *)uprv_malloc(tokenCount*2);
        if(temp==NULL) {
            udata_printError(ds, "out of memory swapping %u unames.icu tokens\n",
                             tokenCount);
            *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
            return 0;
        }

        
        for(i=0; i<tokenCount && i<256; ++i) {
            ds->swapArray16(ds, p+i, 2, temp+map[i], pErrorCode);
        }

        
        for(; i<tokenCount; ++i) {
            ds->swapArray16(ds, p+i, 2, temp+(i&0xffffff00)+trailMap[i&0xff], pErrorCode);
        }

        
        uprv_memcpy(q, temp, tokenCount*2);
        uprv_free(temp);

        



        udata_swapInvStringBlock(ds, inBytes+tokenStringOffset, (int32_t)(groupsOffset-tokenStringOffset),
                                    outBytes+tokenStringOffset, pErrorCode);
        if(U_FAILURE(*pErrorCode)) {
            udata_printError(ds, "uchar_swapNames(token strings) failed\n");
            return 0;
        }

        
        count=ds->readUInt16(*((const uint16_t *)(inBytes+groupsOffset)));
        ds->swapArray16(ds, inBytes+groupsOffset, (int32_t)((1+count*3)*2),
                           outBytes+groupsOffset, pErrorCode);

        



        if(ds->inCharset!=ds->outCharset) {
            uint16_t offsets[LINES_PER_GROUP+1], lengths[LINES_PER_GROUP+1];

            const uint8_t *inStrings, *nextInStrings;
            uint8_t *outStrings;

            uint8_t c;

            inStrings=inBytes+groupStringOffset;
            outStrings=outBytes+groupStringOffset;

            stringsCount=algNamesOffset-groupStringOffset;

            
            while(stringsCount>32) {
                nextInStrings=expandGroupLengths(inStrings, offsets, lengths);

                
                stringsCount-=(uint32_t)(nextInStrings-inStrings);
                outStrings+=nextInStrings-inStrings;
                inStrings=nextInStrings;

                count=offsets[31]+lengths[31]; 
                stringsCount-=count;

                
                while(count>0) {
                    c=*inStrings++;
                    *outStrings++=map[c];
                    if(tokens[c]!=-2) {
                        --count;
                    } else {
                        
                        *outStrings++=trailMap[*inStrings++];
                        count-=2;
                    }
                }
            }
        }

        
        offset=algNamesOffset;
        count=ds->readUInt32(*((const uint32_t *)(inBytes+offset)));
        ds->swapArray32(ds, inBytes+offset, 4, outBytes+offset, pErrorCode);
        offset+=4;

        for(i=0; i<count; ++i) {
            if(offset>(uint32_t)length) {
                udata_printError(ds, "uchar_swapNames(): too few bytes (%d after header) for unames.icu algorithmic range %u\n",
                                 length, i);
                *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
                return 0;
            }

            inRange=(const AlgorithmicRange *)(inBytes+offset);
            outRange=(AlgorithmicRange *)(outBytes+offset);
            offset+=ds->readUInt16(inRange->size);

            ds->swapArray32(ds, inRange, 8, outRange, pErrorCode);
            ds->swapArray16(ds, &inRange->size, 2, &outRange->size, pErrorCode);
            switch(inRange->type) {
            case 0:
                
                ds->swapInvChars(ds, inRange+1, (int32_t)uprv_strlen((const char *)(inRange+1)),
                                    outRange+1, pErrorCode);
                if(U_FAILURE(*pErrorCode)) {
                    udata_printError(ds, "uchar_swapNames(prefix string of algorithmic range %u) failed\n",
                                     i);
                    return 0;
                }
                break;
            case 1:
                {
                    
                    uint32_t factorsCount;

                    factorsCount=inRange->variant;
                    p=(const uint16_t *)(inRange+1);
                    q=(uint16_t *)(outRange+1);
                    ds->swapArray16(ds, p, (int32_t)(factorsCount*2), q, pErrorCode);

                    
                    p+=factorsCount;
                    q+=factorsCount;
                    stringsCount=(uint32_t)((inBytes+offset)-(const uint8_t *)p);
                    while(stringsCount>0 && ((const uint8_t *)p)[stringsCount-1]!=0) {
                        --stringsCount;
                    }
                    ds->swapInvChars(ds, p, (int32_t)stringsCount, q, pErrorCode);
                }
                break;
            default:
                udata_printError(ds, "uchar_swapNames(): unknown type %u of algorithmic range %u\n",
                                 inRange->type, i);
                *pErrorCode=U_UNSUPPORTED_ERROR;
                return 0;
            }
        }
    }

    return headerSize+(int32_t)offset;
}









