




#ifndef nsTreeImageListener_h__
#define nsTreeImageListener_h__

#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsITreeColumns.h"
#include "nsTreeBodyFrame.h"
#include "mozilla/Attributes.h"


class nsTreeImageListener final : public imgINotificationObserver
{
public:
  explicit nsTreeImageListener(nsTreeBodyFrame *aTreeFrame);

  NS_DECL_ISUPPORTS
  NS_DECL_IMGINOTIFICATIONOBSERVER

  NS_IMETHOD ClearFrame();

  friend class nsTreeBodyFrame;

protected:
  ~nsTreeImageListener();

  void UnsuppressInvalidation() { mInvalidationSuppressed = false; }
  void Invalidate();
  void AddCell(int32_t aIndex, nsITreeColumn* aCol);

private:
  nsTreeBodyFrame* mTreeFrame;

  
  bool mInvalidationSuppressed;

  class InvalidationArea {
    public:
      explicit InvalidationArea(nsITreeColumn* aCol);
      ~InvalidationArea() { delete mNext; }

      friend class nsTreeImageListener;

    protected:
      void AddRow(int32_t aIndex);
      nsITreeColumn* GetCol() { return mCol.get(); }
      int32_t GetMin() { return mMin; }
      int32_t GetMax() { return mMax; }
      InvalidationArea* GetNext() { return mNext; }
      void SetNext(InvalidationArea* aNext) { mNext = aNext; }

    private:
      nsCOMPtr<nsITreeColumn> mCol;
      int32_t                 mMin;
      int32_t                 mMax;
      InvalidationArea*       mNext;
  };

  InvalidationArea* mInvalidationArea;
};

#endif 
