




















#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/appendable.h"
#include "unicode/choicfmt.h"
#include "unicode/datefmt.h"
#include "unicode/decimfmt.h"
#include "unicode/localpointer.h"
#include "unicode/msgfmt.h"
#include "unicode/plurfmt.h"
#include "unicode/rbnf.h"
#include "unicode/selfmt.h"
#include "unicode/smpdtfmt.h"
#include "unicode/umsg.h"
#include "unicode/ustring.h"
#include "cmemory.h"
#include "patternprops.h"
#include "messageimpl.h"
#include "msgfmt_impl.h"
#include "uassert.h"
#include "uelement.h"
#include "uhash.h"
#include "ustrfmt.h"
#include "util.h"
#include "uvector.h"





#define SINGLE_QUOTE      ((UChar)0x0027)
#define COMMA             ((UChar)0x002C)
#define LEFT_CURLY_BRACE  ((UChar)0x007B)
#define RIGHT_CURLY_BRACE ((UChar)0x007D)




static const UChar ID_NUMBER[]    = {
    0x6E, 0x75, 0x6D, 0x62, 0x65, 0x72, 0  
};
static const UChar ID_DATE[]      = {
    0x64, 0x61, 0x74, 0x65, 0              
};
static const UChar ID_TIME[]      = {
    0x74, 0x69, 0x6D, 0x65, 0              
};
static const UChar ID_SPELLOUT[]  = {
    0x73, 0x70, 0x65, 0x6c, 0x6c, 0x6f, 0x75, 0x74, 0 
};
static const UChar ID_ORDINAL[]   = {
    0x6f, 0x72, 0x64, 0x69, 0x6e, 0x61, 0x6c, 0 
};
static const UChar ID_DURATION[]  = {
    0x64, 0x75, 0x72, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0 
};


static const UChar * const TYPE_IDS[] = {
    ID_NUMBER,
    ID_DATE,
    ID_TIME,
    ID_SPELLOUT,
    ID_ORDINAL,
    ID_DURATION,
    NULL,
};

static const UChar ID_EMPTY[]     = {
    0 
};
static const UChar ID_CURRENCY[]  = {
    0x63, 0x75, 0x72, 0x72, 0x65, 0x6E, 0x63, 0x79, 0  
};
static const UChar ID_PERCENT[]   = {
    0x70, 0x65, 0x72, 0x63, 0x65, 0x6E, 0x74, 0        
};
static const UChar ID_INTEGER[]   = {
    0x69, 0x6E, 0x74, 0x65, 0x67, 0x65, 0x72, 0        
};


static const UChar * const NUMBER_STYLE_IDS[] = {
    ID_EMPTY,
    ID_CURRENCY,
    ID_PERCENT,
    ID_INTEGER,
    NULL,
};

static const UChar ID_SHORT[]     = {
    0x73, 0x68, 0x6F, 0x72, 0x74, 0        
};
static const UChar ID_MEDIUM[]    = {
    0x6D, 0x65, 0x64, 0x69, 0x75, 0x6D, 0  
};
static const UChar ID_LONG[]      = {
    0x6C, 0x6F, 0x6E, 0x67, 0              
};
static const UChar ID_FULL[]      = {
    0x66, 0x75, 0x6C, 0x6C, 0              
};


static const UChar * const DATE_STYLE_IDS[] = {
    ID_EMPTY,
    ID_SHORT,
    ID_MEDIUM,
    ID_LONG,
    ID_FULL,
    NULL,
};

static const icu::DateFormat::EStyle DATE_STYLES[] = {
    icu::DateFormat::kDefault,
    icu::DateFormat::kShort,
    icu::DateFormat::kMedium,
    icu::DateFormat::kLong,
    icu::DateFormat::kFull,
};

static const int32_t DEFAULT_INITIAL_CAPACITY = 10;

static const UChar NULL_STRING[] = {
    0x6E, 0x75, 0x6C, 0x6C, 0  
};

static const UChar OTHER_STRING[] = {
    0x6F, 0x74, 0x68, 0x65, 0x72, 0  
};

U_CDECL_BEGIN
static UBool U_CALLCONV equalFormatsForHash(const UHashTok key1,
                                            const UHashTok key2) {
    return icu::MessageFormat::equalFormats(key1.pointer, key2.pointer);
}

U_CDECL_END

U_NAMESPACE_BEGIN


UOBJECT_DEFINE_RTTI_IMPLEMENTATION(MessageFormat)
UOBJECT_DEFINE_NO_RTTI_IMPLEMENTATION(MessageFormat::DummyFormat)
UOBJECT_DEFINE_RTTI_IMPLEMENTATION(FormatNameEnumeration)







static UnicodeString& itos(int32_t i, UnicodeString& appendTo) {
    UChar temp[16];
    uprv_itou(temp,16,i,10,0); 
    appendTo.append(temp, -1);
    return appendTo;
}




class AppendableWrapper : public UMemory {
public:
    AppendableWrapper(Appendable& appendable) : app(appendable), len(0) {
    }
    void append(const UnicodeString& s) {
        app.appendString(s.getBuffer(), s.length());
        len += s.length();
    }
    void append(const UChar* s, const int32_t sLength) {
        app.appendString(s, sLength);
        len += sLength;
    }
    void append(const UnicodeString& s, int32_t start, int32_t length) {
        append(s.tempSubString(start, length));
    }
    void formatAndAppend(const Format* formatter, const Formattable& arg, UErrorCode& ec) {
        UnicodeString s;
        formatter->format(arg, s, ec);
        if (U_SUCCESS(ec)) {
            append(s);
        }
    }
    int32_t length() {
        return len;
    }
private:
    Appendable& app;
    int32_t len;
};





MessageFormat::MessageFormat(const UnicodeString& pattern,
                             UErrorCode& success)
: fLocale(Locale::getDefault()),  
  msgPattern(success),
  formatAliases(NULL),
  formatAliasesCapacity(0),
  argTypes(NULL),
  argTypeCount(0),
  argTypeCapacity(0),
  hasArgTypeConflicts(FALSE),
  defaultNumberFormat(NULL),
  defaultDateFormat(NULL),
  cachedFormatters(NULL),
  customFormatArgStarts(NULL),
  pluralProvider(&fLocale, UPLURAL_TYPE_CARDINAL),
  ordinalProvider(&fLocale, UPLURAL_TYPE_ORDINAL)
{
    setLocaleIDs(fLocale.getName(), fLocale.getName());
    applyPattern(pattern, success);
}

MessageFormat::MessageFormat(const UnicodeString& pattern,
                             const Locale& newLocale,
                             UErrorCode& success)
: fLocale(newLocale),
  msgPattern(success),
  formatAliases(NULL),
  formatAliasesCapacity(0),
  argTypes(NULL),
  argTypeCount(0),
  argTypeCapacity(0),
  hasArgTypeConflicts(FALSE),
  defaultNumberFormat(NULL),
  defaultDateFormat(NULL),
  cachedFormatters(NULL),
  customFormatArgStarts(NULL),
  pluralProvider(&fLocale, UPLURAL_TYPE_CARDINAL),
  ordinalProvider(&fLocale, UPLURAL_TYPE_ORDINAL)
{
    setLocaleIDs(fLocale.getName(), fLocale.getName());
    applyPattern(pattern, success);
}

