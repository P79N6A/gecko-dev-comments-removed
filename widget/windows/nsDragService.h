




































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
  NS_IMETHOD IsDataFlavorSupported(const char *aDataFlavor, bool *_retval);
  NS_IMETHOD EndDragSession(bool aDoneDrag);

  
  NS_IMETHOD SetIDataObject(IDataObject * aDataObj);
  NS_IMETHOD StartInvokingDragSession(IDataObject * aDataObj,
                                      PRUint32 aActionType);

  
  void SetDroppedLocal();

protected:
  nsDataObjCollection* GetDataObjCollection(IDataObject * aDataObj);

  
  
  bool IsCollectionObject(IDataObject* inDataObj);

  
  bool CreateDragImage(nsIDOMNode *aDOMNode,
                         nsIScriptableRegion *aRegion,
                         SHDRAGIMAGE *psdi);

  IDropSource * mNativeDragSrc;
  nsNativeDragTarget * mNativeDragTarget;
  IDataObject * mDataObject;
  bool mSentLocalDropEvent;
};

#endif 
