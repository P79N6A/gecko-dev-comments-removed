





































#ifndef nsDragService_h_
#define nsDragService_h_

#include "nsBaseDragService.h"

#include <Cocoa/Cocoa.h>

extern NSString* const kWildcardPboardType;
extern NSString* const kCorePboardType_url;
extern NSString* const kCorePboardType_urld;
extern NSString* const kCorePboardType_urln;

class nsDragService : public nsBaseDragService
{
public:
  nsDragService();
  virtual ~nsDragService();

  
  NS_IMETHOD InvokeDragSession(nsIDOMNode *aDOMNode, nsISupportsArray * anArrayTransferables,
                               nsIScriptableRegion * aRegion, PRUint32 aActionType);
  NS_IMETHOD EndDragSession(bool aDoneDrag);

  
  NS_IMETHOD GetData(nsITransferable * aTransferable, PRUint32 aItemIndex);
  NS_IMETHOD IsDataFlavorSupported(const char *aDataFlavor, bool *_retval);
  NS_IMETHOD GetNumDropItems(PRUint32 * aNumItems);

private:

  NSImage* ConstructDragImage(nsIDOMNode* aDOMNode,
                              nsIntRect* aDragRect,
                              nsIScriptableRegion* aRegion);

  nsCOMPtr<nsISupportsArray> mDataItems; 
  NSView* mNativeDragView;
  NSEvent* mNativeDragEvent;
};

#endif 
