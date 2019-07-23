









































#ifndef nsIStyleRule_h___
#define nsIStyleRule_h___

#include <stdio.h>

#include "nsISupports.h"

class nsIStyleSheet;
class nsPresContext;
class nsIContent;
struct nsRuleData;


#define NS_ISTYLE_RULE_IID     \
{ 0xf75f3f70, 0x435d, 0x43a6, \
 { 0xa0, 0x1b, 0x65, 0x97, 0x04, 0x89, 0xca, 0x26 } }
































class nsIStyleRule : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISTYLE_RULE_IID)

  











  NS_IMETHOD MapRuleInfoInto(nsRuleData* aRuleData)=0;

  virtual nsIStyleRule* GetImportantRule(void) { return nsnull; }

#ifdef DEBUG
  NS_IMETHOD List(FILE* out = stdout, PRInt32 aIndent = 0) const = 0;
#endif
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIStyleRule, NS_ISTYLE_RULE_IID)

#endif 
