





#ifndef mozilla_dom_MediaKeySession_h
#define mozilla_dom_MediaKeySession_h

#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "nsCOMPtr.h"
#include "mozilla/dom/TypedArray.h"
#include "mozilla/Mutex.h"
#include "mozilla/dom/Date.h"
#include "mozilla/dom/Promise.h"
#include "mozilla/dom/MediaKeySessionBinding.h"
#include "mozilla/dom/MediaKeysBinding.h"
#include "mozilla/dom/UnionTypes.h"

struct JSContext;

namespace mozilla {

class CDMProxy;

namespace dom {

class MediaKeyError;

class MediaKeySession MOZ_FINAL : public DOMEventTargetHelper
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(MediaKeySession,
                                           DOMEventTargetHelper)
public:
  MediaKeySession(nsPIDOMWindow* aParent,
                  MediaKeys* aKeys,
                  const nsAString& aKeySystem,
                  SessionType aSessionType,
                  ErrorResult& aRv);

  void Init(const nsAString& aSessionId);

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  
  MediaKeyError* GetError() const;

  void GetKeySystem(nsString& aRetval) const;

  void GetSessionId(nsString& aRetval) const;

  const nsString& GetSessionId() const;

  
  
  
  double Expiration() const;

  Promise* Closed() const;

  already_AddRefed<Promise> GenerateRequest(const nsAString& aInitDataType,
                                            const ArrayBufferViewOrArrayBuffer& aInitData,
                                            ErrorResult& aRv);

  already_AddRefed<Promise> Load(const nsAString& aSessionId,
                                 ErrorResult& aRv);

  already_AddRefed<Promise> Update(const ArrayBufferViewOrArrayBuffer& response,
                                   ErrorResult& aRv);

  already_AddRefed<Promise> Close(ErrorResult& aRv);

  already_AddRefed<Promise> Remove(ErrorResult& aRv);

  already_AddRefed<Promise> GetUsableKeyIds(ErrorResult& aRv);

  void DispatchKeyMessage(const nsTArray<uint8_t>& aMessage,
                          const nsAString& aURL);

  void DispatchKeyError(uint32_t system_code);

  void OnClosed();

  bool IsClosed() const;

private:
  ~MediaKeySession();

  nsRefPtr<Promise> mClosed;

  nsRefPtr<MediaKeyError> mMediaKeyError;
  nsRefPtr<MediaKeys> mKeys;
  const nsString mKeySystem;
  nsString mSessionId;
  const SessionType mSessionType;
  bool mIsClosed;
  bool mUninitialized;
};

} 
} 

#endif
