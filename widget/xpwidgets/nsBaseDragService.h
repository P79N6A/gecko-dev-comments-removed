




#ifndef nsBaseDragService_h__
#define nsBaseDragService_h__

#include "nsIDragService.h"
#include "nsIDragSession.h"
#include "nsITransferable.h"
#include "nsISupportsArray.h"
#include "nsIDOMDocument.h"
#include "nsIDOMDataTransfer.h"
#include "nsCOMPtr.h"
#include "nsPoint.h"

#include "gfxImageSurface.h"


#define DRAG_TRANSLUCENCY 0.65

class nsIContent;
class nsIDOMNode;
class nsIFrame;
class nsPresContext;
class nsIImageLoadingContent;
class nsICanvasElementExternal;





class nsBaseDragService : public nsIDragService,
                          public nsIDragSession
{

public:
  nsBaseDragService();
  virtual ~nsBaseDragService();

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDRAGSERVICE
  NS_DECL_NSIDRAGSESSION

  void SetDragEndPoint(nsIntPoint aEndDragPoint) { mEndDragPoint = aEndDragPoint; }

  uint16_t GetInputSource() { return mInputSource; }

protected:

  





















  nsresult DrawDrag(nsIDOMNode* aDOMNode,
                    nsIScriptableRegion* aRegion,
                    int32_t aScreenX, int32_t aScreenY,
                    nsIntRect* aScreenDragRect,
                    gfxASurface** aSurface,
                    nsPresContext **aPresContext);

  



  nsresult DrawDragForImage(nsPresContext* aPresContext,
                            nsIImageLoadingContent* aImageLoader,
                            nsICanvasElementExternal* aCanvas,
                            int32_t aScreenX, int32_t aScreenY,
                            nsIntRect* aScreenDragRect,
                            gfxASurface** aSurface);

  


  void
  ConvertToUnscaledDevPixels(nsPresContext* aPresContext,
                             int32_t* aScreenX, int32_t* aScreenY);

  


  void OpenDragPopup();

  bool mCanDrop;
  bool mOnlyChromeDrop;
  bool mDoingDrag;
  
  bool mHasImage;
  
  bool mUserCancelled;

  uint32_t mDragAction;
  nsSize mTargetSize;
  nsCOMPtr<nsIDOMNode> mSourceNode;
  nsCOMPtr<nsIDOMDocument> mSourceDocument;       
                                                  
  nsCOMPtr<nsIDOMDataTransfer> mDataTransfer;

  
  nsCOMPtr<nsIDOMNode> mImage;
  
  int32_t mImageX;
  int32_t mImageY;

  
  nsCOMPtr<nsISelection> mSelection;

  
  
  nsCOMPtr<nsIContent> mDragPopup;

  
  
  
  int32_t mScreenX;
  int32_t mScreenY;

  
  nsIntPoint mEndDragPoint;

  uint32_t mSuppressLevel;

  
  uint16_t mInputSource;
};

#endif 
