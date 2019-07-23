




































#ifndef nsDragService_h__
#define nsDragService_h__

#include "nsBaseDragService.h"
#include <windows.h>
#include <shlobj.h>

struct IDropSource;
struct IDataObject;
class  nsNativeDragTarget;
class  nsDataObjCollection;
class  nsString;





class nsDragService : public nsBaseDragService
{
public:
  nsDragService();
  virtual ~nsDragService();
  
  
  NS_IMETHOD InvokeDragSession(nsIDOMNode *aDOMNode,
                               nsISupportsArray *anArrayTransferables,
                               nsIScriptableRegion *aRegion,
                               PRUint32 aActionType);

  
  NS_IMETHOD GetData(nsITransferable * aTransferable, PRUint32 anItem);
  NS_IMETHOD GetNumDropItems(PRUint32 * aNumItems);
  NS_IMETHOD IsDataFlavorSupported(const char *aDataFlavor, PRBool *_retval);
  NS_IMETHOD EndDragSession(PRBool aDoneDrag);

  
  NS_IMETHOD SetIDataObject(IDataObject * aDataObj);
  NS_IMETHOD StartInvokingDragSession(IDataObject * aDataObj,
                                      PRUint32 aActionType);

protected:
  nsDataObjCollection* GetDataObjCollection(IDataObject * aDataObj);

  
  
  PRBool IsCollectionObject(IDataObject* inDataObj);

  
  PRUint64 GetShellVersion();

  IDropSource * mNativeDragSrc;
  nsNativeDragTarget * mNativeDragTarget;
  IDataObject * mDataObject;
};

#endif 
