













#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/messagepattern.h"
#include "unicode/unistr.h"
#include "unicode/utf16.h"
#include "cmemory.h"
#include "cstring.h"
#include "messageimpl.h"
#include "patternprops.h"
#include "putilimp.h"
#include "uassert.h"

U_NAMESPACE_BEGIN



static const UChar u_pound=0x23;
static const UChar u_apos=0x27;
static const UChar u_plus=0x2B;
static const UChar u_comma=0x2C;
static const UChar u_minus=0x2D;
static const UChar u_dot=0x2E;
static const UChar u_colon=0x3A;
static const UChar u_lessThan=0x3C;
static const UChar u_equal=0x3D;
static const UChar u_A=0x41;
static const UChar u_C=0x43;
static const UChar u_D=0x44;
static const UChar u_E=0x45;
static const UChar u_H=0x48;
static const UChar u_I=0x49;
static const UChar u_L=0x4C;
static const UChar u_N=0x4E;
static const UChar u_O=0x4F;
static const UChar u_P=0x50;
static const UChar u_R=0x52;
static const UChar u_S=0x53;
static const UChar u_T=0x54;
static const UChar u_U=0x55;
static const UChar u_Z=0x5A;
static const UChar u_a=0x61;
static const UChar u_c=0x63;
static const UChar u_d=0x64;
static const UChar u_e=0x65;
static const UChar u_f=0x66;
static const UChar u_h=0x68;
static const UChar u_i=0x69;
static const UChar u_l=0x6C;
static const UChar u_n=0x6E;
static const UChar u_o=0x6F;
static const UChar u_p=0x70;
static const UChar u_r=0x72;
static const UChar u_s=0x73;
static const UChar u_t=0x74;
static const UChar u_u=0x75;
static const UChar u_z=0x7A;
static const UChar u_leftCurlyBrace=0x7B;
static const UChar u_pipe=0x7C;
static const UChar u_rightCurlyBrace=0x7D;
static const UChar u_lessOrEqual=0x2264;  

static const UChar kOffsetColon[]={  
    u_o, u_f, u_f, u_s, u_e, u_t, u_colon
};

static const UChar kOther[]={  
    u_o, u_t, u_h, u_e, u_r
};



template<typename T, int32_t stackCapacity>
class MessagePatternList : public UMemory {
public:
    MessagePatternList() {}
    void copyFrom(const MessagePatternList<T, stackCapacity> &other,
                  int32_t length,
                  UErrorCode &errorCode);
    UBool ensureCapacityForOneMore(int32_t oldLength, UErrorCode &errorCode);
    UBool equals(const MessagePatternList<T, stackCapacity> &other, int32_t length) const {
        for(int32_t i=0; i<length; ++i) {
            if(a[i]!=other.a[i]) { return FALSE; }
        }
        return TRUE;
    }

    MaybeStackArray<T, stackCapacity> a;
};

template<typename T, int32_t stackCapacity>
void
MessagePatternList<T, stackCapacity>::copyFrom(
        const MessagePatternList<T, stackCapacity> &other,
        int32_t length,
        UErrorCode &errorCode) {
    if(U_SUCCESS(errorCode) && length>0) {
        if(length>a.getCapacity() && NULL==a.resize(length)) {
            errorCode=U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        uprv_memcpy(a.getAlias(), other.a.getAlias(), length*sizeof(T));
    }
}

template<typename T, int32_t stackCapacity>
UBool
MessagePatternList<T, stackCapacity>::ensureCapacityForOneMore(int32_t oldLength, UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) {
        return FALSE;
    }
    if(a.getCapacity()>oldLength || a.resize(2*oldLength, oldLength)!=NULL) {
        return TRUE;
    }
    errorCode=U_MEMORY_ALLOCATION_ERROR;
    return FALSE;
}



class MessagePatternDoubleList : public MessagePatternList<double, 8> {
};

class MessagePatternPartsList : public MessagePatternList<MessagePattern::Part, 32> {
};



MessagePattern::MessagePattern(UErrorCode &errorCode)
        : aposMode(UCONFIG_MSGPAT_DEFAULT_APOSTROPHE_MODE),
          partsList(NULL), parts(NULL), partsLength(0),
          numericValuesList(NULL), numericValues(NULL), numericValuesLength(0),
          hasArgNames(FALSE), hasArgNumbers(FALSE), needsAutoQuoting(FALSE) {
    init(errorCode);
}

MessagePattern::MessagePattern(UMessagePatternApostropheMode mode, UErrorCode &errorCode)
        : aposMode(mode),
          partsList(NULL), parts(NULL), partsLength(0),
          numericValuesList(NULL), numericValues(NULL), numericValuesLength(0),
          hasArgNames(FALSE), hasArgNumbers(FALSE), needsAutoQuoting(FALSE) {
    init(errorCode);
}

MessagePattern::MessagePattern(const UnicodeString &pattern, UParseError *parseError, UErrorCode &errorCode)
        : aposMode(UCONFIG_MSGPAT_DEFAULT_APOSTROPHE_MODE),
          partsList(NULL), parts(NULL), partsLength(0),
          numericValuesList(NULL), numericValues(NULL), numericValuesLength(0),
          hasArgNames(FALSE), hasArgNumbers(FALSE), needsAutoQuoting(FALSE) {
    if(init(errorCode)) {
        parse(pattern, parseError, errorCode);
    }
}

