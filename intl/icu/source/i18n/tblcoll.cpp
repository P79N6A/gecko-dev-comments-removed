























































#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/tblcoll.h"
#include "unicode/coleitr.h"
#include "unicode/ures.h"
#include "unicode/uset.h"
#include "ucol_imp.h"
#include "uresimp.h"
#include "uhash.h"
#include "cmemory.h"
#include "cstring.h"
#include "putilimp.h"
#include "ustr_imp.h"



U_NAMESPACE_BEGIN




RuleBasedCollator::RuleBasedCollator(const RuleBasedCollator& that)
: Collator(that)
, dataIsOwned(FALSE)
, isWriteThroughAlias(FALSE)
, ucollator(NULL)
{
    RuleBasedCollator::operator=(that);
}

RuleBasedCollator::RuleBasedCollator(const UnicodeString& rules,
                                     UErrorCode& status) :
dataIsOwned(FALSE)
{
    construct(rules,
        UCOL_DEFAULT_STRENGTH,
        UCOL_DEFAULT,
        status);
}

RuleBasedCollator::RuleBasedCollator(const UnicodeString& rules,
                                     ECollationStrength collationStrength,
                                     UErrorCode& status) : dataIsOwned(FALSE)
{
    construct(rules,
        (UColAttributeValue)collationStrength,
        UCOL_DEFAULT,
        status);
}

RuleBasedCollator::RuleBasedCollator(const UnicodeString& rules,
                                     UColAttributeValue decompositionMode,
                                     UErrorCode& status) :
dataIsOwned(FALSE)
{
    construct(rules,
        UCOL_DEFAULT_STRENGTH,
        decompositionMode,
        status);
}

RuleBasedCollator::RuleBasedCollator(const UnicodeString& rules,
                                     ECollationStrength collationStrength,
                                     UColAttributeValue decompositionMode,
                                     UErrorCode& status) : dataIsOwned(FALSE)
{
    construct(rules,
        (UColAttributeValue)collationStrength,
        decompositionMode,
        status);
}
RuleBasedCollator::RuleBasedCollator(const uint8_t *bin, int32_t length,
                    const RuleBasedCollator *base,
                    UErrorCode &status) :
dataIsOwned(TRUE),
isWriteThroughAlias(FALSE)
{
  ucollator = ucol_openBinary(bin, length, base->ucollator, &status);
}

void
RuleBasedCollator::setRuleStringFromCollator()
{
    int32_t length;
    const UChar *r = ucol_getRules(ucollator, &length);

    if (r && length > 0) {
        
        urulestring.setTo(TRUE, r, length);
    }
    else {
        urulestring.truncate(0); 
    }
}


void
RuleBasedCollator::construct(const UnicodeString& rules,
                             UColAttributeValue collationStrength,
                             UColAttributeValue decompositionMode,
                             UErrorCode& status)
{
    ucollator = ucol_openRules(rules.getBuffer(), rules.length(),
        decompositionMode, collationStrength,
        NULL, &status);

    dataIsOwned = TRUE; 
    isWriteThroughAlias = FALSE;

    if(ucollator == NULL) {
        if(U_SUCCESS(status)) {
            status = U_MEMORY_ALLOCATION_ERROR;
        }
        return; 
    }

    setRuleStringFromCollator();
}



RuleBasedCollator::~RuleBasedCollator()
{
    if (dataIsOwned)
    {
        ucol_close(ucollator);
    }
    ucollator = 0;
}



UBool RuleBasedCollator::operator==(const Collator& that) const
{
  
  if (this == &that) {
    return TRUE;
  }
  if (!Collator::operator==(that)) {
    return FALSE;  
  }

  RuleBasedCollator& thatAlias = (RuleBasedCollator&)that;

  return ucol_equals(this->ucollator, thatAlias.ucollator);
}


