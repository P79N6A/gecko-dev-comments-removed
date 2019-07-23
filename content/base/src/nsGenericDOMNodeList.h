












































#ifndef nsGenericDOMNodeList_h__
#define nsGenericDOMNodeList_h__

#include "nsISupports.h"
#include "nsIDOMNodeList.h"
#include "nsINodeList.h"

class nsGenericDOMNodeList : public nsIDOMNodeList,
                             public nsINodeList
{
public:
  nsGenericDOMNodeList();
  virtual ~nsGenericDOMNodeList();

  NS_DECL_ISUPPORTS

  NS_IMETHOD    Item(PRUint32 aIndex, nsIDOMNode** aReturn);

  
  
  NS_IMETHOD    GetLength(PRUint32* aLength)=0;

  
  virtual nsINode* GetNodeAt(PRUint32 aIndex) = 0;
};

#endif 