UBool
MessagePattern::init(UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) {
        return FALSE;
    }
    partsList=new MessagePatternPartsList();
    if(partsList==NULL) {
        errorCode=U_MEMORY_ALLOCATION_ERROR;
        return FALSE;
    }
    parts=partsList->a.getAlias();
    return TRUE;
}

MessagePattern::MessagePattern(const MessagePattern &other)
        : UObject(other), aposMode(other.aposMode), msg(other.msg),
          partsList(NULL), parts(NULL), partsLength(0),
          numericValuesList(NULL), numericValues(NULL), numericValuesLength(0),
          hasArgNames(other.hasArgNames), hasArgNumbers(other.hasArgNumbers),
          needsAutoQuoting(other.needsAutoQuoting) {
    UErrorCode errorCode=U_ZERO_ERROR;
    if(!copyStorage(other, errorCode)) {
        clear();
    }
}

MessagePattern &
MessagePattern::operator=(const MessagePattern &other) {
    if(this==&other) {
        return *this;
    }
    aposMode=other.aposMode;
    msg=other.msg;
    hasArgNames=other.hasArgNames;
    hasArgNumbers=other.hasArgNumbers;
    needsAutoQuoting=other.needsAutoQuoting;
    UErrorCode errorCode=U_ZERO_ERROR;
    if(!copyStorage(other, errorCode)) {
        clear();
    }
    return *this;
}

UBool
MessagePattern::copyStorage(const MessagePattern &other, UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) {
        return FALSE;
    }
    parts=NULL;
    partsLength=0;
    numericValues=NULL;
    numericValuesLength=0;
    if(partsList==NULL) {
        partsList=new MessagePatternPartsList();
        if(partsList==NULL) {
            errorCode=U_MEMORY_ALLOCATION_ERROR;
            return FALSE;
        }
        parts=partsList->a.getAlias();
    }
    if(other.partsLength>0) {
        partsList->copyFrom(*other.partsList, other.partsLength, errorCode);
        if(U_FAILURE(errorCode)) {
            return FALSE;
        }
        parts=partsList->a.getAlias();
        partsLength=other.partsLength;
    }
    if(other.numericValuesLength>0) {
        if(numericValuesList==NULL) {
            numericValuesList=new MessagePatternDoubleList();
            if(numericValuesList==NULL) {
                errorCode=U_MEMORY_ALLOCATION_ERROR;
                return FALSE;
            }
            numericValues=numericValuesList->a.getAlias();
        }
        numericValuesList->copyFrom(
            *other.numericValuesList, other.numericValuesLength, errorCode);
        if(U_FAILURE(errorCode)) {
            return FALSE;
        }
        numericValues=numericValuesList->a.getAlias();
        numericValuesLength=other.numericValuesLength;
    }
    return TRUE;
}

MessagePattern::~MessagePattern() {
    delete partsList;
    delete numericValuesList;
}



MessagePattern &
MessagePattern::parse(const UnicodeString &pattern, UParseError *parseError, UErrorCode &errorCode) {
    preParse(pattern, parseError, errorCode);
    parseMessage(0, 0, 0, UMSGPAT_ARG_TYPE_NONE, parseError, errorCode);
    postParse();
    return *this;
}

MessagePattern &
MessagePattern::parseChoiceStyle(const UnicodeString &pattern,
                                 UParseError *parseError, UErrorCode &errorCode) {
    preParse(pattern, parseError, errorCode);
    parseChoiceStyle(0, 0, parseError, errorCode);
    postParse();
    return *this;
}

MessagePattern &
MessagePattern::parsePluralStyle(const UnicodeString &pattern,
                                 UParseError *parseError, UErrorCode &errorCode) {
    preParse(pattern, parseError, errorCode);
    parsePluralOrSelectStyle(UMSGPAT_ARG_TYPE_PLURAL, 0, 0, parseError, errorCode);
    postParse();
    return *this;
}

MessagePattern &
MessagePattern::parseSelectStyle(const UnicodeString &pattern,
                                 UParseError *parseError, UErrorCode &errorCode) {
    preParse(pattern, parseError, errorCode);
    parsePluralOrSelectStyle(UMSGPAT_ARG_TYPE_SELECT, 0, 0, parseError, errorCode);
    postParse();
    return *this;
}

void
MessagePattern::clear() {
    
    msg.remove();
    hasArgNames=hasArgNumbers=FALSE;
    needsAutoQuoting=FALSE;
    partsLength=0;
    numericValuesLength=0;
}

UBool
MessagePattern::operator==(const MessagePattern &other) const {
    if(this==&other) {
        return TRUE;
    }
    return
        aposMode==other.aposMode &&
        msg==other.msg &&
        
        partsLength==other.partsLength &&
        (partsLength==0 || partsList->equals(*other.partsList, partsLength));
    
}

