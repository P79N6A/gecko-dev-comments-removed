


















#ifndef nsWildCard_h__
#define nsWildCard_h__

#include "nscore.h"












#define NON_SXP -1
#define INVALID_SXP -2
#define VALID_SXP 1

int NS_WildCardValid(const char* aExpr);

int NS_WildCardValid(const char16_t* aExpr);


#define MATCH 0
#define NOMATCH 1
#define ABORTED -1









int NS_WildCardMatch(const char* aStr, const char* aExpr,
                     bool aCaseInsensitive);

int NS_WildCardMatch(const char16_t* aStr, const char16_t* aExpr,
                     bool aCaseInsensitive);

#endif 
