






































#ifndef nsTreeImageListener_h__
#define nsTreeImageListener_h__

#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsITreeColumns.h"
#include "nsStubImageDecoderObserver.h"

class nsITreeBoxObject;



#define NS_ITREEIMAGELISTENER_IID \
{ 0x90586540, 0x2d50, 0x403e, { 0x8d, 0xce, 0x98, 0x1c, 0xaa, 0x77, 0x84, 0x44 } }

class nsITreeImageListener : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ITREEIMAGELISTENER_IID)

  NS_IMETHOD AddCell(PRInt32 aIndex, nsITreeColumn* aCol) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsITreeImageListener, NS_ITREEIMAGELISTENER_IID)


class nsTreeImageListener : public nsStubImageDecoderObserver, public nsITreeImageListener
{
public:
  nsTreeImageListener(nsITreeBoxObject* aTree);
  ~nsTreeImageListener();

  NS_DECL_ISUPPORTS
  
  NS_IMETHOD OnStartContainer(imgIRequest *aRequest, imgIContainer *aImage);
  NS_IMETHOD OnDataAvailable(imgIRequest *aRequest, gfxIImageFrame *aFrame,
                             const nsRect *aRect);
  
  NS_IMETHOD FrameChanged(imgIContainer *aContainer, gfxIImageFrame *newframe,
                          nsRect * dirtyRect);

  NS_IMETHOD AddCell(PRInt32 aIndex, nsITreeColumn* aCol);
 
  friend class nsTreeBodyFrame;

protected:
  void UnsuppressInvalidation() { mInvalidationSuppressed = PR_FALSE; };
  void Invalidate();

private:
  nsITreeBoxObject* mTree;

  
  PRBool mInvalidationSuppressed;

  class InvalidationArea {
    public:
      InvalidationArea(nsITreeColumn* aCol);
      ~InvalidationArea() { delete mNext; };

      friend class nsTreeImageListener;

    protected:
      void AddRow(PRInt32 aIndex);
      nsITreeColumn* GetCol() { return mCol.get(); };
      PRInt32 GetMin() { return mMin; };
      PRInt32 GetMax() { return mMax; };
      InvalidationArea* GetNext() { return mNext; };
      void SetNext(InvalidationArea* aNext) { mNext = aNext; };

    private:
      nsCOMPtr<nsITreeColumn> mCol;
      PRInt32                 mMin;
      PRInt32                 mMax;
      InvalidationArea*       mNext;
  };

  InvalidationArea* mInvalidationArea;
};

#endif 