MessageFormat::MessageFormat(const UnicodeString& pattern,
                             const Locale& newLocale,
                             UParseError& parseError,
                             UErrorCode& success)
: fLocale(newLocale),
  msgPattern(success),
  formatAliases(NULL),
  formatAliasesCapacity(0),
  argTypes(NULL),
  argTypeCount(0),
  argTypeCapacity(0),
  hasArgTypeConflicts(FALSE),
  defaultNumberFormat(NULL),
  defaultDateFormat(NULL),
  cachedFormatters(NULL),
  customFormatArgStarts(NULL),
  pluralProvider(&fLocale, UPLURAL_TYPE_CARDINAL),
  ordinalProvider(&fLocale, UPLURAL_TYPE_ORDINAL)
{
    setLocaleIDs(fLocale.getName(), fLocale.getName());
    applyPattern(pattern, parseError, success);
}

MessageFormat::MessageFormat(const MessageFormat& that)
:
  Format(that),
  fLocale(that.fLocale),
  msgPattern(that.msgPattern),
  formatAliases(NULL),
  formatAliasesCapacity(0),
  argTypes(NULL),
  argTypeCount(0),
  argTypeCapacity(0),
  hasArgTypeConflicts(that.hasArgTypeConflicts),
  defaultNumberFormat(NULL),
  defaultDateFormat(NULL),
  cachedFormatters(NULL),
  customFormatArgStarts(NULL),
  pluralProvider(&fLocale, UPLURAL_TYPE_CARDINAL),
  ordinalProvider(&fLocale, UPLURAL_TYPE_ORDINAL)
{
    
    UErrorCode ec = U_ZERO_ERROR;
    copyObjects(that, ec);
    if (U_FAILURE(ec)) {
        resetPattern();
    }
}

MessageFormat::~MessageFormat()
{
    uhash_close(cachedFormatters);
    uhash_close(customFormatArgStarts);

    uprv_free(argTypes);
    uprv_free(formatAliases);
    delete defaultNumberFormat;
    delete defaultDateFormat;
}











UBool MessageFormat::allocateArgTypes(int32_t capacity, UErrorCode& status) {
    if (U_FAILURE(status)) {
        return FALSE;
    }
    if (argTypeCapacity >= capacity) {
        return TRUE;
    }
    if (capacity < DEFAULT_INITIAL_CAPACITY) {
        capacity = DEFAULT_INITIAL_CAPACITY;
    } else if (capacity < 2*argTypeCapacity) {
        capacity = 2*argTypeCapacity;
    }
    Formattable::Type* a = (Formattable::Type*)
            uprv_realloc(argTypes, sizeof(*argTypes) * capacity);
    if (a == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return FALSE;
    }
    argTypes = a;
    argTypeCapacity = capacity;
    return TRUE;
}




const MessageFormat&
MessageFormat::operator=(const MessageFormat& that)
{
    if (this != &that) {
        
        Format::operator=(that);

        setLocale(that.fLocale);
        msgPattern = that.msgPattern;
        hasArgTypeConflicts = that.hasArgTypeConflicts;

        UErrorCode ec = U_ZERO_ERROR;
        copyObjects(that, ec);
        if (U_FAILURE(ec)) {
            resetPattern();
        }
    }
    return *this;
}

UBool
MessageFormat::operator==(const Format& rhs) const
{
    if (this == &rhs) return TRUE;

    MessageFormat& that = (MessageFormat&)rhs;

    
    if (!Format::operator==(rhs) ||
        msgPattern != that.msgPattern ||
        fLocale != that.fLocale) {
        return FALSE;
    }

    
    if ((customFormatArgStarts == NULL) != (that.customFormatArgStarts == NULL)) {
        return FALSE;
    }
    if (customFormatArgStarts == NULL) {
        return TRUE;
    }

    UErrorCode ec = U_ZERO_ERROR;
    const int32_t count = uhash_count(customFormatArgStarts);
    const int32_t rhs_count = uhash_count(that.customFormatArgStarts);
    if (count != rhs_count) {
        return FALSE;
    }
    int32_t idx = 0, rhs_idx = 0, pos = -1, rhs_pos = -1;
    for (; idx < count && rhs_idx < rhs_count && U_SUCCESS(ec); ++idx, ++rhs_idx) {
        const UHashElement* cur = uhash_nextElement(customFormatArgStarts, &pos);
        const UHashElement* rhs_cur = uhash_nextElement(that.customFormatArgStarts, &rhs_pos);
        if (cur->key.integer != rhs_cur->key.integer) {
            return FALSE;
        }
        const Format* format = (const Format*)uhash_iget(cachedFormatters, cur->key.integer);
        const Format* rhs_format = (const Format*)uhash_iget(that.cachedFormatters, rhs_cur->key.integer);
        if (*format != *rhs_format) {
            return FALSE;
        }
    }
    return TRUE;
}




Format*
MessageFormat::clone() const
{
    return new MessageFormat(*this);
}




void
MessageFormat::setLocale(const Locale& theLocale)
{
    if (fLocale != theLocale) {
        delete defaultNumberFormat;
        defaultNumberFormat = NULL;
        delete defaultDateFormat;
        defaultDateFormat = NULL;
        fLocale = theLocale;
        setLocaleIDs(fLocale.getName(), fLocale.getName());
        pluralProvider.reset(&fLocale);
        ordinalProvider.reset(&fLocale);
    }
}




const Locale&
MessageFormat::getLocale() const
{
    return fLocale;
}

void
MessageFormat::applyPattern(const UnicodeString& newPattern,
                            UErrorCode& status)
{
    UParseError parseError;
    applyPattern(newPattern,parseError,status);
}





void
MessageFormat::applyPattern(const UnicodeString& pattern,
                            UParseError& parseError,
                            UErrorCode& ec)
{
    if(U_FAILURE(ec)) {
        return;
    }
    msgPattern.parse(pattern, &parseError, ec);
    cacheExplicitFormats(ec);

    if (U_FAILURE(ec)) {
        resetPattern();
    }
}

void MessageFormat::resetPattern() {
    msgPattern.clear();
    uhash_close(cachedFormatters);
    cachedFormatters = NULL;
    uhash_close(customFormatArgStarts);
    customFormatArgStarts = NULL;
    argTypeCount = 0;
    hasArgTypeConflicts = FALSE;
}

void
MessageFormat::applyPattern(const UnicodeString& pattern,
                            UMessagePatternApostropheMode aposMode,
                            UParseError* parseError,
                            UErrorCode& status) {
    if (aposMode != msgPattern.getApostropheMode()) {
        msgPattern.clearPatternAndSetApostropheMode(aposMode);
    }
    applyPattern(pattern, *parseError, status);
}




UnicodeString&
MessageFormat::toPattern(UnicodeString& appendTo) const {
    if ((customFormatArgStarts != NULL && 0 != uhash_count(customFormatArgStarts)) ||
        0 == msgPattern.countParts()
    ) {
        appendTo.setToBogus();
        return appendTo;
    }
    return appendTo.append(msgPattern.getPatternString());
}

