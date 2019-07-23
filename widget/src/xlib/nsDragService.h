







































#ifndef nsDragService_h__
#define nsDragService_h__

#include "nsBaseDragService.h"
#include "nsIDragService.h"
#include "nsIDragSessionXlib.h"
#include "nsGUIEvent.h"
#include "nsIWidget.h"
#include "nsWidget.h"





class nsDragService : public nsBaseDragService, public nsIDragSessionXlib
{

public:

  NS_DECL_ISUPPORTS_INHERITED

  nsDragService();
  virtual ~nsDragService();

  
  NS_IMETHOD InvokeDragSession (nsIDOMNode *aDOMNode,
                                nsISupportsArray * anArrayTransferables,
                                nsIScriptableRegion * aRegion,
                                PRUint32 aActionType);
  NS_IMETHOD StartDragSession();
  NS_IMETHOD EndDragSession(PRBool aDoneDrag);

  
  NS_IMETHOD GetCurrentSession     (nsIDragSession **aSession);
  NS_IMETHOD SetCanDrop            (PRBool           aCanDrop);
  NS_IMETHOD GetCanDrop            (PRBool          *aCanDrop);
  NS_IMETHOD GetNumDropItems       (PRUint32 * aNumItems);
  NS_IMETHOD GetData               (nsITransferable * aTransferable, PRUint32 anItemIndex);
  NS_IMETHOD IsDataFlavorSupported (const char *aDataFlavor, PRBool *_retval);

  
  NS_IMETHOD IsDragging(PRBool *result);
  NS_IMETHOD UpdatePosition(PRInt32 x, PRInt32 y);

protected:
  static nsEventStatus PR_CALLBACK Callback(nsGUIEvent *event);

private:
  static nsWidget *sWidget;
  static Window sWindow;
  static XlibRgbHandle *sXlibRgbHandle;
  static Display *sDisplay;
  static PRBool mDragging;
  PRBool mCanDrop;

  nsWidget *mDropWidget;
  nsCOMPtr <nsISupportsArray> mSourceDataItems;

  void CreateDragCursor(PRUint32 aActionType);
};

#endif 
