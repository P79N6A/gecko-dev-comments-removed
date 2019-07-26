




#ifndef nsDOMDataTransfer_h__
#define nsDOMDataTransfer_h__

#include "nsString.h"
#include "nsTArray.h"
#include "nsIVariant.h"
#include "nsIPrincipal.h"
#include "nsIDOMDataTransfer.h"
#include "nsIDragService.h"
#include "nsIDOMElement.h"
#include "nsCycleCollectionParticipant.h"

#include "nsAutoPtr.h"
#include "nsIFile.h"
#include "nsDOMFile.h"
#include "mozilla/Attributes.h"

class nsITransferable;








struct TransferItem {
  nsString mFormat;
  nsCOMPtr<nsIPrincipal> mPrincipal;
  nsCOMPtr<nsIVariant> mData;
};

class nsDOMDataTransfer MOZ_FINAL : public nsIDOMDataTransfer
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSIDOMDATATRANSFER

  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsDOMDataTransfer, nsIDOMDataTransfer)

  friend class nsEventStateManager;

protected:

  
  nsDOMDataTransfer();

  
  
  nsDOMDataTransfer(uint32_t aEventType,
                    const uint32_t aEffectAllowed,
                    bool aCursorState,
                    bool aIsExternal,
                    bool aUserCancelled,
                    bool aIsCrossDomainSubFrameDrop,
                    nsTArray<nsTArray<TransferItem> >& aItems,
                    nsIDOMElement* aDragImage,
                    uint32_t aDragImageX,
                    uint32_t aDragImageY);

  ~nsDOMDataTransfer()
  {
    if (mFiles) {
      mFiles->Disconnect();
    }
  }

  static const char sEffects[8][9];

public:

  
  
  
  
  
  
  
  
  
  nsDOMDataTransfer(uint32_t aEventType, bool aIsExternal);

  void GetDragTarget(nsIDOMElement** aDragTarget)
  {
    *aDragTarget = mDragTarget;
    NS_IF_ADDREF(*aDragTarget);
  }

  
  
  void SetReadOnly() { mReadOnly = true; }

  
  
  already_AddRefed<nsISupportsArray> GetTransferables(nsIDOMNode* aDragTarget);

  
  already_AddRefed<nsITransferable> GetTransferable(uint32_t aIndex,
                                                    nsILoadContext* aLoadContext);

  
  
  bool ConvertFromVariant(nsIVariant* aVariant,
                            nsISupports** aSupports,
                            uint32_t* aLength);

  
  void ClearAll();

  
  
  
  nsresult SetDataWithPrincipal(const nsAString& aFormat,
                                nsIVariant* aData,
                                uint32_t aIndex,
                                nsIPrincipal* aPrincipal);

protected:

  
  nsIDOMElement* GetDragImage(int32_t* aX, int32_t* aY)
  {
    *aX = mDragImageX;
    *aY = mDragImageY;
    return mDragImage;
  }

  
  nsIPrincipal* GetCurrentPrincipal(nsresult* rv);

  
  
  void GetRealFormat(const nsAString& aInFormat, nsAString& aOutFormat);

  
  
  void CacheExternalData(const char* aFormat, uint32_t aIndex, nsIPrincipal* aPrincipal);

  
  
  void CacheExternalDragFormats();

  
  void CacheExternalClipboardFormats();

  
  
  void FillInExternalData(TransferItem& aItem, uint32_t aIndex);

  
  
  uint32_t mEventType;

  
  uint32_t mDropEffect;
  uint32_t mEffectAllowed;

  
  bool mCursorState;

  
  
  bool mReadOnly;

  
  
  bool mIsExternal;

  
  bool mUserCancelled;

  
  
  bool mIsCrossDomainSubFrameDrop;

  
  nsTArray<nsTArray<TransferItem> > mItems;

  
  nsRefPtr<nsDOMFileList> mFiles;

  
  nsCOMPtr<nsIDOMElement> mDragTarget;

  
  
  nsCOMPtr<nsIDOMElement> mDragImage;
  uint32_t mDragImageX;
  uint32_t mDragImageY;
};

#endif 

