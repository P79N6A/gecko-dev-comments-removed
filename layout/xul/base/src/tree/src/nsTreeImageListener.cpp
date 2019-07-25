






































#include "nsTreeImageListener.h"
#include "nsITreeBoxObject.h"
#include "imgIRequest.h"
#include "imgIContainer.h"

NS_IMPL_ISUPPORTS3(nsTreeImageListener, imgIDecoderObserver, imgIContainerObserver, nsITreeImageListener)

nsTreeImageListener::nsTreeImageListener(nsTreeBodyFrame* aTreeFrame)
  : mTreeFrame(aTreeFrame),
    mInvalidationSuppressed(true),
    mInvalidationArea(nsnull)
{
}

nsTreeImageListener::~nsTreeImageListener()
{
  delete mInvalidationArea;
}

NS_IMETHODIMP
nsTreeImageListener::OnImageIsAnimated(imgIRequest *aRequest)
{
  if (!mTreeFrame) {
    return NS_OK;
  }

  return mTreeFrame->OnImageIsAnimated(aRequest);
}

NS_IMETHODIMP nsTreeImageListener::OnStartContainer(imgIRequest *aRequest,
                                                    imgIContainer *aImage)
{
  
  
  
  aRequest->IncrementAnimationConsumers();
  return NS_OK;
}

NS_IMETHODIMP nsTreeImageListener::OnDataAvailable(imgIRequest *aRequest,
                                                   bool aCurrentFrame,
                                                   const nsIntRect *aRect)
{
  Invalidate();
  return NS_OK;
}

NS_IMETHODIMP nsTreeImageListener::FrameChanged(imgIContainer *aContainer,
                                                const nsIntRect *aDirtyRect)
{
  Invalidate();
  return NS_OK;
}


NS_IMETHODIMP
nsTreeImageListener::AddCell(PRInt32 aIndex, nsITreeColumn* aCol)
{
  if (!mInvalidationArea) {
    mInvalidationArea = new InvalidationArea(aCol);
    mInvalidationArea->AddRow(aIndex);
  }
  else {
    InvalidationArea* currArea;
    for (currArea = mInvalidationArea; currArea; currArea = currArea->GetNext()) {
      if (currArea->GetCol() == aCol) {
        currArea->AddRow(aIndex);
        break;
      }
    }
    if (!currArea) {
      currArea = new InvalidationArea(aCol);
      currArea->SetNext(mInvalidationArea);
      mInvalidationArea = currArea;
      mInvalidationArea->AddRow(aIndex);
    }
  }

  return NS_OK;
}


void
nsTreeImageListener::Invalidate()
{
  if (!mInvalidationSuppressed) {
    for (InvalidationArea* currArea = mInvalidationArea; currArea;
         currArea = currArea->GetNext()) {
      
      for (PRInt32 i = currArea->GetMin(); i <= currArea->GetMax(); ++i) {
        if (mTreeFrame) {
          nsITreeBoxObject* tree = mTreeFrame->GetTreeBoxObject();
          if (tree) {
            tree->InvalidateCell(i, currArea->GetCol());
          }
        }
      }
    }
  }
}

nsTreeImageListener::InvalidationArea::InvalidationArea(nsITreeColumn* aCol)
  : mCol(aCol),
    mMin(-1), 
    mMax(0),
    mNext(nsnull)
{
}

void
nsTreeImageListener::InvalidationArea::AddRow(PRInt32 aIndex)
{
  if (mMin == -1)
    mMin = mMax = aIndex;
  else if (aIndex < mMin)
    mMin = aIndex;
  else if (aIndex > mMax)
    mMax = aIndex;
}

NS_IMETHODIMP
nsTreeImageListener::ClearFrame()
{
  mTreeFrame = nsnull;
  return NS_OK;
}
