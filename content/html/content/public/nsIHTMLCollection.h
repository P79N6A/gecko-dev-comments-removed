




































#ifndef nsIHTMLCollection_h___
#define nsIHTMLCollection_h___

#include "nsIDOMHTMLCollection.h"


#define NS_IHTMLCOLLECTION_IID \
{ 0x5709485b, 0xc057, 0x4ba7, \
 { 0x95, 0xbd, 0x98, 0xb7, 0x94, 0x4f, 0x13, 0xe7 } }





class nsIHTMLCollection : public nsIDOMHTMLCollection
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IHTMLCOLLECTION_IID)

  


  virtual nsISupports* GetNodeAt(PRUint32 aIndex, nsresult* aResult) = 0;

  


  virtual nsISupports* GetNamedItem(const nsAString& aName,
                                    nsresult* aResult) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIHTMLCollection, NS_IHTMLCOLLECTION_IID)

#endif 
