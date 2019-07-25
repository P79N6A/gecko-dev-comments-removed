





#include "nsWrapperCache.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/Attributes.h"
#include "nsCOMPtr.h"

class JSContext;
class nsIDOMWindow;

namespace mozilla {

class ErrorResult;

class AudioContext MOZ_FINAL : public nsISupports,
                               public nsWrapperCache
{
  explicit AudioContext(nsIDOMWindow* aParentWindow);

public:
  virtual ~AudioContext();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(AudioContext)

  nsIDOMWindow* GetParentObject() const
  {
    return mWindow;
  }

  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope,
                               bool* aTriedToWrap);

  static already_AddRefed<AudioContext>
  Constructor(nsISupports* aGlobal, ErrorResult& aRv);

private:
  nsCOMPtr<nsIDOMWindow> mWindow;
};

}

