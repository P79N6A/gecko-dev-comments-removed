




































#ifndef nsIHTMLCollection_h___
#define nsIHTMLCollection_h___

#include "nsIDOMHTMLCollection.h"

class nsINode;
class nsIContent;
class nsWrapperCache;


#define NS_IHTMLCOLLECTION_IID \
{ 0xdea91ad6, 0x57d1, 0x4e7a, \
 { 0xb5, 0x5a, 0xdb, 0xfc, 0x36, 0x7b, 0xc8, 0x22 } }




class nsIHTMLCollection : public nsIDOMHTMLCollection
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IHTMLCOLLECTION_IID)

  


  virtual nsINode* GetParentObject() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIHTMLCollection, NS_IHTMLCOLLECTION_IID)

#endif 
