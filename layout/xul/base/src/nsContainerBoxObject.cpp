




































#include "nsCOMPtr.h"
#include "nsIContainerBoxObject.h"
#include "nsIBrowserBoxObject.h"
#include "nsIEditorBoxObject.h"
#include "nsIIFrameBoxObject.h"
#include "nsBoxObject.h"
#include "nsIDocShell.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIFrame.h"
#include "nsIFrameFrame.h"







class nsContainerBoxObject : public nsBoxObject,
                             public nsIBrowserBoxObject,
                             public nsIEditorBoxObject,
                             public nsIIFrameBoxObject
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSICONTAINERBOXOBJECT
  NS_DECL_NSIBROWSERBOXOBJECT
  NS_DECL_NSIEDITORBOXOBJECT
  NS_DECL_NSIIFRAMEBOXOBJECT
};

NS_INTERFACE_MAP_BEGIN(nsContainerBoxObject)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsIContainerBoxObject, nsIBrowserBoxObject)
  NS_INTERFACE_MAP_ENTRY(nsIBrowserBoxObject)
  NS_INTERFACE_MAP_ENTRY(nsIEditorBoxObject)
  NS_INTERFACE_MAP_ENTRY(nsIIFrameBoxObject)
NS_INTERFACE_MAP_END_INHERITING(nsBoxObject)

NS_IMPL_ADDREF_INHERITED(nsContainerBoxObject, nsBoxObject)
NS_IMPL_RELEASE_INHERITED(nsContainerBoxObject, nsBoxObject)

NS_IMETHODIMP nsContainerBoxObject::GetDocShell(nsIDocShell** aResult)
{
  *aResult = nsnull;

  nsIFrame *frame = GetFrame(PR_FALSE);

  if (frame) {
    nsIFrameFrame *frame_frame = nsnull;
    CallQueryInterface(frame, &frame_frame);

    if (frame_frame) {
      
      

      return frame_frame->GetDocShell(aResult);
    }
  }

  if (!mContent) {
    return NS_OK;
  }
  
  
  

  
  nsIDocument *doc = mContent->GetDocument();

  if (!doc) {
    return NS_OK;
  }
  
  nsIDocument *sub_doc = doc->GetSubDocumentFor(mContent);

  if (!sub_doc) {
    return NS_OK;
  }

  nsCOMPtr<nsISupports> container = sub_doc->GetContainer();

  if (!container) {
    return NS_OK;
  }

  return CallQueryInterface(container, aResult);
}

nsresult
NS_NewContainerBoxObject(nsIBoxObject** aResult)
{
  *aResult = new nsContainerBoxObject();
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*aResult);
  return NS_OK;
}

