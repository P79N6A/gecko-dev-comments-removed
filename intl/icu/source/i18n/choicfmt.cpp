
























#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/choicfmt.h"
#include "unicode/numfmt.h"
#include "unicode/locid.h"
#include "cpputils.h"
#include "cstring.h"
#include "messageimpl.h"
#include "putilimp.h"
#include "uassert.h"
#include <stdio.h>
#include <float.h>





U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(ChoiceFormat)




#define SINGLE_QUOTE ((UChar)0x0027)   /*'*/
#define LESS_THAN    ((UChar)0x003C)   /*<*/
#define LESS_EQUAL   ((UChar)0x0023)   /*#*/
#define LESS_EQUAL2  ((UChar)0x2264)
#define VERTICAL_BAR ((UChar)0x007C)   /*|*/
#define MINUS        ((UChar)0x002D)   /*-*/

static const UChar LEFT_CURLY_BRACE = 0x7B;     
static const UChar RIGHT_CURLY_BRACE = 0x7D;    

#ifdef INFINITY
#undef INFINITY
#endif
#define INFINITY     ((UChar)0x221E)



#define POSITIVE_INF_STRLEN 1
#define NEGATIVE_INF_STRLEN 2




ChoiceFormat::ChoiceFormat(const UnicodeString& newPattern,
                           UErrorCode& status)
: constructorErrorCode(status),
  msgPattern(status)
{
    applyPattern(newPattern, status);
}





ChoiceFormat::ChoiceFormat(const double* limits, 
                           const UnicodeString* formats, 
                           int32_t cnt )
: constructorErrorCode(U_ZERO_ERROR),
  msgPattern(constructorErrorCode)
{
    setChoices(limits, NULL, formats, cnt, constructorErrorCode);
}



ChoiceFormat::ChoiceFormat(const double* limits, 
                           const UBool* closures,
                           const UnicodeString* formats, 
                           int32_t cnt )
: constructorErrorCode(U_ZERO_ERROR),
  msgPattern(constructorErrorCode)
{
    setChoices(limits, closures, formats, cnt, constructorErrorCode);
}




ChoiceFormat::ChoiceFormat(const    ChoiceFormat&   that) 
: NumberFormat(that),
  constructorErrorCode(that.constructorErrorCode),
  msgPattern(that.msgPattern)
{
}






ChoiceFormat::ChoiceFormat(const UnicodeString& newPattern,
                           UParseError& parseError,
                           UErrorCode& status)
: constructorErrorCode(status),
  msgPattern(status)
{
    applyPattern(newPattern,parseError, status);
}


UBool
ChoiceFormat::operator==(const Format& that) const
{
    if (this == &that) return TRUE;
    if (!NumberFormat::operator==(that)) return FALSE;
    ChoiceFormat& thatAlias = (ChoiceFormat&)that;
    return msgPattern == thatAlias.msgPattern;
}




const ChoiceFormat&
ChoiceFormat::operator=(const   ChoiceFormat& that)
{
    if (this != &that) {
        NumberFormat::operator=(that);
        constructorErrorCode = that.constructorErrorCode;
        msgPattern = that.msgPattern;
    }
    return *this;
}



ChoiceFormat::~ChoiceFormat()
{
}






UnicodeString&
ChoiceFormat::dtos(double value,
                   UnicodeString& string)
{
    
    char temp[DBL_DIG + 16];
    char *itrPtr = temp;
    char *expPtr;

    sprintf(temp, "%.*g", DBL_DIG, value);

    


    while (*itrPtr && (*itrPtr == '-' || isdigit(*itrPtr))) {
        itrPtr++;
    }
    if (*itrPtr != 0 && *itrPtr != 'e') {
        

        *itrPtr = '.';
        itrPtr++;
    }
    
    while (*itrPtr && *itrPtr != 'e') {
        itrPtr++;
    }
    if (*itrPtr == 'e') {
        itrPtr++;
        
        if (*itrPtr == '+' || *itrPtr == '-') {
            itrPtr++;
        }
        
        expPtr = itrPtr;
        while (*itrPtr == '0') {
            itrPtr++;
        }
        if (*itrPtr && expPtr != itrPtr) {
            
            while (*itrPtr) {
                *(expPtr++)  = *(itrPtr++);
            }
            
            *expPtr = 0;
        }
    }

    string = UnicodeString(temp, -1, US_INV);    
    return string;
}




void
ChoiceFormat::applyPattern(const UnicodeString& pattern,
                           UErrorCode& status)
{
    msgPattern.parseChoiceStyle(pattern, NULL, status);
    constructorErrorCode = status;
}




