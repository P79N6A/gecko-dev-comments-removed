


















































#ifndef nsWildCard_h__
#define nsWildCard_h__

#include "prtypes.h"
#include "nscore.h"












#define NON_SXP -1
#define INVALID_SXP -2
#define VALID_SXP 1

int NS_WildCardValid(const char *expr);

int NS_WildCardValid(const PRUnichar *expr);


#define MATCH 0
#define NOMATCH 1
#define ABORTED -1









int NS_WildCardMatch(const char *str, const char *expr,
                            PRBool case_insensitive);

int NS_WildCardMatch(const PRUnichar *str, const PRUnichar *expr,
                            PRBool case_insensitive);

#endif 
