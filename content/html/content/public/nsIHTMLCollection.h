




































#ifndef nsIHTMLCollection_h___
#define nsIHTMLCollection_h___

#include "nsIDOMHTMLCollection.h"

class nsIContent;
class nsWrapperCache;


#define NS_IHTMLCOLLECTION_IID \
{ 0x84a68396, 0x518d, 0x4fa8, \
 { 0x8f, 0x7f, 0xa0, 0x60, 0x55, 0xff, 0xef, 0xba } }





class nsIHTMLCollection : public nsIDOMHTMLCollection
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IHTMLCOLLECTION_IID)

  


  virtual nsIContent* GetNodeAt(PRUint32 aIndex) = 0;

  


  virtual nsISupports* GetNamedItem(const nsAString& aName,
                                    nsWrapperCache** aCache) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIHTMLCollection, NS_IHTMLCOLLECTION_IID)

#endif 
