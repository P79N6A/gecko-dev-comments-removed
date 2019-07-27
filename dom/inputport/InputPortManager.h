





#ifndef mozilla_dom_InputPortManager_h
#define mozilla_dom_InputPortManager_h

#include "nsIInputPortService.h"
#include "nsISupports.h"
#include "nsWrapperCache.h"

class nsIInputPortService;
class nsPIDOMWindow;

namespace mozilla {
namespace dom {

class Promise;
class InputPort;

class InputPortManager final : public nsIInputPortServiceCallback,
                               public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(InputPortManager)
  NS_DECL_NSIINPUTPORTSERVICECALLBACK

  static already_AddRefed<InputPortManager> Create(nsPIDOMWindow* aWindow, ErrorResult& aRv);

  
  nsPIDOMWindow* GetParentObject() const;

  virtual JSObject* WrapObject(JSContext *aCx, JS::Handle<JSObject*> aGivenProto) override;

  
  already_AddRefed<Promise> GetInputPorts(ErrorResult& aRv);

private:
  explicit InputPortManager(nsPIDOMWindow* aWindow);

  ~InputPortManager();

  void Init(ErrorResult& aRv);

  void RejectPendingGetInputPortsPromises(nsresult aRv);

  nsresult SetInputPorts(const nsTArray<nsRefPtr<InputPort>>& aPorts);

  nsTArray<nsRefPtr<Promise>> mPendingGetInputPortsPromises;
  nsTArray<nsRefPtr<InputPort>> mInputPorts;
  nsCOMPtr<nsPIDOMWindow> mParent;
  nsCOMPtr<nsIInputPortService> mInputPortService;
  bool mIsReady;
};

} 
} 

#endif 