int32_t
MessagePattern::hashCode() const {
    int32_t hash=(aposMode*37+msg.hashCode())*37+partsLength;
    for(int32_t i=0; i<partsLength; ++i) {
        hash=hash*37+parts[i].hashCode();
    }
    return hash;
}

int32_t
MessagePattern::validateArgumentName(const UnicodeString &name) {
    if(!PatternProps::isIdentifier(name.getBuffer(), name.length())) {
        return UMSGPAT_ARG_NAME_NOT_VALID;
    }
    return parseArgNumber(name, 0, name.length());
}

UnicodeString
MessagePattern::autoQuoteApostropheDeep() const {
    if(!needsAutoQuoting) {
        return msg;
    }
    UnicodeString modified(msg);
    
    int32_t count=countParts();
    for(int32_t i=count; i>0;) {
        const Part &part=getPart(--i);
        if(part.getType()==UMSGPAT_PART_TYPE_INSERT_CHAR) {
           modified.insert(part.index, (UChar)part.value);
        }
    }
    return modified;
}

double
MessagePattern::getNumericValue(const Part &part) const {
    UMessagePatternPartType type=part.type;
    if(type==UMSGPAT_PART_TYPE_ARG_INT) {
        return part.value;
    } else if(type==UMSGPAT_PART_TYPE_ARG_DOUBLE) {
        return numericValues[part.value];
    } else {
        return UMSGPAT_NO_NUMERIC_VALUE;
    }
}







double
MessagePattern::getPluralOffset(int32_t pluralStart) const {
    const Part &part=getPart(pluralStart);
    if(Part::hasNumericValue(part.type)) {
        return getNumericValue(part);
    } else {
        return 0;
    }
}



UBool
MessagePattern::Part::operator==(const Part &other) const {
    if(this==&other) {
        return TRUE;
    }
    return
        type==other.type &&
        index==other.index &&
        length==other.length &&
        value==other.value &&
        limitPartIndex==other.limitPartIndex;
}



void
MessagePattern::preParse(const UnicodeString &pattern, UParseError *parseError, UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) {
        return;
    }
    if(parseError!=NULL) {
        parseError->line=0;
        parseError->offset=0;
        parseError->preContext[0]=0;
        parseError->postContext[0]=0;
    }
    msg=pattern;
    hasArgNames=hasArgNumbers=FALSE;
    needsAutoQuoting=FALSE;
    partsLength=0;
    numericValuesLength=0;
}

void
MessagePattern::postParse() {
    if(partsList!=NULL) {
        parts=partsList->a.getAlias();
    }
    if(numericValuesList!=NULL) {
        numericValues=numericValuesList->a.getAlias();
    }
}

int32_t
MessagePattern::parseMessage(int32_t index, int32_t msgStartLength,
                             int32_t nestingLevel, UMessagePatternArgType parentType,
                             UParseError *parseError, UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) {
        return 0;
    }
    if(nestingLevel>Part::MAX_VALUE) {
        errorCode=U_INDEX_OUTOFBOUNDS_ERROR;
        return 0;
    }
    int32_t msgStart=partsLength;
    addPart(UMSGPAT_PART_TYPE_MSG_START, index, msgStartLength, nestingLevel, errorCode);
    index+=msgStartLength;
    for(;;) {  
        if(U_FAILURE(errorCode)) {
            return 0;
        }
        if(index>=msg.length()) {
            break;
        }
        UChar c=msg.charAt(index++);
        if(c==u_apos) {
            if(index==msg.length()) {
                
                
                addPart(UMSGPAT_PART_TYPE_INSERT_CHAR, index, 0,
                        u_apos, errorCode);  
                needsAutoQuoting=TRUE;
            } else {
                c=msg.charAt(index);
                if(c==u_apos) {
                    
                    addPart(UMSGPAT_PART_TYPE_SKIP_SYNTAX, index++, 1, 0, errorCode);
                } else if(
                    aposMode==UMSGPAT_APOS_DOUBLE_REQUIRED ||
                    c==u_leftCurlyBrace || c==u_rightCurlyBrace ||
                    (parentType==UMSGPAT_ARG_TYPE_CHOICE && c==u_pipe) ||
                    (UMSGPAT_ARG_TYPE_HAS_PLURAL_STYLE(parentType) && c==u_pound)
                ) {
                    
                    addPart(UMSGPAT_PART_TYPE_SKIP_SYNTAX, index-1, 1, 0, errorCode);
                    
                    for(;;) {
                        index=msg.indexOf(u_apos, index+1);
                        if(index>=0) {
                            if( msg.charAt(index+1)==u_apos) {
                                
                                
                                addPart(UMSGPAT_PART_TYPE_SKIP_SYNTAX, ++index, 1, 0, errorCode);
                            } else {
                                
                                addPart(UMSGPAT_PART_TYPE_SKIP_SYNTAX, index++, 1, 0, errorCode);
                                break;
                            }
                        } else {
                            
                            index=msg.length();
                            
                            addPart(UMSGPAT_PART_TYPE_INSERT_CHAR, index, 0,
                                    u_apos, errorCode);  
                            needsAutoQuoting=TRUE;
                            break;
                        }
                    }
                } else {
                    
                    
                    addPart(UMSGPAT_PART_TYPE_INSERT_CHAR, index, 0,
                            u_apos, errorCode);  
                    needsAutoQuoting=TRUE;
                }
            }
        } else if(UMSGPAT_ARG_TYPE_HAS_PLURAL_STYLE(parentType) && c==u_pound) {
            
            
            addPart(UMSGPAT_PART_TYPE_REPLACE_NUMBER, index-1, 1, 0, errorCode);
        } else if(c==u_leftCurlyBrace) {
            index=parseArg(index-1, 1, nestingLevel, parseError, errorCode);
        } else if((nestingLevel>0 && c==u_rightCurlyBrace) ||
                  (parentType==UMSGPAT_ARG_TYPE_CHOICE && c==u_pipe)) {
            
            
            
            int32_t limitLength=(parentType==UMSGPAT_ARG_TYPE_CHOICE && c==u_rightCurlyBrace) ? 0 : 1;
            addLimitPart(msgStart, UMSGPAT_PART_TYPE_MSG_LIMIT, index-1, limitLength,
                         nestingLevel, errorCode);
            if(parentType==UMSGPAT_ARG_TYPE_CHOICE) {
                
                return index-1;
            } else {
                
                return index;
            }
        }  
    }
    if(nestingLevel>0 && !inTopLevelChoiceMessage(nestingLevel, parentType)) {
        setParseError(parseError, 0);  
        errorCode=U_UNMATCHED_BRACES;
        return 0;
    }
    addLimitPart(msgStart, UMSGPAT_PART_TYPE_MSG_LIMIT, index, 0, nestingLevel, errorCode);
    return index;
}