RuleBasedCollator& RuleBasedCollator::operator=(const RuleBasedCollator& that)
{
    if (this == &that) { return *this; }

    UErrorCode intStatus = U_ZERO_ERROR;
    int32_t buffersize = U_COL_SAFECLONE_BUFFERSIZE;
    UCollator *ucol = ucol_safeClone(that.ucollator, NULL, &buffersize, &intStatus);
    if (U_FAILURE(intStatus)) { return *this; }

    if (dataIsOwned) {
        ucol_close(ucollator);
    }
    ucollator = ucol;
    dataIsOwned = TRUE;
    isWriteThroughAlias = FALSE;
    setRuleStringFromCollator();
    return *this;
}


Collator* RuleBasedCollator::clone() const
{
    RuleBasedCollator* coll = new RuleBasedCollator(*this);
    
    if (coll != NULL && coll->ucollator == NULL) {
        delete coll;
        return NULL;
    }
    return coll;
}


CollationElementIterator* RuleBasedCollator::createCollationElementIterator
                                           (const UnicodeString& source) const
{
    UErrorCode status = U_ZERO_ERROR;
    CollationElementIterator *result = new CollationElementIterator(source, this,
                                                                    status);
    if (U_FAILURE(status)) {
        delete result;
        return NULL;
    }

    return result;
}






CollationElementIterator* RuleBasedCollator::createCollationElementIterator
                                       (const CharacterIterator& source) const
{
    UErrorCode status = U_ZERO_ERROR;
    CollationElementIterator *result = new CollationElementIterator(source, this,
                                                                    status);

    if (U_FAILURE(status)) {
        delete result;
        return NULL;
    }

    return result;
}








const UnicodeString& RuleBasedCollator::getRules() const
{
    return urulestring;
}

void RuleBasedCollator::getRules(UColRuleOption delta, UnicodeString &buffer)
{
    int32_t rulesize = ucol_getRulesEx(ucollator, delta, NULL, -1);

    if (rulesize > 0) {
        UChar *rules = (UChar*) uprv_malloc( sizeof(UChar) * (rulesize) );
        if(rules != NULL) {
            ucol_getRulesEx(ucollator, delta, rules, rulesize);
            buffer.setTo(rules, rulesize);
            uprv_free(rules);
        } else { 
            buffer.remove();
        }
    }
    else {
        buffer.remove();
    }
}

UnicodeSet *
RuleBasedCollator::getTailoredSet(UErrorCode &status) const
{
    if(U_FAILURE(status)) {
        return NULL;
    }
    return (UnicodeSet *)ucol_getTailoredSet(this->ucollator, &status);
}


void RuleBasedCollator::getVersion(UVersionInfo versionInfo) const
{
    if (versionInfo!=NULL){
        ucol_getVersion(ucollator, versionInfo);
    }
}




UCollationResult RuleBasedCollator::compare(
                                               const UnicodeString& source,
                                               const UnicodeString& target,
                                               int32_t length,
                                               UErrorCode &status) const
{
    return compare(source.getBuffer(), uprv_min(length,source.length()), target.getBuffer(), uprv_min(length,target.length()), status);
}

UCollationResult RuleBasedCollator::compare(const UChar* source,
                                                       int32_t sourceLength,
                                                       const UChar* target,
                                                       int32_t targetLength,
                                                       UErrorCode &status) const
{
    if(U_SUCCESS(status)) {
        return  ucol_strcoll(ucollator, source, sourceLength, target, targetLength);
    } else {
        return UCOL_EQUAL;
    }
}

UCollationResult RuleBasedCollator::compare(
                                             const UnicodeString& source,
                                             const UnicodeString& target,
                                             UErrorCode &status) const
{
    if(U_SUCCESS(status)) {
        return ucol_strcoll(ucollator, source.getBuffer(), source.length(),
                                       target.getBuffer(), target.length());
    } else {
        return UCOL_EQUAL;
    }
}

