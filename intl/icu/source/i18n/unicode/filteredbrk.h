






#ifndef FILTEREDBRK_H
#define FILTEREDBRK_H

#include "unicode/utypes.h"
#include "unicode/brkiter.h"

#if !UCONFIG_NO_BREAK_ITERATION && !UCONFIG_NO_FILTERED_BREAK_ITERATION
#ifndef U_HIDE_INTERNAL_API

U_NAMESPACE_BEGIN





























class U_I18N_API FilteredBreakIteratorBuilder : public UObject {
 public:
  



  virtual ~FilteredBreakIteratorBuilder();

  











  static FilteredBreakIteratorBuilder *createInstance(const Locale& where, UErrorCode& status);

  






  static FilteredBreakIteratorBuilder *createInstance(UErrorCode &status);

  









  virtual UBool suppressBreakAfter(const UnicodeString& string, UErrorCode& status) = 0;

  










  virtual UBool unsuppressBreakAfter(const UnicodeString& string, UErrorCode& status) = 0;

  











  virtual BreakIterator *build(BreakIterator* adoptBreakIterator, UErrorCode& status) = 0;

 protected:
  



  FilteredBreakIteratorBuilder();
};


U_NAMESPACE_END

#endif  
#endif 

#endif 
