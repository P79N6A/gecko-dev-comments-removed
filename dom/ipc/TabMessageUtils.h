





































#ifndef TABMESSAGE_UTILS_H
#define TABMESSAGE_UTILS_H

#include "IPC/IPCMessageUtils.h"
#include "nsIPrivateDOMEvent.h"
#include "nsCOMPtr.h"

namespace mozilla {
namespace dom {
struct RemoteDOMEvent
{
  nsCOMPtr<nsIPrivateDOMEvent> mEvent;
};

bool ReadRemoteEvent(const IPC::Message* aMsg, void** aIter,
                     mozilla::dom::RemoteDOMEvent* aResult);

}
}

namespace IPC {

template<>
struct ParamTraits<mozilla::dom::RemoteDOMEvent>
{
  typedef mozilla::dom::RemoteDOMEvent paramType;

  static void Write(Message* aMsg, const paramType& aParam)
  {
    aParam.mEvent->Serialize(aMsg, PR_TRUE);
  }

  static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
  {
    return mozilla::dom::ReadRemoteEvent(aMsg, aIter, aResult);
  }

  static void Log(const paramType& aParam, std::wstring* aLog)
  {
  }
};


}


#endif