int32_t
MessagePattern::parseArg(int32_t index, int32_t argStartLength, int32_t nestingLevel,
                         UParseError *parseError, UErrorCode &errorCode) {
    int32_t argStart=partsLength;
    UMessagePatternArgType argType=UMSGPAT_ARG_TYPE_NONE;
    addPart(UMSGPAT_PART_TYPE_ARG_START, index, argStartLength, argType, errorCode);
    if(U_FAILURE(errorCode)) {
        return 0;
    }
    int32_t nameIndex=index=skipWhiteSpace(index+argStartLength);
    if(index==msg.length()) {
        setParseError(parseError, 0);  
        errorCode=U_UNMATCHED_BRACES;
        return 0;
    }
    
    index=skipIdentifier(index);
    int32_t number=parseArgNumber(nameIndex, index);
    if(number>=0) {
        int32_t length=index-nameIndex;
        if(length>Part::MAX_LENGTH || number>Part::MAX_VALUE) {
            setParseError(parseError, nameIndex);  
            errorCode=U_INDEX_OUTOFBOUNDS_ERROR;
            return 0;
        }
        hasArgNumbers=TRUE;
        addPart(UMSGPAT_PART_TYPE_ARG_NUMBER, nameIndex, length, number, errorCode);
    } else if(number==UMSGPAT_ARG_NAME_NOT_NUMBER) {
        int32_t length=index-nameIndex;
        if(length>Part::MAX_LENGTH) {
            setParseError(parseError, nameIndex);  
            errorCode=U_INDEX_OUTOFBOUNDS_ERROR;
            return 0;
        }
        hasArgNames=TRUE;
        addPart(UMSGPAT_PART_TYPE_ARG_NAME, nameIndex, length, 0, errorCode);
    } else {  
        setParseError(parseError, nameIndex);  
        errorCode=U_PATTERN_SYNTAX_ERROR;
        return 0;
    }
    index=skipWhiteSpace(index);
    if(index==msg.length()) {
        setParseError(parseError, 0);  
        errorCode=U_UNMATCHED_BRACES;
        return 0;
    }
    UChar c=msg.charAt(index);
    if(c==u_rightCurlyBrace) {
        
    } else if(c!=u_comma) {
        setParseError(parseError, nameIndex);  
        errorCode=U_PATTERN_SYNTAX_ERROR;
        return 0;
    } else  {
        
        int32_t typeIndex=index=skipWhiteSpace(index+1);
        while(index<msg.length() && isArgTypeChar(msg.charAt(index))) {
            ++index;
        }
        int32_t length=index-typeIndex;
        index=skipWhiteSpace(index);
        if(index==msg.length()) {
            setParseError(parseError, 0);  
            errorCode=U_UNMATCHED_BRACES;
            return 0;
        }
        if(length==0 || ((c=msg.charAt(index))!=u_comma && c!=u_rightCurlyBrace)) {
            setParseError(parseError, nameIndex);  
            errorCode=U_PATTERN_SYNTAX_ERROR;
            return 0;
        }
        if(length>Part::MAX_LENGTH) {
            setParseError(parseError, nameIndex);  
            errorCode=U_INDEX_OUTOFBOUNDS_ERROR;
            return 0;
        }
        argType=UMSGPAT_ARG_TYPE_SIMPLE;
        if(length==6) {
            
            if(isChoice(typeIndex)) {
                argType=UMSGPAT_ARG_TYPE_CHOICE;
            } else if(isPlural(typeIndex)) {
                argType=UMSGPAT_ARG_TYPE_PLURAL;
            } else if(isSelect(typeIndex)) {
                argType=UMSGPAT_ARG_TYPE_SELECT;
            }
        } else if(length==13) {
            if(isSelect(typeIndex) && isOrdinal(typeIndex+6)) {
                argType=UMSGPAT_ARG_TYPE_SELECTORDINAL;
            }
        }
        
        partsList->a[argStart].value=(int16_t)argType;
        if(argType==UMSGPAT_ARG_TYPE_SIMPLE) {
            addPart(UMSGPAT_PART_TYPE_ARG_TYPE, typeIndex, length, 0, errorCode);
        }
        
        if(c==u_rightCurlyBrace) {
            if(argType!=UMSGPAT_ARG_TYPE_SIMPLE) {
                setParseError(parseError, nameIndex);  
                errorCode=U_PATTERN_SYNTAX_ERROR;
                return 0;
            }
        } else  {
            ++index;
            if(argType==UMSGPAT_ARG_TYPE_SIMPLE) {
                index=parseSimpleStyle(index, parseError, errorCode);
            } else if(argType==UMSGPAT_ARG_TYPE_CHOICE) {
                index=parseChoiceStyle(index, nestingLevel, parseError, errorCode);
            } else {
                index=parsePluralOrSelectStyle(argType, index, nestingLevel, parseError, errorCode);
            }
        }
    }
    
    addLimitPart(argStart, UMSGPAT_PART_TYPE_ARG_LIMIT, index, 1, argType, errorCode);
    return index+1;
}

