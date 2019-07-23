





































#include "nsIURIRefObject.h"

#include "nsCOMPtr.h"
#include "nsIDOMNode.h"
#include "nsIDOMNamedNodeMap.h"
#include "nsString.h"

#ifndef nsHTMLURIRefObject_h__
#define nsHTMLURIRefObject_h__

#define NS_URI_REF_OBJECT_CID                          \
{ /* {bdd79df6-1dd1-11b2-b29c-c3d63a58f1d2} */         \
    0xbdd79df6, 0x1dd1, 0x11b2,                        \
    { 0xb2, 0x9c, 0xc3, 0xd6, 0x3a, 0x58, 0xf1, 0xd2 } \
}

class nsHTMLURIRefObject : public nsIURIRefObject
{
public:
  nsHTMLURIRefObject();
  virtual ~nsHTMLURIRefObject();

  
  NS_DECL_ISUPPORTS

  NS_DECL_NSIURIREFOBJECT

protected:
  nsCOMPtr<nsIDOMNode> mNode;
  nsCOMPtr<nsIDOMNamedNodeMap> mAttributes;
  PRUint32 mCurAttrIndex;
  PRUint32 mAttributeCnt;
};

nsresult NS_NewHTMLURIRefObject(nsIURIRefObject** aResult, nsIDOMNode* aNode);

#endif 

