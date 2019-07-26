













#include "unicode/messagepattern.h"
#include "unicode/plurfmt.h"
#include "unicode/plurrule.h"
#include "unicode/utypes.h"
#include "cmemory.h"
#include "messageimpl.h"
#include "plurrule_impl.h"
#include "uassert.h"
#include "uhash.h"

#if !UCONFIG_NO_FORMATTING

U_NAMESPACE_BEGIN

static const UChar OTHER_STRING[] = {
    0x6F, 0x74, 0x68, 0x65, 0x72, 0  
};

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(PluralFormat)

PluralFormat::PluralFormat(UErrorCode& status)
        : locale(Locale::getDefault()),
          msgPattern(status),
          numberFormat(NULL),
          offset(0) {
    init(NULL, UPLURAL_TYPE_CARDINAL, status);
}

PluralFormat::PluralFormat(const Locale& loc, UErrorCode& status)
        : locale(loc),
          msgPattern(status),
          numberFormat(NULL),
          offset(0) {
    init(NULL, UPLURAL_TYPE_CARDINAL, status);
}

PluralFormat::PluralFormat(const PluralRules& rules, UErrorCode& status)
        : locale(Locale::getDefault()),
          msgPattern(status),
          numberFormat(NULL),
          offset(0) {
    init(&rules, UPLURAL_TYPE_COUNT, status);
}

PluralFormat::PluralFormat(const Locale& loc,
                           const PluralRules& rules,
                           UErrorCode& status)
        : locale(loc),
          msgPattern(status),
          numberFormat(NULL),
          offset(0) {
    init(&rules, UPLURAL_TYPE_COUNT, status);
}

PluralFormat::PluralFormat(const Locale& loc,
                           UPluralType type,
                           UErrorCode& status)
        : locale(loc),
          msgPattern(status),
          numberFormat(NULL),
          offset(0) {
    init(NULL, type, status);
}

PluralFormat::PluralFormat(const UnicodeString& pat,
                           UErrorCode& status)
        : locale(Locale::getDefault()),
          msgPattern(status),
          numberFormat(NULL),
          offset(0) {
    init(NULL, UPLURAL_TYPE_CARDINAL, status);
    applyPattern(pat, status);
}

PluralFormat::PluralFormat(const Locale& loc,
                           const UnicodeString& pat,
                           UErrorCode& status)
        : locale(loc),
          msgPattern(status),
          numberFormat(NULL),
          offset(0) {
    init(NULL, UPLURAL_TYPE_CARDINAL, status);
    applyPattern(pat, status);
}

PluralFormat::PluralFormat(const PluralRules& rules,
                           const UnicodeString& pat,
                           UErrorCode& status)
        : locale(Locale::getDefault()),
          msgPattern(status),
          numberFormat(NULL),
          offset(0) {
    init(&rules, UPLURAL_TYPE_COUNT, status);
    applyPattern(pat, status);
}

PluralFormat::PluralFormat(const Locale& loc,
                           const PluralRules& rules,
                           const UnicodeString& pat,
                           UErrorCode& status)
        : locale(loc),
          msgPattern(status),
          numberFormat(NULL),
          offset(0) {
    init(&rules, UPLURAL_TYPE_COUNT, status);
    applyPattern(pat, status);
}

PluralFormat::PluralFormat(const Locale& loc,
                           UPluralType type,
                           const UnicodeString& pat,
                           UErrorCode& status)
        : locale(loc),
          msgPattern(status),
          numberFormat(NULL),
          offset(0) {
    init(NULL, type, status);
    applyPattern(pat, status);
}

PluralFormat::PluralFormat(const PluralFormat& other)
        : Format(other),
          locale(other.locale),
          msgPattern(other.msgPattern),
          numberFormat(NULL),
          offset(other.offset) {
    copyObjects(other);
}

void
PluralFormat::copyObjects(const PluralFormat& other) {
    UErrorCode status = U_ZERO_ERROR;
    if (numberFormat != NULL) {
        delete numberFormat;
    }
    if (pluralRulesWrapper.pluralRules != NULL) {
        delete pluralRulesWrapper.pluralRules;
    }

    if (other.numberFormat == NULL) {
        numberFormat = NumberFormat::createInstance(locale, status);
    } else {
        numberFormat = (NumberFormat*)other.numberFormat->clone();
    }
    if (other.pluralRulesWrapper.pluralRules == NULL) {
        pluralRulesWrapper.pluralRules = PluralRules::forLocale(locale, status);
    } else {
        pluralRulesWrapper.pluralRules = other.pluralRulesWrapper.pluralRules->clone();
    }
}


