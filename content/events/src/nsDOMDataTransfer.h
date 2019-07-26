




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

  friend class nsDOMDragEvent;
  friend class nsEventStateManager;
  friend class nsContentUtils;

protected:

  

  
  
  nsDOMDataTransfer();

  
  
  
  
  nsDOMDataTransfer(PRUint32 aEventType);

  
  
  nsDOMDataTransfer(PRUint32 aEventType,
                    const PRUint32 aEffectAllowed,
                    bool aCursorState,
                    bool aIsExternal,
                    bool aUserCancelled,
                    bool aIsCrossDomainSubFrameDrop,
                    nsTArray<nsTArray<TransferItem> >& aItems,
                    nsIDOMElement* aDragImage,
                    PRUint32 aDragImageX,
                    PRUint32 aDragImageY);

  ~nsDOMDataTransfer()
  {
    if (mFiles) {
      mFiles->Disconnect();
    }
  }

  static const char sEffects[8][9];

public:

  void GetDragTarget(nsIDOMElement** aDragTarget)
  {
    *aDragTarget = mDragTarget;
    NS_IF_ADDREF(*aDragTarget);
  }

  
  
  void SetReadOnly() { mReadOnly = true; }

  
  
  void GetTransferables(nsISupportsArray** transferables,
                        nsIDOMNode* aDragTarget);

  
  
  bool ConvertFromVariant(nsIVariant* aVariant,
                            nsISupports** aSupports,
                            PRUint32* aLength);

  
  void ClearAll();

  
  
  nsresult SetDataWithPrincipal(const nsAString& aFormat,
                                nsIVariant* aData,
                                PRUint32 aIndex,
                                nsIPrincipal* aPrincipal);

protected:

  
  nsIDOMElement* GetDragImage(PRInt32* aX, PRInt32* aY)
  {
    *aX = mDragImageX;
    *aY = mDragImageY;
    return mDragImage;
  }

  
  nsIPrincipal* GetCurrentPrincipal(nsresult* rv);

  
  
  void GetRealFormat(const nsAString& aInFormat, nsAString& aOutFormat);

  
  
  void CacheExternalFormats();

  
  
  void FillInExternalDragData(TransferItem& aItem, PRUint32 aIndex);

  
  
  PRUint32 mEventType;

  
  PRUint32 mDropEffect;
  PRUint32 mEffectAllowed;

  
  bool mCursorState;

  
  
  bool mReadOnly;

  
  
  bool mIsExternal;

  
  bool mUserCancelled;

  
  
  bool mIsCrossDomainSubFrameDrop;

  
  nsTArray<nsTArray<TransferItem> > mItems;

  
  nsRefPtr<nsDOMFileList> mFiles;

  
  nsCOMPtr<nsIDOMElement> mDragTarget;

  
  
  nsCOMPtr<nsIDOMElement> mDragImage;
  PRUint32 mDragImageX;
  PRUint32 mDragImageY;
};

#endif 

