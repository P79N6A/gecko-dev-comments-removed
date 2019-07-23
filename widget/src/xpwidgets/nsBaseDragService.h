




































#ifndef nsBaseDragService_h__
#define nsBaseDragService_h__

#include "nsIDragService.h"
#include "nsIDragSession.h"
#include "nsITransferable.h"
#include "nsISupportsArray.h"
#include "nsIDOMDocument.h"
#include "nsCOMPtr.h"
#include "nsIRenderingContext.h"

#ifdef MOZ_CAIRO_GFX
#include "gfxImageSurface.h"
#endif

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

#ifdef MOZ_CAIRO_GFX
  

















  nsresult DrawDrag(nsIDOMNode* aDOMNode,
                    nsIScriptableRegion* aRegion,
                    PRInt32 aScreenX, PRInt32 aScreenY,
                    nsRect* aScreenDragRect,
                    gfxASurface** aSurface);

  


  nsresult DrawDragForImage(nsPresContext* aPresContext,
                            nsIImageLoadingContent* aImageLoader,
                            PRInt32 aScreenX, PRInt32 aScreenY,
                            nsRect* aScreenDragRect,
                            gfxASurface** aSurface);
#endif

  PRPackedBool mCanDrop;
  PRPackedBool mDoingDrag;
  
  PRPackedBool mHasImage;

  PRUint32 mDragAction;
  nsSize mTargetSize;
  nsCOMPtr<nsIDOMNode> mSourceNode;
  nsCOMPtr<nsIDOMDocument> mSourceDocument;       
                                                  

  
  nsCOMPtr<nsIDOMNode> mImage;
  
  PRInt32 mImageX;
  PRInt32 mImageY;

  
  nsCOMPtr<nsISelection> mSelection;

  
  
  
  PRInt32 mScreenX;
  PRInt32 mScreenY;
};

#endif 
