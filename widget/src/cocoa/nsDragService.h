





































#ifndef nsDragService_h_
#define nsDragService_h_

#include "nsBaseDragService.h"

#include <Cocoa/Cocoa.h>

extern NSString* const kWildcardPboardType;

class nsDragService : public nsBaseDragService
{
public:
  nsDragService();
  virtual ~nsDragService();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD InvokeDragSession(nsIDOMNode *aDOMNode, nsISupportsArray * anArrayTransferables,
                               nsIScriptableRegion * aRegion, PRUint32 aActionType);

  
  NS_IMETHOD GetData(nsITransferable * aTransferable, PRUint32 aItemIndex);
  NS_IMETHOD IsDataFlavorSupported(const char *aDataFlavor, PRBool *_retval);
  NS_IMETHOD GetNumDropItems(PRUint32 * aNumItems);

private:

  NSImage* ConstructDragImage(nsIDOMNode* aDOMNode,
                              nsRect* aDragRect,
                              nsIScriptableRegion* aRegion);
};

#endif 
