





#include "nsWrapperCache.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/Attributes.h"

class JSContext;

namespace mozilla {
namespace dom {

class Skeleton MOZ_FINAL : public nsISupports,
                           public nsWrapperCache
{
public:
  Skeleton();
  ~Skeleton();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(Skeleton)

  void* GetParentObject() const
  {
    
    return somethingSensible;
  }

  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope,
                               bool* aTriedToWrap);
};

}
}