int32_t MessageFormat::nextTopLevelArgStart(int32_t partIndex) const {
    if (partIndex != 0) {
        partIndex = msgPattern.getLimitPartIndex(partIndex);
    }
    for (;;) {
        UMessagePatternPartType type = msgPattern.getPartType(++partIndex);
        if (type == UMSGPAT_PART_TYPE_ARG_START) {
            return partIndex;
        }
        if (type == UMSGPAT_PART_TYPE_MSG_LIMIT) {
            return -1;
        }
    }
}

void MessageFormat::setArgStartFormat(int32_t argStart,
                                      Format* formatter,
                                      UErrorCode& status) {
    if (U_FAILURE(status)) {
        delete formatter;
    }
    if (cachedFormatters == NULL) {
        cachedFormatters=uhash_open(uhash_hashLong, uhash_compareLong,
                                    equalFormatsForHash, &status);
        if (U_FAILURE(status)) {
            delete formatter;
            return;
        }
        uhash_setValueDeleter(cachedFormatters, uprv_deleteUObject);
    }
    if (formatter == NULL) {
        formatter = new DummyFormat();
    }
    uhash_iput(cachedFormatters, argStart, formatter, &status);
}


UBool MessageFormat::argNameMatches(int32_t partIndex, const UnicodeString& argName, int32_t argNumber) {
    const MessagePattern::Part& part = msgPattern.getPart(partIndex);
    return part.getType() == UMSGPAT_PART_TYPE_ARG_NAME ?
        msgPattern.partSubstringMatches(part, argName) :
        part.getValue() == argNumber;  
}



void MessageFormat::setCustomArgStartFormat(int32_t argStart,
                                            Format* formatter,
                                            UErrorCode& status) {
    setArgStartFormat(argStart, formatter, status);
    if (customFormatArgStarts == NULL) {
        customFormatArgStarts=uhash_open(uhash_hashLong, uhash_compareLong,
                                         NULL, &status);
    }
    uhash_iputi(customFormatArgStarts, argStart, 1, &status);
}

Format* MessageFormat::getCachedFormatter(int32_t argumentNumber) const {
    if (cachedFormatters == NULL) {
        return NULL;
    }
    void* ptr = uhash_iget(cachedFormatters, argumentNumber);
    if (ptr != NULL && dynamic_cast<DummyFormat*>((Format*)ptr) == NULL) {
        return (Format*) ptr;
    } else {
        
        return NULL;
    }
}




void
MessageFormat::adoptFormats(Format** newFormats,
                            int32_t count) {
    if (newFormats == NULL || count < 0) {
        return;
    }
    
    if (cachedFormatters != NULL) {
        uhash_removeAll(cachedFormatters);
    }
    if (customFormatArgStarts != NULL) {
        uhash_removeAll(customFormatArgStarts);
    }

    int32_t formatNumber = 0;
    UErrorCode status = U_ZERO_ERROR;
    for (int32_t partIndex = 0;
        formatNumber < count && U_SUCCESS(status) &&
            (partIndex = nextTopLevelArgStart(partIndex)) >= 0;) {
        setCustomArgStartFormat(partIndex, newFormats[formatNumber], status);
        ++formatNumber;
    }
    
    for (; formatNumber < count; ++formatNumber) {
        delete newFormats[formatNumber];
    }

}





void
MessageFormat::setFormats(const Format** newFormats,
                          int32_t count) {
    if (newFormats == NULL || count < 0) {
        return;
    }
    
    if (cachedFormatters != NULL) {
        uhash_removeAll(cachedFormatters);
    }
    if (customFormatArgStarts != NULL) {
        uhash_removeAll(customFormatArgStarts);
    }

    UErrorCode status = U_ZERO_ERROR;
    int32_t formatNumber = 0;
    for (int32_t partIndex = 0;
        formatNumber < count && U_SUCCESS(status) && (partIndex = nextTopLevelArgStart(partIndex)) >= 0;) {
      Format* newFormat = NULL;
      if (newFormats[formatNumber] != NULL) {
          newFormat = newFormats[formatNumber]->clone();
          if (newFormat == NULL) {
              status = U_MEMORY_ALLOCATION_ERROR;
          }
      }
      setCustomArgStartFormat(partIndex, newFormat, status);
      ++formatNumber;
    }
    if (U_FAILURE(status)) {
        resetPattern();
    }
}





void
MessageFormat::adoptFormat(int32_t n, Format *newFormat) {
    LocalPointer<Format> p(newFormat);
    if (n >= 0) {
        int32_t formatNumber = 0;
        for (int32_t partIndex = 0; (partIndex = nextTopLevelArgStart(partIndex)) >= 0;) {
            if (n == formatNumber) {
                UErrorCode status = U_ZERO_ERROR;
                setCustomArgStartFormat(partIndex, p.orphan(), status);
                return;
            }
            ++formatNumber;
        }
    }
}




void
MessageFormat::adoptFormat(const UnicodeString& formatName,
                           Format* formatToAdopt,
                           UErrorCode& status) {
    LocalPointer<Format> p(formatToAdopt);
    if (U_FAILURE(status)) {
        return;
    }
    int32_t argNumber = MessagePattern::validateArgumentName(formatName);
    if (argNumber < UMSGPAT_ARG_NAME_NOT_NUMBER) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    for (int32_t partIndex = 0;
        (partIndex = nextTopLevelArgStart(partIndex)) >= 0 && U_SUCCESS(status);
    ) {
        if (argNameMatches(partIndex + 1, formatName, argNumber)) {
            Format* f;
            if (p.isValid()) {
                f = p.orphan();
            } else if (formatToAdopt == NULL) {
                f = NULL;
            } else {
                f = formatToAdopt->clone();
                if (f == NULL) {
                    status = U_MEMORY_ALLOCATION_ERROR;
                    return;
                }
            }
            setCustomArgStartFormat(partIndex, f, status);
        }
    }
}




void
MessageFormat::setFormat(int32_t n, const Format& newFormat) {

    if (n >= 0) {
        int32_t formatNumber = 0;
        for (int32_t partIndex = 0;
             (partIndex = nextTopLevelArgStart(partIndex)) >= 0;) {
            if (n == formatNumber) {
                Format* new_format = newFormat.clone();
                if (new_format) {
                    UErrorCode status = U_ZERO_ERROR;
                    setCustomArgStartFormat(partIndex, new_format, status);
                }
                return;
            }
            ++formatNumber;
        }
    }
}




Format *
MessageFormat::getFormat(const UnicodeString& formatName, UErrorCode& status) {
    if (U_FAILURE(status) || cachedFormatters == NULL) return NULL;

    int32_t argNumber = MessagePattern::validateArgumentName(formatName);
    if (argNumber < UMSGPAT_ARG_NAME_NOT_NUMBER) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }
    for (int32_t partIndex = 0; (partIndex = nextTopLevelArgStart(partIndex)) >= 0;) {
        if (argNameMatches(partIndex + 1, formatName, argNumber)) {
            return getCachedFormatter(partIndex);
        }
    }
    return NULL;
}




