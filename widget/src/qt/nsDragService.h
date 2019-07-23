




































#ifndef nsDragService_h__
#define nsDragService_h__

#include "nsBaseDragService.h"
#include "nsClipboard.h"
#include "nsIDragSessionQt.h"

#include <qwidget.h> 
#include <qdragobject.h> 



class nsDragService : public nsBaseDragService, public nsIDragSessionQt
{
public:
  nsDragService();
  virtual ~nsDragService();

  
  NS_DECL_ISUPPORTS_INHERITED
  
    
  NS_IMETHOD InvokeDragSession(nsIDOMNode *aDOMNode,
                               nsISupportsArray *anArrayTransferables,
                               nsIScriptableRegion *aRegion,
                               PRUint32 aActionType);
  NS_IMETHOD StartDragSession();
  NS_IMETHOD EndDragSession();

  
  NS_IMETHOD SetCanDrop(PRBool aCanDrop);
  NS_IMETHOD GetCanDrop(PRBool *aCanDrop);
  NS_IMETHOD GetNumDropItems(PRUint32 *aNumItems);
  NS_IMETHOD GetData(nsITransferable *aTransferable,PRUint32 aItemIndex);
  NS_IMETHOD IsDataFlavorSupported(const char *aDataFlavor,PRBool *_retval);

  
  NS_IMETHOD SetDragReference(QMimeSource* aDragRef); 

protected:
  QDragObject *RegisterDragFlavors(nsITransferable* transferable);

private:
  
  QWidget     *mHiddenWidget;
  QDragObject *mDragObject;

  
  nsCOMPtr<nsISupportsArray> mSourceDataItems;
};

#endif 
