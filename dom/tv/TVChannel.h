





#ifndef mozilla_dom_TVChannel_h__
#define mozilla_dom_TVChannel_h__

#include "mozilla/DOMEventTargetHelper.h"

#include "mozilla/dom/TVChannelBinding.h"

namespace mozilla {
namespace dom {

class Promise;
class TVProgram;
class TVSource;

class TVChannel MOZ_FINAL : public DOMEventTargetHelper
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(TVChannel, DOMEventTargetHelper)

  explicit TVChannel(nsPIDOMWindow* aWindow);

  

  virtual JSObject* WrapObject(JSContext *aCx) MOZ_OVERRIDE;

  

  already_AddRefed<Promise> GetPrograms(const TVGetProgramsOptions& aOptions,
                                        ErrorResult& aRv);

  already_AddRefed<Promise> GetCurrentProgram(ErrorResult& aRv);

  void GetNetworkId(nsAString& aNetworkId) const;

  void GetTransportStreamId(nsAString& aTransportStreamId) const;

  void GetServiceId(nsAString& aServiceId) const;

  already_AddRefed<TVSource> Source() const;

  TVChannelType Type() const;

  void GetName(nsAString& aName) const;

  void GetNumber(nsAString& aNumber) const;

  bool IsEmergency() const;

  bool IsFree() const;

private:
  ~TVChannel();

};

} 
} 

#endif 