void
MessageFormat::setFormat(const UnicodeString& formatName,
                         const Format& newFormat,
                         UErrorCode& status) {
    if (U_FAILURE(status)) return;

    int32_t argNumber = MessagePattern::validateArgumentName(formatName);
    if (argNumber < UMSGPAT_ARG_NAME_NOT_NUMBER) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    for (int32_t partIndex = 0;
        (partIndex = nextTopLevelArgStart(partIndex)) >= 0 && U_SUCCESS(status);
    ) {
        if (argNameMatches(partIndex + 1, formatName, argNumber)) {
            if (&newFormat == NULL) {
                setCustomArgStartFormat(partIndex, NULL, status);
            } else {
                Format* new_format = newFormat.clone();
                if (new_format == NULL) {
                    status = U_MEMORY_ALLOCATION_ERROR;
                    return;
                }
                setCustomArgStartFormat(partIndex, new_format, status);
            }
        }
    }
}



const Format**
MessageFormat::getFormats(int32_t& cnt) const
{
    
    
    
    
    
    MessageFormat* t = const_cast<MessageFormat*> (this);
    cnt = 0;
    if (formatAliases == NULL) {
        t->formatAliasesCapacity = (argTypeCount<10) ? 10 : argTypeCount;
        Format** a = (Format**)
            uprv_malloc(sizeof(Format*) * formatAliasesCapacity);
        if (a == NULL) {
            t->formatAliasesCapacity = 0;
            return NULL;
        }
        t->formatAliases = a;
    } else if (argTypeCount > formatAliasesCapacity) {
        Format** a = (Format**)
            uprv_realloc(formatAliases, sizeof(Format*) * argTypeCount);
        if (a == NULL) {
            t->formatAliasesCapacity = 0;
            return NULL;
        }
        t->formatAliases = a;
        t->formatAliasesCapacity = argTypeCount;
    }

    for (int32_t partIndex = 0; (partIndex = nextTopLevelArgStart(partIndex)) >= 0;) {
        t->formatAliases[cnt++] = getCachedFormatter(partIndex);
    }

    return (const Format**)formatAliases;
}


UnicodeString MessageFormat::getArgName(int32_t partIndex) {
    const MessagePattern::Part& part = msgPattern.getPart(partIndex);
    if (part.getType() == UMSGPAT_PART_TYPE_ARG_NAME) {
        return msgPattern.getSubstring(part);
    } else {
        UnicodeString temp;
        return itos(part.getValue(), temp);
    }
}

StringEnumeration*
MessageFormat::getFormatNames(UErrorCode& status) {
    if (U_FAILURE(status))  return NULL;

    UVector *fFormatNames = new UVector(status);
    if (U_FAILURE(status)) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    fFormatNames->setDeleter(uprv_deleteUObject);

    for (int32_t partIndex = 0; (partIndex = nextTopLevelArgStart(partIndex)) >= 0;) {
        fFormatNames->addElement(new UnicodeString(getArgName(partIndex + 1)), status);
    }

    StringEnumeration* nameEnumerator = new FormatNameEnumeration(fFormatNames, status);
    return nameEnumerator;
}





UnicodeString&
MessageFormat::format(const Formattable* source,
                      int32_t cnt,
                      UnicodeString& appendTo,
                      FieldPosition& ignore,
                      UErrorCode& success) const
{
    return format(source, NULL, cnt, appendTo, &ignore, success);
}






UnicodeString&
MessageFormat::format(  const UnicodeString& pattern,
                        const Formattable* arguments,
                        int32_t cnt,
                        UnicodeString& appendTo,
                        UErrorCode& success)
{
    MessageFormat temp(pattern, success);
    return temp.format(arguments, NULL, cnt, appendTo, NULL, success);
}






UnicodeString&
MessageFormat::format(const Formattable& source,
                      UnicodeString& appendTo,
                      FieldPosition& ignore,
                      UErrorCode& success) const
{
    if (U_FAILURE(success))
        return appendTo;
    if (source.getType() != Formattable::kArray) {
        success = U_ILLEGAL_ARGUMENT_ERROR;
        return appendTo;
    }
    int32_t cnt;
    const Formattable* tmpPtr = source.getArray(cnt);
    return format(tmpPtr, NULL, cnt, appendTo, &ignore, success);
}

UnicodeString&
MessageFormat::format(const UnicodeString* argumentNames,
                      const Formattable* arguments,
                      int32_t count,
                      UnicodeString& appendTo,
                      UErrorCode& success) const {
    return format(arguments, argumentNames, count, appendTo, NULL, success);
}


const Formattable* MessageFormat::getArgFromListByName(const Formattable* arguments,
                                                       const UnicodeString *argumentNames,
                                                       int32_t cnt, UnicodeString& name) const {
    for (int32_t i = 0; i < cnt; ++i) {
        if (0 == argumentNames[i].compare(name)) {
            return arguments + i;
        }
    }
    return NULL;
}


UnicodeString&
MessageFormat::format(const Formattable* arguments,
                      const UnicodeString *argumentNames,
                      int32_t cnt,
                      UnicodeString& appendTo,
                      FieldPosition* pos,
                      UErrorCode& status) const {
    if (U_FAILURE(status)) {
        return appendTo;
    }

    UnicodeStringAppendable usapp(appendTo);
    AppendableWrapper app(usapp);
    format(0, 0.0, arguments, argumentNames, cnt, app, pos, status);
    return appendTo;
}