PluralFormat::~PluralFormat() {
    delete numberFormat;
}

void
PluralFormat::init(const PluralRules* rules, UPluralType type, UErrorCode& status) {
    if (U_FAILURE(status)) {
        return;
    }

    if (rules==NULL) {
        pluralRulesWrapper.pluralRules = PluralRules::forLocale(locale, type, status);
    } else {
        pluralRulesWrapper.pluralRules = rules->clone();
        if (pluralRulesWrapper.pluralRules == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
    }

    numberFormat= NumberFormat::createInstance(locale, status);
}

void
PluralFormat::applyPattern(const UnicodeString& newPattern, UErrorCode& status) {
    msgPattern.parsePluralStyle(newPattern, NULL, status);
    if (U_FAILURE(status)) {
        msgPattern.clear();
        offset = 0;
        return;
    }
    offset = msgPattern.getPluralOffset(0);
}

UnicodeString&
PluralFormat::format(const Formattable& obj,
                   UnicodeString& appendTo,
                   FieldPosition& pos,
                   UErrorCode& status) const
{
    if (U_FAILURE(status)) return appendTo;

    if (obj.isNumeric()) {
        return format(obj.getDouble(), appendTo, pos, status);
    } else {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return appendTo;
    }
}

UnicodeString
PluralFormat::format(int32_t number, UErrorCode& status) const {
    FieldPosition fpos(0);
    UnicodeString result;
    return format(number, result, fpos, status);
}

UnicodeString
PluralFormat::format(double number, UErrorCode& status) const {
    FieldPosition fpos(0);
    UnicodeString result;
    return format(number, result, fpos, status);
}


UnicodeString&
PluralFormat::format(int32_t number,
                     UnicodeString& appendTo,
                     FieldPosition& pos,
                     UErrorCode& status) const {
    return format((double)number, appendTo, pos, status);
}

UnicodeString&
PluralFormat::format(double number,
                     UnicodeString& appendTo,
                     FieldPosition& pos,
                     UErrorCode& status) const {
    if (U_FAILURE(status)) {
        return appendTo;
    }
    if (msgPattern.countParts() == 0) {
        return numberFormat->format(number, appendTo, pos);
    }
    
    int32_t partIndex = findSubMessage(msgPattern, 0, pluralRulesWrapper, number, status);
    
    
    const UnicodeString& pattern = msgPattern.getPatternString();
    number -= offset;
    int32_t prevIndex = msgPattern.getPart(partIndex).getLimit();
    for (;;) {
        const MessagePattern::Part& part = msgPattern.getPart(++partIndex);
        const UMessagePatternPartType type = part.getType();
        int32_t index = part.getIndex();
        if (type == UMSGPAT_PART_TYPE_MSG_LIMIT) {
            return appendTo.append(pattern, prevIndex, index - prevIndex);
        } else if ((type == UMSGPAT_PART_TYPE_REPLACE_NUMBER) ||
            (type == UMSGPAT_PART_TYPE_SKIP_SYNTAX && MessageImpl::jdkAposMode(msgPattern))) {
            appendTo.append(pattern, prevIndex, index - prevIndex);
            if (type == UMSGPAT_PART_TYPE_REPLACE_NUMBER) {
                numberFormat->format(number, appendTo);
            }
            prevIndex = part.getLimit();
        } else if (type == UMSGPAT_PART_TYPE_ARG_START) {
            appendTo.append(pattern, prevIndex, index - prevIndex);
            prevIndex = index;
            partIndex = msgPattern.getLimitPartIndex(partIndex);
            index = msgPattern.getPart(partIndex).getLimit();
            MessageImpl::appendReducedApostrophes(pattern, prevIndex, index, appendTo);
            prevIndex = index;
        }
    }
}

UnicodeString&
PluralFormat::toPattern(UnicodeString& appendTo) {
    if (0 == msgPattern.countParts()) {
        appendTo.setToBogus();
    } else {
        appendTo.append(msgPattern.getPatternString());
    }
    return appendTo;
}

void
PluralFormat::setLocale(const Locale& loc, UErrorCode& status) {
    if (U_FAILURE(status)) {
        return;
    }
    locale = loc;
    msgPattern.clear();
    delete numberFormat;
    offset = 0;
    numberFormat = NULL;
    pluralRulesWrapper.reset();
    init(NULL, UPLURAL_TYPE_CARDINAL, status);
}

void
PluralFormat::setNumberFormat(const NumberFormat* format, UErrorCode& status) {
    if (U_FAILURE(status)) {
        return;
    }
    NumberFormat* nf = (NumberFormat*)format->clone();
    if (nf != NULL) {
        delete numberFormat;
        numberFormat = nf;
    } else {
        status = U_MEMORY_ALLOCATION_ERROR;
    }
}

Format*
PluralFormat::clone() const
{
    return new PluralFormat(*this);
}


PluralFormat&
PluralFormat::operator=(const PluralFormat& other) {
    if (this != &other) {
        locale = other.locale;
        msgPattern = other.msgPattern;
        offset = other.offset;
        copyObjects(other);
    }

    return *this;
}

UBool
PluralFormat::operator==(const Format& other) const {
    if (this == &other) {
        return TRUE;
    }
    if (!Format::operator==(other)) {
        return FALSE;
    }
    const PluralFormat& o = (const PluralFormat&)other;
    return
        locale == o.locale &&
        msgPattern == o.msgPattern &&  
        (numberFormat == NULL) == (o.numberFormat == NULL) &&
        (numberFormat == NULL || *numberFormat == *o.numberFormat) &&
        (pluralRulesWrapper.pluralRules == NULL) == (o.pluralRulesWrapper.pluralRules == NULL) &&
        (pluralRulesWrapper.pluralRules == NULL ||
            *pluralRulesWrapper.pluralRules == *o.pluralRulesWrapper.pluralRules);
}

UBool
PluralFormat::operator!=(const Format& other) const {
    return  !operator==(other);
}

void
PluralFormat::parseObject(const UnicodeString& ,
                        Formattable& ,
                        ParsePosition& pos) const
{
    
    pos.setErrorIndex(pos.getIndex());
}

int32_t PluralFormat::findSubMessage(const MessagePattern& pattern, int32_t partIndex,
                                     const PluralSelector& selector, double number, UErrorCode& ec) {
    if (U_FAILURE(ec)) {
        return 0;
    }
    int32_t count=pattern.countParts();
    double offset;
    const MessagePattern::Part* part=&pattern.getPart(partIndex);
    if (MessagePattern::Part::hasNumericValue(part->getType())) {
        offset=pattern.getNumericValue(*part);
        ++partIndex;
    } else {
        offset=0;
    }
    
    
    
    
    UnicodeString keyword;
    UnicodeString other(FALSE, OTHER_STRING, 5);
    
    
    
    UBool haveKeywordMatch=FALSE;
    
    
    
    
    
    
    
    
    
    
    int32_t msgStart=0;
    
    
    do {
        part=&pattern.getPart(partIndex++);
        const UMessagePatternPartType type = part->getType();
        if(type==UMSGPAT_PART_TYPE_ARG_LIMIT) {
            break;
        }
        U_ASSERT (type==UMSGPAT_PART_TYPE_ARG_SELECTOR);
        
        if(MessagePattern::Part::hasNumericValue(pattern.getPartType(partIndex))) {
            
            part=&pattern.getPart(partIndex++);
            if(number==pattern.getNumericValue(*part)) {
                
                return partIndex;
            }
        } else if(!haveKeywordMatch) {
            
            
            if(pattern.partSubstringMatches(*part, other)) {
                if(msgStart==0) {
                    msgStart=partIndex;
                    if(0 == keyword.compare(other)) {
                        
                        
                        
                        haveKeywordMatch=TRUE;
                    }
                }
            } else {
                if(keyword.isEmpty()) {
                    keyword=selector.select(number-offset, ec);
                    if(msgStart!=0 && (0 == keyword.compare(other))) {
                        
                        
                        haveKeywordMatch=TRUE;
                        
                    }
                }
                if(!haveKeywordMatch && pattern.partSubstringMatches(*part, keyword)) {
                    
                    msgStart=partIndex;
                    
                    haveKeywordMatch=TRUE;
                }
            }
        }
        partIndex=pattern.getLimitPartIndex(partIndex);
    } while(++partIndex<count);
    return msgStart;
}

PluralFormat::PluralSelector::~PluralSelector() {}

PluralFormat::PluralSelectorAdapter::~PluralSelectorAdapter() {
    delete pluralRules;
}

UnicodeString PluralFormat::PluralSelectorAdapter::select(double number,
                                                          UErrorCode& ) const {
    return pluralRules->select(number);
}

void PluralFormat::PluralSelectorAdapter::reset() {
    delete pluralRules;
    pluralRules = NULL;
}


U_NAMESPACE_END


#endif 


