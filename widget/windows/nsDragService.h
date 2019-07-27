




#ifndef nsDragService_h__
#define nsDragService_h__

#include "nsBaseDragService.h"
#include <windows.h>
#include <shlobj.h>

struct IDataObject;
class  nsDataObjCollection;





class nsDragService : public nsBaseDragService
{
public:
  nsDragService();
  virtual ~nsDragService();
  
  
  NS_IMETHOD InvokeDragSession(nsIDOMNode *aDOMNode,
                               nsISupportsArray *anArrayTransferables,
                               nsIScriptableRegion *aRegion,
                               uint32_t aActionType);

  
  NS_IMETHOD GetData(nsITransferable * aTransferable, uint32_t anItem);
  NS_IMETHOD GetNumDropItems(uint32_t * aNumItems);
  NS_IMETHOD IsDataFlavorSupported(const char *aDataFlavor, bool *_retval);
  NS_IMETHOD EndDragSession(bool aDoneDrag);

  
  NS_IMETHOD SetIDataObject(IDataObject * aDataObj);
  NS_IMETHOD StartInvokingDragSession(IDataObject * aDataObj,
                                      uint32_t aActionType);

  
  void SetDroppedLocal();

protected:
  nsDataObjCollection* GetDataObjCollection(IDataObject * aDataObj);

  
  
  bool IsCollectionObject(IDataObject* inDataObj);

  
  bool CreateDragImage(nsIDOMNode *aDOMNode,
                         nsIScriptableRegion *aRegion,
                         SHDRAGIMAGE *psdi);

  IDataObject * mDataObject;
  bool mSentLocalDropEvent;
};

#endif 
