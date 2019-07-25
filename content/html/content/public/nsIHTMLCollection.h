




































#ifndef nsIHTMLCollection_h___
#define nsIHTMLCollection_h___

#include "nsIDOMHTMLCollection.h"

class nsINode;
class nsIContent;
class nsWrapperCache;


#define NS_IHTMLCOLLECTION_IID \
{ 0x3fe47ab6, 0xb60f, 0x49aa, \
 { 0xb5, 0x93, 0xcc, 0xf8, 0xbe, 0xd0, 0x83, 0x19 } }





class nsIHTMLCollection : public nsIDOMHTMLCollection
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IHTMLCOLLECTION_IID)

  


  virtual nsIContent* GetNodeAt(PRUint32 aIndex) = 0;

  


  virtual nsISupports* GetNamedItem(const nsAString& aName,
                                    nsWrapperCache** aCache) = 0;

  


  virtual nsINode* GetParentObject() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIHTMLCollection, NS_IHTMLCOLLECTION_IID)

#endif 
