




#ifndef mozilla_dom_DataTransfer_h
#define mozilla_dom_DataTransfer_h

#include "nsString.h"
#include "nsTArray.h"
#include "nsIVariant.h"
#include "nsIPrincipal.h"
#include "nsIDOMDataTransfer.h"
#include "nsIDOMElement.h"
#include "nsIDragService.h"
#include "nsCycleCollectionParticipant.h"

#include "nsAutoPtr.h"
#include "mozilla/Attributes.h"
#include "mozilla/dom/File.h"

class nsINode;
class nsITransferable;
class nsISupportsArray;
class nsILoadContext;

namespace mozilla {

class EventStateManager;

namespace dom {

class DOMStringList;
class Element;
template<typename T> class Optional;








struct TransferItem {
  nsString mFormat;
  nsCOMPtr<nsIPrincipal> mPrincipal;
  nsCOMPtr<nsIVariant> mData;
};

#define NS_DATATRANSFER_IID \
{ 0x43ee0327, 0xde5d, 0x463d, \
  { 0x9b, 0xd0, 0xf1, 0x79, 0x09, 0x69, 0xf2, 0xfb } }

class DataTransfer final : public nsIDOMDataTransfer,
                           public nsWrapperCache
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_DATATRANSFER_IID)

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSIDOMDATATRANSFER

  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(DataTransfer)

  friend class mozilla::EventStateManager;

protected:

  
  DataTransfer();

  
  
  DataTransfer(nsISupports* aParent,
               uint32_t aEventType,
               const uint32_t aEffectAllowed,
               bool aCursorState,
               bool aIsExternal,
               bool aUserCancelled,
               bool aIsCrossDomainSubFrameDrop,
               int32_t aClipboardType,
               nsTArray<nsTArray<TransferItem> >& aItems,
               Element* aDragImage,
               uint32_t aDragImageX,
               uint32_t aDragImageY);

  ~DataTransfer();

  static const char sEffects[8][9];

public:

  
  
  
  
  
  
  
  
  
  
  
  DataTransfer(nsISupports* aParent, uint32_t aEventType, bool aIsExternal,
               int32_t aClipboardType);

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;
  nsISupports* GetParentObject()
  {
    return mParent;
  }

  void SetParentObject(nsISupports* aNewParent)
  {
    MOZ_ASSERT(aNewParent);
    
    
    MOZ_ASSERT(!GetWrapperPreserveColor());
    mParent = aNewParent;
  }

  static already_AddRefed<DataTransfer>
  Constructor(const GlobalObject& aGlobal, const nsAString& aEventType,
              bool aIsExternal, ErrorResult& aRv);

  void GetDropEffect(nsString& aDropEffect)
  {
    aDropEffect.AssignASCII(sEffects[mDropEffect]);
  }
  void GetEffectAllowed(nsString& aEffectAllowed)
  {
    if (mEffectAllowed == nsIDragService::DRAGDROP_ACTION_UNINITIALIZED) {
      aEffectAllowed.AssignLiteral("uninitialized");
    } else {
      aEffectAllowed.AssignASCII(sEffects[mEffectAllowed]);
    }
  }
  void SetDragImage(Element& aElement, int32_t aX, int32_t aY,
                    ErrorResult& aRv);
  already_AddRefed<DOMStringList> Types();
  void GetData(const nsAString& aFormat, nsAString& aData, ErrorResult& aRv);
  void SetData(const nsAString& aFormat, const nsAString& aData,
               ErrorResult& aRv);
  void ClearData(const mozilla::dom::Optional<nsAString>& aFormat,
                 mozilla::ErrorResult& aRv);
  FileList* GetFiles(mozilla::ErrorResult& aRv);
  void AddElement(Element& aElement, mozilla::ErrorResult& aRv);
  uint32_t MozItemCount()
  {
    return mItems.Length();
  }
  void GetMozCursor(nsString& aCursor)
  {
    if (mCursorState) {
      aCursor.AssignLiteral("default");
    } else {
      aCursor.AssignLiteral("auto");
    }
  }
  already_AddRefed<DOMStringList> MozTypesAt(uint32_t aIndex,
                                             mozilla::ErrorResult& aRv);
  void MozClearDataAt(const nsAString& aFormat, uint32_t aIndex,
                      mozilla::ErrorResult& aRv);
  void MozSetDataAt(JSContext* aCx, const nsAString& aFormat,
                    JS::Handle<JS::Value> aData, uint32_t aIndex,
                    mozilla::ErrorResult& aRv);
  void MozGetDataAt(JSContext* aCx, const nsAString& aFormat,
                    uint32_t aIndex, JS::MutableHandle<JS::Value> aRetval,
                    mozilla::ErrorResult& aRv);
  bool MozUserCancelled()
  {
    return mUserCancelled;
  }
  already_AddRefed<nsINode> GetMozSourceNode();

  mozilla::dom::Element* GetDragTarget()
  {
    return mDragTarget;
  }

  
  
  void SetReadOnly() { mReadOnly = true; }

  
  
  already_AddRefed<nsISupportsArray> GetTransferables(nsIDOMNode* aDragTarget);
  already_AddRefed<nsISupportsArray> GetTransferables(nsILoadContext* aLoadContext);

  
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

  
  Element* GetDragImage(int32_t* aX, int32_t* aY)
  {
    *aX = mDragImageX;
    *aY = mDragImageY;
    return mDragImage;
  }

  nsresult Clone(nsISupports* aParent, uint32_t aEventType, bool aUserCancelled,
                 bool aIsCrossDomainSubFrameDrop, DataTransfer** aResult);

protected:

  
  
  void GetRealFormat(const nsAString& aInFormat, nsAString& aOutFormat);

  
  
  void CacheExternalData(const char* aFormat, uint32_t aIndex, nsIPrincipal* aPrincipal);

  
  
  void CacheExternalDragFormats();

  
  void CacheExternalClipboardFormats();

  
  
  void FillInExternalData(TransferItem& aItem, uint32_t aIndex);

  friend class ContentParent;
  void FillAllExternalData();

  void MozClearDataAtHelper(const nsAString& aFormat, uint32_t aIndex,
                            mozilla::ErrorResult& aRv);

  nsCOMPtr<nsISupports> mParent;

  
  
  uint32_t mEventType;

  
  uint32_t mDropEffect;
  uint32_t mEffectAllowed;

  
  bool mCursorState;

  
  
  bool mReadOnly;

  
  
  bool mIsExternal;

  
  bool mUserCancelled;

  
  
  bool mIsCrossDomainSubFrameDrop;

  
  
  int32_t mClipboardType;

  
  nsTArray<nsTArray<TransferItem> > mItems;

  
  nsRefPtr<FileList> mFiles;

  
  nsCOMPtr<mozilla::dom::Element> mDragTarget;

  
  
  nsCOMPtr<mozilla::dom::Element> mDragImage;
  uint32_t mDragImageX;
  uint32_t mDragImageY;
};

NS_DEFINE_STATIC_IID_ACCESSOR(DataTransfer, NS_DATATRANSFER_IID)

} 
} 

#endif 