int32_t
MessagePattern::parseSimpleStyle(int32_t index, UParseError *parseError, UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) {
        return 0;
    }
    int32_t start=index;
    int32_t nestedBraces=0;
    while(index<msg.length()) {
        UChar c=msg.charAt(index++);
        if(c==u_apos) {
            
            
            index=msg.indexOf(u_apos, index);
            if(index<0) {
                
                setParseError(parseError, start);
                errorCode=U_PATTERN_SYNTAX_ERROR;
                return 0;
            }
            
            ++index;
        } else if(c==u_leftCurlyBrace) {
            ++nestedBraces;
        } else if(c==u_rightCurlyBrace) {
            if(nestedBraces>0) {
                --nestedBraces;
            } else {
                int32_t length=--index-start;
                if(length>Part::MAX_LENGTH) {
                    setParseError(parseError, start);  
                    errorCode=U_INDEX_OUTOFBOUNDS_ERROR;
                    return 0;
                }
                addPart(UMSGPAT_PART_TYPE_ARG_STYLE, start, length, 0, errorCode);
                return index;
            }
        }  
    }
    setParseError(parseError, 0);  
    errorCode=U_UNMATCHED_BRACES;
    return 0;
}

int32_t
MessagePattern::parseChoiceStyle(int32_t index, int32_t nestingLevel,
                                 UParseError *parseError, UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) {
        return 0;
    }
    int32_t start=index;
    index=skipWhiteSpace(index);
    if(index==msg.length() || msg.charAt(index)==u_rightCurlyBrace) {
        setParseError(parseError, 0);  
        errorCode=U_PATTERN_SYNTAX_ERROR;
        return 0;
    }
    for(;;) {
        
        
        int32_t numberIndex=index;
        index=skipDouble(index);
        int32_t length=index-numberIndex;
        if(length==0) {
            setParseError(parseError, start);  
            errorCode=U_PATTERN_SYNTAX_ERROR;
            return 0;
        }
        if(length>Part::MAX_LENGTH) {
            setParseError(parseError, numberIndex);  
            errorCode=U_INDEX_OUTOFBOUNDS_ERROR;
            return 0;
        }
        parseDouble(numberIndex, index, TRUE, parseError, errorCode);  
        if(U_FAILURE(errorCode)) {
            return 0;
        }
        
        index=skipWhiteSpace(index);
        if(index==msg.length()) {
            setParseError(parseError, start);  
            errorCode=U_PATTERN_SYNTAX_ERROR;
            return 0;
        }
        UChar c=msg.charAt(index);
        if(!(c==u_pound || c==u_lessThan || c==u_lessOrEqual)) {  
            setParseError(parseError, start);  
            errorCode=U_PATTERN_SYNTAX_ERROR;
            return 0;
        }
        addPart(UMSGPAT_PART_TYPE_ARG_SELECTOR, index, 1, 0, errorCode);
        
        index=parseMessage(++index, 0, nestingLevel+1, UMSGPAT_ARG_TYPE_CHOICE, parseError, errorCode);
        if(U_FAILURE(errorCode)) {
            return 0;
        }
        
        if(index==msg.length()) {
            return index;
        }
        if(msg.charAt(index)==u_rightCurlyBrace) {
            if(!inMessageFormatPattern(nestingLevel)) {
                setParseError(parseError, start);  
                errorCode=U_PATTERN_SYNTAX_ERROR;
                return 0;
            }
            return index;
        }  
        index=skipWhiteSpace(index+1);
    }
}