void
ChoiceFormat::applyPattern(const UnicodeString& pattern,
                           UParseError& parseError,
                           UErrorCode& status)
{
    msgPattern.parseChoiceStyle(pattern, &parseError, status);
    constructorErrorCode = status;
}



UnicodeString&
ChoiceFormat::toPattern(UnicodeString& result) const
{
    return result = msgPattern.getPatternString();
}



void
ChoiceFormat::setChoices(  const double* limits, 
                           const UnicodeString* formats, 
                           int32_t cnt )
{
    UErrorCode errorCode = U_ZERO_ERROR;
    setChoices(limits, NULL, formats, cnt, errorCode);
}



void
ChoiceFormat::setChoices(  const double* limits, 
                           const UBool* closures,
                           const UnicodeString* formats, 
                           int32_t cnt )
{
    UErrorCode errorCode = U_ZERO_ERROR;
    setChoices(limits, closures, formats, cnt, errorCode);
}

void
ChoiceFormat::setChoices(const double* limits,
                         const UBool* closures,
                         const UnicodeString* formats,
                         int32_t count,
                         UErrorCode &errorCode) {
    if (U_FAILURE(errorCode)) {
        return;
    }
    if (limits == NULL || formats == NULL) {
        errorCode = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    
    
    UnicodeString result;
    for (int32_t i = 0; i < count; ++i) {
        if (i != 0) {
            result += VERTICAL_BAR;
        }
        UnicodeString buf;
        if (uprv_isPositiveInfinity(limits[i])) {
            result += INFINITY;
        } else if (uprv_isNegativeInfinity(limits[i])) {
            result += MINUS;
            result += INFINITY;
        } else {
            result += dtos(limits[i], buf);
        }
        if (closures != NULL && closures[i]) {
            result += LESS_THAN;
        } else {
            result += LESS_EQUAL;
        }
        
        
        
        const UnicodeString& text = formats[i];
        int32_t textLength = text.length();
        int32_t nestingLevel = 0;
        for (int32_t j = 0; j < textLength; ++j) {
            UChar c = text[j];
            if (c == SINGLE_QUOTE && nestingLevel == 0) {
                
                result.append(c);
            } else if (c == VERTICAL_BAR && nestingLevel == 0) {
                
                
                
                
                
                
                
                
                result.append(SINGLE_QUOTE).append(c).append(SINGLE_QUOTE);
                continue;  
            } else if (c == LEFT_CURLY_BRACE) {
                ++nestingLevel;
            } else if (c == RIGHT_CURLY_BRACE && nestingLevel > 0) {
                --nestingLevel;
            }
            result.append(c);
        }
    }
    
    applyPattern(result, errorCode);
}




const double*
ChoiceFormat::getLimits(int32_t& cnt) const 
{
    cnt = 0;
    return NULL;
}




const UBool*
ChoiceFormat::getClosures(int32_t& cnt) const 
{
    cnt = 0;
    return NULL;
}




const UnicodeString*
ChoiceFormat::getFormats(int32_t& cnt) const
{
    cnt = 0;
    return NULL;
}






UnicodeString&
ChoiceFormat::format(int64_t number, 
                     UnicodeString& appendTo, 
                     FieldPosition& status) const
{
    return format((double) number, appendTo, status);
}





UnicodeString&
ChoiceFormat::format(int32_t number, 
                     UnicodeString& appendTo, 
                     FieldPosition& status) const
{
    return format((double) number, appendTo, status);
}




UnicodeString&
ChoiceFormat::format(double number, 
                     UnicodeString& appendTo, 
                     FieldPosition& ) const
{
    if (msgPattern.countParts() == 0) {
        
        return appendTo;
    }
    
    int32_t msgStart = findSubMessage(msgPattern, 0, number);
    if (!MessageImpl::jdkAposMode(msgPattern)) {
        int32_t patternStart = msgPattern.getPart(msgStart).getLimit();
        int32_t msgLimit = msgPattern.getLimitPartIndex(msgStart);
        appendTo.append(msgPattern.getPatternString(),
                        patternStart,
                        msgPattern.getPatternIndex(msgLimit) - patternStart);
        return appendTo;
    }
    
    return MessageImpl::appendSubMessageWithoutSkipSyntax(msgPattern, msgStart, appendTo);
}

int32_t
ChoiceFormat::findSubMessage(const MessagePattern &pattern, int32_t partIndex, double number) {
    int32_t count = pattern.countParts();
    int32_t msgStart;
    
    
    
    partIndex += 2;
    for (;;) {
        
        msgStart = partIndex;
        partIndex = pattern.getLimitPartIndex(partIndex);
        if (++partIndex >= count) {
            
            
            break;
        }
        const MessagePattern::Part &part = pattern.getPart(partIndex++);
        UMessagePatternPartType type = part.getType();
        if (type == UMSGPAT_PART_TYPE_ARG_LIMIT) {
            
            
            break;
        }
        
        U_ASSERT(MessagePattern::Part::hasNumericValue(type));
        double boundary = pattern.getNumericValue(part);
        
        int32_t selectorIndex = pattern.getPatternIndex(partIndex++);
        UChar boundaryChar = pattern.getPatternString().charAt(selectorIndex);
        if (boundaryChar == LESS_THAN ? !(number > boundary) : !(number >= boundary)) {
            
            
            
            
            break;
        }
    }
    return msgStart;
}





UnicodeString&
ChoiceFormat::format(const Formattable* objs,
                     int32_t cnt,
                     UnicodeString& appendTo,
                     FieldPosition& pos,
                     UErrorCode& status) const
{
    if(cnt < 0) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return appendTo;
    }
    if (msgPattern.countParts() == 0) {
        status = U_INVALID_STATE_ERROR;
        return appendTo;
    }

    for (int32_t i = 0; i < cnt; i++) {
        double objDouble = objs[i].getDouble(status);
        if (U_SUCCESS(status)) {
            format(objDouble, appendTo, pos);
        }
    }

    return appendTo;
}