void MessageFormat::format(int32_t msgStart, double pluralNumber,
                           const Formattable* arguments,
                           const UnicodeString *argumentNames,
                           int32_t cnt,
                           AppendableWrapper& appendTo,
                           FieldPosition* ignore,
                           UErrorCode& success) const {
    if (U_FAILURE(success)) {
        return;
    }

    const UnicodeString& msgString = msgPattern.getPatternString();
    int32_t prevIndex = msgPattern.getPart(msgStart).getLimit();
    for (int32_t i = msgStart + 1; U_SUCCESS(success) ; ++i) {
        const MessagePattern::Part* part = &msgPattern.getPart(i);
        const UMessagePatternPartType type = part->getType();
        int32_t index = part->getIndex();
        appendTo.append(msgString, prevIndex, index - prevIndex);
        if (type == UMSGPAT_PART_TYPE_MSG_LIMIT) {
            return;
        }
        prevIndex = part->getLimit();
        if (type == UMSGPAT_PART_TYPE_REPLACE_NUMBER) {
            const NumberFormat* nf = getDefaultNumberFormat(success);
            appendTo.formatAndAppend(nf, Formattable(pluralNumber), success);
            continue;
        }
        if (type != UMSGPAT_PART_TYPE_ARG_START) {
            continue;
        }
        int32_t argLimit = msgPattern.getLimitPartIndex(i);
        UMessagePatternArgType argType = part->getArgType();
        part = &msgPattern.getPart(++i);
        const Formattable* arg;
        UnicodeString noArg;
        if (argumentNames == NULL) {
            int32_t argNumber = part->getValue();  
            if (0 <= argNumber && argNumber < cnt) {
                arg = arguments + argNumber;
            } else {
                arg = NULL;
                noArg.append(LEFT_CURLY_BRACE);
                itos(argNumber, noArg);
                noArg.append(RIGHT_CURLY_BRACE);
            }
        } else {
            UnicodeString key;
            if (part->getType() == UMSGPAT_PART_TYPE_ARG_NAME) {
                key = msgPattern.getSubstring(*part);
            } else  {
                itos(part->getValue(), key);
            }
            arg = getArgFromListByName(arguments, argumentNames, cnt, key);
            if (arg == NULL) {
                noArg.append(LEFT_CURLY_BRACE);
                noArg.append(key);
                noArg.append(RIGHT_CURLY_BRACE);
            }
        }
        ++i;
        int32_t prevDestLength = appendTo.length();
        const Format* formatter = NULL;
        if (!noArg.isEmpty()) {
            appendTo.append(noArg);
        } else if (arg == NULL) {
            appendTo.append(NULL_STRING, 4);
        } else if ((formatter = getCachedFormatter(i -2))) {
            
            if (dynamic_cast<const ChoiceFormat*>(formatter) ||
                dynamic_cast<const PluralFormat*>(formatter) ||
                dynamic_cast<const SelectFormat*>(formatter)) {
                
                
                
                UnicodeString subMsgString;
                formatter->format(*arg, subMsgString, success);
                if (subMsgString.indexOf(LEFT_CURLY_BRACE) >= 0 ||
                    (subMsgString.indexOf(SINGLE_QUOTE) >= 0 && !MessageImpl::jdkAposMode(msgPattern))
                ) {
                    MessageFormat subMsgFormat(subMsgString, fLocale, success);
                    subMsgFormat.format(0, 0, arguments, argumentNames, cnt, appendTo, ignore, success);
                } else {
                    appendTo.append(subMsgString);
                }
            } else {
                appendTo.formatAndAppend(formatter, *arg, success);
            }
        } else if (argType == UMSGPAT_ARG_TYPE_NONE || (cachedFormatters && uhash_iget(cachedFormatters, i - 2))) {
            
            
            
            if (arg->isNumeric()) {
                const NumberFormat* nf = getDefaultNumberFormat(success);
                appendTo.formatAndAppend(nf, *arg, success);
            } else if (arg->getType() == Formattable::kDate) {
                const DateFormat* df = getDefaultDateFormat(success);
                appendTo.formatAndAppend(df, *arg, success);
            } else {
                appendTo.append(arg->getString(success));
            }
        } else if (argType == UMSGPAT_ARG_TYPE_CHOICE) {
            if (!arg->isNumeric()) {
                success = U_ILLEGAL_ARGUMENT_ERROR;
                return;
            }
            
            
            const double number = arg->getDouble(success);
            int32_t subMsgStart = ChoiceFormat::findSubMessage(msgPattern, i, number);
            formatComplexSubMessage(subMsgStart, 0, arguments, argumentNames,
                                    cnt, appendTo, success);
        } else if (UMSGPAT_ARG_TYPE_HAS_PLURAL_STYLE(argType)) {
            if (!arg->isNumeric()) {
                success = U_ILLEGAL_ARGUMENT_ERROR;
                return;
            }
            const PluralFormat::PluralSelector &selector =
                argType == UMSGPAT_ARG_TYPE_PLURAL ? pluralProvider : ordinalProvider;
            
            
            double number = arg->getDouble(success);
            int32_t subMsgStart = PluralFormat::findSubMessage(msgPattern, i, selector, number,
                                                               success);
            double offset = msgPattern.getPluralOffset(i);
            formatComplexSubMessage(subMsgStart, number-offset, arguments, argumentNames,
                                    cnt, appendTo, success);
        } else if (argType == UMSGPAT_ARG_TYPE_SELECT) {
            int32_t subMsgStart = SelectFormat::findSubMessage(msgPattern, i, arg->getString(success), success);
            formatComplexSubMessage(subMsgStart, 0, arguments, argumentNames,
                                    cnt, appendTo, success);
        } else {
            
            success = U_INTERNAL_PROGRAM_ERROR;
            return;
        }
        ignore = updateMetaData(appendTo, prevDestLength, ignore, arg);
        prevIndex = msgPattern.getPart(argLimit).getLimit();
        i = argLimit;
    }
}


void MessageFormat::formatComplexSubMessage(int32_t msgStart,
                                            double pluralNumber,
                                            const Formattable* arguments,
                                            const UnicodeString *argumentNames,
                                            int32_t cnt,
                                            AppendableWrapper& appendTo,
                                            UErrorCode& success) const {
    if (U_FAILURE(success)) {
        return;
    }

    if (!MessageImpl::jdkAposMode(msgPattern)) {
        format(msgStart, pluralNumber, arguments, argumentNames, cnt, appendTo, NULL, success);
        return;
    }

    
    
    
    
    
    const UnicodeString& msgString = msgPattern.getPatternString();
    UnicodeString sb;
    int32_t prevIndex = msgPattern.getPart(msgStart).getLimit();
    for (int32_t i = msgStart;;) {
        const MessagePattern::Part& part = msgPattern.getPart(++i);
        const UMessagePatternPartType type = part.getType();
        int32_t index = part.getIndex();
        if (type == UMSGPAT_PART_TYPE_MSG_LIMIT) {
            sb.append(msgString, prevIndex, index - prevIndex);
            break;
        } else if (type == UMSGPAT_PART_TYPE_REPLACE_NUMBER || type == UMSGPAT_PART_TYPE_SKIP_SYNTAX) {
            sb.append(msgString, prevIndex, index - prevIndex);
            if (type == UMSGPAT_PART_TYPE_REPLACE_NUMBER) {
                const NumberFormat* nf = getDefaultNumberFormat(success);
                sb.append(nf->format(pluralNumber, sb, success));
            }
            prevIndex = part.getLimit();
        } else if (type == UMSGPAT_PART_TYPE_ARG_START) {
            sb.append(msgString, prevIndex, index - prevIndex);
            prevIndex = index;
            i = msgPattern.getLimitPartIndex(i);
            index = msgPattern.getPart(i).getLimit();
            MessageImpl::appendReducedApostrophes(msgString, prevIndex, index, sb);
            prevIndex = index;
        }
    }
    if (sb.indexOf(LEFT_CURLY_BRACE) >= 0) {
        UnicodeString emptyPattern;  
        MessageFormat subMsgFormat(emptyPattern, fLocale, success);
        subMsgFormat.applyPattern(sb, UMSGPAT_APOS_DOUBLE_REQUIRED, NULL, success);
        subMsgFormat.format(0, 0, arguments, argumentNames, cnt, appendTo, NULL, success);
    } else {
        appendTo.append(sb);
    }
}


