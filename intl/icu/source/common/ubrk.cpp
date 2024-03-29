






#include "unicode/utypes.h"

#if !UCONFIG_NO_BREAK_ITERATION

#include "unicode/ubrk.h"

#include "unicode/brkiter.h"
#include "unicode/uloc.h"
#include "unicode/ustring.h"
#include "unicode/uchriter.h"
#include "unicode/rbbi.h"
#include "rbbirb.h"
#include "uassert.h"

U_NAMESPACE_USE







U_CAPI UBreakIterator* U_EXPORT2
ubrk_open(UBreakIteratorType type,
      const char *locale,
      const UChar *text,
      int32_t textLength,
      UErrorCode *status)
{

  if(U_FAILURE(*status)) return 0;

  BreakIterator *result = 0;

  switch(type) {

  case UBRK_CHARACTER:
    result = BreakIterator::createCharacterInstance(Locale(locale), *status);
    break;

  case UBRK_WORD:
    result = BreakIterator::createWordInstance(Locale(locale), *status);
    break;

  case UBRK_LINE:
    result = BreakIterator::createLineInstance(Locale(locale), *status);
    break;

  case UBRK_SENTENCE:
    result = BreakIterator::createSentenceInstance(Locale(locale), *status);
    break;

  case UBRK_TITLE:
    result = BreakIterator::createTitleInstance(Locale(locale), *status);
    break;

  default:
    *status = U_ILLEGAL_ARGUMENT_ERROR;
  }

  
  if (U_FAILURE(*status)) {
     return 0;
  }
  if(result == 0) {
    *status = U_MEMORY_ALLOCATION_ERROR;
    return 0;
  }


  UBreakIterator *uBI = (UBreakIterator *)result;
  if (text != NULL) {
      ubrk_setText(uBI, text, textLength, status);
  }
  return uBI;
}









U_CAPI UBreakIterator* U_EXPORT2
ubrk_openRules(  const UChar        *rules,
                       int32_t       rulesLength,
                 const UChar        *text,
                       int32_t       textLength,
                       UParseError  *parseErr,
                       UErrorCode   *status)  {

    if (status == NULL || U_FAILURE(*status)){
        return 0;
    }

    BreakIterator *result = 0;
    UnicodeString ruleString(rules, rulesLength);
    result = RBBIRuleBuilder::createRuleBasedBreakIterator(ruleString, parseErr, *status);
    if(U_FAILURE(*status)) {
        return 0;
    }

    UBreakIterator *uBI = (UBreakIterator *)result;
    if (text != NULL) {
        ubrk_setText(uBI, text, textLength, status);
    }
    return uBI;
}





U_CAPI UBreakIterator * U_EXPORT2
ubrk_safeClone(
          const UBreakIterator *bi,
          void * ,
          int32_t *pBufferSize,
          UErrorCode *status)
{
    if (status == NULL || U_FAILURE(*status)){
        return NULL;
    }
    if (bi == NULL) {
       *status = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }
    if (pBufferSize != NULL) {
        int32_t inputSize = *pBufferSize;
        *pBufferSize = 1;
        if (inputSize == 0) {
            return NULL;  
        }
    }
    BreakIterator *newBI = ((BreakIterator *)bi)->clone();
    if (newBI == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
    } else {
        *status = U_SAFECLONE_ALLOCATED_WARNING;
    }
    return (UBreakIterator *)newBI;
}



U_CAPI void U_EXPORT2
ubrk_close(UBreakIterator *bi)
{
    delete (BreakIterator *)bi;
}

U_CAPI void U_EXPORT2
ubrk_setText(UBreakIterator* bi,
             const UChar*    text,
             int32_t         textLength,
             UErrorCode*     status)
{
    BreakIterator *brit = (BreakIterator *)bi;
    UText  ut = UTEXT_INITIALIZER;
    utext_openUChars(&ut, text, textLength, status);
    brit->setText(&ut, *status);
    
    
}



U_CAPI void U_EXPORT2
ubrk_setUText(UBreakIterator *bi,
             UText          *text,
             UErrorCode     *status)
{
    RuleBasedBreakIterator *brit = (RuleBasedBreakIterator *)bi;
    brit->RuleBasedBreakIterator::setText(text, *status);
}





U_CAPI int32_t U_EXPORT2
ubrk_current(const UBreakIterator *bi)
{

  return ((RuleBasedBreakIterator*)bi)->RuleBasedBreakIterator::current();
}

U_CAPI int32_t U_EXPORT2
ubrk_next(UBreakIterator *bi)
{

  return ((RuleBasedBreakIterator*)bi)->RuleBasedBreakIterator::next();
}

U_CAPI int32_t U_EXPORT2
ubrk_previous(UBreakIterator *bi)
{

  return ((RuleBasedBreakIterator*)bi)->RuleBasedBreakIterator::previous();
}

U_CAPI int32_t U_EXPORT2
ubrk_first(UBreakIterator *bi)
{

  return ((RuleBasedBreakIterator*)bi)->RuleBasedBreakIterator::first();
}

U_CAPI int32_t U_EXPORT2
ubrk_last(UBreakIterator *bi)
{

  return ((RuleBasedBreakIterator*)bi)->RuleBasedBreakIterator::last();
}

U_CAPI int32_t U_EXPORT2
ubrk_preceding(UBreakIterator *bi,
           int32_t offset)
{

  return ((RuleBasedBreakIterator*)bi)->RuleBasedBreakIterator::preceding(offset);
}

U_CAPI int32_t U_EXPORT2
ubrk_following(UBreakIterator *bi,
           int32_t offset)
{

  return ((RuleBasedBreakIterator*)bi)->RuleBasedBreakIterator::following(offset);
}

U_CAPI const char* U_EXPORT2
ubrk_getAvailable(int32_t index)
{

  return uloc_getAvailable(index);
}

U_CAPI int32_t U_EXPORT2
ubrk_countAvailable()
{

  return uloc_countAvailable();
}


U_CAPI  UBool U_EXPORT2
ubrk_isBoundary(UBreakIterator *bi, int32_t offset)
{
    return ((RuleBasedBreakIterator *)bi)->RuleBasedBreakIterator::isBoundary(offset);
}


U_CAPI  int32_t U_EXPORT2
ubrk_getRuleStatus(UBreakIterator *bi)
{
    return ((RuleBasedBreakIterator *)bi)->RuleBasedBreakIterator::getRuleStatus();
}

U_CAPI  int32_t U_EXPORT2
ubrk_getRuleStatusVec(UBreakIterator *bi, int32_t *fillInVec, int32_t capacity, UErrorCode *status)
{
    return ((RuleBasedBreakIterator *)bi)->RuleBasedBreakIterator::getRuleStatusVec(fillInVec, capacity, *status);
}


U_CAPI const char* U_EXPORT2
ubrk_getLocaleByType(const UBreakIterator *bi,
                     ULocDataLocaleType type,
                     UErrorCode* status)
{
    if (bi == NULL) {
        if (U_SUCCESS(*status)) {
            *status = U_ILLEGAL_ARGUMENT_ERROR;
        }
        return NULL;
    }
    return ((BreakIterator*)bi)->getLocaleID(type, *status);
}


void ubrk_refreshUText(UBreakIterator *bi,
                       UText          *text,
                       UErrorCode     *status)
{
    BreakIterator *bii = reinterpret_cast<BreakIterator *>(bi);
    bii->refreshInputText(text, *status);
}



#endif 
