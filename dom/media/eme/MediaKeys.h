





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
#include "mozIGeckoMediaPluginService.h"

namespace mozilla {

class CDMProxy;

namespace dom {

class ArrayBufferViewOrArrayBuffer;
class MediaKeySession;
class HTMLMediaElement;

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

  already_AddRefed<Promise> Init(ErrorResult& aRv);

  nsPIDOMWindow* GetParentObject() const;

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  nsresult Bind(HTMLMediaElement* aElement);

  
  void GetKeySystem(nsString& retval) const;

  
  already_AddRefed<MediaKeySession> CreateSession(JSContext* aCx,
                                                  SessionType aSessionType,
                                                  ErrorResult& aRv);

  
  already_AddRefed<Promise> SetServerCertificate(const ArrayBufferViewOrArrayBuffer& aServerCertificate,
                                                 ErrorResult& aRv);

  already_AddRefed<MediaKeySession> GetSession(const nsAString& aSessionId);

  
  
  already_AddRefed<MediaKeySession> GetPendingSession(uint32_t aToken);

  
  void OnCDMCreated(PromiseId aId, const nsACString& aNodeId);

  
  
  
  void OnSessionIdReady(MediaKeySession* aSession);

  
  void OnSessionLoaded(PromiseId aId, bool aSuccess);

  
  void OnSessionClosed(MediaKeySession* aSession);

  CDMProxy* GetCDMProxy() { return mProxy; }

  
  already_AddRefed<Promise> MakePromise(ErrorResult& aRv);
  
  
  
  PromiseId StorePromise(Promise* aPromise);

  
  void RejectPromise(PromiseId aId, nsresult aExceptionCode);
  
  void ResolvePromise(PromiseId aId);

  const nsCString& GetNodeId() const;

  void Shutdown();

  
  
  void Terminated();

  
  bool IsBoundToMediaElement() const;

private:

  bool IsInPrivateBrowsing();

  
  already_AddRefed<Promise> RetrievePromise(PromiseId aId);

  
  
  nsRefPtr<CDMProxy> mProxy;

  nsRefPtr<HTMLMediaElement> mElement;

  nsCOMPtr<nsPIDOMWindow> mParent;
  nsString mKeySystem;
  nsCString mNodeId;
  KeySessionHashMap mKeySessions;
  PromiseHashMap mPromises;
  PendingKeySessionsHashMap mPendingSessions;
  PromiseId mCreatePromiseId;

  nsRefPtr<nsIPrincipal> mPrincipal;
  nsRefPtr<nsIPrincipal> mTopLevelPrincipal;

};

} 
} 

#endif 
