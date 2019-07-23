








































#ifndef EmbedContextMenuInfo_h__
#define EmbedContextMenuInfo_h__
#include "nsCOMPtr.h"
#include "nsIDOMNode.h"
#include "nsIDOMEvent.h"
#include "imgIContainer.h"
#include "imgIRequest.h"
#include "nsIDOMEventTarget.h"
#include "nsRect.h"

#ifdef MOZILLA_INTERNAL_API
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#endif
#include "EmbedWindow.h"
#include "nsIDOMNSHTMLElement.h"




class EmbedContextMenuInfo : public nsISupports
{
public:
  EmbedContextMenuInfo(EmbedPrivate *aOwner);
  virtual ~EmbedContextMenuInfo(void);
  NS_DECL_ISUPPORTS
  nsresult          GetFormControlType(nsIDOMEvent *aDOMEvent);
  nsresult          UpdateContextData(nsIDOMEvent *aDOMEvent);
  nsresult          UpdateContextData(void *aEvent);
  const char*       GetSelectedText();
  nsresult          GetElementForScroll(nsIDOMDocument *targetDOMDocument);
  nsresult          GetElementForScroll(nsIDOMEvent *aEvent);
  nsresult          CheckDomImageElement(nsIDOMNode *node, nsString& aHref,
                                       PRInt32 *aWidth, PRInt32 *aHeight);
  nsresult          GetImageRequest(imgIRequest **aRequest, nsIDOMNode *aDOMNode);
  nsString          GetCtxDocTitle(void) { return mCtxDocTitle; };


  PRInt32                 mX, mY, mObjWidth, mObjHeight, mCtxFrameNum;
  nsString                mCtxURI, mCtxHref, mCtxImgHref;
  PRUint32                mEmbedCtxType;
  PRInt32 mCtxFormType;
  nsCOMPtr<nsIDOMNode>    mEventNode;
  nsCOMPtr<nsIDOMEventTarget> mEventTarget;
  nsCOMPtr<nsIDOMDocument>mCtxDocument;
  nsRect               mFormRect;
  nsCOMPtr<nsIDOMWindow>  mCtxDomWindow;
  nsCOMPtr<nsIDOMEvent>   mCtxEvent;
  nsCOMPtr<nsIDOMNSHTMLElement> mNSHHTMLElement;
  nsCOMPtr<nsIDOMNSHTMLElement> mNSHHTMLElementSc;
private:
  nsresult          SetFrameIndex();
  nsresult          SetFormControlType(nsIDOMEventTarget *originalTarget);
  nsresult          CheckDomHtmlNode(nsIDOMNode *aNode = nsnull);

  EmbedPrivate           *mOwner;
  nsCOMPtr<nsIDOMNode>    mOrigNode;
  nsString                mCtxDocTitle;
}; 
#endif 
