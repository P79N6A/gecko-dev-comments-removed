



































#ifndef nsHTMLEntities_h___
#define nsHTMLEntities_h___

#include "nsString.h"

class nsHTMLEntities {
public:

  static nsresult AddRefTable(void);
  static void ReleaseTable(void);







  static PRInt32 EntityToUnicode(const nsAString& aEntity);
  static PRInt32 EntityToUnicode(const nsCString& aEntity);







  static const char* UnicodeToEntity(PRInt32 aUnicode);
};


#endif 
