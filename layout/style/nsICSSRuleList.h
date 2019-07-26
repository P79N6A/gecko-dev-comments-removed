




#ifndef nsICSSRuleList_h
#define nsICSSRuleList_h

#include "nsIDOMCSSRule.h"
#include "nsIDOMCSSRuleList.h"


#define NS_ICSSRULELIST_IID \
{ 0xccc0135a, 0xbcb6, 0x4654, \
  { 0x8c, 0x78, 0x74, 0x35, 0x97, 0x9b, 0x88, 0x19 } }

class nsICSSRuleList : public nsIDOMCSSRuleList
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICSSRULELIST_IID)

  NS_IMETHOD
  GetLength(uint32_t* aLength) MOZ_OVERRIDE MOZ_FINAL
  {
    *aLength = Length();
    return NS_OK;
  }
  NS_IMETHOD
  Item(uint32_t aIndex, nsIDOMCSSRule** aReturn) MOZ_OVERRIDE MOZ_FINAL
  {
    NS_IF_ADDREF(*aReturn = Item(aIndex));
    return NS_OK;
  }

  
  nsIDOMCSSRule* Item(uint32_t aIndex)
  {
    bool unused;
    return IndexedGetter(aIndex, unused);
  }

  virtual nsIDOMCSSRule* IndexedGetter(uint32_t aIndex, bool& aFound) = 0;
  virtual uint32_t Length() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsICSSRuleList, NS_ICSSRULELIST_IID)

#endif 
