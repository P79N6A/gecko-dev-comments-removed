





#ifndef mozilla_dom_MediaKeyStatuses_h
#define mozilla_dom_MediaKeyStatuses_h

#include "mozilla/ErrorResult.h"
#include "mozilla/Attributes.h"
#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"

#include "mozilla/dom/TypedArray.h"
#include "mozilla/dom/MediaKeyStatusMapBinding.h"
#include "mozilla/CDMCaps.h"

class nsPIDOMWindow;

namespace mozilla {
namespace dom {

class ArrayBufferViewOrArrayBuffer;

class MediaKeyStatusMap MOZ_FINAL : public nsISupports,
                                    public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(MediaKeyStatusMap)

public:
  explicit MediaKeyStatusMap(JSContext* aCx,
                             nsPIDOMWindow* aParent,
                             ErrorResult& aRv);

protected:
  ~MediaKeyStatusMap();

public:
  nsPIDOMWindow* GetParentObject() const;

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  MediaKeyStatus Get(JSContext* aCx,
                     const ArrayBufferViewOrArrayBuffer& aKey,
                     ErrorResult& aRv) const;

  bool Has(JSContext* aCx,
           const ArrayBufferViewOrArrayBuffer& aKey,
           ErrorResult& aRv) const;

  void Keys(JSContext* aCx,
            JS::MutableHandle<JSObject*> aResult,
            ErrorResult& aRv) const;

  void Values(JSContext* aCx,
              JS::MutableHandle<JSObject*> aResult,
              ErrorResult& aRv) const;

  void Entries(JSContext* aCx,
               JS::MutableHandle<JSObject*> aResult,
               ErrorResult& aRv) const;

  uint32_t GetSize(JSContext* aCx, ErrorResult& aRv) const;

  void Update(const nsTArray<CDMCaps::KeyStatus>& keys);

private:
  nsresult UpdateInternal(const nsTArray<CDMCaps::KeyStatus>& keys);

  nsCOMPtr<nsPIDOMWindow> mParent;
  JS::Heap<JSObject*> mMap;
  nsresult mUpdateError;
};

} 
} 

#endif
