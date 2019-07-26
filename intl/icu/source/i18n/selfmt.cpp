















#include "utypeinfo.h"  

#include "unicode/messagepattern.h"
#include "unicode/rbnf.h"
#include "unicode/selfmt.h"
#include "unicode/uchar.h"
#include "unicode/ucnv_err.h"
#include "unicode/umsg.h"
#include "unicode/ustring.h"
#include "unicode/utypes.h"
#include "cmemory.h"
#include "messageimpl.h"
#include "patternprops.h"
#include "selfmtimpl.h"
#include "uassert.h"
#include "ustrfmt.h"
#include "util.h"
#include "uvector.h"

#if !UCONFIG_NO_FORMATTING

U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(SelectFormat)

static const UChar SELECT_KEYWORD_OTHER[] = {LOW_O, LOW_T, LOW_H, LOW_E, LOW_R, 0};

SelectFormat::SelectFormat(const UnicodeString& pat,
                           UErrorCode& status) : msgPattern(status) {
   applyPattern(pat, status);
}

SelectFormat::SelectFormat(const SelectFormat& other) : Format(other),
                                                        msgPattern(other.msgPattern) {
}

SelectFormat::~SelectFormat() {
}

void
SelectFormat::applyPattern(const UnicodeString& newPattern, UErrorCode& status) {
    if (U_FAILURE(status)) {
      return;
    }

    msgPattern.parseSelectStyle(newPattern, NULL, status);
    if (U_FAILURE(status)) {
        msgPattern.clear();
    }
}

UnicodeString&
SelectFormat::format(const Formattable& obj,
                   UnicodeString& appendTo,
                   FieldPosition& pos,
                   UErrorCode& status) const
{
    if (U_FAILURE(status)) {
        return appendTo;
    }
    if (obj.getType() == Formattable::kString) {
        return format(obj.getString(status), appendTo, pos, status);
    } else {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return appendTo;
    }
}

UnicodeString&
SelectFormat::format(const UnicodeString& keyword,
                     UnicodeString& appendTo,
                     FieldPosition& ,
                     UErrorCode& status) const {
    if (U_FAILURE(status)) {
        return appendTo;
    }
    
    if (!PatternProps::isIdentifier(keyword.getBuffer(), keyword.length())) {
        status = U_ILLEGAL_ARGUMENT_ERROR;  
    }
    if (msgPattern.countParts() == 0) {
        status = U_INVALID_STATE_ERROR;
        return appendTo;
    }
    int32_t msgStart = findSubMessage(msgPattern, 0, keyword, status);
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

UnicodeString&
SelectFormat::toPattern(UnicodeString& appendTo) {
    if (0 == msgPattern.countParts()) {
        appendTo.setToBogus();
    } else {
        appendTo.append(msgPattern.getPatternString());
    }
    return appendTo;
}


int32_t SelectFormat::findSubMessage(const MessagePattern& pattern, int32_t partIndex,
                                     const UnicodeString& keyword, UErrorCode& ec) {
    if (U_FAILURE(ec)) {
        return 0;
    }
    UnicodeString other(FALSE, SELECT_KEYWORD_OTHER, 5);
    int32_t count = pattern.countParts();
    int32_t msgStart=0;
    
    do {
        const MessagePattern::Part& part=pattern.getPart(partIndex++);
        const UMessagePatternPartType type=part.getType();
        if(type==UMSGPAT_PART_TYPE_ARG_LIMIT) {
            break;
        }
        
        if(pattern.partSubstringMatches(part, keyword)) {
            
            return partIndex;
        } else if(msgStart==0 && pattern.partSubstringMatches(part, other)) {
            msgStart=partIndex;
        }
        partIndex=pattern.getLimitPartIndex(partIndex);
    } while(++partIndex<count);
    return msgStart;
}

Format* SelectFormat::clone() const
{
    return new SelectFormat(*this);
}

SelectFormat&
SelectFormat::operator=(const SelectFormat& other) {
    if (this != &other) {
        msgPattern = other.msgPattern;
    }
    return *this;
}

UBool
SelectFormat::operator==(const Format& other) const {
    if (this == &other) {
        return TRUE;
    }
    if (!Format::operator==(other)) {
        return FALSE;
    }
    const SelectFormat& o = (const SelectFormat&)other;
    return msgPattern == o.msgPattern;
}

UBool
SelectFormat::operator!=(const Format& other) const {
    return  !operator==(other);
}

void
SelectFormat::parseObject(const UnicodeString& ,
                        Formattable& ,
                        ParsePosition& pos) const
{
    
    pos.setErrorIndex(pos.getIndex());
}

U_NAMESPACE_END

#endif 


