





#ifndef mozilla_dom_TVSource_h__
#define mozilla_dom_TVSource_h__

#include "mozilla/DOMEventTargetHelper.h"

#include "mozilla/dom/TVSourceBinding.h"

namespace mozilla {
namespace dom {

class Promise;
class TVChannel;
class TVTuner;

class TVSource MOZ_FINAL : public DOMEventTargetHelper
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(TVSource, DOMEventTargetHelper)

  explicit TVSource(nsPIDOMWindow* aWindow);

  

  virtual JSObject* WrapObject(JSContext *aCx) MOZ_OVERRIDE;

  

  already_AddRefed<Promise> GetChannels(ErrorResult& aRv);

  already_AddRefed<Promise> SetCurrentChannel(const nsAString& aChannelNumber,
                                              ErrorResult& aRv);

  already_AddRefed<Promise> StartScanning(const TVStartScanningOptions& aOptions,
                                          ErrorResult& aRv);

  already_AddRefed<Promise> StopScanning(ErrorResult& aRv);

  already_AddRefed<TVTuner> Tuner() const;

  TVSourceType Type() const;

  bool IsScanning() const;

  already_AddRefed<TVChannel> GetCurrentChannel() const;

  IMPL_EVENT_HANDLER(currentchannelchanged);
  IMPL_EVENT_HANDLER(eitbroadcasted);
  IMPL_EVENT_HANDLER(scanningstatechanged);

private:
  ~TVSource();

};

} 
} 

#endif 
