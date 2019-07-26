






#ifndef UBRK_H
#define UBRK_H

#include "unicode/utypes.h"
#include "unicode/uloc.h"
#include "unicode/utext.h"
#include "unicode/localpointer.h"





#ifndef UBRK_TYPEDEF_UBREAK_ITERATOR
#   define UBRK_TYPEDEF_UBREAK_ITERATOR




    typedef struct UBreakIterator UBreakIterator;
#endif

#if !UCONFIG_NO_BREAK_ITERATION

#include "unicode/parseerr.h"























































typedef enum UBreakIteratorType {
  
  UBRK_CHARACTER = 0,
  
  UBRK_WORD = 1,
  
  UBRK_LINE = 2,
  
  UBRK_SENTENCE = 3,

#ifndef U_HIDE_DEPRECATED_API
  







  UBRK_TITLE = 4,
#endif 
  UBRK_COUNT = 5
} UBreakIteratorType;




#define UBRK_DONE ((int32_t) -1)










typedef enum UWordBreak {
    

    UBRK_WORD_NONE           = 0,
    
    UBRK_WORD_NONE_LIMIT     = 100,
    
    UBRK_WORD_NUMBER         = 100,
    
    UBRK_WORD_NUMBER_LIMIT   = 200,
    

    UBRK_WORD_LETTER         = 200,
    
    UBRK_WORD_LETTER_LIMIT   = 300,
    
    UBRK_WORD_KANA           = 300,
    
    UBRK_WORD_KANA_LIMIT     = 400,
    
    UBRK_WORD_IDEO           = 400,
    
    UBRK_WORD_IDEO_LIMIT     = 500
} UWordBreak;









typedef enum ULineBreakTag {
    

    UBRK_LINE_SOFT            = 0,
    
    UBRK_LINE_SOFT_LIMIT      = 100,
    
    UBRK_LINE_HARD            = 100,
    
    UBRK_LINE_HARD_LIMIT      = 200
} ULineBreakTag;











typedef enum USentenceBreakTag {
    



    UBRK_SENTENCE_TERM       = 0,
    
    UBRK_SENTENCE_TERM_LIMIT = 100,
    



    UBRK_SENTENCE_SEP        = 100,
    
    UBRK_SENTENCE_SEP_LIMIT  = 200
    
} USentenceBreakTag;
















U_STABLE UBreakIterator* U_EXPORT2
ubrk_open(UBreakIteratorType type,
      const char *locale,
      const UChar *text,
      int32_t textLength,
      UErrorCode *status);
















U_STABLE UBreakIterator* U_EXPORT2
ubrk_openRules(const UChar     *rules,
               int32_t         rulesLength,
               const UChar     *text,
               int32_t          textLength,
               UParseError     *parseErr,
               UErrorCode      *status);

















U_STABLE UBreakIterator * U_EXPORT2
ubrk_safeClone(
          const UBreakIterator *bi,
          void *stackBuffer,
          int32_t *pBufferSize,
          UErrorCode *status);





#define U_BRK_SAFECLONE_BUFFERSIZE 528







U_STABLE void U_EXPORT2
ubrk_close(UBreakIterator *bi);

#if U_SHOW_CPLUSPLUS_API

U_NAMESPACE_BEGIN










U_DEFINE_LOCAL_OPEN_POINTER(LocalUBreakIteratorPointer, UBreakIterator, ubrk_close);

U_NAMESPACE_END

#endif









U_STABLE void U_EXPORT2
ubrk_setText(UBreakIterator* bi,
             const UChar*    text,
             int32_t         textLength,
             UErrorCode*     status);













U_STABLE void U_EXPORT2
ubrk_setUText(UBreakIterator* bi,
             UText*          text,
             UErrorCode*     status);











U_STABLE int32_t U_EXPORT2
ubrk_current(const UBreakIterator *bi);










U_STABLE int32_t U_EXPORT2
ubrk_next(UBreakIterator *bi);










U_STABLE int32_t U_EXPORT2
ubrk_previous(UBreakIterator *bi);









U_STABLE int32_t U_EXPORT2
ubrk_first(UBreakIterator *bi);










U_STABLE int32_t U_EXPORT2
ubrk_last(UBreakIterator *bi);










U_STABLE int32_t U_EXPORT2
ubrk_preceding(UBreakIterator *bi,
           int32_t offset);










U_STABLE int32_t U_EXPORT2
ubrk_following(UBreakIterator *bi,
           int32_t offset);










U_STABLE const char* U_EXPORT2
ubrk_getAvailable(int32_t index);









U_STABLE int32_t U_EXPORT2
ubrk_countAvailable(void);











U_STABLE  UBool U_EXPORT2
ubrk_isBoundary(UBreakIterator *bi, int32_t offset);










U_STABLE  int32_t U_EXPORT2
ubrk_getRuleStatus(UBreakIterator *bi);


















U_STABLE  int32_t U_EXPORT2
ubrk_getRuleStatusVec(UBreakIterator *bi, int32_t *fillInVec, int32_t capacity, UErrorCode *status);










U_STABLE const char* U_EXPORT2
ubrk_getLocaleByType(const UBreakIterator *bi, ULocDataLocaleType type, UErrorCode* status);

#ifndef U_HIDE_DRAFT_API

























U_DRAFT void U_EXPORT2
ubrk_refreshUText(UBreakIterator *bi,
                       UText          *text,
                       UErrorCode     *status);
#endif  

#endif 

#endif