UnicodeString&
ChoiceFormat::format(const Formattable& obj, 
                     UnicodeString& appendTo, 
                     FieldPosition& pos,
                     UErrorCode& status) const
{
    return NumberFormat::format(obj, appendTo, pos, status);
}


void
ChoiceFormat::parse(const UnicodeString& text, 
                    Formattable& result,
                    ParsePosition& pos) const
{
    result.setDouble(parseArgument(msgPattern, 0, text, pos));
}

double
ChoiceFormat::parseArgument(
        const MessagePattern &pattern, int32_t partIndex,
        const UnicodeString &source, ParsePosition &pos) {
    
    int32_t start = pos.getIndex();
    int32_t furthest = start;
    double bestNumber = uprv_getNaN();
    double tempNumber = 0.0;
    int32_t count = pattern.countParts();
    while (partIndex < count && pattern.getPartType(partIndex) != UMSGPAT_PART_TYPE_ARG_LIMIT) {
        tempNumber = pattern.getNumericValue(pattern.getPart(partIndex));
        partIndex += 2;  
        int32_t msgLimit = pattern.getLimitPartIndex(partIndex);
        int32_t len = matchStringUntilLimitPart(pattern, partIndex, msgLimit, source, start);
        if (len >= 0) {
            int32_t newIndex = start + len;
            if (newIndex > furthest) {
                furthest = newIndex;
                bestNumber = tempNumber;
                if (furthest == source.length()) {
                    break;
                }
            }
        }
        partIndex = msgLimit + 1;
    }
    if (furthest == start) {
        pos.setErrorIndex(start);
    } else {
        pos.setIndex(furthest);
    }
    return bestNumber;
}

int32_t
ChoiceFormat::matchStringUntilLimitPart(
        const MessagePattern &pattern, int32_t partIndex, int32_t limitPartIndex,
        const UnicodeString &source, int32_t sourceOffset) {
    int32_t matchingSourceLength = 0;
    const UnicodeString &msgString = pattern.getPatternString();
    int32_t prevIndex = pattern.getPart(partIndex).getLimit();
    for (;;) {
        const MessagePattern::Part &part = pattern.getPart(++partIndex);
        if (partIndex == limitPartIndex || part.getType() == UMSGPAT_PART_TYPE_SKIP_SYNTAX) {
            int32_t index = part.getIndex();
            int32_t length = index - prevIndex;
            if (length != 0 && 0 != source.compare(sourceOffset, length, msgString, prevIndex, length)) {
                return -1;  
            }
            matchingSourceLength += length;
            if (partIndex == limitPartIndex) {
                return matchingSourceLength;
            }
            prevIndex = part.getLimit();  
        }
    }
}




void
ChoiceFormat::parse(const UnicodeString& text, 
                    Formattable& result,
                    UErrorCode& status) const
{
    NumberFormat::parse(text, result, status);
}



Format*
ChoiceFormat::clone() const
{
    ChoiceFormat *aCopy = new ChoiceFormat(*this);
    return aCopy;
}

U_NAMESPACE_END

#endif 


