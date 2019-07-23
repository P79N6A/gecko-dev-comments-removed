





























































 
#ifndef nsWildCard_h__
#define nsWildCard_h__

#include "prtypes.h"
#include "nscore.h"
#include <ctype.h>  
#include <string.h> 












#define NON_SXP -1
#define INVALID_SXP -2
#define VALID_SXP 1

extern int NS_WildCardValid(PRUnichar *expr);



#define MATCH 0
#define NOMATCH 1
#define ABORTED -1









extern int NS_WildCardMatch(const PRUnichar *str, const PRUnichar *expr,
                            PRBool case_insensitive);






extern int NS_WildCardSearch(const PRUnichar *str, const PRUnichar *expr);




extern int NS_WildCardCaseSearch(const PRUnichar *str, const PRUnichar *expr);

#endif 
