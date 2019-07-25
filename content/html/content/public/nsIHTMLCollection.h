




































#ifndef nsIHTMLCollection_h___
#define nsIHTMLCollection_h___

#include "nsIDOMHTMLCollection.h"

class nsIContent;


#define NS_IHTMLCOLLECTION_IID \
{ 0x81e08958, 0x76e3, 0x4055, \
 { 0xa0, 0xf3, 0xb3, 0x6b, 0x85, 0xfe, 0xed, 0x73 } }





class nsIHTMLCollection : public nsIDOMHTMLCollection
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IHTMLCOLLECTION_IID)

  


  virtual nsIContent* GetNodeAt(PRUint32 aIndex, nsresult* aResult) = 0;

  


  virtual nsISupports* GetNamedItem(const nsAString& aName,
                                    nsresult* aResult) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIHTMLCollection, NS_IHTMLCOLLECTION_IID)

#endif 