int32_t
MessagePattern::parsePluralOrSelectStyle(UMessagePatternArgType argType,
                                         int32_t index, int32_t nestingLevel,
                                         UParseError *parseError, UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) {
        return 0;
    }
    int32_t start=index;
    UBool isEmpty=TRUE;
    UBool hasOther=FALSE;
    for(;;) {
        
        
        
        index=skipWhiteSpace(index);
        UBool eos=index==msg.length();
        if(eos || msg.charAt(index)==u_rightCurlyBrace) {
            if(eos==inMessageFormatPattern(nestingLevel)) {
                setParseError(parseError, start);  
                errorCode=U_PATTERN_SYNTAX_ERROR;
                return 0;
            }
            if(!hasOther) {
                setParseError(parseError, 0);  
                errorCode=U_DEFAULT_KEYWORD_MISSING;
                return 0;
            }
            return index;
        }
        int32_t selectorIndex=index;
        if(UMSGPAT_ARG_TYPE_HAS_PLURAL_STYLE(argType) && msg.charAt(selectorIndex)==u_equal) {
            
            index=skipDouble(index+1);
            int32_t length=index-selectorIndex;
            if(length==1) {
                setParseError(parseError, start);  
                errorCode=U_PATTERN_SYNTAX_ERROR;
                return 0;
            }
            if(length>Part::MAX_LENGTH) {
                setParseError(parseError, selectorIndex);  
                errorCode=U_INDEX_OUTOFBOUNDS_ERROR;
                return 0;
            }
            addPart(UMSGPAT_PART_TYPE_ARG_SELECTOR, selectorIndex, length, 0, errorCode);
            parseDouble(selectorIndex+1, index, FALSE,
                        parseError, errorCode);  
        } else {
            index=skipIdentifier(index);
            int32_t length=index-selectorIndex;
            if(length==0) {
                setParseError(parseError, start);  
                errorCode=U_PATTERN_SYNTAX_ERROR;
                return 0;
            }
            
            if( UMSGPAT_ARG_TYPE_HAS_PLURAL_STYLE(argType) && length==6 && index<msg.length() &&
                0==msg.compare(selectorIndex, 7, kOffsetColon, 0, 7)
            ) {
                
                if(!isEmpty) {
                    
                    setParseError(parseError, start);
                    errorCode=U_PATTERN_SYNTAX_ERROR;
                    return 0;
                }
                
                int32_t valueIndex=skipWhiteSpace(index+1);  
                index=skipDouble(valueIndex);
                if(index==valueIndex) {
                    setParseError(parseError, start);  
                    errorCode=U_PATTERN_SYNTAX_ERROR;
                    return 0;
                }
                if((index-valueIndex)>Part::MAX_LENGTH) {
                    setParseError(parseError, valueIndex);  
                    errorCode=U_INDEX_OUTOFBOUNDS_ERROR;
                    return 0;
                }
                parseDouble(valueIndex, index, FALSE,
                            parseError, errorCode);  
                if(U_FAILURE(errorCode)) {
                    return 0;
                }
                isEmpty=FALSE;
                continue;  
            } else {
                
                if(length>Part::MAX_LENGTH) {
                    setParseError(parseError, selectorIndex);  
                    errorCode=U_INDEX_OUTOFBOUNDS_ERROR;
                    return 0;
                }
                addPart(UMSGPAT_PART_TYPE_ARG_SELECTOR, selectorIndex, length, 0, errorCode);
                if(0==msg.compare(selectorIndex, length, kOther, 0, 5)) {
                    hasOther=TRUE;
                }
            }
        }
        if(U_FAILURE(errorCode)) {
            return 0;
        }

        
        index=skipWhiteSpace(index);
        if(index==msg.length() || msg.charAt(index)!=u_leftCurlyBrace) {
            setParseError(parseError, selectorIndex);  
            errorCode=U_PATTERN_SYNTAX_ERROR;
            return 0;
        }
        index=parseMessage(index, 1, nestingLevel+1, argType, parseError, errorCode);
        if(U_FAILURE(errorCode)) {
            return 0;
        }
        isEmpty=FALSE;
    }
}

int32_t
MessagePattern::parseArgNumber(const UnicodeString &s, int32_t start, int32_t limit) {
    
    
    
    if(start>=limit) {
        return UMSGPAT_ARG_NAME_NOT_VALID;
    }
    int32_t number;
    
    UBool badNumber;
    UChar c=s.charAt(start++);
    if(c==0x30) {
        if(start==limit) {
            return 0;
        } else {
            number=0;
            badNumber=TRUE;  
        }
    } else if(0x31<=c && c<=0x39) {
        number=c-0x30;
        badNumber=FALSE;
    } else {
        return UMSGPAT_ARG_NAME_NOT_NUMBER;
    }
    while(start<limit) {
        c=s.charAt(start++);
        if(0x30<=c && c<=0x39) {
            if(number>=INT32_MAX/10) {
                badNumber=TRUE;  
            }
            number=number*10+(c-0x30);
        } else {
            return UMSGPAT_ARG_NAME_NOT_NUMBER;
        }
    }
    
    if(badNumber) {
        return UMSGPAT_ARG_NAME_NOT_VALID;
    } else {
        return number;
    }
}