UnicodeString MessageFormat::getLiteralStringUntilNextArgument(int32_t from) const {
    const UnicodeString& msgString=msgPattern.getPatternString();
    int32_t prevIndex=msgPattern.getPart(from).getLimit();
    UnicodeString b;
    for (int32_t i = from + 1; ; ++i) {
        const MessagePattern::Part& part = msgPattern.getPart(i);
        const UMessagePatternPartType type=part.getType();
        int32_t index=part.getIndex();
        b.append(msgString, prevIndex, index - prevIndex);
        if(type==UMSGPAT_PART_TYPE_ARG_START || type==UMSGPAT_PART_TYPE_MSG_LIMIT) {
            return b;
        }
        
        U_ASSERT(type==UMSGPAT_PART_TYPE_SKIP_SYNTAX || type==UMSGPAT_PART_TYPE_INSERT_CHAR);
        prevIndex=part.getLimit();
    }
}


FieldPosition* MessageFormat::updateMetaData(AppendableWrapper& , int32_t ,
                             FieldPosition* , const Formattable* ) const {
    
    return NULL;
    







}

void MessageFormat::copyObjects(const MessageFormat& that, UErrorCode& ec) {
    
    
    
    
    
    argTypeCount = that.argTypeCount;
    if (argTypeCount > 0) {
        if (!allocateArgTypes(argTypeCount, ec)) {
            return;
        }
        uprv_memcpy(argTypes, that.argTypes, argTypeCount * sizeof(argTypes[0]));
    }
    if (cachedFormatters != NULL) {
        uhash_removeAll(cachedFormatters);
    }
    if (customFormatArgStarts != NULL) {
        uhash_removeAll(customFormatArgStarts);
    }
    if (that.cachedFormatters) {
        if (cachedFormatters == NULL) {
            cachedFormatters=uhash_open(uhash_hashLong, uhash_compareLong,
                                        equalFormatsForHash, &ec);
            if (U_FAILURE(ec)) {
                return;
            }
            uhash_setValueDeleter(cachedFormatters, uprv_deleteUObject);
        }

        const int32_t count = uhash_count(that.cachedFormatters);
        int32_t pos, idx;
        for (idx = 0, pos = -1; idx < count && U_SUCCESS(ec); ++idx) {
            const UHashElement* cur = uhash_nextElement(that.cachedFormatters, &pos);
            Format* newFormat = ((Format*)(cur->value.pointer))->clone();
            if (newFormat) {
                uhash_iput(cachedFormatters, cur->key.integer, newFormat, &ec);
            } else {
                ec = U_MEMORY_ALLOCATION_ERROR;
                return;
            }
        }
    }
    if (that.customFormatArgStarts) {
        if (customFormatArgStarts == NULL) {
            customFormatArgStarts=uhash_open(uhash_hashLong, uhash_compareLong,
                                              NULL, &ec);
        }
        const int32_t count = uhash_count(that.customFormatArgStarts);
        int32_t pos, idx;
        for (idx = 0, pos = -1; idx < count && U_SUCCESS(ec); ++idx) {
            const UHashElement* cur = uhash_nextElement(that.customFormatArgStarts, &pos);
            uhash_iputi(customFormatArgStarts, cur->key.integer, cur->value.integer, &ec);
        }
    }
}


Formattable*
MessageFormat::parse(int32_t msgStart,
                     const UnicodeString& source,
                     ParsePosition& pos,
                     int32_t& count,
                     UErrorCode& ec) const {
    count = 0;
    if (U_FAILURE(ec)) {
        pos.setErrorIndex(pos.getIndex());
        return NULL;
    }
    
    if (msgPattern.hasNamedArguments()) {
        ec = U_ARGUMENT_TYPE_MISMATCH;
        pos.setErrorIndex(pos.getIndex());
        return NULL;
    }
    LocalArray<Formattable> resultArray(new Formattable[argTypeCount ? argTypeCount : 1]);
    const UnicodeString& msgString=msgPattern.getPatternString();
    int32_t prevIndex=msgPattern.getPart(msgStart).getLimit();
    int32_t sourceOffset = pos.getIndex();
    ParsePosition tempStatus(0);

    for(int32_t i=msgStart+1; ; ++i) {
        UBool haveArgResult = FALSE;
        const MessagePattern::Part* part=&msgPattern.getPart(i);
        const UMessagePatternPartType type=part->getType();
        int32_t index=part->getIndex();
        
        int32_t len = index - prevIndex;
        if (len == 0 || (0 == msgString.compare(prevIndex, len, source, sourceOffset, len))) {
            sourceOffset += len;
            prevIndex += len;
        } else {
            pos.setErrorIndex(sourceOffset);
            return NULL; 
        }
        if(type==UMSGPAT_PART_TYPE_MSG_LIMIT) {
            
            pos.setIndex(sourceOffset);
            return resultArray.orphan();
        }
        if(type==UMSGPAT_PART_TYPE_SKIP_SYNTAX || type==UMSGPAT_PART_TYPE_INSERT_CHAR) {
            prevIndex=part->getLimit();
            continue;
        }
        
        
        U_ASSERT(type==UMSGPAT_PART_TYPE_ARG_START);
        int32_t argLimit=msgPattern.getLimitPartIndex(i);

        UMessagePatternArgType argType=part->getArgType();
        part=&msgPattern.getPart(++i);
        int32_t argNumber = part->getValue();  
        UnicodeString key;
        ++i;
        const Format* formatter = NULL;
        Formattable& argResult = resultArray[argNumber];

        if(cachedFormatters!=NULL && (formatter = getCachedFormatter(i - 2))!=NULL) {
            
            tempStatus.setIndex(sourceOffset);
            formatter->parseObject(source, argResult, tempStatus);
            if (tempStatus.getIndex() == sourceOffset) {
                pos.setErrorIndex(sourceOffset);
                return NULL; 
            }
            sourceOffset = tempStatus.getIndex();
            haveArgResult = TRUE;
        } else if(
            argType==UMSGPAT_ARG_TYPE_NONE || (cachedFormatters && uhash_iget(cachedFormatters, i -2))) {
            
            
            

            
            
            
            
            UnicodeString stringAfterArgument = getLiteralStringUntilNextArgument(argLimit);
            int32_t next;
            if (!stringAfterArgument.isEmpty()) {
                next = source.indexOf(stringAfterArgument, sourceOffset);
            } else {
                next = source.length();
            }
            if (next < 0) {
                pos.setErrorIndex(sourceOffset);
                return NULL; 
            } else {
                UnicodeString strValue(source.tempSubString(sourceOffset, next - sourceOffset));
                UnicodeString compValue;
                compValue.append(LEFT_CURLY_BRACE);
                itos(argNumber, compValue);
                compValue.append(RIGHT_CURLY_BRACE);
                if (0 != strValue.compare(compValue)) {
                    argResult.setString(strValue);
                    haveArgResult = TRUE;
                }
                sourceOffset = next;
            }
        } else if(argType==UMSGPAT_ARG_TYPE_CHOICE) {
            tempStatus.setIndex(sourceOffset);
            double choiceResult = ChoiceFormat::parseArgument(msgPattern, i, source, tempStatus);
            if (tempStatus.getIndex() == sourceOffset) {
                pos.setErrorIndex(sourceOffset);
                return NULL; 
            }
            argResult.setDouble(choiceResult);
            haveArgResult = TRUE;
            sourceOffset = tempStatus.getIndex();
        } else if(UMSGPAT_ARG_TYPE_HAS_PLURAL_STYLE(argType) || argType==UMSGPAT_ARG_TYPE_SELECT) {
            
            ec = U_UNSUPPORTED_ERROR;
            return NULL;
        } else {
            
            ec = U_INTERNAL_PROGRAM_ERROR;
            return NULL;
        }
        if (haveArgResult && count <= argNumber) {
            count = argNumber + 1;
        }
        prevIndex=msgPattern.getPart(argLimit).getLimit();
        i=argLimit;
    }
}





