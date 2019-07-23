





































#ifndef nsStringUtils_h__
#define nsStringUtils_h__



#include "nsStringAPI.h"




void AppendInt(nsAString &str, PRInt32 val);
void AppendInt(nsAString &str, PRInt64 val);





PRInt32 FindChar(const nsAString &str, PRUnichar c);





PRInt32 RFindChar(const nsAString &str, PRUnichar c);

#endif  
