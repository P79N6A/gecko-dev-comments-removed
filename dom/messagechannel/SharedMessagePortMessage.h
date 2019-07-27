




#ifndef mozilla_dom_SharedMessagePortMessage_h
#define mozilla_dom_SharedMessagePortMessage_h

#include "MessagePortUtils.h"

namespace mozilla {
namespace dom {

class MessagePortChild;
class MessagePortMessage;
class MessagePortParent;

class SharedMessagePortMessage final
{
public:
  NS_INLINE_DECL_REFCOUNTING(SharedMessagePortMessage)

  nsTArray<uint8_t> mData;
  messageport::StructuredCloneClosure mClosure;

  SharedMessagePortMessage()
  {}

  static void
  FromSharedToMessagesChild(
                      MessagePortChild* aActor,
                      const nsTArray<nsRefPtr<SharedMessagePortMessage>>& aData,
                      nsTArray<MessagePortMessage>& aArray);

  static bool
  FromMessagesToSharedChild(
                     nsTArray<MessagePortMessage>& aArray,
                     FallibleTArray<nsRefPtr<SharedMessagePortMessage>>& aData);

  static bool
  FromSharedToMessagesParent(
                      MessagePortParent* aActor,
                      const nsTArray<nsRefPtr<SharedMessagePortMessage>>& aData,
                      FallibleTArray<MessagePortMessage>& aArray);

  static bool
  FromMessagesToSharedParent(
                     nsTArray<MessagePortMessage>& aArray,
                     FallibleTArray<nsRefPtr<SharedMessagePortMessage>>& aData);

private:
  ~SharedMessagePortMessage();
};

} 
} 

#endif 
