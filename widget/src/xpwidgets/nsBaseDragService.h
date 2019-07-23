




































#ifndef nsBaseDragService_h__
#define nsBaseDragService_h__

#include "nsIDragService.h"
#include "nsIDragSession.h"
#include "nsITransferable.h"
#include "nsISupportsArray.h"
#include "nsIDOMDocument.h"
#include "nsCOMPtr.h"
#include "nsIRenderingContext.h"
#include "nsIDOMDataTransfer.h"

#include "gfxImageSurface.h"


#define DRAG_TRANSLUCENCY 0.65

class nsIDOMNode;
class nsIFrame;
class nsPresContext;
class nsIImageLoadingContent;





class nsBaseDragService : public nsIDragService,
                          public nsIDragSession
{

public:
  nsBaseDragService();
  virtual ~nsBaseDragService();

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDRAGSERVICE
  NS_DECL_NSIDRAGSESSION

protected:

  




















  nsresult DrawDrag(nsIDOMNode* aDOMNode,
                    nsIScriptableRegion* aRegion,
                    PRInt32 aScreenX, PRInt32 aScreenY,
                    nsRect* aScreenDragRect,
                    gfxASurface** aSurface,
                    nsPresContext **aPresContext);

  


  nsresult DrawDragForImage(nsPresContext* aPresContext,
                            nsIImageLoadingContent* aImageLoader,
                            PRInt32 aScreenX, PRInt32 aScreenY,
                            nsRect* aScreenDragRect,
                            gfxASurface** aSurface);

  


  void
  ConvertToUnscaledDevPixels(nsPresContext* aPresContext,
                             PRInt32* aScreenX, PRInt32* aScreenY);

  PRPackedBool mCanDrop;
  PRPackedBool mDoingDrag;
  
  PRPackedBool mHasImage;

  PRUint32 mDragAction;
  nsSize mTargetSize;
  nsCOMPtr<nsIDOMNode> mSourceNode;
  nsCOMPtr<nsIDOMDocument> mSourceDocument;       
                                                  
  nsCOMPtr<nsIDOMDataTransfer> mDataTransfer;

  
  nsCOMPtr<nsIDOMNode> mImage;
  
  PRInt32 mImageX;
  PRInt32 mImageY;

  
  nsCOMPtr<nsISelection> mSelection;

  
  
  
  PRInt32 mScreenX;
  PRInt32 mScreenY;

  PRUint32 mSuppressLevel;
};

#endif 
