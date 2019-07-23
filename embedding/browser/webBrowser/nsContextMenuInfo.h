






































#ifndef nsContextMenuInfo_h__
#define nsContextMenuInfo_h__

#include "nsCOMPtr.h"
#include "nsIContextMenuListener2.h"
#include "nsIDOMNode.h"
#include "nsIDOMEvent.h"
#include "imgIContainer.h"
#include "imgIRequest.h"

class ChromeContextMenuListener;






 
class nsContextMenuInfo : public nsIContextMenuInfo
{
  friend class ChromeContextMenuListener;
  
public:    
                    nsContextMenuInfo();

  NS_DECL_ISUPPORTS
  NS_DECL_NSICONTEXTMENUINFO
  
private:
  virtual           ~nsContextMenuInfo();

  void              SetMouseEvent(nsIDOMEvent *aEvent)
                    { mMouseEvent = aEvent; }

  void              SetDOMNode(nsIDOMNode *aNode)
                    { mDOMNode = aNode; }
                    
  void              SetAssociatedLink(nsIDOMNode *aLink)
                    { mAssociatedLink = aLink; }

  nsresult          GetImageRequest(nsIDOMNode *aDOMNode,
                                    imgIRequest **aRequest);

  PRBool            HasBackgroundImage(nsIDOMNode *aDOMNode);

  nsresult          GetBackgroundImageRequest(nsIDOMNode *aDOMNode,
                                              imgIRequest **aRequest);

  nsresult          GetBackgroundImageRequestInternal(nsIDOMNode *aDOMNode,
                                                      imgIRequest **aRequest);
  
private:
  nsCOMPtr<nsIDOMEvent>   mMouseEvent;
  nsCOMPtr<nsIDOMNode>    mDOMNode;  
  nsCOMPtr<nsIDOMNode>    mAssociatedLink;

}; 


#endif 
