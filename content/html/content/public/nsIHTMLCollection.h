




































#ifndef nsIHTMLCollection_h___
#define nsIHTMLCollection_h___

#include "nsIDOMHTMLCollection.h"

class nsIContent;
class nsWrapperCache;


#define NS_IHTMLCOLLECTION_IID \
{ 0xf38b43dc, 0x74d4, 0x4b11, \
 { 0xa6, 0xc9, 0xf8, 0xf4, 0xb5, 0xd3, 0x84, 0xe3 } }





class nsIHTMLCollection : public nsIDOMHTMLCollection
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IHTMLCOLLECTION_IID)

  


  virtual nsIContent* GetNodeAt(PRUint32 aIndex, nsresult* aResult) = 0;

  


  virtual nsISupports* GetNamedItem(const nsAString& aName,
                                    nsWrapperCache** aCache,
                                    nsresult* aResult) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIHTMLCollection, NS_IHTMLCOLLECTION_IID)

#endif 
