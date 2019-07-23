




































#ifndef nsIHTMLCollection_h___
#define nsIHTMLCollection_h___

#include "nsIDOMHTMLCollection.h"


#define NS_IHTMLCOLLECTION_IID \
{ 0xb90f2c8c, 0xc564, 0x4464, \
 { 0x97, 0x01, 0x05, 0x14, 0xe4, 0xeb, 0x69, 0x65 } }





class nsIHTMLCollection : public nsIDOMHTMLCollection
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IHTMLCOLLECTION_IID)

  


  virtual nsISupports* GetNodeAt(PRUint32 aIndex, nsresult* aResult) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIHTMLCollection, NS_IHTMLCOLLECTION_IID)

#endif 
