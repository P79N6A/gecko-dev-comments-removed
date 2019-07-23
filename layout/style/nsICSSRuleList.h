




































#ifndef nsICSSRuleList_h___
#define nsICSSRuleList_h___

#include "nsIDOMCSSRuleList.h"


#define NS_ICSSRULELIST_IID \
{ 0x7ae746fd, 0x259a, 0x4a69, \
 { 0x97, 0x2d, 0x2c, 0x10, 0xf7, 0xb0, 0x04, 0xa1 } }

class nsICSSRuleList : public nsIDOMCSSRuleList
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICSSRULELIST_IID)

  virtual nsIDOMCSSRule* GetItemAt(PRUint32 aIndex, nsresult* aResult) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsICSSRuleList, NS_ICSSRULELIST_IID)

#endif 