UCollationResult RuleBasedCollator::compare(UCharIterator &sIter,
                                            UCharIterator &tIter,
                                            UErrorCode &status) const {
    if(U_SUCCESS(status)) {
        return ucol_strcollIter(ucollator, &sIter, &tIter, &status);
    } else {
        return UCOL_EQUAL;
    }
}


































CollationKey& RuleBasedCollator::getCollationKey(
                                                  const UnicodeString& source,
                                                  CollationKey& sortkey,
                                                  UErrorCode& status) const
{
    return getCollationKey(source.getBuffer(), source.length(), sortkey, status);
}

CollationKey& RuleBasedCollator::getCollationKey(const UChar* source,
                                                    int32_t sourceLen,
                                                    CollationKey& sortkey,
                                                    UErrorCode& status) const
{
    if (U_FAILURE(status)) {
        return sortkey.setToBogus();
    }
    if (sourceLen < -1 || (source == NULL && sourceLen != 0)) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return sortkey.setToBogus();
    }

    if (sourceLen < 0) {
        sourceLen = u_strlen(source);
    }
    if (sourceLen == 0) {
        return sortkey.reset();
    }

    int32_t resultLen = ucol_getCollationKey(ucollator, source, sourceLen, sortkey, status);

    if (U_SUCCESS(status)) {
        sortkey.setLength(resultLen);
    } else {
        sortkey.setToBogus();
    }
    return sortkey;
}










int32_t RuleBasedCollator::getMaxExpansion(int32_t order) const
{
    uint8_t result;
    UCOL_GETMAXEXPANSION(ucollator, (uint32_t)order, result);
    return result;
}

uint8_t* RuleBasedCollator::cloneRuleData(int32_t &length,
                                              UErrorCode &status)
{
    return ucol_cloneRuleData(ucollator, &length, &status);
}


int32_t RuleBasedCollator::cloneBinary(uint8_t *buffer, int32_t capacity, UErrorCode &status)
{
  return ucol_cloneBinary(ucollator, buffer, capacity, &status);
}

void RuleBasedCollator::setAttribute(UColAttribute attr,
                                     UColAttributeValue value,
                                     UErrorCode &status)
{
    if (U_FAILURE(status))
        return;
    checkOwned();
    ucol_setAttribute(ucollator, attr, value, &status);
}

UColAttributeValue RuleBasedCollator::getAttribute(UColAttribute attr,
                                                      UErrorCode &status) const
{
    if (U_FAILURE(status))
        return UCOL_DEFAULT;
    return ucol_getAttribute(ucollator, attr, &status);
}

uint32_t RuleBasedCollator::setVariableTop(const UChar *varTop, int32_t len, UErrorCode &status) {
    checkOwned();
    return ucol_setVariableTop(ucollator, varTop, len, &status);
}

uint32_t RuleBasedCollator::setVariableTop(const UnicodeString &varTop, UErrorCode &status) {
    checkOwned();
    return ucol_setVariableTop(ucollator, varTop.getBuffer(), varTop.length(), &status);
}

void RuleBasedCollator::setVariableTop(uint32_t varTop, UErrorCode &status) {
    checkOwned();
    ucol_restoreVariableTop(ucollator, varTop, &status);
}

uint32_t RuleBasedCollator::getVariableTop(UErrorCode &status) const {
  return ucol_getVariableTop(ucollator, &status);
}

int32_t RuleBasedCollator::getSortKey(const UnicodeString& source,
                                         uint8_t *result, int32_t resultLength)
                                         const
{
    return ucol_getSortKey(ucollator, source.getBuffer(), source.length(), result, resultLength);
}

int32_t RuleBasedCollator::getSortKey(const UChar *source,
                                         int32_t sourceLength, uint8_t *result,
                                         int32_t resultLength) const
{
    return ucol_getSortKey(ucollator, source, sourceLength, result, resultLength);
}

