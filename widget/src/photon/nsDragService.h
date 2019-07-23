




































#ifndef nsDragService_h__
#define nsDragService_h__

#include <Pt.h>

#include "nsBaseDragService.h"







class nsDragService : public nsBaseDragService
{

public:
  nsDragService();
  virtual ~nsDragService();
  
  NS_DECL_ISUPPORTS_INHERITED

	
  NS_IMETHOD SetNativeDndData( PtWidget_t * widget, PhEvent_t *event );


  
  NS_IMETHOD InvokeDragSession (nsIDOMNode *aDOMNode, nsISupportsArray * anArrayTransferables,
                                nsIScriptableRegion * aRegion, PRUint32 aActionType);

  
  NS_IMETHOD GetData (nsITransferable * aTransferable, PRUint32 anItem);
  NS_IMETHOD GetNumDropItems (PRUint32 * aNumItems);
  NS_IMETHOD IsDataFlavorSupported(const char *aDataFlavor, PRBool *_retval);
	NS_IMETHOD SetDropData( char *data );
	void SourceEndDrag(void);

	PRUint32 mActionType;

private:
  PtWidget_t *mDndWidget;
	static char *mDndEvent;
	static int mDndEventLen;
	nsCOMPtr<nsISupportsArray> mSourceDataItems;

	PtTransportCtrl_t *mNativeCtrl;
	char *mRawData, *mFlavourStr, *mTransportFile;
};

#endif 
