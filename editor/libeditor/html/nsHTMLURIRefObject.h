





#include "nsCOMPtr.h"
#include "nsISupportsImpl.h"
#include "nsIURIRefObject.h"
#include "nscore.h"

class nsIDOMMozNamedAttrMap;
class nsIDOMNode;

#ifndef nsHTMLURIRefObject_h__
#define nsHTMLURIRefObject_h__

#define NS_URI_REF_OBJECT_CID                          \
{ /* {bdd79df6-1dd1-11b2-b29c-c3d63a58f1d2} */         \
    0xbdd79df6, 0x1dd1, 0x11b2,                        \
    { 0xb2, 0x9c, 0xc3, 0xd6, 0x3a, 0x58, 0xf1, 0xd2 } \
}

class nsHTMLURIRefObject MOZ_FINAL : public nsIURIRefObject
{
public:
  nsHTMLURIRefObject();

  
  NS_DECL_ISUPPORTS

  NS_DECL_NSIURIREFOBJECT

protected:
  virtual ~nsHTMLURIRefObject();

  nsCOMPtr<nsIDOMNode> mNode;
  nsCOMPtr<nsIDOMMozNamedAttrMap> mAttributes;
  uint32_t mCurAttrIndex;
  uint32_t mAttributeCnt;
};

nsresult NS_NewHTMLURIRefObject(nsIURIRefObject** aResult, nsIDOMNode* aNode);

#endif 

