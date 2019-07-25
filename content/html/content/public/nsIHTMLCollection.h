




































#ifndef nsIHTMLCollection_h___
#define nsIHTMLCollection_h___

#include "nsIDOMHTMLCollection.h"

class nsIContent;
class nsWrapperCache;


#define NS_IHTMLCOLLECTION_IID \
{ 0xf615e447, 0xbdab, 0x4469, \
 { 0x92, 0x7c, 0x15, 0xb3, 0xed, 0x07, 0x36, 0x2e } }





class nsIHTMLCollection : public nsIDOMHTMLCollection
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IHTMLCOLLECTION_IID)

  


  virtual nsIContent* GetNodeAt(PRUint32 aIndex) = 0;

  


  virtual nsISupports* GetNamedItem(const nsAString& aName,
                                    nsWrapperCache** aCache,
                                    nsresult* aResult) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIHTMLCollection, NS_IHTMLCOLLECTION_IID)

#endif 
