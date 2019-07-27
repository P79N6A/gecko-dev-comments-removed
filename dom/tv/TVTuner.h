





#ifndef mozilla_dom_TVTuner_h
#define mozilla_dom_TVTuner_h

#include "mozilla/DOMEventTargetHelper.h"

#include "mozilla/dom/TVTunerBinding.h"

class nsITVService;
class nsITVTunerData;

namespace mozilla {

class DOMMediaStream;

namespace dom {

class Promise;
class TVSource;

class TVTuner final : public DOMEventTargetHelper
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(TVTuner, DOMEventTargetHelper)

  static already_AddRefed<TVTuner> Create(nsPIDOMWindow* aWindow,
                                          nsITVTunerData* aData);
  nsresult NotifyImageSizeChanged(uint32_t aWidth, uint32_t aHeight);

  

  virtual JSObject* WrapObject(JSContext *aCx, JS::Handle<JSObject*> aGivenProto) override;

  nsresult SetCurrentSource(TVSourceType aSourceType);

  nsresult DispatchTVEvent(nsIDOMEvent* aEvent);

  

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
  explicit TVTuner(nsPIDOMWindow* aWindow);

  ~TVTuner();

  bool Init(nsITVTunerData* aData);

  nsresult InitMediaStream();

  nsresult DispatchCurrentSourceChangedEvent(TVSource* aSource);

  nsCOMPtr<nsITVService> mTVService;
  nsRefPtr<DOMMediaStream> mStream;
  nsRefPtr<TVSource> mCurrentSource;
  nsTArray<nsRefPtr<TVSource>> mSources;
  nsString mId;
  nsTArray<TVSourceType> mSupportedSourceTypes;
};

} 
} 

#endif 
