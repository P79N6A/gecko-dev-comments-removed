



#ifndef mozilla_dom_SpeakerManager_h
#define mozilla_dom_SpeakerManager_h

#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/dom/MozSpeakerManagerBinding.h"

namespace mozilla {
namespace dom {





class SpeakerManager MOZ_FINAL
  : public DOMEventTargetHelper
  , public nsIDOMEventListener
{
  friend class SpeakerManagerService;
  friend class SpeakerManagerServiceChild;

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMEVENTLISTENER

public:
  void Init(nsPIDOMWindow* aWindow);

  nsPIDOMWindow* GetParentObject() const;

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;
  


  
  bool Forcespeaker();
  
  
  
  void SetForcespeaker(bool aEnable);
  
  bool Speakerforced();

  void SetAudioChannelActive(bool aIsActive);
  IMPL_EVENT_HANDLER(speakerforcedchange)

  static already_AddRefed<SpeakerManager>
  Constructor(const GlobalObject& aGlobal, ErrorResult& aRv);

protected:
  SpeakerManager();
  ~SpeakerManager();
  void DispatchSimpleEvent(const nsAString& aStr);
  
  bool mForcespeaker;
  bool mVisible;
};

} 
} 

#endif 
