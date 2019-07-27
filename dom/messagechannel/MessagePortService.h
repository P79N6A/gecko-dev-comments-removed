



#ifndef mozilla_dom_MessagePortService_h
#define mozilla_dom_MessagePortService_h

#include "nsClassHashtable.h"
#include "nsHashKeys.h"
#include "nsISupportsImpl.h"

namespace mozilla {
namespace dom {

class MessagePortParent;
class SharedMessagePortMessage;

class MessagePortService final
{
public:
  NS_INLINE_DECL_REFCOUNTING(MessagePortService)

  static MessagePortService* GetOrCreate();

  bool RequestEntangling(MessagePortParent* aParent,
                         const nsID& aDestinationUUID,
                         const uint32_t& aSequenceID);

  bool DisentanglePort(
                 MessagePortParent* aParent,
                 FallibleTArray<nsRefPtr<SharedMessagePortMessage>>& aMessages);

  bool ClosePort(MessagePortParent* aParent);

  bool PostMessages(
                 MessagePortParent* aParent,
                 FallibleTArray<nsRefPtr<SharedMessagePortMessage>>& aMessages);

  void ParentDestroy(MessagePortParent* aParent);

private:
  ~MessagePortService() {}

  void CloseAll(const nsID& aUUID);
  void MaybeShutdown();

  class MessagePortServiceData;

#ifdef DEBUG
  static PLDHashOperator
  CloseAllDebugCheck(const nsID& aID, MessagePortServiceData* aData,
                     void* aPtr);
#endif

  nsClassHashtable<nsIDHashKey, MessagePortServiceData> mPorts;
};

} 
} 

#endif 