Formattable*
MessageFormat::parse(const UnicodeString& source,
                     ParsePosition& pos,
                     int32_t& count) const {
    UErrorCode ec = U_ZERO_ERROR;
    return parse(0, source, pos, count, ec);
}






Formattable*
MessageFormat::parse(const UnicodeString& source,
                     int32_t& cnt,
                     UErrorCode& success) const
{
    if (msgPattern.hasNamedArguments()) {
        success = U_ARGUMENT_TYPE_MISMATCH;
        return NULL;
    }
    ParsePosition status(0);
    
    
    Formattable* result = parse(source, status, cnt);
    if (status.getIndex() == 0) {
        success = U_MESSAGE_PARSE_ERROR;
        delete[] result;
        return NULL;
    }
    return result;
}




void
MessageFormat::parseObject( const UnicodeString& source,
                            Formattable& result,
                            ParsePosition& status) const
{
    int32_t cnt = 0;
    Formattable* tmpResult = parse(source, status, cnt);
    if (tmpResult != NULL)
        result.adoptArray(tmpResult, cnt);
}

UnicodeString
MessageFormat::autoQuoteApostrophe(const UnicodeString& pattern, UErrorCode& status) {
    UnicodeString result;
    if (U_SUCCESS(status)) {
        int32_t plen = pattern.length();
        const UChar* pat = pattern.getBuffer();
        int32_t blen = plen * 2 + 1; 
        UChar* buf = result.getBuffer(blen);
        if (buf == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
        } else {
            int32_t len = umsg_autoQuoteApostrophe(pat, plen, buf, blen, &status);
            result.releaseBuffer(U_SUCCESS(status) ? len : 0);
        }
    }
    if (U_FAILURE(status)) {
        result.setToBogus();
    }
    return result;
}



static Format* makeRBNF(URBNFRuleSetTag tag, const Locale& locale, const UnicodeString& defaultRuleSet, UErrorCode& ec) {
    RuleBasedNumberFormat* fmt = new RuleBasedNumberFormat(tag, locale, ec);
    if (fmt == NULL) {
        ec = U_MEMORY_ALLOCATION_ERROR;
    } else if (U_SUCCESS(ec) && defaultRuleSet.length() > 0) {
        UErrorCode localStatus = U_ZERO_ERROR; 
        fmt->setDefaultRuleSet(defaultRuleSet, localStatus);
    }
    return fmt;
}

void MessageFormat::cacheExplicitFormats(UErrorCode& status) {
    if (U_FAILURE(status)) {
        return;
    }

    if (cachedFormatters != NULL) {
        uhash_removeAll(cachedFormatters);
    }
    if (customFormatArgStarts != NULL) {
        uhash_removeAll(customFormatArgStarts);
    }

    
    
    int32_t limit = msgPattern.countParts() - 2;
    argTypeCount = 0;
    
    
    
    
    
    for (int32_t i = 2; i < limit && U_SUCCESS(status); ++i) {
        const MessagePattern::Part& part = msgPattern.getPart(i);
        if (part.getType() == UMSGPAT_PART_TYPE_ARG_NUMBER) {
            const int argNumber = part.getValue();
            if (argNumber >= argTypeCount) {
                argTypeCount = argNumber + 1;
            }
        }
    }
    if (!allocateArgTypes(argTypeCount, status)) {
        return;
    }
    
    
    
    for (int32_t i = 0; i < argTypeCount; ++i) {
        argTypes[i] = Formattable::kObject;
    }
    hasArgTypeConflicts = FALSE;

    
    
    for (int32_t i = 1; i < limit && U_SUCCESS(status); ++i) {
        const MessagePattern::Part* part = &msgPattern.getPart(i);
        if (part->getType() != UMSGPAT_PART_TYPE_ARG_START) {
            continue;
        }
        UMessagePatternArgType argType = part->getArgType();

        int32_t argNumber = -1;
        part = &msgPattern.getPart(i + 1);
        if (part->getType() == UMSGPAT_PART_TYPE_ARG_NUMBER) {
            argNumber = part->getValue();
        }
        Formattable::Type formattableType;

        switch (argType) {
        case UMSGPAT_ARG_TYPE_NONE:
            formattableType = Formattable::kString;
            break;
        case UMSGPAT_ARG_TYPE_SIMPLE: {
            int32_t index = i;
            i += 2;
            UnicodeString explicitType = msgPattern.getSubstring(msgPattern.getPart(i++));
            UnicodeString style;
            if ((part = &msgPattern.getPart(i))->getType() == UMSGPAT_PART_TYPE_ARG_STYLE) {
                style = msgPattern.getSubstring(*part);
                ++i;
            }
            UParseError parseError;
            Format* formatter = createAppropriateFormat(explicitType, style, formattableType, parseError, status);
            setArgStartFormat(index, formatter, status);
            break;
        }
        case UMSGPAT_ARG_TYPE_CHOICE:
        case UMSGPAT_ARG_TYPE_PLURAL:
        case UMSGPAT_ARG_TYPE_SELECTORDINAL:
            formattableType = Formattable::kDouble;
            break;
        case UMSGPAT_ARG_TYPE_SELECT:
            formattableType = Formattable::kString;
            break;
        default:
            status = U_INTERNAL_PROGRAM_ERROR;  
            formattableType = Formattable::kString;
            break;
        }
        if (argNumber != -1) {
            if (argTypes[argNumber] != Formattable::kObject && argTypes[argNumber] != formattableType) {
                hasArgTypeConflicts = TRUE;
            }
            argTypes[argNumber] = formattableType;
        }
    }
}


