





#ifndef mozilla_dom_InputPort_h
#define mozilla_dom_InputPort_h

#include "InputPortListeners.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/dom/InputPortBinding.h"
#include "nsIInputPortService.h"

namespace mozilla {

class DOMMediaStream;

namespace dom {

class InputPort : public DOMEventTargetHelper
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(InputPort, DOMEventTargetHelper)

  
  virtual JSObject* WrapObject(JSContext *aCx, JS::Handle<JSObject*> aGivenProto) override;

  void NotifyConnectionChanged(bool aIsConnected);

  
  void GetId(nsAString& aId) const;

  DOMMediaStream* Stream() const;

  bool Connected() const;

  IMPL_EVENT_HANDLER(connect);
  IMPL_EVENT_HANDLER(disconnect);

protected:
  explicit InputPort(nsPIDOMWindow* aWindow);

  virtual ~InputPort();

  void Init(nsIInputPortData* aData, nsIInputPortListener* aListener, ErrorResult& aRv);
  void Shutdown();

  nsString mId;
  nsRefPtr<DOMMediaStream> mStream;
  nsRefPtr<InputPortListener> mInputPortListener;
  bool mIsConnected;
};

} 
} 

#endif 
