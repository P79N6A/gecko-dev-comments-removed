



#ifndef nsHTMLEntities_h___
#define nsHTMLEntities_h___

#include "nsString.h"

class nsHTMLEntities {
public:

  static nsresult AddRefTable(void);
  static void ReleaseTable(void);







  static int32_t EntityToUnicode(const nsAString& aEntity);
  static int32_t EntityToUnicode(const nsCString& aEntity);







  static const char* UnicodeToEntity(int32_t aUnicode);
};


#endif 