Format* MessageFormat::createAppropriateFormat(UnicodeString& type, UnicodeString& style,
                                               Formattable::Type& formattableType, UParseError& parseError,
                                               UErrorCode& ec) {
    if (U_FAILURE(ec)) {
        return NULL;
    }
    Format* fmt = NULL;
    int32_t typeID, styleID;
    DateFormat::EStyle date_style;

    switch (typeID = findKeyword(type, TYPE_IDS)) {
    case 0: 
        formattableType = Formattable::kDouble;
        switch (findKeyword(style, NUMBER_STYLE_IDS)) {
        case 0: 
            fmt = NumberFormat::createInstance(fLocale, ec);
            break;
        case 1: 
            fmt = NumberFormat::createCurrencyInstance(fLocale, ec);
            break;
        case 2: 
            fmt = NumberFormat::createPercentInstance(fLocale, ec);
            break;
        case 3: 
            formattableType = Formattable::kLong;
            fmt = createIntegerFormat(fLocale, ec);
            break;
        default: 
            fmt = NumberFormat::createInstance(fLocale, ec);
            if (fmt) {
                DecimalFormat* decfmt = dynamic_cast<DecimalFormat*>(fmt);
                if (decfmt != NULL) {
                    decfmt->applyPattern(style,parseError,ec);
                }
            }
            break;
        }
        break;

    case 1: 
    case 2: 
        formattableType = Formattable::kDate;
        styleID = findKeyword(style, DATE_STYLE_IDS);
        date_style = (styleID >= 0) ? DATE_STYLES[styleID] : DateFormat::kDefault;

        if (typeID == 1) {
            fmt = DateFormat::createDateInstance(date_style, fLocale);
        } else {
            fmt = DateFormat::createTimeInstance(date_style, fLocale);
        }

        if (styleID < 0 && fmt != NULL) {
            SimpleDateFormat* sdtfmt = dynamic_cast<SimpleDateFormat*>(fmt);
            if (sdtfmt != NULL) {
                sdtfmt->applyPattern(style);
            }
        }
        break;

    case 3: 
        formattableType = Formattable::kDouble;
        fmt = makeRBNF(URBNF_SPELLOUT, fLocale, style, ec);
        break;
    case 4: 
        formattableType = Formattable::kDouble;
        fmt = makeRBNF(URBNF_ORDINAL, fLocale, style, ec);
        break;
    case 5: 
        formattableType = Formattable::kDouble;
        fmt = makeRBNF(URBNF_DURATION, fLocale, style, ec);
        break;
    default:
        formattableType = Formattable::kString;
        ec = U_ILLEGAL_ARGUMENT_ERROR;
        break;
    }

    return fmt;
}




int32_t MessageFormat::findKeyword(const UnicodeString& s,
                                   const UChar * const *list)
{
    if (s.isEmpty()) {
        return 0; 
    }

    int32_t length = s.length();
    const UChar *ps = PatternProps::trimWhiteSpace(s.getBuffer(), length);
    UnicodeString buffer(FALSE, ps, length);
    
    
    buffer.toLower("");
    for (int32_t i = 0; list[i]; ++i) {
        if (!buffer.compare(list[i], u_strlen(list[i]))) {
            return i;
        }
    }
    return -1;
}




NumberFormat*
MessageFormat::createIntegerFormat(const Locale& locale, UErrorCode& status) const {
    NumberFormat *temp = NumberFormat::createInstance(locale, status);
    DecimalFormat *temp2;
    if (temp != NULL && (temp2 = dynamic_cast<DecimalFormat*>(temp)) != NULL) {
        temp2->setMaximumFractionDigits(0);
        temp2->setDecimalSeparatorAlwaysShown(FALSE);
        temp2->setParseIntegerOnly(TRUE);
    }

    return temp;
}








const NumberFormat* MessageFormat::getDefaultNumberFormat(UErrorCode& ec) const {
    if (defaultNumberFormat == NULL) {
        MessageFormat* t = (MessageFormat*) this;
        t->defaultNumberFormat = NumberFormat::createInstance(fLocale, ec);
        if (U_FAILURE(ec)) {
            delete t->defaultNumberFormat;
            t->defaultNumberFormat = NULL;
        } else if (t->defaultNumberFormat == NULL) {
            ec = U_MEMORY_ALLOCATION_ERROR;
        }
    }
    return defaultNumberFormat;
}








const DateFormat* MessageFormat::getDefaultDateFormat(UErrorCode& ec) const {
    if (defaultDateFormat == NULL) {
        MessageFormat* t = (MessageFormat*) this;
        t->defaultDateFormat = DateFormat::createDateTimeInstance(DateFormat::kShort, DateFormat::kShort, fLocale);
        if (t->defaultDateFormat == NULL) {
            ec = U_MEMORY_ALLOCATION_ERROR;
        }
    }
    return defaultDateFormat;
}

UBool
MessageFormat::usesNamedArguments() const {
    return msgPattern.hasNamedArguments();
}

int32_t
MessageFormat::getArgTypeCount() const {
    return argTypeCount;
}

UBool MessageFormat::equalFormats(const void* left, const void* right) {
    return *(const Format*)left==*(const Format*)right;
}


UBool MessageFormat::DummyFormat::operator==(const Format&) const {
    return TRUE;
}

Format* MessageFormat::DummyFormat::clone() const {
    return new DummyFormat();
}

UnicodeString& MessageFormat::DummyFormat::format(const Formattable&,
                          UnicodeString& appendTo,
                          UErrorCode& status) const {
    if (U_SUCCESS(status)) {
        status = U_UNSUPPORTED_ERROR;
    }
    return appendTo;
}

UnicodeString& MessageFormat::DummyFormat::format(const Formattable&,
                          UnicodeString& appendTo,
                          FieldPosition&,
                          UErrorCode& status) const {
    if (U_SUCCESS(status)) {
        status = U_UNSUPPORTED_ERROR;
    }
    return appendTo;
}

UnicodeString& MessageFormat::DummyFormat::format(const Formattable&,
                          UnicodeString& appendTo,
                          FieldPositionIterator*,
                          UErrorCode& status) const {
    if (U_SUCCESS(status)) {
        status = U_UNSUPPORTED_ERROR;
    }
    return appendTo;
}

void MessageFormat::DummyFormat::parseObject(const UnicodeString&,
                                                     Formattable&,
                                                     ParsePosition& ) const {
}


FormatNameEnumeration::FormatNameEnumeration(UVector *fNameList, UErrorCode& ) {
    pos=0;
    fFormatNames = fNameList;
}

const UnicodeString*
FormatNameEnumeration::snext(UErrorCode& status) {
    if (U_SUCCESS(status) && pos < fFormatNames->size()) {
        return (const UnicodeString*)fFormatNames->elementAt(pos++);
    }
    return NULL;
}

void
FormatNameEnumeration::reset(UErrorCode& ) {
    pos=0;
}

int32_t
FormatNameEnumeration::count(UErrorCode& ) const {
    return (fFormatNames==NULL) ? 0 : fFormatNames->size();
}

FormatNameEnumeration::~FormatNameEnumeration() {
    delete fFormatNames;
}


MessageFormat::PluralSelectorProvider::PluralSelectorProvider(const Locale* loc, UPluralType t)
        : locale(loc), rules(NULL), type(t) {
}

MessageFormat::PluralSelectorProvider::~PluralSelectorProvider() {
    
    delete rules;
}

UnicodeString MessageFormat::PluralSelectorProvider::select(double number, UErrorCode& ec) const {
    if (U_FAILURE(ec)) {
        return UnicodeString(FALSE, OTHER_STRING, 5);
    }
    MessageFormat::PluralSelectorProvider* t = const_cast<MessageFormat::PluralSelectorProvider*>(this);
    if(rules == NULL) {
        t->rules = PluralRules::forLocale(*locale, type, ec);
        if (U_FAILURE(ec)) {
            return UnicodeString(FALSE, OTHER_STRING, 5);
        }
    }
    return rules->select(number);
}

void MessageFormat::PluralSelectorProvider::reset(const Locale* loc) {
    locale = loc;
    delete rules;
    rules = NULL;
}


U_NAMESPACE_END

#endif 


