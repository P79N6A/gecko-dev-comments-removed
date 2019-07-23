









































#ifndef nsIStyleRule_h___
#define nsIStyleRule_h___

#include <stdio.h>

#include "nsISupports.h"

class nsIStyleSheet;
class nsPresContext;
class nsIContent;
struct nsRuleData;


#define NS_ISTYLE_RULE_IID     \
{0x40ae5c90, 0xad6a, 0x11d1, {0x80, 0x31, 0x00, 0x60, 0x08, 0x15, 0x9b, 0x5a}}
































class nsIStyleRule : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISTYLE_RULE_IID)

  










  NS_IMETHOD MapRuleInfoInto(nsRuleData* aRuleData)=0;

#ifdef DEBUG
  NS_IMETHOD List(FILE* out = stdout, PRInt32 aIndent = 0) const = 0;
#endif
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIStyleRule, NS_ISTYLE_RULE_IID)

#endif 
