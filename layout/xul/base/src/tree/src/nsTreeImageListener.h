






































#ifndef nsTreeImageListener_h__
#define nsTreeImageListener_h__

#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsITreeColumns.h"
#include "nsStubImageDecoderObserver.h"
#include "nsTreeBodyFrame.h"


class nsTreeImageListener : public nsStubImageDecoderObserver
{
public:
  nsTreeImageListener(nsTreeBodyFrame *aTreeFrame);
  ~nsTreeImageListener();

  NS_DECL_ISUPPORTS
  
  NS_IMETHOD OnStartContainer(imgIRequest *aRequest, imgIContainer *aImage);
  NS_IMETHOD OnImageIsAnimated(imgIRequest* aRequest);
  NS_IMETHOD OnDataAvailable(imgIRequest *aRequest, bool aCurrentFrame,
                             const nsIntRect *aRect);
  
  NS_IMETHOD FrameChanged(imgIRequest *aRequest,
                          imgIContainer *aContainer,
                          const nsIntRect *aDirtyRect);

  NS_IMETHOD ClearFrame();

  friend class nsTreeBodyFrame;

protected:
  void UnsuppressInvalidation() { mInvalidationSuppressed = false; }
  void Invalidate();
  void AddCell(PRInt32 aIndex, nsITreeColumn* aCol);

private:
  nsTreeBodyFrame* mTreeFrame;

  
  bool mInvalidationSuppressed;

  class InvalidationArea {
    public:
      InvalidationArea(nsITreeColumn* aCol);
      ~InvalidationArea() { delete mNext; }

      friend class nsTreeImageListener;

    protected:
      void AddRow(PRInt32 aIndex);
      nsITreeColumn* GetCol() { return mCol.get(); }
      PRInt32 GetMin() { return mMin; }
      PRInt32 GetMax() { return mMax; }
      InvalidationArea* GetNext() { return mNext; }
      void SetNext(InvalidationArea* aNext) { mNext = aNext; }

    private:
      nsCOMPtr<nsITreeColumn> mCol;
      PRInt32                 mMin;
      PRInt32                 mMax;
      InvalidationArea*       mNext;
  };

  InvalidationArea* mInvalidationArea;
};

#endif 