void
MessagePattern::parseDouble(int32_t start, int32_t limit, UBool allowInfinity,
                            UParseError *parseError, UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) {
        return;
    }
    U_ASSERT(start<limit);
    
    for(;;) { 
        
        int32_t value=0;
        int32_t isNegative=0;  
        int32_t index=start;
        UChar c=msg.charAt(index++);
        if(c==u_minus) {
            isNegative=1;
            if(index==limit) {
                break;  
            }
            c=msg.charAt(index++);
        } else if(c==u_plus) {
            if(index==limit) {
                break;  
            }
            c=msg.charAt(index++);
        }
        if(c==0x221e) {  
            if(allowInfinity && index==limit) {
                double infinity=uprv_getInfinity();
                addArgDoublePart(
                    isNegative!=0 ? -infinity : infinity,
                    start, limit-start, errorCode);
                return;
            } else {
                break;
            }
        }
        
        while('0'<=c && c<='9') {
            value=value*10+(c-'0');
            if(value>(Part::MAX_VALUE+isNegative)) {
                break;  
            }
            if(index==limit) {
                addPart(UMSGPAT_PART_TYPE_ARG_INT, start, limit-start,
                        isNegative!=0 ? -value : value, errorCode);
                return;
            }
            c=msg.charAt(index++);
        }
        
        char numberChars[128];
        int32_t capacity=(int32_t)sizeof(numberChars);
        int32_t length=limit-start;
        if(length>=capacity) {
            break;  
        }
        msg.extract(start, length, numberChars, capacity, US_INV);
        if((int32_t)uprv_strlen(numberChars)<length) {
            break;  
        }
        char *end;
        double numericValue=uprv_strtod(numberChars, &end);
        if(end!=(numberChars+length)) {
            break;  
        }
        addArgDoublePart(numericValue, start, length, errorCode);
        return;
    }
    setParseError(parseError, start );  
    errorCode=U_PATTERN_SYNTAX_ERROR;
    return;
}

int32_t
MessagePattern::skipWhiteSpace(int32_t index) {
    const UChar *s=msg.getBuffer();
    int32_t msgLength=msg.length();
    const UChar *t=PatternProps::skipWhiteSpace(s+index, msgLength-index);
    return (int32_t)(t-s);
}

int32_t
MessagePattern::skipIdentifier(int32_t index) {
    const UChar *s=msg.getBuffer();
    int32_t msgLength=msg.length();
    const UChar *t=PatternProps::skipIdentifier(s+index, msgLength-index);
    return (int32_t)(t-s);
}

int32_t
MessagePattern::skipDouble(int32_t index) {
    int32_t msgLength=msg.length();
    while(index<msgLength) {
        UChar c=msg.charAt(index);
        
        if((c<0x30 && c!=u_plus && c!=u_minus && c!=u_dot) || (c>0x39 && c!=u_e && c!=u_E && c!=0x221e)) {
            break;
        }
        ++index;
    }
    return index;
}

UBool
MessagePattern::isArgTypeChar(UChar32 c) {
    return (u_a<=c && c<=u_z) || (u_A<=c && c<=u_Z);
}

UBool
MessagePattern::isChoice(int32_t index) {
    UChar c;
    return
        ((c=msg.charAt(index++))==u_c || c==u_C) &&
        ((c=msg.charAt(index++))==u_h || c==u_H) &&
        ((c=msg.charAt(index++))==u_o || c==u_O) &&
        ((c=msg.charAt(index++))==u_i || c==u_I) &&
        ((c=msg.charAt(index++))==u_c || c==u_C) &&
        ((c=msg.charAt(index))==u_e || c==u_E);
}

UBool
MessagePattern::isPlural(int32_t index) {
    UChar c;
    return
        ((c=msg.charAt(index++))==u_p || c==u_P) &&
        ((c=msg.charAt(index++))==u_l || c==u_L) &&
        ((c=msg.charAt(index++))==u_u || c==u_U) &&
        ((c=msg.charAt(index++))==u_r || c==u_R) &&
        ((c=msg.charAt(index++))==u_a || c==u_A) &&
        ((c=msg.charAt(index))==u_l || c==u_L);
}

UBool
MessagePattern::isSelect(int32_t index) {
    UChar c;
    return
        ((c=msg.charAt(index++))==u_s || c==u_S) &&
        ((c=msg.charAt(index++))==u_e || c==u_E) &&
        ((c=msg.charAt(index++))==u_l || c==u_L) &&
        ((c=msg.charAt(index++))==u_e || c==u_E) &&
        ((c=msg.charAt(index++))==u_c || c==u_C) &&
        ((c=msg.charAt(index))==u_t || c==u_T);
}

UBool
MessagePattern::isOrdinal(int32_t index) {
    UChar c;
    return
        ((c=msg.charAt(index++))==u_o || c==u_O) &&
        ((c=msg.charAt(index++))==u_r || c==u_R) &&
        ((c=msg.charAt(index++))==u_d || c==u_D) &&
        ((c=msg.charAt(index++))==u_i || c==u_I) &&
        ((c=msg.charAt(index++))==u_n || c==u_N) &&
        ((c=msg.charAt(index++))==u_a || c==u_A) &&
        ((c=msg.charAt(index))==u_l || c==u_L);
}

