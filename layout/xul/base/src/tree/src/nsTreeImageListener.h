






































#ifndef nsTreeImageListener_h__
#define nsTreeImageListener_h__

#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsITreeColumns.h"
#include "nsStubImageDecoderObserver.h"
#include "nsTreeBodyFrame.h"
#include "nsITreeImageListener.h"


class nsTreeImageListener : public nsStubImageDecoderObserver, public nsITreeImageListener
{
public:
  nsTreeImageListener(nsTreeBodyFrame *aTreeFrame);
  ~nsTreeImageListener();

  NS_DECL_ISUPPORTS
  
  NS_IMETHOD OnStartDecode(imgIRequest *aRequest);
  NS_IMETHOD OnStopDecode(imgIRequest *aRequest,
                          nsresult aStatus, const PRUnichar *aStatusArg);
  NS_IMETHOD OnStartContainer(imgIRequest *aRequest, imgIContainer *aImage);
  NS_IMETHOD OnDataAvailable(imgIRequest *aRequest, bool aCurrentFrame,
                             const nsIntRect *aRect);
  
  NS_IMETHOD FrameChanged(imgIContainer *aContainer,
                          const nsIntRect *aDirtyRect);

  NS_IMETHOD AddCell(PRInt32 aIndex, nsITreeColumn* aCol);
  NS_IMETHOD ClearFrame();

  friend class nsTreeBodyFrame;

protected:
  void UnsuppressInvalidation() { mInvalidationSuppressed = false; }
  void Invalidate();

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
