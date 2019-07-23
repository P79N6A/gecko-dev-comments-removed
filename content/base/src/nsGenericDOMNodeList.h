









































#ifndef nsGenericDOMNodeList_h__
#define nsGenericDOMNodeList_h__

#include "nsISupports.h"
#include "nsIDOMNodeList.h"

class nsGenericDOMNodeList : public nsIDOMNodeList 
{
public:
  nsGenericDOMNodeList();
  virtual ~nsGenericDOMNodeList();

  NS_DECL_ISUPPORTS

  
  
  NS_IMETHOD    GetLength(PRUint32* aLength)=0;
  NS_IMETHOD    Item(PRUint32 aIndex, nsIDOMNode** aReturn)=0;
};

#endif 