int32_t RuleBasedCollator::getReorderCodes(int32_t *dest,
                                          int32_t destCapacity,
                                          UErrorCode& status) const
{
    return ucol_getReorderCodes(ucollator, dest, destCapacity, &status);
}

void RuleBasedCollator::setReorderCodes(const int32_t *reorderCodes,
                                       int32_t reorderCodesLength,
                                       UErrorCode& status)
{
    checkOwned();
    ucol_setReorderCodes(ucollator, reorderCodes, reorderCodesLength, &status);
}

int32_t RuleBasedCollator::getEquivalentReorderCodes(int32_t reorderCode,
                                int32_t* dest,
                                int32_t destCapacity,
                                UErrorCode& status)
{
    return ucol_getEquivalentReorderCodes(reorderCode, dest, destCapacity, &status);
}





int32_t RuleBasedCollator::hashCode() const
{
    int32_t length;
    const UChar *rules = ucol_getRules(ucollator, &length);
    return ustr_hashUCharsN(rules, length);
}




Locale RuleBasedCollator::getLocale(ULocDataLocaleType type, UErrorCode &status) const {
    const char *result = ucol_getLocaleByType(ucollator, type, &status);
    if(result == NULL) {
        Locale res("");
        res.setToBogus();
        return res;
    } else {
        return Locale(result);
    }
}

void
RuleBasedCollator::setLocales(const Locale& requestedLocale, const Locale& validLocale, const Locale& actualLocale) {
    checkOwned();
    char* rloc  = uprv_strdup(requestedLocale.getName());
    if (rloc) {
        char* vloc = uprv_strdup(validLocale.getName());
        if (vloc) {
            char* aloc = uprv_strdup(actualLocale.getName());
            if (aloc) {
                ucol_setReqValidLocales(ucollator, rloc, vloc, aloc);
                return;
            }
            uprv_free(vloc);
        }
        uprv_free(rloc);
    }
}



RuleBasedCollator::RuleBasedCollator()
  : dataIsOwned(FALSE), isWriteThroughAlias(FALSE), ucollator(NULL)
{
}

RuleBasedCollator::RuleBasedCollator(const Locale& desiredLocale,
                                           UErrorCode& status)
 : dataIsOwned(FALSE), isWriteThroughAlias(FALSE), ucollator(NULL)
{
    if (U_FAILURE(status))
        return;

    






















    setUCollator(desiredLocale, status);

    if (U_FAILURE(status))
    {
        status = U_ZERO_ERROR;

        setUCollator(kRootLocaleName, status);
        if (status == U_ZERO_ERROR) {
            status = U_USING_DEFAULT_WARNING;
        }
    }

    if (U_SUCCESS(status))
    {
        setRuleStringFromCollator();
    }
}

void
RuleBasedCollator::setUCollator(const char *locale,
                                UErrorCode &status)
{
    if (U_FAILURE(status)) {
        return;
    }
    if (ucollator && dataIsOwned)
        ucol_close(ucollator);
    ucollator = ucol_open_internal(locale, &status);
    dataIsOwned = TRUE;
    isWriteThroughAlias = FALSE;
}


void
RuleBasedCollator::checkOwned() {
    if (!(dataIsOwned || isWriteThroughAlias)) {
        UErrorCode status = U_ZERO_ERROR;
        ucollator = ucol_safeClone(ucollator, NULL, NULL, &status);
        setRuleStringFromCollator();
        dataIsOwned = TRUE;
        isWriteThroughAlias = FALSE;
    }
}


int32_t RuleBasedCollator::internalGetShortDefinitionString(const char *locale,
                                                                      char *buffer,
                                                                      int32_t capacity,
                                                                      UErrorCode &status) const {
  
  return ucol_getShortDefinitionString(ucollator, locale, buffer, capacity, &status);
}


UOBJECT_DEFINE_RTTI_IMPLEMENTATION(RuleBasedCollator)

U_NAMESPACE_END

#endif 
