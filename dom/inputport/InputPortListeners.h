





#ifndef mozilla_dom_InputPortListeners_h
#define mozilla_dom_InputPortListeners_h

#include "nsCycleCollectionParticipant.h"
#include "nsIInputPortService.h"
#include "nsTArray.h"

namespace mozilla {
namespace dom {

class InputPort;

class InputPortListener final : public nsIInputPortListener
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(InputPortListener)
  NS_DECL_NSIINPUTPORTLISTENER

  void RegisterInputPort(InputPort* aPort);

  void UnregisterInputPort(InputPort* aPort);

private:
  ~InputPortListener() {}

  nsTArray<nsRefPtr<InputPort>> mInputPorts;
};

} 
} 

#endif 
