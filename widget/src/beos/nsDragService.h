





































#ifndef nsDragService_h__
#define nsDragService_h__

#include "nsBaseDragService.h"
#include "nsIDragSessionBeOS.h"
#include <String.h>

class BMessage;




class nsDragService : public nsBaseDragService, public nsIDragSessionBeOS
{

public:
  nsDragService();
  virtual ~nsDragService();

  
  NS_DECL_ISUPPORTS_INHERITED
  
  
  NS_IMETHOD InvokeDragSession(nsIDOMNode *aDOMNode,
                               nsISupportsArray * anArrayTransferables,
                               nsIScriptableRegion * aRegion,
                               PRUint32 aActionType);
  NS_IMETHOD StartDragSession();
  NS_IMETHOD EndDragSession();
  
  
  NS_IMETHOD GetNumDropItems      (PRUint32 * aNumItems);
  NS_IMETHOD GetData              (nsITransferable * aTransferable,
                                   PRUint32 aItemIndex);
  NS_IMETHOD IsDataFlavorSupported(const char *aDataFlavor, 
                                   PRBool *_retval);

  NS_IMETHOD GetCanDrop(PRBool * aCanDrop);
  NS_IMETHOD SetCanDrop(PRBool aCanDrop);

  
  NS_IMETHOD UpdateDragMessageIfNeeded(BMessage *aDragMessage);
  NS_IMETHOD TransmitData(BMessage *aNegotiationReply);


protected:
  nsCOMPtr<nsISupportsArray> mSourceDataItems;

private:
  
  void ResetDragInfo(void);
  
  BMessage      *mDragMessage;
  
  
  

  
  BString        gMimeListType;
  PRPackedBool         IsDragList(void);
  PRPackedBool         mDragIsList;

  
  BMessage * CreateDragMessage();
  static const char * FlavorToBeMime(const char * flavor);
};

#endif 
