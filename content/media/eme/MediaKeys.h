





#ifndef mozilla_dom_mediakeys_h__
#define mozilla_dom_mediakeys_h__

#include "nsIDOMMediaError.h"
#include "nsWrapperCache.h"
#include "nsISupports.h"
#include "mozilla/Attributes.h"
#include "mozilla/RefPtr.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsRefPtrHashtable.h"
#include "mozilla/dom/Promise.h"
#include "mozilla/dom/MediaKeysBinding.h"
#include "mozilla/dom/UnionTypes.h"

namespace mozilla {

class CDMProxy;

namespace dom {

class MediaKeySession;

typedef nsRefPtrHashtable<nsStringHashKey, MediaKeySession> KeySessionHashMap;
typedef nsRefPtrHashtable<nsUint32HashKey, dom::Promise> PromiseHashMap;
typedef nsRefPtrHashtable<nsUint32HashKey, MediaKeySession> PendingKeySessionsHashMap;
typedef uint32_t PromiseId;



bool
CopyArrayBufferViewOrArrayBufferData(const ArrayBufferViewOrArrayBuffer& aBufferOrView,
                                     nsTArray<uint8_t>& aOutData);



class MediaKeys MOZ_FINAL : public nsISupports,
                            public nsWrapperCache
{
  ~MediaKeys();

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(MediaKeys)

  MediaKeys(nsPIDOMWindow* aParentWindow, const nsAString& aKeySystem);

  nsPIDOMWindow* GetParentObject() const;

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  
  void GetKeySystem(nsString& retval) const;

  
  already_AddRefed<MediaKeySession> CreateSession(SessionType aSessionType,
                                                  ErrorResult& aRv);

  
  already_AddRefed<Promise> SetServerCertificate(const ArrayBufferViewOrArrayBuffer& aServerCertificate,
                                                 ErrorResult& aRv);

  
  static
  already_AddRefed<Promise> Create(const GlobalObject& aGlobal,
                                   const nsAString& aKeySystem,
                                   ErrorResult& aRv);

  
  static IsTypeSupportedResult IsTypeSupported(const GlobalObject& aGlobal,
                                               const nsAString& aKeySystem,
                                               const Optional<nsAString>& aInitDataType,
                                               const Optional<nsAString>& aContentType,
                                               const Optional<nsAString>& aCapability);

  already_AddRefed<MediaKeySession> GetSession(const nsAString& aSessionId);

  
  void OnCDMCreated(PromiseId aId);
  
  
  void OnSessionPending(PromiseId aId, MediaKeySession* aSession);
  
  void OnSessionCreated(PromiseId aId, const nsAString& aSessionId);
  
  void OnSessionLoaded(PromiseId aId, bool aSuccess);
  
  void OnSessionClosed(MediaKeySession* aSession);

  CDMProxy* GetCDMProxy() { return mProxy; }

  
  already_AddRefed<Promise> MakePromise(ErrorResult& aRv);
  
  
  
  PromiseId StorePromise(Promise* aPromise);

  
  void RejectPromise(PromiseId aId, nsresult aExceptionCode);
  
  void ResolvePromise(PromiseId aId);

  nsresult GetOrigin(nsString& aOutOrigin);

  void Shutdown();

private:

  
  already_AddRefed<Promise> RetrievePromise(PromiseId aId);

  
  
  nsRefPtr<CDMProxy> mProxy;

  nsCOMPtr<nsPIDOMWindow> mParent;
  nsString mKeySystem;
  KeySessionHashMap mKeySessions;
  PromiseHashMap mPromises;
  PendingKeySessionsHashMap mPendingSessions;
  PromiseId mCreatePromiseId;
};

} 
} 

#endif 