UBool
MessagePattern::inMessageFormatPattern(int32_t nestingLevel) {
    return nestingLevel>0 || partsList->a[0].type==UMSGPAT_PART_TYPE_MSG_START;
}

UBool
MessagePattern::inTopLevelChoiceMessage(int32_t nestingLevel, UMessagePatternArgType parentType) {
    return
        nestingLevel==1 &&
        parentType==UMSGPAT_ARG_TYPE_CHOICE &&
        partsList->a[0].type!=UMSGPAT_PART_TYPE_MSG_START;
}

void
MessagePattern::addPart(UMessagePatternPartType type, int32_t index, int32_t length,
                        int32_t value, UErrorCode &errorCode) {
    if(partsList->ensureCapacityForOneMore(partsLength, errorCode)) {
        Part &part=partsList->a[partsLength++];
        part.type=type;
        part.index=index;
        part.length=(uint16_t)length;
        part.value=(int16_t)value;
        part.limitPartIndex=0;
    }
}

void
MessagePattern::addLimitPart(int32_t start,
                             UMessagePatternPartType type, int32_t index, int32_t length,
                             int32_t value, UErrorCode &errorCode) {
    partsList->a[start].limitPartIndex=partsLength;
    addPart(type, index, length, value, errorCode);
}

void
MessagePattern::addArgDoublePart(double numericValue, int32_t start, int32_t length,
                                 UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) {
        return;
    }
    int32_t numericIndex=numericValuesLength;
    if(numericValuesList==NULL) {
        numericValuesList=new MessagePatternDoubleList();
        if(numericValuesList==NULL) {
            errorCode=U_MEMORY_ALLOCATION_ERROR;
            return;
        }
    } else if(!numericValuesList->ensureCapacityForOneMore(numericValuesLength, errorCode)) {
        return;
    } else {
        if(numericIndex>Part::MAX_VALUE) {
            errorCode=U_INDEX_OUTOFBOUNDS_ERROR;
            return;
        }
    }
    numericValuesList->a[numericValuesLength++]=numericValue;
    addPart(UMSGPAT_PART_TYPE_ARG_DOUBLE, start, length, numericIndex, errorCode);
}

void
MessagePattern::setParseError(UParseError *parseError, int32_t index) {
    if(parseError==NULL) {
        return;
    }
    parseError->offset=index;

    
    
    int32_t length=index;
    if(length>=U_PARSE_CONTEXT_LEN) {
        length=U_PARSE_CONTEXT_LEN-1;
        if(length>0 && U16_IS_TRAIL(msg[index-length])) {
            --length;
        }
    }
    msg.extract(index-length, length, parseError->preContext);
    parseError->preContext[length]=0;

    
    length=msg.length()-index;
    if(length>=U_PARSE_CONTEXT_LEN) {
        length=U_PARSE_CONTEXT_LEN-1;
        if(length>0 && U16_IS_LEAD(msg[index+length-1])) {
            --length;
        }
    }
    msg.extract(index, length, parseError->postContext);
    parseError->postContext[length]=0;
}

UOBJECT_DEFINE_NO_RTTI_IMPLEMENTATION(MessagePattern)



void
MessageImpl::appendReducedApostrophes(const UnicodeString &s, int32_t start, int32_t limit,
                                      UnicodeString &sb) {
    int32_t doubleApos=-1;
    for(;;) {
        int32_t i=s.indexOf(u_apos, start);
        if(i<0 || i>=limit) {
            sb.append(s, start, limit-start);
            break;
        }
        if(i==doubleApos) {
            
            sb.append(u_apos);
            ++start;
            doubleApos=-1;
        } else {
            
            sb.append(s, start, i-start);
            doubleApos=start=i+1;
        }
    }
}


UnicodeString &
MessageImpl::appendSubMessageWithoutSkipSyntax(const MessagePattern &msgPattern,
                                               int32_t msgStart,
                                               UnicodeString &result) {
    const UnicodeString &msgString=msgPattern.getPatternString();
    int32_t prevIndex=msgPattern.getPart(msgStart).getLimit();
    for(int32_t i=msgStart;;) {
        const MessagePattern::Part &part=msgPattern.getPart(++i);
        UMessagePatternPartType type=part.getType();
        int32_t index=part.getIndex();
        if(type==UMSGPAT_PART_TYPE_MSG_LIMIT) {
            return result.append(msgString, prevIndex, index-prevIndex);
        } else if(type==UMSGPAT_PART_TYPE_SKIP_SYNTAX) {
            result.append(msgString, prevIndex, index-prevIndex);
            prevIndex=part.getLimit();
        } else if(type==UMSGPAT_PART_TYPE_ARG_START) {
            result.append(msgString, prevIndex, index-prevIndex);
            prevIndex=index;
            i=msgPattern.getLimitPartIndex(i);
            index=msgPattern.getPart(i).getLimit();
            appendReducedApostrophes(msgString, prevIndex, index, result);
            prevIndex=index;
        }
    }
}

U_NAMESPACE_END

#endif  
