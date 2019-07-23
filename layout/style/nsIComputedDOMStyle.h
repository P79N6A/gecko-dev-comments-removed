






































#ifndef nsIComputedDOMStyle_h___
#define nsIComputedDOMStyle_h___

#include "nsICSSDeclaration.h"

class nsIDOMElement;
class nsIPresShell;

#define NS_ICOMPUTEDDOMSTYLE_IID \
 { 0x5f0197a1, 0xa873, 0x44e5, \
    {0x96, 0x31, 0xac, 0xd6, 0xca, 0xb4, 0xf1, 0xe0 } }

class nsIComputedDOMStyle : public nsICSSDeclaration
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICOMPUTEDDOMSTYLE_IID)

  NS_IMETHOD Init(nsIDOMElement *aElement, const nsAString& aPseudoElt,
                  nsIPresShell *aPresShell) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIComputedDOMStyle, NS_ICOMPUTEDDOMSTYLE_IID)

nsresult 
NS_NewComputedDOMStyle(nsIComputedDOMStyle** aComputedStyle);

#endif 
