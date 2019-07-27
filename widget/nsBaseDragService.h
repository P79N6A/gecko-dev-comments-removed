




#ifndef nsBaseDragService_h__
#define nsBaseDragService_h__

#include "nsIDragService.h"
#include "nsIDragSession.h"
#include "nsITransferable.h"
#include "nsIDOMDocument.h"
#include "nsIDOMDataTransfer.h"
#include "nsCOMPtr.h"
#include "nsRect.h"
#include "nsPoint.h"
#include "mozilla/RefPtr.h"
#include "mozilla/dom/ContentParent.h"
#include "mozilla/dom/HTMLCanvasElement.h"
#include "nsTArray.h"


#define DRAG_TRANSLUCENCY 0.65

class nsIContent;
class nsIDOMNode;
class nsPresContext;
class nsIImageLoadingContent;

namespace mozilla {
namespace gfx {
class SourceSurface;
}
}





class nsBaseDragService : public nsIDragService,
                          public nsIDragSession
{

public:
  typedef mozilla::gfx::SourceSurface SourceSurface;

  nsBaseDragService();

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDRAGSERVICE
  NS_DECL_NSIDRAGSESSION

  void SetDragEndPoint(nsIntPoint aEndDragPoint) { mEndDragPoint = aEndDragPoint; }

  uint16_t GetInputSource() { return mInputSource; }

protected:
  virtual ~nsBaseDragService();

  





















  nsresult DrawDrag(nsIDOMNode* aDOMNode,
                    nsIScriptableRegion* aRegion,
                    int32_t aScreenX, int32_t aScreenY,
                    nsIntRect* aScreenDragRect,
                    mozilla::RefPtr<SourceSurface>* aSurface,
                    nsPresContext **aPresContext);

  



  nsresult DrawDragForImage(nsPresContext* aPresContext,
                            nsIImageLoadingContent* aImageLoader,
                            mozilla::dom::HTMLCanvasElement* aCanvas,
                            int32_t aScreenX, int32_t aScreenY,
                            nsIntRect* aScreenDragRect,
                            mozilla::RefPtr<SourceSurface>* aSurface);

  


  void
  ConvertToUnscaledDevPixels(nsPresContext* aPresContext,
                             int32_t* aScreenX, int32_t* aScreenY);

  


  void OpenDragPopup();

  
  
  bool TakeDragEventDispatchedToChildProcess()
  {
    bool retval = mDragEventDispatchedToChildProcess;
    mDragEventDispatchedToChildProcess = false;
    return retval;
  }

  bool mCanDrop;
  bool mOnlyChromeDrop;
  bool mDoingDrag;
  
  bool mHasImage;
  
  bool mUserCancelled;

  bool mDragEventDispatchedToChildProcess;

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

  nsTArray<nsRefPtr<mozilla::dom::ContentParent>> mChildProcesses;
};

#endif 
