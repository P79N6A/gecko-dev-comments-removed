














































#ifndef nsDragService_h__
#define nsDragService_h__


#include "nsBaseDragService.h"
#include "nsIDragSessionMac.h"
#include <Drag.h>
#include <MacWindows.h>

class nsILocalFile;

class nsDragService : public nsBaseDragService, public nsIDragSessionMac
{
public:
  nsDragService();
  virtual ~nsDragService();

  
  NS_DECL_ISUPPORTS_INHERITED
  
  
  NS_IMETHOD InvokeDragSession (nsIDOMNode *aDOMNode, nsISupportsArray * anArrayTransferables, nsIScriptableRegion * aRegion, PRUint32 aActionType);

  
  NS_IMETHOD GetData (nsITransferable * aTransferable, PRUint32 aItemIndex);
  NS_IMETHOD IsDataFlavorSupported(const char *aDataFlavor, PRBool *_retval);
  NS_IMETHOD GetNumDropItems (PRUint32 * aNumItems);
  NS_IMETHOD SetDragAction ( PRUint32 anAction ) ;

  
  NS_IMETHOD SetDragReference ( DragReference aDragRef ) ;  

private:

  char* LookupMimeMappingsForItem ( DragReference inDragRef, ItemReference itemRef ) ;

  void RegisterDragItemsAndFlavors ( nsISupportsArray * inArray, RgnHandle inDragRgn ) ;
  PRBool BuildDragRegion ( nsIScriptableRegion* inRegion, nsIDOMNode* inNode, RgnHandle ioDragRgn ) ;
  OSErr GetDataForFlavor ( nsISupportsArray* inDragItems, DragReference inDragRef, unsigned int inItemIndex, 
                             FlavorType inFlavor, void** outData, unsigned int * outSize ) ;
  nsresult ExtractDataFromOS ( DragReference inDragRef, ItemReference inItemRef, ResType inFlavor, 
                                 void** outBuffer, PRUint32* outBuffSize ) ;

    
  PRBool ComputeGlobalRectFromFrame ( nsIDOMNode* aDOMNode, Rect & outScreenRect ) ;

  OSErr GetHFSPromiseDropDirectory(DragReference inDragRef, unsigned int inItemIndex,
                                  FlavorType inFlavor, nsILocalFile** outDir);

  OSErr SetDropFileInDrag(DragReference inDragRef, unsigned int inItemIndex,
                                  FlavorType inFlavor, nsILocalFile* inFile);

    
  static pascal OSErr DragSendDataProc ( FlavorType inFlavor, void* inRefCon,
  										 ItemReference theItemRef, DragReference inDragRef ) ;

  DragSendDataUPP mDragSendDataUPP;
  DragReference mDragRef;        
  nsISupportsArray* mDataItems;  
                                 
                                 
  PRBool mImageDraggingSupported;

}; 


#endif 

