





#ifndef mozilla_dom_TVTuner_h__
#define mozilla_dom_TVTuner_h__

#include "mozilla/DOMEventTargetHelper.h"

#include "mozilla/dom/TVTunerBinding.h"

namespace mozilla {

class DOMMediaStream;

namespace dom {

class Promise;
class TVSource;

class TVTuner MOZ_FINAL : public DOMEventTargetHelper
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(TVTuner, DOMEventTargetHelper)

  explicit TVTuner(nsPIDOMWindow* aWindow);

  

  virtual JSObject* WrapObject(JSContext *aCx) MOZ_OVERRIDE;

  

  void GetSupportedSourceTypes(nsTArray<TVSourceType>& aSourceTypes,
                               ErrorResult& aRv) const;

  already_AddRefed<Promise> GetSources(ErrorResult& aRv);

  already_AddRefed<Promise> SetCurrentSource(const TVSourceType aSourceType,
                                             ErrorResult& aRv);

  void GetId(nsAString& aId) const;

  already_AddRefed<TVSource> GetCurrentSource() const;

  already_AddRefed<DOMMediaStream> GetStream() const;

  IMPL_EVENT_HANDLER(currentsourcechanged);

private:
  ~TVTuner();

};

} 
} 

#endif 
