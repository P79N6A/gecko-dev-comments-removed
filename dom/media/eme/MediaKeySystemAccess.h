





#ifndef mozilla_dom_MediaKeySystemAccess_h
#define mozilla_dom_MediaKeySystemAccess_h

#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"

#include "mozilla/dom/Promise.h"
#include "mozilla/dom/MediaKeySystemAccessBinding.h"
#include "js/TypeDecls.h"

namespace mozilla {
namespace dom {

class MediaKeySystemAccess MOZ_FINAL : public nsISupports,
                                       public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(MediaKeySystemAccess)

public:
  explicit MediaKeySystemAccess(nsPIDOMWindow* aParent,
                                const nsAString& aKeySystem);

protected:
  ~MediaKeySystemAccess();

public:
  nsPIDOMWindow* GetParentObject() const;

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  void GetKeySystem(nsString& aRetVal) const;

  already_AddRefed<Promise> CreateMediaKeys(ErrorResult& aRv);

  static bool IsKeySystemSupported(const nsAString& aKeySystem);

  static bool IsSupported(const nsAString& aKeySystem,
                          const Sequence<MediaKeySystemOptions>& aOptions);

private:
  nsCOMPtr<nsPIDOMWindow> mParent;
  const nsString mKeySystem;
};

} 
} 

#endif 
